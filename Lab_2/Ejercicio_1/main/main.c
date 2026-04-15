#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "index_html.h"

#define WIFI_SSID           "ESP32-Autito"
#define WIFI_PASS           "autito123"
#define WIFI_CHANNEL        6
#define MAX_STA_CONN        4

#define PIN_ENA             GPIO_NUM_15
#define PIN_IN1             GPIO_NUM_4
#define PIN_IN2             GPIO_NUM_5
#define PIN_IN3             GPIO_NUM_6
#define PIN_IN4             GPIO_NUM_7
#define PIN_ENB             GPIO_NUM_16

#define PWM_FREQUENCY_HZ    20000
#define PWM_RESOLUTION      LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY        255
#define DEFAULT_SPEED_PCT   70

static const char *TAG = "web_car";

typedef enum {
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT,
} motion_cmd_t;

static httpd_handle_t server = NULL;
static volatile motion_cmd_t g_current_cmd = CMD_STOP;
static volatile int g_speed_percent = DEFAULT_SPEED_PCT;

static const char *command_to_string(motion_cmd_t cmd)
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

static void set_motion(motion_cmd_t cmd)
{
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
    ESP_LOGI(TAG, "Command=%s, speed=%d%%", command_to_string(cmd), g_speed_percent);
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
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch0 = {
        .gpio_num = PIN_ENA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

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

    set_motion(CMD_STOP);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Client joined, AID=%d", event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Client left, AID=%d", event->aid);
    }
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP started. SSID=%s password=%s", WIFI_SSID, WIFI_PASS);
    ESP_LOGI(TAG, "Open http://192.168.4.1 once connected c:");
}

static esp_err_t send_json_status(httpd_req_t *req)
{
    char resp[128];
    snprintf(resp, sizeof(resp),
             "{\"ok\":true,\"command\":\"%s\",\"speed_percent\":%d}",
             command_to_string(g_current_cmd), g_speed_percent);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, resp);
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    return send_json_status(req);
}

static esp_err_t forward_handler(httpd_req_t *req)
{
    set_motion(CMD_FORWARD);
    return send_json_status(req);
}

static esp_err_t backward_handler(httpd_req_t *req)
{
    set_motion(CMD_BACKWARD);
    return send_json_status(req);
}

static esp_err_t left_handler(httpd_req_t *req)
{
    set_motion(CMD_LEFT);
    return send_json_status(req);
}

static esp_err_t right_handler(httpd_req_t *req)
{
    set_motion(CMD_RIGHT);
    return send_json_status(req);
}

static esp_err_t stop_handler(httpd_req_t *req)
{
    set_motion(CMD_STOP);
    return send_json_status(req);
}

static esp_err_t speed_handler(httpd_req_t *req)
{
    const char *prefix = "/api/speed/";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(req->uri, prefix, prefix_len) == 0) {
        const char *num = req->uri + prefix_len;
        int value = atoi(num);
        if (value < 30) value = 30;
        if (value > 100) value = 100;
        g_speed_percent = value;
        if (g_current_cmd != CMD_STOP) {
            set_motion(g_current_cmd);
        }
    }
    return send_json_status(req);
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t http_server = NULL;
    if (httpd_start(&http_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server :c");
        return NULL;
    }

    const httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_POST,
        .handler = status_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t forward_uri = {
        .uri = "/api/forward",
        .method = HTTP_POST,
        .handler = forward_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t backward_uri = {
        .uri = "/api/backward",
        .method = HTTP_POST,
        .handler = backward_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t left_uri = {
        .uri = "/api/left",
        .method = HTTP_POST,
        .handler = left_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t right_uri = {
        .uri = "/api/right",
        .method = HTTP_POST,
        .handler = right_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t stop_uri = {
        .uri = "/api/stop",
        .method = HTTP_POST,
        .handler = stop_handler,
        .user_ctx = NULL,
    };

    const httpd_uri_t speed_uri = {
        .uri = "/api/speed/*",
        .method = HTTP_POST,
        .handler = speed_handler,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &index_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &status_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &forward_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &backward_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &left_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &right_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &stop_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &speed_uri));

    ESP_LOGI(TAG, "Web server started :D");
    return http_server;
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    motor_init();
    wifi_init_softap();
    server = start_webserver();

    if (server == NULL) {
        ESP_LOGE(TAG, "Server did not start D:");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
