#include "audio_control.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ADC_WIDTH_USED          ADC_BITWIDTH_12
#define ADC_ATTEN_USED          ADC_ATTEN_DB_12

#define CMD_FORWARD_HZ          650.0f
#define CMD_BACKWARD_HZ         900.0f
#define CMD_LEFT_HZ             1150.0f
#define CMD_RIGHT_HZ            1400.0f
#define CMD_TOLERANCE_HZ        60.0f

#define MIN_SEARCH_FREQ_HZ      300.0f
#define MAX_SEARCH_FREQ_HZ      2500.0f
#define COMMAND_CONFIRM_FRAMES  2

#define MIC_ADC_UNIT            ADC_UNIT_1
#define MIC_ADC_CHANNEL         ADC_CHANNEL_6 // GPIO 34 on ESP32

static const char *TAG = "audio_control";

typedef struct {
    float re;
    float im;
} complex_t;

static adc_oneshot_unit_handle_t adc_handle = NULL;
static complex_t fft_buffer[AUDIO_FFT_SIZE];
static float mag_buffer[AUDIO_FFT_SIZE / 2];

volatile float g_audio_peak_freq = 0.0f;
volatile float g_audio_peak_mag = 0.0f;
volatile motion_cmd_t g_audio_cmd = CMD_STOP;

static unsigned reverse_bits(unsigned x, int bits)
{
    unsigned y = 0;
    for (int i = 0; i < bits; i++) {
        y = (y << 1) | (x & 1U);
        x >>= 1U;
    }
    return y;
}

static void fft_inplace(complex_t *x, size_t n)
{
    int levels = 0;
    for (size_t temp = n; temp > 1; temp >>= 1) {
        levels++;
    }

    for (size_t i = 0; i < n; i++) {
        size_t j = reverse_bits((unsigned)i, levels);
        if (j > i) {
            complex_t t = x[i];
            x[i] = x[j];
            x[j] = t;
        }
    }

    for (size_t size = 2; size <= n; size <<= 1) {
        size_t halfsize = size >> 1;
        float theta = -2.0f * (float)M_PI / (float)size;
        float wtemp_re = cosf(theta);
        float wtemp_im = sinf(theta);

        for (size_t i = 0; i < n; i += size) {
            float w_re = 1.0f;
            float w_im = 0.0f;

            for (size_t j = 0; j < halfsize; j++) {
                size_t even = i + j;
                size_t odd  = i + j + halfsize;

                float t_re = w_re * x[odd].re - w_im * x[odd].im;
                float t_im = w_re * x[odd].im + w_im * x[odd].re;

                float u_re = x[even].re;
                float u_im = x[even].im;

                x[even].re = u_re + t_re;
                x[even].im = u_im + t_im;
                x[odd].re  = u_re - t_re;
                x[odd].im  = u_im - t_im;

                float next_w_re = w_re * wtemp_re - w_im * wtemp_im;
                float next_w_im = w_re * wtemp_im + w_im * wtemp_re;
                w_re = next_w_re;
                w_im = next_w_im;
            }
        }
    }
}

void audio_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config1;
    memset(&init_config1, 0, sizeof(init_config1));
    init_config1.unit_id = MIC_ADC_UNIT;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config;
    memset(&config, 0, sizeof(config));
    config.bitwidth = ADC_WIDTH_USED;
    config.atten = ADC_ATTEN_USED;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MIC_ADC_CHANNEL, &config));
    ESP_LOGI(TAG, "Audio ADC initialized on GPIO 34 (ADC1_CH6).");
}

float audio_acquire_samples(float *out, size_t n, float sim_frequency)
{
    if (sim_frequency > 0.0f) {
        // Digital Simulation Mode
        for (size_t i = 0; i < n; i++) {
            float val = sinf(2.0f * (float)M_PI * sim_frequency * (float)i / (float)AUDIO_SAMPLE_RATE_HZ) * 1500.0f;
            out[i] = val + 2048.0f;
        }
        return (float)AUDIO_SAMPLE_RATE_HZ;
    }

    // Physical sampling mode via high precision oneshot ADC reads
    const int64_t period_us = 1000000LL / (int64_t)AUDIO_SAMPLE_RATE_HZ;
    int raw = 0;

    int64_t t_start = esp_timer_get_time();
    int64_t next_t = t_start;

    for (size_t i = 0; i < n; i++) {
        while (esp_timer_get_time() < next_t) {
            // Busy wait for high-precision microsecond-level timing
        }
        adc_oneshot_read(adc_handle, MIC_ADC_CHANNEL, &raw);
        out[i] = (float)raw;
        next_t += period_us;
    }

    int64_t t_end = esp_timer_get_time();
    float elapsed_s = (float)(t_end - t_start) / 1000000.0f;
    if (elapsed_s <= 0.0f) {
        return (float)AUDIO_SAMPLE_RATE_HZ;
    }
    return (float)n / elapsed_s;
}

float audio_preprocess_and_fft(const float *in, size_t n)
{
    float mean = 0.0f;
    for (size_t i = 0; i < n; i++) {
        mean += in[i];
    }
    mean /= (float)n;

    float rms = 0.0f;
    for (size_t i = 0; i < n; i++) {
        float x = in[i] - mean;
        float w = 0.5f - 0.5f * cosf((2.0f * (float)M_PI * (float)i) / (float)(n - 1)); // Hann
        fft_buffer[i].re = x * w;
        fft_buffer[i].im = 0.0f;
        rms += x * x;
    }
    rms = sqrtf(rms / (float)n);

    fft_inplace(fft_buffer, n);

    for (size_t k = 0; k < n / 2; k++) {
        float re = fft_buffer[k].re;
        float im = fft_buffer[k].im;
        mag_buffer[k] = sqrtf(re * re + im * im);
    }

    return rms;
}

float audio_find_peak_frequency(float actual_fs, float *peak_mag_out, float *noise_avg_out)
{
    size_t min_bin = (size_t)(MIN_SEARCH_FREQ_HZ * (float)AUDIO_FFT_SIZE / actual_fs);
    size_t max_bin = (size_t)(MAX_SEARCH_FREQ_HZ * (float)AUDIO_FFT_SIZE / actual_fs);

    if (min_bin < 1) {
        min_bin = 1;
    }
    if (max_bin >= AUDIO_FFT_SIZE / 2) {
        max_bin = AUDIO_FFT_SIZE / 2 - 1;
    }

    size_t peak_bin = min_bin;
    float peak_mag = -FLT_MAX;
    float noise_sum = 0.0f;
    size_t count = 0;

    for (size_t k = min_bin; k <= max_bin; k++) {
        float mag = mag_buffer[k];
        noise_sum += mag;
        count++;
        if (mag > peak_mag) {
            peak_mag = mag;
            peak_bin = k;
        }
    }

    float noise_avg = (count > 0) ? (noise_sum / (float)count) : 1.0f;
    if (peak_mag_out) {
        *peak_mag_out = peak_mag;
    }
    if (noise_avg_out) {
        *noise_avg_out = noise_avg;
    }

    return ((float)peak_bin * actual_fs) / (float)AUDIO_FFT_SIZE;
}

motion_cmd_t audio_classify_command(float freq_hz, float peak_mag, float noise_avg, float rms)
{
    if (rms < 8.0f) {
        return CMD_STOP;
    }

    if (noise_avg <= 0.0f) {
        noise_avg = 1.0f;
    }

    float prominence = peak_mag / noise_avg;
    float required_prominence = 4.0f;

    if (prominence < required_prominence) {
        return CMD_STOP;
    }

    if (fabsf(freq_hz - CMD_FORWARD_HZ) <= CMD_TOLERANCE_HZ) {
        return CMD_FORWARD;
    }
    if (fabsf(freq_hz - CMD_BACKWARD_HZ) <= CMD_TOLERANCE_HZ) {
        return CMD_BACKWARD;
    }
    if (fabsf(freq_hz - CMD_LEFT_HZ) <= CMD_TOLERANCE_HZ) {
        return CMD_LEFT;
    }
    if (fabsf(freq_hz - CMD_RIGHT_HZ) <= CMD_TOLERANCE_HZ) {
        return CMD_RIGHT;
    }

    return CMD_STOP;
}

motion_cmd_t audio_stabilize_command(motion_cmd_t current)
{
    static motion_cmd_t last_candidate = CMD_STOP;
    static motion_cmd_t stable_cmd = CMD_STOP;
    static int repeated = 0;

    if (current == last_candidate) {
        repeated++;
    } else {
        last_candidate = current;
        repeated = 1;
    }

    if (repeated >= COMMAND_CONFIRM_FRAMES) {
        stable_cmd = current;
    }

    return stable_cmd;
}
