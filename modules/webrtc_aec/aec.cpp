/**
 * @file aec.cpp  WebRTC Acoustic Echo Cancellation (AEC)
 *
 * Copyright (C) 2010 Alfred E. Heggestad
 */

#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "aec.h"
#include <memory>
#include <string>


/**
 * @defgroup webrtc_aec webrtc_aec
 *
 * Acoustic Echo Cancellation (AEC) using WebRTC SDK.
 *
 * Configuration options:
 *
 \verbatim
  webrtc_aec_extended_filter {yes,no} # Enable extended_filter
 \endverbatim
 *
 * This code is experimental.
 *
 * Reference:
 *
 *     https://webrtc.org/native-code/
 */


using namespace webrtc;

static bool webrtc_aec_extended_filter;
static uint32_t stream_delay_ms;
uint32_t webrtc_aec_stream_delay_ms(void) { return stream_delay_ms; }
static webrtc::EchoCanceller3Config aec3_config;
static webrtc::AudioProcessing::Config apm_config;

class BaresipAec3Factory final : public webrtc::EchoControlFactory {
public:
	std::unique_ptr<webrtc::EchoControl> Create(int sample_rate_hz,
			int render_channels, int capture_channels) override
	{
		return std::unique_ptr<webrtc::EchoControl>(
			new webrtc::EchoCanceller3(aec3_config, sample_rate_hz,
						   render_channels, capture_channels));
	}
};

static void get_bool(struct conf *conf, const char *key, bool *value)
{ (void)conf_get_bool(conf, key, value); }
static void get_float(struct conf *conf, const char *key, float *value)
{ double v = *value; if (!conf_get_float(conf, key, &v)) *value = (float)v; }
static void get_u32(struct conf *conf, const char *key, size_t *value)
{ uint32_t v = (uint32_t)*value; if (!conf_get_u32(conf, key, &v)) *value = v; }
static void get_i32(struct conf *conf, const char *key, int *value)
{ int32_t v = (int32_t)*value; if (!conf_get_i32(conf, key, &v)) *value = v; }

#define AEC3_BOOL(k, f) get_bool(conf, "webrtc_aec3_" k, &aec3_config.f)
#define AEC3_FLOAT(k, f) get_float(conf, "webrtc_aec3_" k, &aec3_config.f)
#define AEC3_U32(k, f) get_u32(conf, "webrtc_aec3_" k, &aec3_config.f)
#define AEC3_I32(k, f) get_i32(conf, "webrtc_aec3_" k, &aec3_config.f)

static void load_aec3_config(struct conf *conf)
{
	AEC3_U32("buffering.excess_render_detection_interval_blocks", buffering.excess_render_detection_interval_blocks);
	AEC3_U32("buffering.max_allowed_excess_render_blocks", buffering.max_allowed_excess_render_blocks);
	AEC3_U32("delay.default_delay", delay.default_delay);
	AEC3_U32("delay.down_sampling_factor", delay.down_sampling_factor);
	AEC3_U32("delay.num_filters", delay.num_filters);
	AEC3_U32("delay.delay_headroom_samples", delay.delay_headroom_samples);
	AEC3_U32("delay.hysteresis_limit_blocks", delay.hysteresis_limit_blocks);
	AEC3_U32("delay.fixed_capture_delay_samples", delay.fixed_capture_delay_samples);
	AEC3_FLOAT("delay.delay_estimate_smoothing", delay.delay_estimate_smoothing);
	AEC3_FLOAT("delay.delay_candidate_detection_threshold", delay.delay_candidate_detection_threshold);
	AEC3_I32("delay.delay_selection_thresholds.initial", delay.delay_selection_thresholds.initial);
	AEC3_I32("delay.delay_selection_thresholds.converged", delay.delay_selection_thresholds.converged);
	AEC3_BOOL("delay.use_external_delay_estimator", delay.use_external_delay_estimator);
	AEC3_BOOL("delay.log_warning_on_delay_changes", delay.log_warning_on_delay_changes);
	AEC3_BOOL("delay.render_alignment_mixing.downmix", delay.render_alignment_mixing.downmix);
	AEC3_BOOL("delay.render_alignment_mixing.adaptive_selection", delay.render_alignment_mixing.adaptive_selection);
	AEC3_FLOAT("delay.render_alignment_mixing.activity_power_threshold", delay.render_alignment_mixing.activity_power_threshold);
	AEC3_BOOL("delay.render_alignment_mixing.prefer_first_two_channels", delay.render_alignment_mixing.prefer_first_two_channels);
	AEC3_BOOL("delay.capture_alignment_mixing.downmix", delay.capture_alignment_mixing.downmix);
	AEC3_BOOL("delay.capture_alignment_mixing.adaptive_selection", delay.capture_alignment_mixing.adaptive_selection);
	AEC3_FLOAT("delay.capture_alignment_mixing.activity_power_threshold", delay.capture_alignment_mixing.activity_power_threshold);
	AEC3_BOOL("delay.capture_alignment_mixing.prefer_first_two_channels", delay.capture_alignment_mixing.prefer_first_two_channels);
#define CFG(k, f) AEC3_U32(k, f)
#define FLT(k, f) AEC3_FLOAT(k, f)
#define BOL(k, f) AEC3_BOOL(k, f)
	CFG("filter.refined.length_blocks", filter.refined.length_blocks);
	FLT("filter.refined.leakage_converged", filter.refined.leakage_converged);
	FLT("filter.refined.leakage_diverged", filter.refined.leakage_diverged);
	FLT("filter.refined.error_floor", filter.refined.error_floor);
	FLT("filter.refined.error_ceil", filter.refined.error_ceil);
	FLT("filter.refined.noise_gate", filter.refined.noise_gate);
	CFG("filter.coarse.length_blocks", filter.coarse.length_blocks);
	FLT("filter.coarse.rate", filter.coarse.rate);
	FLT("filter.coarse.noise_gate", filter.coarse.noise_gate);
	CFG("filter.refined_initial.length_blocks", filter.refined_initial.length_blocks);
	FLT("filter.refined_initial.leakage_converged", filter.refined_initial.leakage_converged);
	FLT("filter.refined_initial.leakage_diverged", filter.refined_initial.leakage_diverged);
	FLT("filter.refined_initial.error_floor", filter.refined_initial.error_floor);
	FLT("filter.refined_initial.error_ceil", filter.refined_initial.error_ceil);
	FLT("filter.refined_initial.noise_gate", filter.refined_initial.noise_gate);
	CFG("filter.coarse_initial.length_blocks", filter.coarse_initial.length_blocks);
	FLT("filter.coarse_initial.rate", filter.coarse_initial.rate);
	FLT("filter.coarse_initial.noise_gate", filter.coarse_initial.noise_gate);
	CFG("filter.config_change_duration_blocks", filter.config_change_duration_blocks);
	FLT("filter.initial_state_seconds", filter.initial_state_seconds);
	BOL("filter.conservative_initial_phase", filter.conservative_initial_phase);
	BOL("filter.enable_coarse_filter_output_usage", filter.enable_coarse_filter_output_usage);
	BOL("filter.use_linear_filter", filter.use_linear_filter);
	BOL("filter.export_linear_aec_output", filter.export_linear_aec_output);
	FLT("erle.min", erle.min); FLT("erle.max_l", erle.max_l); FLT("erle.max_h", erle.max_h);
	BOL("erle.onset_detection", erle.onset_detection); CFG("erle.num_sections", erle.num_sections);
	BOL("erle.clamp_quality_estimate_to_zero", erle.clamp_quality_estimate_to_zero);
	BOL("erle.clamp_quality_estimate_to_one", erle.clamp_quality_estimate_to_one);
	FLT("ep_strength.default_gain", ep_strength.default_gain); FLT("ep_strength.default_len", ep_strength.default_len);
	BOL("ep_strength.echo_can_saturate", ep_strength.echo_can_saturate); BOL("ep_strength.bounded_erl", ep_strength.bounded_erl);
	FLT("echo_audibility.low_render_limit", echo_audibility.low_render_limit);
	FLT("echo_audibility.normal_render_limit", echo_audibility.normal_render_limit);
	FLT("echo_audibility.floor_power", echo_audibility.floor_power);
	FLT("echo_audibility.audibility_threshold_lf", echo_audibility.audibility_threshold_lf);
	FLT("echo_audibility.audibility_threshold_mf", echo_audibility.audibility_threshold_mf);
	FLT("echo_audibility.audibility_threshold_hf", echo_audibility.audibility_threshold_hf);
	BOL("echo_audibility.use_stationarity_properties", echo_audibility.use_stationarity_properties);
	BOL("echo_audibility.use_stationarity_properties_at_init", echo_audibility.use_stationarity_properties_at_init);
	FLT("render_levels.active_render_limit", render_levels.active_render_limit);
	FLT("render_levels.poor_excitation_render_limit", render_levels.poor_excitation_render_limit);
	FLT("render_levels.poor_excitation_render_limit_ds8", render_levels.poor_excitation_render_limit_ds8);
	FLT("render_levels.render_power_gain_db", render_levels.render_power_gain_db);
	BOL("echo_removal_control.has_clock_drift", echo_removal_control.has_clock_drift);
	BOL("echo_removal_control.linear_and_stable_echo_path", echo_removal_control.linear_and_stable_echo_path);
	CFG("echo_model.noise_floor_hold", echo_model.noise_floor_hold);
	FLT("echo_model.min_noise_floor_power", echo_model.min_noise_floor_power);
	FLT("echo_model.stationary_gate_slope", echo_model.stationary_gate_slope);
	FLT("echo_model.noise_gate_power", echo_model.noise_gate_power);
	FLT("echo_model.noise_gate_slope", echo_model.noise_gate_slope);
	CFG("echo_model.render_pre_window_size", echo_model.render_pre_window_size);
	CFG("echo_model.render_post_window_size", echo_model.render_post_window_size);
	BOL("echo_model.model_reverb_in_nonlinear_mode", echo_model.model_reverb_in_nonlinear_mode);
	FLT("comfort_noise.noise_floor_dbfs", comfort_noise.noise_floor_dbfs);
	CFG("suppressor.nearend_average_blocks", suppressor.nearend_average_blocks);
	FLT("suppressor.normal_tuning.mask_lf.enr_transparent", suppressor.normal_tuning.mask_lf.enr_transparent);
	FLT("suppressor.normal_tuning.mask_lf.enr_suppress", suppressor.normal_tuning.mask_lf.enr_suppress);
	FLT("suppressor.normal_tuning.mask_lf.emr_transparent", suppressor.normal_tuning.mask_lf.emr_transparent);
	FLT("suppressor.normal_tuning.mask_hf.enr_transparent", suppressor.normal_tuning.mask_hf.enr_transparent);
	FLT("suppressor.normal_tuning.mask_hf.enr_suppress", suppressor.normal_tuning.mask_hf.enr_suppress);
	FLT("suppressor.normal_tuning.mask_hf.emr_transparent", suppressor.normal_tuning.mask_hf.emr_transparent);
	FLT("suppressor.normal_tuning.max_inc_factor", suppressor.normal_tuning.max_inc_factor);
	FLT("suppressor.normal_tuning.max_dec_factor_lf", suppressor.normal_tuning.max_dec_factor_lf);
	FLT("suppressor.nearend_tuning.mask_lf.enr_transparent", suppressor.nearend_tuning.mask_lf.enr_transparent);
	FLT("suppressor.nearend_tuning.mask_lf.enr_suppress", suppressor.nearend_tuning.mask_lf.enr_suppress);
	FLT("suppressor.nearend_tuning.mask_lf.emr_transparent", suppressor.nearend_tuning.mask_lf.emr_transparent);
	FLT("suppressor.nearend_tuning.mask_hf.enr_transparent", suppressor.nearend_tuning.mask_hf.enr_transparent);
	FLT("suppressor.nearend_tuning.mask_hf.enr_suppress", suppressor.nearend_tuning.mask_hf.enr_suppress);
	FLT("suppressor.nearend_tuning.mask_hf.emr_transparent", suppressor.nearend_tuning.mask_hf.emr_transparent);
	FLT("suppressor.nearend_tuning.max_inc_factor", suppressor.nearend_tuning.max_inc_factor);
	FLT("suppressor.nearend_tuning.max_dec_factor_lf", suppressor.nearend_tuning.max_dec_factor_lf);
	FLT("suppressor.dominant_nearend_detection.enr_threshold", suppressor.dominant_nearend_detection.enr_threshold);
	FLT("suppressor.dominant_nearend_detection.enr_exit_threshold", suppressor.dominant_nearend_detection.enr_exit_threshold);
	FLT("suppressor.dominant_nearend_detection.snr_threshold", suppressor.dominant_nearend_detection.snr_threshold);
	AEC3_I32("suppressor.dominant_nearend_detection.hold_duration", suppressor.dominant_nearend_detection.hold_duration);
	AEC3_I32("suppressor.dominant_nearend_detection.trigger_threshold", suppressor.dominant_nearend_detection.trigger_threshold);
	AEC3_BOOL("suppressor.dominant_nearend_detection.use_during_initial_phase", suppressor.dominant_nearend_detection.use_during_initial_phase);
	CFG("suppressor.subband_nearend_detection.nearend_average_blocks", suppressor.subband_nearend_detection.nearend_average_blocks);
	CFG("suppressor.subband_nearend_detection.subband1.low", suppressor.subband_nearend_detection.subband1.low);
	CFG("suppressor.subband_nearend_detection.subband1.high", suppressor.subband_nearend_detection.subband1.high);
	CFG("suppressor.subband_nearend_detection.subband2.low", suppressor.subband_nearend_detection.subband2.low);
	CFG("suppressor.subband_nearend_detection.subband2.high", suppressor.subband_nearend_detection.subband2.high);
	FLT("suppressor.subband_nearend_detection.nearend_threshold", suppressor.subband_nearend_detection.nearend_threshold);
	FLT("suppressor.subband_nearend_detection.snr_threshold", suppressor.subband_nearend_detection.snr_threshold);
	BOL("suppressor.use_subband_nearend_detection", suppressor.use_subband_nearend_detection);
	FLT("suppressor.high_bands_suppression.enr_threshold", suppressor.high_bands_suppression.enr_threshold);
	FLT("suppressor.high_bands_suppression.max_gain_during_echo", suppressor.high_bands_suppression.max_gain_during_echo);
	FLT("suppressor.high_bands_suppression.anti_howling_activation_threshold", suppressor.high_bands_suppression.anti_howling_activation_threshold);
	FLT("suppressor.high_bands_suppression.anti_howling_gain", suppressor.high_bands_suppression.anti_howling_gain);
	FLT("suppressor.floor_first_increase", suppressor.floor_first_increase);
#undef CFG
#undef FLT
#undef BOL
	if (!webrtc::EchoCanceller3Config::Validate(&aec3_config))
		warning("webrtc_aec: invalid AEC3 values were clamped to valid ranges\n");
}


static void aec_destructor(void *arg)
{
	struct aec *st = (struct aec *)arg;

	if (st->inst)
		delete st->inst;

	mtx_destroy(&st->mutex);
}


int webrtc_aec_alloc(struct aec **stp, void **ctx, struct aufilt_prm *prm)
{
	struct aec *aec;
	int err = 0;


	if (!stp || !ctx || !prm)
		return EINVAL;

	if (prm->ch > MAX_CHANNELS) {
		warning("webrtc_aec: unsupported channels (%u > %u)\n",
			prm->ch, MAX_CHANNELS);
		return ENOTSUP;
	}

	if (*ctx) {
		aec = (struct aec *)*ctx;

		if (prm->srate != aec->srate) {

			warning("webrtc_aec: srate mismatch\n");
			return ENOTSUP;
		}

		*stp = (struct aec *)mem_ref(*ctx);
		return 0;
	}

	aec = (struct aec *)mem_zalloc(sizeof(*aec), aec_destructor);
	if (!aec)
		return ENOMEM;

	aec->srate = prm->srate;
	aec->ch    = prm->ch;

	err = mtx_init(&aec->mutex, mtx_plain) != thrd_success;
	if (err) {
		err = ENOMEM;
		goto out;
	}

	// NOTE: excluding channel count
	aec->blocksize  = prm->srate * BLOCKSIZE / 1000;

	info("webrtc_aec: creating shared state:"
	     " [%u Hz, %u channels, blocksize %u samples]\n",
	     prm->srate, prm->ch, aec->blocksize);

	aec->inst = AudioProcessingBuilder()
		.SetEchoControlFactory(std::unique_ptr<EchoControlFactory>(
			new BaresipAec3Factory()))
		.Create();
	if (aec->inst)
		aec->inst->ApplyConfig(apm_config);
	if (!aec->inst) {
		err = ENODEV;
		goto out;
	}

 out:
	if (err)
		mem_deref(aec);
	else {
		*stp = aec;
		*ctx = aec;
	}

	return err;
}


static struct aufilt webrtc_aec = {
	.name    = "webrtc_aec",
	.encupdh = webrtc_aec_encode_update,
	.ench    = webrtc_aec_encode,
	.decupdh = webrtc_aec_decode_update,
	.dech    = webrtc_aec_decode
};


static int module_init(void)
{
	aufilt_register(baresip_aufiltl(), &webrtc_aec);

	struct conf *conf = conf_cur();
	apm_config.echo_canceller.enabled = true;
	apm_config.echo_canceller.mobile_mode = false;
	conf_get_u32(conf, "webrtc_aec_stream_delay_ms", &stream_delay_ms);
	get_i32(conf, "webrtc_apm_max_internal_processing_rate", &apm_config.pipeline.maximum_internal_processing_rate);
	get_bool(conf, "webrtc_apm_multi_channel_render", &apm_config.pipeline.multi_channel_render);
	get_bool(conf, "webrtc_apm_multi_channel_capture", &apm_config.pipeline.multi_channel_capture);
	get_bool(conf, "webrtc_apm_pre_amplifier_enabled", &apm_config.pre_amplifier.enabled);
	get_float(conf, "webrtc_apm_pre_amplifier_gain", &apm_config.pre_amplifier.fixed_gain_factor);
	get_bool(conf, "webrtc_apm_high_pass_enabled", &apm_config.high_pass_filter.enabled);
	get_bool(conf, "webrtc_apm_high_pass_full_band", &apm_config.high_pass_filter.apply_in_full_band);
	get_bool(conf, "webrtc_apm_export_linear_aec", &apm_config.echo_canceller.export_linear_aec_output);
	get_bool(conf, "webrtc_apm_echo_enforce_high_pass", &apm_config.echo_canceller.enforce_high_pass_filtering);
	get_bool(conf, "webrtc_apm_noise_suppression_enabled", &apm_config.noise_suppression.enabled);
	get_bool(conf, "webrtc_apm_noise_analyze_linear_aec", &apm_config.noise_suppression.analyze_linear_aec_output_when_available);
	int32_t enum_value = (int32_t)apm_config.noise_suppression.level;
	if (!conf_get_i32(conf, "webrtc_apm_noise_suppression_level", &enum_value) && enum_value >= 0 && enum_value <= 3)
		apm_config.noise_suppression.level = (webrtc::AudioProcessing::Config::NoiseSuppression::Level)enum_value;
	get_bool(conf, "webrtc_apm_transient_suppression_enabled", &apm_config.transient_suppression.enabled);
	get_bool(conf, "webrtc_apm_voice_detection_enabled", &apm_config.voice_detection.enabled);
	get_bool(conf, "webrtc_apm_gain_controller1_enabled", &apm_config.gain_controller1.enabled);
	enum_value = (int32_t)apm_config.gain_controller1.mode;
	if (!conf_get_i32(conf, "webrtc_apm_gain_controller1_mode", &enum_value) && enum_value >= 0 && enum_value <= 2)
		apm_config.gain_controller1.mode = (webrtc::AudioProcessing::Config::GainController1::Mode)enum_value;
	get_i32(conf, "webrtc_apm_gain_controller1_target_dbfs", &apm_config.gain_controller1.target_level_dbfs);
	get_i32(conf, "webrtc_apm_gain_controller1_compression_gain_db", &apm_config.gain_controller1.compression_gain_db);
	get_bool(conf, "webrtc_apm_gain_controller1_limiter", &apm_config.gain_controller1.enable_limiter);
	get_i32(conf, "webrtc_apm_gain_controller1_analog_min", &apm_config.gain_controller1.analog_level_minimum);
	get_i32(conf, "webrtc_apm_gain_controller1_analog_max", &apm_config.gain_controller1.analog_level_maximum);
	get_bool(conf, "webrtc_apm_gain_controller1_analog_enabled", &apm_config.gain_controller1.analog_gain_controller.enabled);
	get_i32(conf, "webrtc_apm_gain_controller1_startup_min_volume", &apm_config.gain_controller1.analog_gain_controller.startup_min_volume);
	get_i32(conf, "webrtc_apm_gain_controller1_clipped_level_min", &apm_config.gain_controller1.analog_gain_controller.clipped_level_min);
	get_bool(conf, "webrtc_apm_gain_controller1_agc2_level_estimator", &apm_config.gain_controller1.analog_gain_controller.enable_agc2_level_estimator);
	get_bool(conf, "webrtc_apm_gain_controller1_digital_adaptive", &apm_config.gain_controller1.analog_gain_controller.enable_digital_adaptive);
	get_bool(conf, "webrtc_apm_gain_controller2_enabled", &apm_config.gain_controller2.enabled);
	get_float(conf, "webrtc_apm_gain_controller2_fixed_gain_db", &apm_config.gain_controller2.fixed_digital.gain_db);
	get_bool(conf, "webrtc_apm_gain_controller2_adaptive_enabled", &apm_config.gain_controller2.adaptive_digital.enabled);
	get_float(conf, "webrtc_apm_gain_controller2_vad_probability_attack", &apm_config.gain_controller2.adaptive_digital.vad_probability_attack);
	enum_value = (int32_t)apm_config.gain_controller2.adaptive_digital.level_estimator;
	if (!conf_get_i32(conf, "webrtc_apm_gain_controller2_level_estimator", &enum_value) && enum_value >= 0 && enum_value <= 1)
		apm_config.gain_controller2.adaptive_digital.level_estimator = (webrtc::AudioProcessing::Config::GainController2::LevelEstimator)enum_value;
	get_i32(conf, "webrtc_apm_gain_controller2_adjacent_speech_frames", &apm_config.gain_controller2.adaptive_digital.level_estimator_adjacent_speech_frames_threshold);
	get_bool(conf, "webrtc_apm_gain_controller2_saturation_protector", &apm_config.gain_controller2.adaptive_digital.use_saturation_protector);
	get_float(conf, "webrtc_apm_gain_controller2_initial_saturation_margin_db", &apm_config.gain_controller2.adaptive_digital.initial_saturation_margin_db);
	get_float(conf, "webrtc_apm_gain_controller2_extra_saturation_margin_db", &apm_config.gain_controller2.adaptive_digital.extra_saturation_margin_db);
	get_i32(conf, "webrtc_apm_gain_controller2_gain_applier_speech_frames", &apm_config.gain_controller2.adaptive_digital.gain_applier_adjacent_speech_frames_threshold);
	get_float(conf, "webrtc_apm_gain_controller2_max_gain_change_db_per_second", &apm_config.gain_controller2.adaptive_digital.max_gain_change_db_per_second);
	get_float(conf, "webrtc_apm_gain_controller2_max_output_noise_dbfs", &apm_config.gain_controller2.adaptive_digital.max_output_noise_level_dbfs);
	get_bool(conf, "webrtc_apm_residual_echo_detector_enabled", &apm_config.residual_echo_detector.enabled);
	get_bool(conf, "webrtc_apm_level_estimation_enabled", &apm_config.level_estimation.enabled);
	load_aec3_config(conf);
	conf_get_bool(
		conf,
		"webrtc_aec_extended_filter",
		&webrtc_aec_extended_filter
	);

	return 0;
}


static int module_close(void)
{
	aufilt_unregister(&webrtc_aec);
	return 0;
}


extern "C" const struct mod_export DECL_EXPORTS(webrtc_aec) = {
	"webrtc_aec",
	"aufilt",
	module_init,
	module_close
};
