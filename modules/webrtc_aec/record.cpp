#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aec.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>

static std::atomic<uint32_t> record_sequence{0};

static void put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xff);
	p[1] = (uint8_t)(v >> 8);
}

static void put_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v & 0xff);
	p[1] = (uint8_t)((v >> 8) & 0xff);
	p[2] = (uint8_t)((v >> 16) & 0xff);
	p[3] = (uint8_t)(v >> 24);
}

static void wav_update(struct aec *aec, unsigned track)
{
	uint8_t h[44] = {0};
	FILE *f = aec->record[track];
	uint32_t bytes = aec->record_data_bytes[track];

	if (!f)
		return;

	memcpy(h, "RIFF", 4);
	put_le32(h + 4, 36 + bytes);
	memcpy(h + 8, "WAVEfmt ", 8);
	put_le32(h + 16, 16);
	put_le16(h + 20, 1);
	put_le16(h + 22, aec->ch);
	put_le32(h + 24, aec->srate);
	put_le32(h + 28, aec->srate * aec->ch * 2);
	put_le16(h + 32, (uint16_t)(aec->ch * 2));
	put_le16(h + 34, 16);
	memcpy(h + 36, "data", 4);
	put_le32(h + 40, bytes);

	fflush(f);
	if (!fseek(f, 0, SEEK_SET)) {
		(void)fwrite(h, 1, sizeof(h), f);
		(void)fseek(f, 0, SEEK_END);
		fflush(f);
	}
}

void aec_record_open(struct aec *aec, bool enabled, const char *dir)
{
	static const char *const suffix[] = {
		"rx-render", "tx-pre-aec", "tx-post-aec"
	};
	char prefix[512];
	char path[600];
	unsigned i;

	if (!aec || !enabled)
		return;

	if (!dir || !*dir)
		dir = "/root/.baresip/aec-recordings";
	if (!fs_isdir(dir)) {
		int err = fs_mkdir(dir, 0700);
		if (err && !fs_isdir(dir)) {
			warning("webrtc_aec: cannot create recording directory %s (%m)\n",
				dir, err);
			return;
		}
	}

	{
		int n = snprintf(prefix, sizeof(prefix), "%s/aec-%llu-%u", dir,
				 (unsigned long long)time(NULL),
				 record_sequence.fetch_add(1));
		if (n < 0 || (size_t)n >= sizeof(prefix))
			return;
	}

	for (i = 0; i < 3; ++i) {
		if (re_snprintf(path, sizeof(path), "%s-%s.wav", prefix,
				suffix[i]) < 0)
			goto fail;
		aec->record[i] = fopen(path, "wb+");
		if (!aec->record[i]) {
			warning("webrtc_aec: cannot open recording %s (%s)\n",
				path, strerror(errno));
			goto fail;
		}
		wav_update(aec, i);
	}

	info("webrtc_aec: writing AEC recordings to %s-*.wav\n", prefix);
	return;

fail:
	for (i = 0; i < 3; ++i) {
		if (aec->record[i]) {
			fclose(aec->record[i]);
			aec->record[i] = NULL;
		}
	}
}

void aec_record_write(struct aec *aec, unsigned track,
		      const float *samples, size_t sample_count)
{
	int16_t *pcm;
	size_t written;

	if (!aec || track >= 3 || !aec->record[track] || !samples || !sample_count)
		return;

	pcm = (int16_t *)malloc(sample_count * sizeof(*pcm));
	if (!pcm)
		return;

	auconv_to_s16(pcm, AUFMT_FLOAT, (void *)samples, sample_count);
	written = fwrite(pcm, sizeof(*pcm), sample_count, aec->record[track]);
	aec->record_data_bytes[track] += (uint32_t)(written * sizeof(*pcm));
	free(pcm);

	if (written != sample_count)
		warning("webrtc_aec: short write while recording audio\n");

	if (++aec->record_update_count >= 100) {
		unsigned i;
		aec->record_update_count = 0;
		for (i = 0; i < 3; ++i)
			wav_update(aec, i);
	}
}

void aec_record_close(struct aec *aec)
{
	unsigned i;

	if (!aec)
		return;

	for (i = 0; i < 3; ++i) {
		if (aec->record[i]) {
			wav_update(aec, i);
			fclose(aec->record[i]);
			aec->record[i] = NULL;
		}
	}
}
