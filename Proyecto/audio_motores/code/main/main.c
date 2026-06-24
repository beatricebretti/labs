#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define PIN_ENA ((gpio_num_t) CONFIG_AUDIO_MOTOR_LEFT_ENABLE_GPIO)
#define PIN_IN1 ((gpio_num_t) CONFIG_AUDIO_MOTOR_LEFT_IN1_GPIO)
#define PIN_IN2 ((gpio_num_t) CONFIG_AUDIO_MOTOR_LEFT_IN2_GPIO)
#define PIN_IN3 ((gpio_num_t) CONFIG_AUDIO_MOTOR_RIGHT_IN1_GPIO)
#define PIN_IN4 ((gpio_num_t) CONFIG_AUDIO_MOTOR_RIGHT_IN2_GPIO)
#define PIN_ENB ((gpio_num_t) CONFIG_AUDIO_MOTOR_RIGHT_ENABLE_GPIO)

#define PWM_FREQUENCY_HZ 20000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY 255
#define AUDIO_SAMPLE_PERIOD_MS 2
#define CLAP_REFRACTORY_US 180000
#define CLAP_SEQUENCE_TIMEOUT_US 900000

static const char *TAG = "audio_motor";

typedef enum {
    MOTION_STOP = 0,
    MOTION_FORWARD,
    MOTION_BACKWARD,
    MOTION_LEFT,
    MOTION_RIGHT,
} motion_t;

typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t channel;
    int baseline;
    int noise_peak;
    int threshold;
    int clap_count;
    int last_delta;
    int64_t last_clap_us;
} audio_detector_t;

static audio_detector_t g_audio = {
    .channel = (adc_channel_t) CONFIG_AUDIO_MIC_ADC_CHANNEL,
};

static volatile motion_t g_motion = MOTION_STOP;
static volatile int64_t g_override_until_us = 0;

static const char *motion_name(motion_t motion)
{
    switch (motion) {
    case MOTION_FORWARD:
        return "forward";
    case MOTION_BACKWARD:
        return "backward";
    case MOTION_LEFT:
        return "left";
    case MOTION_RIGHT:
        return "right";
    case MOTION_STOP:
    default:
        return "stop";
    }
}

static uint32_t speed_to_duty(int speed_percent)
{
    if (speed_percent < 0) {
        speed_percent = 0;
    }
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    return (uint32_t) ((PWM_MAX_DUTY * speed_percent) / 100);
}

static void motor_pwm_set(uint32_t duty_left, uint32_t duty_right)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_left));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_right));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

static void set_motion(motion_t motion, int speed_percent)
{
    const uint32_t duty = speed_to_duty(speed_percent);

    switch (motion) {
    case MOTION_FORWARD:
        gpio_set_level(PIN_IN1, 1);
        gpio_set_level(PIN_IN2, 0);
        gpio_set_level(PIN_IN3, 1);
        gpio_set_level(PIN_IN4, 0);
        motor_pwm_set(duty, duty);
        break;
    case MOTION_BACKWARD:
        gpio_set_level(PIN_IN1, 0);
        gpio_set_level(PIN_IN2, 1);
        gpio_set_level(PIN_IN3, 0);
        gpio_set_level(PIN_IN4, 1);
        motor_pwm_set(duty, duty);
        break;
    case MOTION_LEFT:
        gpio_set_level(PIN_IN1, 0);
        gpio_set_level(PIN_IN2, 1);
        gpio_set_level(PIN_IN3, 1);
        gpio_set_level(PIN_IN4, 0);
        motor_pwm_set(duty, duty);
        break;
    case MOTION_RIGHT:
        gpio_set_level(PIN_IN1, 1);
        gpio_set_level(PIN_IN2, 0);
        gpio_set_level(PIN_IN3, 0);
        gpio_set_level(PIN_IN4, 1);
        motor_pwm_set(duty, duty);
        break;
    case MOTION_STOP:
    default:
        gpio_set_level(PIN_IN1, 0);
        gpio_set_level(PIN_IN2, 0);
        gpio_set_level(PIN_IN3, 0);
        gpio_set_level(PIN_IN4, 0);
        motor_pwm_set(0, 0);
        break;
    }

    g_motion = motion;
    ESP_LOGI(TAG, "Motion=%s speed=%d%%", motion_name(motion), speed_percent);
}

static void motor_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << PIN_IN1) | (1ULL << PIN_IN2) |
                        (1ULL << PIN_IN3) | (1ULL << PIN_IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_config));

    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t left_pwm = {
        .gpio_num = PIN_ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&left_pwm));

    ledc_channel_config_t right_pwm = {
        .gpio_num = PIN_ENB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&right_pwm));

    set_motion(MOTION_STOP, 0);
}

static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

static void audio_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &g_audio.adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_audio.adc_handle, g_audio.channel, &channel_config));

    int raw = 0;
    int min_raw = 4095;
    int max_raw = 0;
    int64_t sum = 0;
    const int sample_count = 300;

    ESP_LOGI(TAG, "Calibrating microphone for %d samples. Keep quiet.", sample_count);
    for (int i = 0; i < sample_count; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(g_audio.adc_handle, g_audio.channel, &raw));
        if (raw < min_raw) {
            min_raw = raw;
        }
        if (raw > max_raw) {
            max_raw = raw;
        }
        sum += raw;
        vTaskDelay(pdMS_TO_TICKS(AUDIO_SAMPLE_PERIOD_MS));
    }

    g_audio.baseline = (int) (sum / sample_count);
    g_audio.noise_peak = (max_raw - min_raw) / 2;
    g_audio.threshold = CONFIG_AUDIO_CLAP_MIN_DELTA;
    if (g_audio.threshold < g_audio.noise_peak + 180) {
        g_audio.threshold = g_audio.noise_peak + 180;
    }

    ESP_LOGI(TAG, "Audio ready: baseline=%d noise_peak=%d threshold=%d adc1_ch=%d",
             g_audio.baseline, g_audio.noise_peak, g_audio.threshold, (int) g_audio.channel);
}

static void start_override(motion_t motion, int speed_percent, int duration_ms)
{
    set_motion(motion, speed_percent);
    g_override_until_us = esp_timer_get_time() + ((int64_t) duration_ms * 1000);
}

static void execute_audio_command(int clap_count)
{
    if (clap_count <= 0) {
        return;
    }

    const int64_t now_us = esp_timer_get_time();
    if (clap_count == 1) {
        printf("AUDIO,command=turn_left,claps=1,delta=%d,threshold=%d,t_ms=%" PRId64 "\n",
               g_audio.last_delta, g_audio.threshold, now_us / 1000);
        start_override(MOTION_LEFT, CONFIG_AUDIO_MOTOR_TURN_SPEED, CONFIG_AUDIO_LEFT_TURN_MS);
    } else {
        printf("AUDIO,command=retreat,claps=%d,delta=%d,threshold=%d,t_ms=%" PRId64 "\n",
               clap_count, g_audio.last_delta, g_audio.threshold, now_us / 1000);
        start_override(MOTION_BACKWARD, CONFIG_AUDIO_MOTOR_RETREAT_SPEED,
                       CONFIG_AUDIO_RETREAT_REVERSE_MS);
    }
}

static void audio_task(void *arg)
{
    (void) arg;
    int raw = 0;

    while (true) {
        if (adc_oneshot_read(g_audio.adc_handle, g_audio.channel, &raw) == ESP_OK) {
            g_audio.baseline = ((g_audio.baseline * 31) + raw) / 32;
            const int delta = abs_int(raw - g_audio.baseline);
            g_audio.last_delta = delta;
            const int64_t now_us = esp_timer_get_time();

            if (delta >= g_audio.threshold &&
                now_us - g_audio.last_clap_us >= CLAP_REFRACTORY_US) {
                g_audio.clap_count++;
                g_audio.last_clap_us = now_us;
                ESP_LOGI(TAG, "Clap candidate count=%d raw=%d baseline=%d delta=%d",
                         g_audio.clap_count, raw, g_audio.baseline, delta);
            }

            if (g_audio.clap_count > 0 &&
                now_us - g_audio.last_clap_us >= CLAP_SEQUENCE_TIMEOUT_US) {
                const int clap_count = g_audio.clap_count;
                g_audio.clap_count = 0;
                execute_audio_command(clap_count);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(AUDIO_SAMPLE_PERIOD_MS));
    }
}

static void control_task(void *arg)
{
    (void) arg;
    bool retreat_turn_pending = false;
    int64_t retreat_turn_at_us = 0;

    set_motion(MOTION_FORWARD, CONFIG_AUDIO_MOTOR_DEFAULT_SPEED);

    while (true) {
        const int64_t now_us = esp_timer_get_time();

        if (g_motion == MOTION_BACKWARD && g_override_until_us > 0 &&
            now_us >= g_override_until_us && !retreat_turn_pending) {
            retreat_turn_pending = true;
            retreat_turn_at_us = now_us;
            start_override(MOTION_RIGHT, CONFIG_AUDIO_MOTOR_RETREAT_SPEED,
                           CONFIG_AUDIO_RETREAT_TURN_MS);
        }

        if (retreat_turn_pending && now_us - retreat_turn_at_us > 100000 &&
            g_motion != MOTION_RIGHT) {
            retreat_turn_pending = false;
        }

        if (g_override_until_us > 0 && now_us >= g_override_until_us &&
            g_motion != MOTION_BACKWARD) {
            g_override_until_us = 0;
            retreat_turn_pending = false;
            set_motion(MOTION_FORWARD, CONFIG_AUDIO_MOTOR_DEFAULT_SPEED);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Robot sumo audio motor controller starting");
    ESP_LOGI(TAG, "Commands: 1 clap=turn_left, 2 claps=retreat");
    ESP_LOGI(TAG, "Pins: ENA=%d IN1=%d IN2=%d IN3=%d IN4=%d ENB=%d mic_adc1_ch=%d",
             (int) PIN_ENA, (int) PIN_IN1, (int) PIN_IN2, (int) PIN_IN3,
             (int) PIN_IN4, (int) PIN_ENB, CONFIG_AUDIO_MIC_ADC_CHANNEL);

    motor_init();
    audio_init();

    xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(control_task, "control_task", 4096, NULL, 5, NULL, 1);
}
