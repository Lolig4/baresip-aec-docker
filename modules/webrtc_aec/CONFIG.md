# webrtc_aec runtime configuration

All keys are read from the regular baresip `config` when `webrtc_aec.so` loads.
Unset values retain the WebRTC 1.3 defaults. Restart baresip after changing them.

## Main APM options

`webrtc_aec` always enables the AEC3 echo canceller in desktop mode. Other
`AudioProcessing::Config` fields can be set with these keys:

- `webrtc_apm_max_internal_processing_rate` (32000 or 48000)
- `webrtc_apm_multi_channel_render`, `webrtc_apm_multi_channel_capture`
- `webrtc_apm_pre_amplifier_enabled`, `webrtc_apm_pre_amplifier_gain`
- `webrtc_apm_high_pass_enabled`, `webrtc_apm_high_pass_full_band`
- `webrtc_apm_export_linear_aec`, `webrtc_apm_echo_enforce_high_pass`
- `webrtc_apm_noise_suppression_enabled`, `webrtc_apm_noise_analyze_linear_aec`
- `webrtc_apm_noise_suppression_level` (0 low, 1 moderate, 2 high, 3 very high)
- `webrtc_apm_transient_suppression_enabled`, `webrtc_apm_voice_detection_enabled`
- `webrtc_apm_gain_controller1_enabled`, `webrtc_apm_gain_controller1_mode` (0 adaptive analog, 1 adaptive digital, 2 fixed digital)
- `webrtc_apm_gain_controller1_target_dbfs`, `webrtc_apm_gain_controller1_compression_gain_db`, `webrtc_apm_gain_controller1_limiter`
- `webrtc_apm_gain_controller1_analog_min`, `webrtc_apm_gain_controller1_analog_max`
- `webrtc_apm_gain_controller1_analog_enabled`, `webrtc_apm_gain_controller1_startup_min_volume`
- `webrtc_apm_gain_controller1_clipped_level_min`, `webrtc_apm_gain_controller1_agc2_level_estimator`, `webrtc_apm_gain_controller1_digital_adaptive`
- `webrtc_apm_gain_controller2_enabled`, `webrtc_apm_gain_controller2_fixed_gain_db`
- `webrtc_apm_gain_controller2_adaptive_enabled`, `webrtc_apm_gain_controller2_vad_probability_attack`
- `webrtc_apm_gain_controller2_level_estimator` (0 RMS, 1 peak)
- `webrtc_apm_gain_controller2_adjacent_speech_frames`, `webrtc_apm_gain_controller2_saturation_protector`
- `webrtc_apm_gain_controller2_initial_saturation_margin_db`, `webrtc_apm_gain_controller2_extra_saturation_margin_db`
- `webrtc_apm_gain_controller2_gain_applier_speech_frames`, `webrtc_apm_gain_controller2_max_gain_change_db_per_second`
- `webrtc_apm_gain_controller2_max_output_noise_dbfs`
- `webrtc_apm_residual_echo_detector_enabled`, `webrtc_apm_level_estimation_enabled`

## External stream delay

`webrtc_aec_stream_delay_ms` is optional and defaults to `0`. At zero, baresip
does not call `set_stream_delay_ms()` and AEC3 uses its internal delay
estimator. Set a nonzero value only when an independently measured device/audio
pipeline delay is available. This is the WebRTC render-to-capture audio delay,
not the round-trip echo heard by the remote caller.

## AEC3 options

Every public member of WebRTC 1.3 `EchoCanceller3Config` is exposed as a flat
baresip key. Prefix each C++ member path with `webrtc_aec3_`, replacing nested
member separators with dots. Examples:

```ini
webrtc_aec3_delay.use_external_delay_estimator     no
webrtc_aec3_delay.default_delay                     5
webrtc_aec3_filter.coarse.length_blocks             13
webrtc_aec3_filter.coarse.rate                      0.7
webrtc_aec3_suppressor.normal_tuning.max_inc_factor 2.0
webrtc_aec3_suppressor.floor_first_increase         0.00001
```

The exposed groups and member paths follow `EchoCanceller3Config`: `buffering`,
`delay`, `filter`, `erle`, `ep_strength`, `echo_audibility`, `render_levels`,
`echo_removal_control`, `echo_model`, `comfort_noise`, and `suppressor`.
Values are validated by WebRTC; invalid/out-of-range values are clamped by its
`Validate()` function.
