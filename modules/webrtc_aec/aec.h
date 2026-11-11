/**
 * @file aec.h  WebRTC Acoustic Echo Cancellation (AEC) -- internal API
 *
 * Copyright (C) 2010 Alfred E. Heggestad
 */

#define WEBRTC_POSIX 1
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/audio_processing/aec3/echo_canceller3.h>
#include <api/audio/echo_canceller3_config.h>


#define MAX_CHANNELS         1
#define BLOCKSIZE           10  /* ms */


using namespace webrtc;


struct aec {
	AudioProcessing *inst;
	mtx_t mutex;
	uint32_t srate;
	uint8_t ch;
	uint32_t blocksize;
};


/* Encoder */

int webrtc_aec_encode_update(struct aufilt_enc_st **stp, void **ctx,
			     const struct aufilt *af, struct aufilt_prm *prm,
			     const struct audio *au);
int webrtc_aec_encode(struct aufilt_enc_st *st, struct auframe *af);


/* Decoder */

int webrtc_aec_decode_update(struct aufilt_dec_st **stp, void **ctx,
			     const struct aufilt *af, struct aufilt_prm *prm,
			     const struct audio *au);
int webrtc_aec_decode(struct aufilt_dec_st *st, struct auframe *af);


/* Common */
uint32_t webrtc_aec_stream_delay_ms(void);

int  webrtc_aec_alloc(struct aec **stp, void **ctx, struct aufilt_prm *prm);
