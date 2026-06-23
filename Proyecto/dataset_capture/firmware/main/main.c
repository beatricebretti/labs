#include <stdio.h>
#include <string.h>

#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"

// AI-Thinker ESP32-CAM pins. This matches cam_identificador/code/sdkconfig.defaults.
#define CAMERA_PIN_PWDN 32
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK 0
#define CAMERA_PIN_SIOD 26
#define CAMERA_PIN_SIOC 27
#define CAMERA_PIN_D7 35
#define CAMERA_PIN_D6 34
#define CAMERA_PIN_D5 39
#define CAMERA_PIN_D4 36
#define CAMERA_PIN_D3 21
#define CAMERA_PIN_D2 19
#define CAMERA_PIN_D1 18
#define CAMERA_PIN_D0 5
#define CAMERA_PIN_VSYNC 25
#define CAMERA_PIN_HREF 23
#define CAMERA_PIN_PCLK 22

#define CAPTURE_INTERVAL_MS 2000
#define BASE64_INPUT_CHUNK 48
#define XCLK_FREQ_HZ 15000000

static const char *TAG = "dataset_capture";

static esp_err_t init_camera(void) {
  camera_config_t config;
  memset(&config, 0, sizeof(config));

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAMERA_PIN_D0;
  config.pin_d1 = CAMERA_PIN_D1;
  config.pin_d2 = CAMERA_PIN_D2;
  config.pin_d3 = CAMERA_PIN_D3;
  config.pin_d4 = CAMERA_PIN_D4;
  config.pin_d5 = CAMERA_PIN_D5;
  config.pin_d6 = CAMERA_PIN_D6;
  config.pin_d7 = CAMERA_PIN_D7;
  config.pin_xclk = CAMERA_PIN_XCLK;
  config.pin_pclk = CAMERA_PIN_PCLK;
  config.pin_vsync = CAMERA_PIN_VSYNC;
  config.pin_href = CAMERA_PIN_HREF;
  config.pin_sccb_sda = CAMERA_PIN_SIOD;
  config.pin_sccb_scl = CAMERA_PIN_SIOC;
  config.pin_pwdn = CAMERA_PIN_PWDN;
  config.pin_reset = CAMERA_PIN_RESET;
  config.xclk_freq_hz = XCLK_FREQ_HZ;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_96X96;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return err;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != NULL) {
    sensor->set_vflip(sensor, 1);
    if (sensor->id.PID == OV3660_PID) {
      sensor->set_brightness(sensor, 1);
      sensor->set_saturation(sensor, -2);
    }
  }

  return ESP_OK;
}

static void print_base64(const uint8_t *data, size_t len) {
  unsigned char encoded[80];
  size_t encoded_len = 0;

  for (size_t offset = 0; offset < len; offset += BASE64_INPUT_CHUNK) {
    size_t chunk_len = len - offset;
    if (chunk_len > BASE64_INPUT_CHUNK) {
      chunk_len = BASE64_INPUT_CHUNK;
    }

    int ret = mbedtls_base64_encode(
        encoded, sizeof(encoded) - 1, &encoded_len, data + offset, chunk_len);
    if (ret != 0) {
      printf("FRAME_ERROR base64_ret=%d\n", ret);
      return;
    }

    encoded[encoded_len] = '\0';
    printf("%s\n", encoded);
  }
}

void app_main(void) {
  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set(TAG, ESP_LOG_INFO);

  ESP_ERROR_CHECK(init_camera());
  printf("DATASET_CAPTURE_READY width=96 height=96 interval_ms=%d format=gray8\n",
         CAPTURE_INTERVAL_MS);
  fflush(stdout);

  uint32_t frame_id = 0;
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
      printf("FRAME_ERROR capture_failed\n");
      fflush(stdout);
      vTaskDelay(pdMS_TO_TICKS(CAPTURE_INTERVAL_MS));
      continue;
    }

    frame_id++;
    printf("FRAME_BEGIN id=%lu width=%u height=%u bytes=%u encoding=base64\n",
           (unsigned long) frame_id,
           (unsigned) fb->width,
           (unsigned) fb->height,
           (unsigned) fb->len);
    print_base64((const uint8_t *) fb->buf, fb->len);
    printf("FRAME_END id=%lu\n", (unsigned long) frame_id);
    fflush(stdout);

    esp_camera_fb_return(fb);
    vTaskDelay(pdMS_TO_TICKS(CAPTURE_INTERVAL_MS));
  }
}

