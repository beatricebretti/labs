#include <stdio.h>
#include <stdlib.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "mbedtls/base64.h"

static const char *TAG = "camera_example";

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void app_main(void)
{
    camera_config_t config = {0};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG; // Output format: JPEG
    config.frame_size = FRAMESIZE_VGA;    // Picture size: VGA (640x480)
    config.jpeg_quality = 12;             // Quality: 0-63 (lower means higher quality)
    config.fb_count = 1;                  // Number of frame buffers

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    
    ESP_LOGI(TAG, "Camera initialized successfully!");

    // --- TAKE A PICTURE ---
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return;
    }

    ESP_LOGI(TAG, "Picture taken! Image size: %zu bytes", fb->len);

    // --- CONVERT AND PRINT AS BASE64 ---
    size_t output_len;
    size_t base64_buf_len = ((fb->len + 2) / 3) * 4 + 1;
    unsigned char *base64_buf = calloc(1, base64_buf_len);
    
    if (base64_buf) {
        mbedtls_base64_encode(base64_buf, base64_buf_len, &output_len, fb->buf, fb->len);
        printf("\n========== COPY THE TEXT BELOW ==========\n");
        printf("data:image/jpeg;base64,");
        // Print in chunks to avoid terminal line truncation limits
        for (size_t i = 0; i < output_len; i += 500) {
            printf("%.*s", (int)((output_len - i > 500) ? 500 : output_len - i), base64_buf + i);
        }
        printf("\n=========================================\n\n");
        free(base64_buf);
        ESP_LOGI(TAG, "Paste the text above directly into your browser's address bar to see the image!");
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for Base64 buffer");
    }

    // Return the frame buffer to the camera driver so it can be reused
    esp_camera_fb_return(fb);
}
