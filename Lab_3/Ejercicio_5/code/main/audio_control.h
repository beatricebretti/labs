#ifndef AUDIO_CONTROL_H
#define AUDIO_CONTROL_H

#include "motor_control.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SAMPLE_RATE_HZ   8000U
#define AUDIO_FFT_SIZE         512U

// Global status variables exposed to the web server
extern volatile float g_audio_peak_freq;
extern volatile float g_audio_peak_mag;
extern volatile motion_cmd_t g_audio_cmd;

void audio_init(void);
float audio_acquire_samples(float *out, size_t n, float sim_frequency);
float audio_preprocess_and_fft(const float *in, size_t n);
float audio_find_peak_frequency(float actual_fs, float *peak_mag_out, float *noise_avg_out);
motion_cmd_t audio_classify_command(float freq_hz, float peak_mag, float noise_avg, float rms);
motion_cmd_t audio_stabilize_command(motion_cmd_t current);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_CONTROL_H
