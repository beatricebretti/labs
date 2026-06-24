#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define PIN_ENA ((gpio_num_t) CONFIG_DUMMY_MOTOR_LEFT_ENABLE_GPIO)
#define PIN_IN1 ((gpio_num_t) CONFIG_DUMMY_MOTOR_LEFT_IN1_GPIO)
#define PIN_IN2 ((gpio_num_t) CONFIG_DUMMY_MOTOR_LEFT_IN2_GPIO)
#define PIN_IN3 ((gpio_num_t) CONFIG_DUMMY_MOTOR_RIGHT_IN1_GPIO)
#define PIN_IN4 ((gpio_num_t) CONFIG_DUMMY_MOTOR_RIGHT_IN2_GPIO)
#define PIN_ENB ((gpio_num_t) CONFIG_DUMMY_MOTOR_RIGHT_ENABLE_GPIO)

#define PWM_FREQUENCY_HZ 20000
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY 255
#define UART_BUFFER_SIZE 2048
#define LINE_BUFFER_SIZE 256

static const char *TAG = "dummy_ctrl";

typedef enum {
    MOTION_STOP = 0,
    MOTION_FORWARD,
    MOTION_BACKWARD,
    MOTION_LEFT,
    MOTION_RIGHT,
} motion_t;

typedef enum {
    ZONE_NONE = 0,
    ZONE_LEFT,
    ZONE_CENTER,
    ZONE_RIGHT,
} zone_t;

typedef enum {
    EDGE_ACTION_FORWARD = 0,
    EDGE_ACTION_TURN_LEFT,
    EDGE_ACTION_TURN_RIGHT,
    EDGE_ACTION_REVERSE,
} edge_action_t;

typedef struct {
    bool detected;
    zone_t zone;
    int best_score;
    int64_t updated_us;
} identifier_state_t;

typedef struct {
    bool detected;
    zone_t zone;
    edge_action_t action;
    int confidence;
    int64_t updated_us;
} edge_state_t;

typedef struct {
    uart_port_t uart_num;
    gpio_num_t rx_gpio;
    const char *name;
} uart_reader_config_t;

static identifier_state_t g_identifier = {0};
static edge_state_t g_edge = {0};
static volatile motion_t g_motion = MOTION_STOP;
static volatile int64_t g_override_until_us = 0;
static const char *g_mode = "boot";

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

static const char *zone_name(zone_t zone)
{
    switch (zone) {
    case ZONE_LEFT:
        return "left";
    case ZONE_CENTER:
        return "center";
    case ZONE_RIGHT:
        return "right";
    case ZONE_NONE:
    default:
        return "none";
    }
}

static const char *edge_action_name(edge_action_t action)
{
    switch (action) {
    case EDGE_ACTION_TURN_LEFT:
        return "turn_left";
    case EDGE_ACTION_TURN_RIGHT:
        return "turn_right";
    case EDGE_ACTION_REVERSE:
        return "reverse";
    case EDGE_ACTION_FORWARD:
    default:
        return "forward";
    }
}

static zone_t parse_zone_name(const char *value)
{
    if (strcmp(value, "left") == 0) {
        return ZONE_LEFT;
    }
    if (strcmp(value, "center") == 0) {
        return ZONE_CENTER;
    }
    if (strcmp(value, "right") == 0) {
        return ZONE_RIGHT;
    }
    return ZONE_NONE;
}

static edge_action_t parse_edge_action_name(const char *value)
{
    if (strcmp(value, "turn_left") == 0) {
        return EDGE_ACTION_TURN_LEFT;
    }
    if (strcmp(value, "turn_right") == 0) {
        return EDGE_ACTION_TURN_RIGHT;
    }
    if (strcmp(value, "reverse") == 0) {
        return EDGE_ACTION_REVERSE;
    }
    return EDGE_ACTION_FORWARD;
}

static bool parse_int_field(const char *line, const char *key, int *value)
{
    const char *field = strstr(line, key);
    if (field == NULL) {
        return false;
    }
    field += strlen(key);
    *value = atoi(field);
    return true;
}

static bool parse_text_field(const char *line, const char *key, char *value, size_t value_size)
{
    const char *field = strstr(line, key);
    if (field == NULL || value_size == 0) {
        return false;
    }
    field += strlen(key);

    size_t index = 0;
    while (field[index] != '\0' && field[index] != ',' &&
           field[index] != '\r' && field[index] != '\n' &&
           index + 1 < value_size) {
        value[index] = field[index];
        ++index;
    }
    value[index] = '\0';
    return index > 0;
}

static uint32_t speed_to_duty(int speed_percent)
{
    if (speed_percent < 0) {
        speed_percent = 0;
    }
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    return (uint32_t)((PWM_MAX_DUTY * speed_percent) / 100);
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

    if (g_motion != motion) {
        ESP_LOGI(TAG, "Motion=%s speed=%d%%", motion_name(motion), speed_percent);
    }
    g_motion = motion;
}

static void start_override(motion_t motion, int speed_percent, int duration_ms)
{
    set_motion(motion, speed_percent);
    g_override_until_us = esp_timer_get_time() + ((int64_t)duration_ms * 1000);
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

static void uart_init(uart_port_t uart_num, gpio_num_t rx_gpio)
{
    uart_config_t uart_config = {
        .baud_rate = CONFIG_DUMMY_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, rx_gpio,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void handle_identifier_line(const char *line)
{
    int detected = 0;
    int best_score = 0;
    char zone_text[16] = {0};

    if (!parse_int_field(line, "detected=", &detected)) {
        return;
    }
    parse_int_field(line, "best_score=", &best_score);
    parse_text_field(line, "zone=", zone_text, sizeof(zone_text));

    g_identifier.detected = detected != 0;
    g_identifier.zone = parse_zone_name(zone_text);
    g_identifier.best_score = best_score;
    g_identifier.updated_us = esp_timer_get_time();
}

static void handle_edge_line(const char *line)
{
    int edge = 0;
    int confidence = 0;
    char zone_text[16] = {0};
    char action_text[24] = {0};

    if (!parse_int_field(line, "edge=", &edge)) {
        return;
    }
    parse_int_field(line, "confidence=", &confidence);
    parse_text_field(line, "zone=", zone_text, sizeof(zone_text));
    parse_text_field(line, "action=", action_text, sizeof(action_text));

    g_edge.detected = edge != 0;
    g_edge.zone = parse_zone_name(zone_text);
    g_edge.action = parse_edge_action_name(action_text);
    g_edge.confidence = confidence;
    g_edge.updated_us = esp_timer_get_time();
}

static void handle_line(const char *line)
{
    if (strncmp(line, "IDENTIFIER,", 11) == 0) {
        handle_identifier_line(line);
    } else if (strncmp(line, "EDGE,", 5) == 0) {
        handle_edge_line(line);
    }
}

static void uart_reader_task(void *arg)
{
    const uart_reader_config_t *config = (const uart_reader_config_t *)arg;
    uint8_t byte = 0;
    char line[LINE_BUFFER_SIZE] = {0};
    size_t line_len = 0;

    ESP_LOGI(TAG, "Reading %s on UART%d RX GPIO%d",
             config->name, (int)config->uart_num, (int)config->rx_gpio);

    while (true) {
        const int read_len = uart_read_bytes(config->uart_num, &byte, 1, pdMS_TO_TICKS(50));
        if (read_len <= 0) {
            continue;
        }

        if (byte == '\n') {
            line[line_len] = '\0';
            if (line_len > 0) {
                handle_line(line);
            }
            line_len = 0;
            continue;
        }

        if (byte == '\r') {
            continue;
        }

        if (line_len + 1 < sizeof(line)) {
            line[line_len++] = (char)byte;
        } else {
            line_len = 0;
        }
    }
}

static bool is_recent(int64_t updated_us, int stale_ms, int64_t now_us)
{
    return updated_us > 0 && (now_us - updated_us) <= ((int64_t)stale_ms * 1000);
}

static void apply_edge_avoidance(edge_action_t action)
{
    g_mode = "avoid_edge";
    switch (action) {
    case EDGE_ACTION_TURN_LEFT:
        start_override(MOTION_LEFT, CONFIG_DUMMY_AVOID_SPEED, CONFIG_DUMMY_EDGE_TURN_MS);
        break;
    case EDGE_ACTION_TURN_RIGHT:
        start_override(MOTION_RIGHT, CONFIG_DUMMY_AVOID_SPEED, CONFIG_DUMMY_EDGE_TURN_MS);
        break;
    case EDGE_ACTION_REVERSE:
        start_override(MOTION_BACKWARD, CONFIG_DUMMY_AVOID_SPEED, CONFIG_DUMMY_REVERSE_MS);
        break;
    case EDGE_ACTION_FORWARD:
    default:
        set_motion(MOTION_FORWARD, CONFIG_DUMMY_ATTACK_SPEED);
        break;
    }
}

static void control_task(void *arg)
{
    (void)arg;
    int64_t last_status_us = 0;

    while (true) {
        const int64_t now_us = esp_timer_get_time();

        if (g_override_until_us > 0 && now_us < g_override_until_us) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (g_override_until_us > 0 && now_us >= g_override_until_us) {
            g_override_until_us = 0;
        }

        const bool edge_recent = is_recent(g_edge.updated_us, CONFIG_DUMMY_EDGE_STALE_MS, now_us);
        const bool identifier_recent =
            is_recent(g_identifier.updated_us, CONFIG_DUMMY_IDENTIFIER_STALE_MS, now_us);

        if (edge_recent && g_edge.detected) {
            apply_edge_avoidance(g_edge.action);
        } else if (identifier_recent && g_identifier.detected) {
            if (g_identifier.zone == ZONE_LEFT) {
                g_mode = "align_left";
                set_motion(MOTION_LEFT, CONFIG_DUMMY_ALIGN_SPEED);
            } else if (g_identifier.zone == ZONE_RIGHT) {
                g_mode = "align_right";
                set_motion(MOTION_RIGHT, CONFIG_DUMMY_ALIGN_SPEED);
            } else if (g_identifier.zone == ZONE_CENTER) {
                g_mode = "attack";
                set_motion(MOTION_FORWARD, CONFIG_DUMMY_ATTACK_SPEED);
            } else {
                g_mode = "search";
                set_motion(MOTION_LEFT, CONFIG_DUMMY_SEARCH_SPEED);
            }
        } else {
            g_mode = "search";
            set_motion(MOTION_LEFT, CONFIG_DUMMY_SEARCH_SPEED);
        }

        if (now_us - last_status_us >= 1000000) {
            last_status_us = now_us;
            printf("CTRL,mode=%s,motion=%s,identifier=%d,identifier_zone=%s,"
                   "identifier_score=%d,edge=%d,edge_zone=%s,edge_action=%s,"
                   "edge_confidence=%d,t_ms=%" PRId64 "\n",
                   g_mode,
                   motion_name(g_motion),
                   g_identifier.detected ? 1 : 0,
                   zone_name(g_identifier.zone),
                   g_identifier.best_score,
                   g_edge.detected ? 1 : 0,
                   zone_name(g_edge.zone),
                   edge_action_name(g_edge.action),
                   g_edge.confidence,
                   now_us / 1000);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    static const uart_reader_config_t identifier_uart = {
        .uart_num = UART_NUM_1,
        .rx_gpio = (gpio_num_t)CONFIG_DUMMY_IDENTIFIER_RX_GPIO,
        .name = "identifier camera",
    };
    static const uart_reader_config_t edge_uart = {
        .uart_num = UART_NUM_2,
        .rx_gpio = (gpio_num_t)CONFIG_DUMMY_EDGE_RX_GPIO,
        .name = "edge camera",
    };

    ESP_LOGI(TAG, "Dummy qualification controller starting");
    ESP_LOGI(TAG, "Identifier RX GPIO%d, edge RX GPIO%d, baud=%d",
             CONFIG_DUMMY_IDENTIFIER_RX_GPIO, CONFIG_DUMMY_EDGE_RX_GPIO,
             CONFIG_DUMMY_UART_BAUD);

    motor_init();
    uart_init(identifier_uart.uart_num, identifier_uart.rx_gpio);
    uart_init(edge_uart.uart_num, edge_uart.rx_gpio);

    xTaskCreatePinnedToCore(uart_reader_task, "identifier_uart", 4096,
                            (void *)&identifier_uart, 7, NULL, 0);
    xTaskCreatePinnedToCore(uart_reader_task, "edge_uart", 4096,
                            (void *)&edge_uart, 7, NULL, 0);
    xTaskCreatePinnedToCore(control_task, "control_task", 4096,
                            NULL, 6, NULL, 1);
}
