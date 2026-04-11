#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================
 * USER CONFIGURATION
 * ========================= */
#define SAMPLE_RATE_HZ          8000U // esta cambiar para analisis probar con 4000, 8000, 12000
#define FFT_SIZE                512U     // esta cambiar para analisis probar con 256, 512, 1024
#define ADC_WIDTH_USED          ADC_BITWIDTH_12 //  esta cambiar para analiss probar con ADC_BITWIDTH_9, ADC_BITWIDTH_10, ADC_BITWIDTH_11, ADC_BITWIDTH_12
//#define ANALYSIS_BITWIDTH       11  // PARA ANALISIS ADC
#define ADC_ATTEN_USED          ADC_ATTEN_DB_12 

// Tone map (Hz)
#define CMD_FORWARD_HZ          650.0f
#define CMD_BACKWARD_HZ         900.0f
#define CMD_LEFT_HZ             1150.0f
#define CMD_RIGHT_HZ            1400.0f
#define CMD_TOLERANCE_HZ        60.0f

// Search band to avoid DC and useless high bins
#define MIN_SEARCH_FREQ_HZ      300.0f
#define MAX_SEARCH_FREQ_HZ      2500.0f

// Stability and motor behavior
#define COMMAND_CONFIRM_FRAMES  2
#define MOTOR_PWM_RESOLUTION    LEDC_TIMER_8_BIT
#define MOTOR_PWM_FREQ_HZ       20000
#define MOTOR_DUTY              200       // 0..255 for 8-bit

// Serial output control
#define PRINT_SPECTRUM_EVERY_N_FRAMES  25
#define PRINT_MAX_BINS                 140

// ADC pin (ESP32-S3 ADC1_CH0 on GPIO1)
#define MIC_ADC_UNIT            ADC_UNIT_1
#define MIC_ADC_CHANNEL         ADC_CHANNEL_0
#define MIC_ADC_GPIO            GPIO_NUM_1

// L298N pins
#define PIN_ENA                 GPIO_NUM_15
#define PIN_IN1                 GPIO_NUM_4
#define PIN_IN2                 GPIO_NUM_5
#define PIN_IN3                 GPIO_NUM_6
#define PIN_IN4                 GPIO_NUM_7
#define PIN_ENB                 GPIO_NUM_16

static const char *TAG = "Ejercicio_5";

typedef struct {
    float re;
    float im;
} complex_t;

typedef enum {
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT
} motion_cmd_t;

static adc_oneshot_unit_handle_t adc_handle;
static float sample_buffer[FFT_SIZE];
static complex_t fft_buffer[FFT_SIZE];
static float mag_buffer[FFT_SIZE / 2];

static const char *cmd_to_string(motion_cmd_t cmd)
{
    switch (cmd) {
        case CMD_FORWARD:  return "ADELANTE";
        case CMD_BACKWARD: return "ATRAS";
        case CMD_LEFT:     return "IZQUIERDA";
        case CMD_RIGHT:    return "DERECHA";
        default:           return "STOP";
    }
}

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

static void motor_pwm_set(uint32_t duty_a, uint32_t duty_b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

static void set_motion(motion_cmd_t cmd)
{
    switch (cmd) {
        case CMD_FORWARD:
            gpio_set_level(PIN_IN1, 1);
            gpio_set_level(PIN_IN2, 0);
            gpio_set_level(PIN_IN3, 1);
            gpio_set_level(PIN_IN4, 0);
            motor_pwm_set(MOTOR_DUTY, MOTOR_DUTY);
            break;

        case CMD_BACKWARD:
            gpio_set_level(PIN_IN1, 0);
            gpio_set_level(PIN_IN2, 1);
            gpio_set_level(PIN_IN3, 0);
            gpio_set_level(PIN_IN4, 1);
            motor_pwm_set(MOTOR_DUTY, MOTOR_DUTY);
            break;

        case CMD_LEFT:
            gpio_set_level(PIN_IN1, 0);
            gpio_set_level(PIN_IN2, 1);
            gpio_set_level(PIN_IN3, 1);
            gpio_set_level(PIN_IN4, 0);
            motor_pwm_set(MOTOR_DUTY, MOTOR_DUTY);
            break;

        case CMD_RIGHT:
            gpio_set_level(PIN_IN1, 1);
            gpio_set_level(PIN_IN2, 0);
            gpio_set_level(PIN_IN3, 0);
            gpio_set_level(PIN_IN4, 1);
            motor_pwm_set(MOTOR_DUTY, MOTOR_DUTY);
            break;

        case CMD_STOP:
        default:
            gpio_set_level(PIN_IN1, 0);
            gpio_set_level(PIN_IN2, 0);
            gpio_set_level(PIN_IN3, 0);
            gpio_set_level(PIN_IN4, 0);
            motor_pwm_set(0, 0);
            break;
    }
}

static void motor_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_IN1) | (1ULL << PIN_IN2) |
                        (1ULL << PIN_IN3) | (1ULL << PIN_IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_ch0 = {
        .gpio_num = PIN_ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config_t ledc_ch1 = {
        .gpio_num = PIN_ENB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config(&ledc_ch0);
    ledc_channel_config(&ledc_ch1);

    set_motion(CMD_STOP);
}

static void adc_init_custom(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = MIC_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_WIDTH_USED,
        .atten = ADC_ATTEN_USED,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MIC_ADC_CHANNEL, &config));
}

static float acquire_samples(float *out, size_t n, uint32_t fs_hz)
{
    const int64_t period_us = 1000000LL / (int64_t)fs_hz;
    int raw = 0;

    //para analisis
    //const int shift = 12 - ANALYSIS_BITWIDTH;

    int64_t t_start = esp_timer_get_time();
    int64_t next_t = t_start;

    for (size_t i = 0; i < n; i++) {
        while (esp_timer_get_time() < next_t) {
        }
        adc_oneshot_read(adc_handle, MIC_ADC_CHANNEL, &raw);
        //analisis
        //int simulated = raw >> shift;
        //int quantized = simulated << shift;
        //out[i] = (float)quantized;

        out[i] = (float)raw;
        next_t += period_us;
    }

    int64_t t_end = esp_timer_get_time();
    float elapsed_s = (float)(t_end - t_start) / 1000000.0f;
    if (elapsed_s <= 0.0f) {
        return (float)fs_hz;
    }
    return (float)n / elapsed_s;
}

static float preprocess_and_fft(const float *in, size_t n)
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

static float find_peak_frequency(float actual_fs, float *peak_mag_out, float *noise_avg_out)
{
    size_t min_bin = (size_t)(MIN_SEARCH_FREQ_HZ * (float)FFT_SIZE / actual_fs);
    size_t max_bin = (size_t)(MAX_SEARCH_FREQ_HZ * (float)FFT_SIZE / actual_fs);

    if (min_bin < 1) {
        min_bin = 1;
    }
    if (max_bin >= FFT_SIZE / 2) {
        max_bin = FFT_SIZE / 2 - 1;
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

    return ((float)peak_bin * actual_fs) / (float)FFT_SIZE;
}

static motion_cmd_t classify_command(float freq_hz, float peak_mag, float noise_avg, float rms)
{
    if (rms < 8.0f) {
        return CMD_STOP;
    }

    if (noise_avg <= 0.0f) {
        noise_avg = 1.0f;
    }

    float prominence = peak_mag / noise_avg;
    if (prominence < 4.0f) {
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

static motion_cmd_t stabilize_command(motion_cmd_t current)
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

static void print_spectrum_csv(float actual_fs)
{
    printf("SPECTRUM_START\n");
    printf("bin,freq_hz,magnitude\n");

    size_t max_bins = PRINT_MAX_BINS;
    if (max_bins > FFT_SIZE / 2) {
        max_bins = FFT_SIZE / 2;
    }

    for (size_t k = 0; k < max_bins; k++) {
        float freq = ((float)k * actual_fs) / (float)FFT_SIZE;
        printf("%u,%.2f,%.4f\n", (unsigned)k, freq, mag_buffer[k]);
    }

    printf("SPECTRUM_END\n");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Ejercicio_5: audio control with FFT");
    ESP_LOGI(TAG, "GPIO mic=%d, ENA=%d IN1=%d IN2=%d IN3=%d IN4=%d ENB=%d",
             MIC_ADC_GPIO, PIN_ENA, PIN_IN1, PIN_IN2, PIN_IN3, PIN_IN4, PIN_ENB);
    ESP_LOGI(TAG, "Fs=%u Hz, N=%u, theoretical delta_f=%.3f Hz, frame_ms=%.2f ms",
             SAMPLE_RATE_HZ, FFT_SIZE,
             (float)SAMPLE_RATE_HZ / (float)FFT_SIZE,
             1000.0f * (float)FFT_SIZE / (float)SAMPLE_RATE_HZ);
    ESP_LOGI(TAG, "Commands: %.0fHz=ADELANTE, %.0fHz=ATRAS, %.0fHz=IZQUIERDA, %.0fHz=DERECHA",
             CMD_FORWARD_HZ, CMD_BACKWARD_HZ, CMD_LEFT_HZ, CMD_RIGHT_HZ);

    motor_init();
    adc_init_custom();

    uint32_t frame_counter = 0;
    motion_cmd_t last_applied = CMD_STOP;

    while (1) {
        float actual_fs = acquire_samples(sample_buffer, FFT_SIZE, SAMPLE_RATE_HZ);
        float rms = preprocess_and_fft(sample_buffer, FFT_SIZE);

        float peak_mag = 0.0f;
        float noise_avg = 0.0f;
        float peak_freq = find_peak_frequency(actual_fs, &peak_mag, &noise_avg);

        motion_cmd_t detected = classify_command(peak_freq, peak_mag, noise_avg, rms);
        motion_cmd_t stable = stabilize_command(detected);

        if (stable != last_applied) {
            set_motion(stable);
            last_applied = stable;
        }

        printf("peak_freq=%.2fHz, peak_mag=%.2f, noise_avg=%.2f, rms=%.2f, cmd=%s, actual_fs=%.2f, delta_f=%.2f\n",
               peak_freq, peak_mag, noise_avg, rms, cmd_to_string(stable), actual_fs, actual_fs / (float)FFT_SIZE);

        frame_counter++;
        if ((frame_counter % PRINT_SPECTRUM_EVERY_N_FRAMES) == 0) {
            print_spectrum_csv(actual_fs);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
