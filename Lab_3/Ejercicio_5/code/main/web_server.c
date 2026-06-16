#include "web_server.h"
#include "motor_control.h"
#include "audio_control.h"
#include "index_html.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "web_server";

volatile sumo_mode_t g_sumo_mode = MODE_MANUAL;
volatile sim_border_t g_sim_border = SIM_NONE;
volatile bool g_border_detected = false;
volatile border_zone_t g_detected_zone = ZONE_NONE;

// Audio variables
volatile sim_audio_t g_sim_audio = SIM_AUDIO_NONE;
volatile float g_sim_audio_freq = 0.0f;

static const char *sim_to_string(sim_border_t sim)
{
    switch (sim) {
        case SIM_FRONT: return "front";
        case SIM_LEFT:  return "left";
        case SIM_RIGHT: return "right";
        case SIM_NONE:
        default:        return "none";
    }
}

static const char *sim_audio_to_string(sim_audio_t sim)
{
    switch (sim) {
        case SIM_AUDIO_650:  return "650";
        case SIM_AUDIO_900:  return "900";
        case SIM_AUDIO_1150: return "1150";
        case SIM_AUDIO_1400: return "1400";
        case SIM_AUDIO_NONE:
        default:             return "none";
    }
}

static const char *zone_to_string(border_zone_t zone)
{
    switch (zone) {
        case ZONE_FRONT: return "front";
        case ZONE_LEFT:  return "left";
        case ZONE_RIGHT: return "right";
        case ZONE_NONE:
        default:         return "none";
    }
}

static const char *mode_to_string(sumo_mode_t mode)
{
    switch (mode) {
        case MODE_AUTO:  return "auto";
        case MODE_AUDIO: return "audio";
        case MODE_MANUAL:
        default:         return "manual";
    }
}

static esp_err_t send_json_status(httpd_req_t *req)
{
    char resp[512];
    snprintf(resp, sizeof(resp),
             "{\"ok\":true,\"command\":\"%s\",\"speed_percent\":%d,\"mode\":\"%s\",\"simulation\":\"%s\","
             "\"border_detected\":%s,\"detected_zone\":\"%s\","
             "\"audio_peak_freq\":%.2f,\"audio_peak_mag\":%.2f,\"audio_cmd\":\"%s\",\"audio_simulation\":\"%s\"}",
             command_to_string(g_current_cmd), g_speed_percent,
             mode_to_string(g_sumo_mode),
             sim_to_string(g_sim_border),
             g_border_detected ? "true" : "false",
             zone_to_string(g_detected_zone),
             g_audio_peak_freq,
             g_audio_peak_mag,
             command_to_string(g_audio_cmd),
             sim_audio_to_string(g_sim_audio));
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
    if (g_sumo_mode == MODE_MANUAL) {
        set_motion(CMD_FORWARD, g_speed_percent);
    }
    return send_json_status(req);
}

static esp_err_t backward_handler(httpd_req_t *req)
{
    if (g_sumo_mode == MODE_MANUAL) {
        set_motion(CMD_BACKWARD, g_speed_percent);
    }
    return send_json_status(req);
}

static esp_err_t left_handler(httpd_req_t *req)
{
    if (g_sumo_mode == MODE_MANUAL) {
        set_motion(CMD_LEFT, g_speed_percent);
    }
    return send_json_status(req);
}

static esp_err_t right_handler(httpd_req_t *req)
{
    if (g_sumo_mode == MODE_MANUAL) {
        set_motion(CMD_RIGHT, g_speed_percent);
    }
    return send_json_status(req);
}

static esp_err_t stop_handler(httpd_req_t *req)
{
    if (g_sumo_mode == MODE_MANUAL) {
        set_motion(CMD_STOP, g_speed_percent);
    }
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
        if (g_sumo_mode == MODE_MANUAL && g_current_cmd != CMD_STOP) {
            set_motion(g_current_cmd, g_speed_percent);
        }
    }
    return send_json_status(req);
}

static esp_err_t mode_handler(httpd_req_t *req)
{
    const char *prefix = "/api/mode/";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(req->uri, prefix, prefix_len) == 0) {
        const char *mode_str = req->uri + prefix_len;
        if (strcmp(mode_str, "auto") == 0) {
            g_sumo_mode = MODE_AUTO;
            ESP_LOGI(TAG, "Switched to AUTONOMOUS Sumo mode");
        } else if (strcmp(mode_str, "audio") == 0) {
            g_sumo_mode = MODE_AUDIO;
            set_motion(CMD_STOP, g_speed_percent);
            ESP_LOGI(TAG, "Switched to VOICE CONTROL (Audio) mode");
        } else {
            g_sumo_mode = MODE_MANUAL;
            set_motion(CMD_STOP, g_speed_percent);
            ESP_LOGI(TAG, "Switched to MANUAL mode");
        }
    }
    return send_json_status(req);
}

static esp_err_t simulate_handler(httpd_req_t *req)
{
    const char *prefix = "/api/simulate/";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(req->uri, prefix, prefix_len) == 0) {
        const char *sim_str = req->uri + prefix_len;
        if (strcmp(sim_str, "front") == 0) {
            g_sim_border = SIM_FRONT;
        } else if (strcmp(sim_str, "left") == 0) {
            g_sim_border = SIM_LEFT;
        } else if (strcmp(sim_str, "right") == 0) {
            g_sim_border = SIM_RIGHT;
        } else {
            g_sim_border = SIM_NONE;
        }
        ESP_LOGI(TAG, "Injected simulation border state: %s", sim_to_string(g_sim_border));
    }
    return send_json_status(req);
}

static esp_err_t simulate_audio_handler(httpd_req_t *req)
{
    const char *prefix = "/api/simulate_audio/";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(req->uri, prefix, prefix_len) == 0) {
        const char *sim_str = req->uri + prefix_len;
        if (strcmp(sim_str, "650") == 0) {
            g_sim_audio = SIM_AUDIO_650;
            g_sim_audio_freq = 650.0f;
        } else if (strcmp(sim_str, "900") == 0) {
            g_sim_audio = SIM_AUDIO_900;
            g_sim_audio_freq = 900.0f;
        } else if (strcmp(sim_str, "1150") == 0) {
            g_sim_audio = SIM_AUDIO_1150;
            g_sim_audio_freq = 1150.0f;
        } else if (strcmp(sim_str, "1400") == 0) {
            g_sim_audio = SIM_AUDIO_1400;
            g_sim_audio_freq = 1400.0f;
        } else {
            g_sim_audio = SIM_AUDIO_NONE;
            g_sim_audio_freq = 0.0f;
        }
        ESP_LOGI(TAG, "Injected simulation audio frequency: %s Hz", sim_audio_to_string(g_sim_audio));
    }
    return send_json_status(req);
}

httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t http_server = NULL;
    if (httpd_start(&http_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return NULL;
    }

    const httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
    };

    const httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_POST,
        .handler = status_handler,
    };

    const httpd_uri_t forward_uri = {
        .uri = "/api/forward",
        .method = HTTP_POST,
        .handler = forward_handler,
    };

    const httpd_uri_t backward_uri = {
        .uri = "/api/backward",
        .method = HTTP_POST,
        .handler = backward_handler,
    };

    const httpd_uri_t left_uri = {
        .uri = "/api/left",
        .method = HTTP_POST,
        .handler = left_handler,
    };

    const httpd_uri_t right_uri = {
        .uri = "/api/right",
        .method = HTTP_POST,
        .handler = right_handler,
    };

    const httpd_uri_t stop_uri = {
        .uri = "/api/stop",
        .method = HTTP_POST,
        .handler = stop_handler,
    };

    const httpd_uri_t speed_uri = {
        .uri = "/api/speed/*",
        .method = HTTP_POST,
        .handler = speed_handler,
    };

    const httpd_uri_t mode_uri = {
        .uri = "/api/mode/*",
        .method = HTTP_POST,
        .handler = mode_handler,
    };

    const httpd_uri_t simulate_uri = {
        .uri = "/api/simulate/*",
        .method = HTTP_POST,
        .handler = simulate_handler,
    };

    const httpd_uri_t simulate_audio_uri = {
        .uri = "/api/simulate_audio/*",
        .method = HTTP_POST,
        .handler = simulate_audio_handler,
    };

    httpd_register_uri_handler(http_server, &index_uri);
    httpd_register_uri_handler(http_server, &status_uri);
    httpd_register_uri_handler(http_server, &forward_uri);
    httpd_register_uri_handler(http_server, &backward_uri);
    httpd_register_uri_handler(http_server, &left_uri);
    httpd_register_uri_handler(http_server, &right_uri);
    httpd_register_uri_handler(http_server, &stop_uri);
    httpd_register_uri_handler(http_server, &speed_uri);
    httpd_register_uri_handler(http_server, &mode_uri);
    httpd_register_uri_handler(http_server, &simulate_uri);
    httpd_register_uri_handler(http_server, &simulate_audio_uri);

    ESP_LOGI(TAG, "Web server started on port 80!");
    return http_server;
}
