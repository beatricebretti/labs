#include "motor_control.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/ledc.h"

#define PWM_FREQUENCY_HZ    20000
#define PWM_RESOLUTION      LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY        255
#define DEFAULT_SPEED_PCT   70

static const char *TAG = "motor_control";

volatile motion_cmd_t g_current_cmd = CMD_STOP;
volatile int g_speed_percent = DEFAULT_SPEED_PCT;

const char *command_to_string(motion_cmd_t cmd)
{
    switch (cmd) {
        case CMD_FORWARD:  return "forward";
        case CMD_BACKWARD: return "backward";
        case CMD_LEFT:     return "left";
        case CMD_RIGHT:    return "right";
        case CMD_STOP:
        default:           return "stop";
    }
}

static uint32_t speed_percent_to_duty(int speed_percent)
{
    if (speed_percent < 0) speed_percent = 0;
    if (speed_percent > 100) speed_percent = 100;
    return (uint32_t)((PWM_MAX_DUTY * speed_percent) / 100);
}

static void motor_pwm_set(uint32_t duty_a, uint32_t duty_b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_a);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void set_motion(motion_cmd_t cmd, int speed_percent)
{
    g_speed_percent = speed_percent;
    const uint32_t duty = speed_percent_to_duty(g_speed_percent);

    switch (cmd) {
        case CMD_FORWARD:
            gpio_set_level(PIN_IN1, 1);
            gpio_set_level(PIN_IN2, 0);
            gpio_set_level(PIN_IN3, 1);
            gpio_set_level(PIN_IN4, 0);
            motor_pwm_set(duty, duty);
            break;

        case CMD_BACKWARD:
            gpio_set_level(PIN_IN1, 0);
            gpio_set_level(PIN_IN2, 1);
            gpio_set_level(PIN_IN3, 0);
            gpio_set_level(PIN_IN4, 1);
            motor_pwm_set(duty, duty);
            break;

        case CMD_LEFT:
            gpio_set_level(PIN_IN1, 0);
            gpio_set_level(PIN_IN2, 1);
            gpio_set_level(PIN_IN3, 1);
            gpio_set_level(PIN_IN4, 0);
            motor_pwm_set(duty, duty);
            break;

        case CMD_RIGHT:
            gpio_set_level(PIN_IN1, 1);
            gpio_set_level(PIN_IN2, 0);
            gpio_set_level(PIN_IN3, 0);
            gpio_set_level(PIN_IN4, 1);
            motor_pwm_set(duty, duty);
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

    g_current_cmd = cmd;
    ESP_LOGI(TAG, "Command set: %s, speed: %d%%", command_to_string(cmd), g_speed_percent);
}

void motor_init(void)
{
    // Configure IN1, IN2, IN3, IN4 as Outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_IN1) | (1ULL << PIN_IN2) |
                        (1ULL << PIN_IN3) | (1ULL << PIN_IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Configure LEDC Timer for PWM on ENA and ENB
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // Channel 0 -> ENA
    ledc_channel_config_t ch0 = {
        .gpio_num = PIN_ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    // Channel 1 -> ENB
    ledc_channel_config_t ch1 = {
        .gpio_num = PIN_ENB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ch0));
    ESP_ERROR_CHECK(ledc_channel_config(&ch1));

    set_motion(CMD_STOP, g_speed_percent);
    ESP_LOGI(TAG, "Motors initialized successfully!");
}
