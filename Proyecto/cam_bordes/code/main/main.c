#include <inttypes.h>
#include <stdio.h>

#include "app_camera_esp.h"
#include "edge_detector.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "edge_main";

void app_main(void)
{
    if (app_camera_init() != 0) {
        ESP_LOGE(TAG, "Cannot start edge detector without camera");
        return;
    }

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb == NULL) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(pdMS_TO_TICKS(CONFIG_EDGE_LOOP_PERIOD_MS));
            continue;
        }

        edge_result_t result;
        edge_detector_analyze((const uint8_t *)fb->buf, fb->width, fb->height,
                              &result);

        const int64_t now_ms = esp_timer_get_time() / 1000;
        printf("EDGE,t_ms=%" PRId64 ",edge=%d,zone=%s,action=%s,steer=%d,"
               "confidence=%u,distance_px=%d,left=%u,center=%u,right=%u,"
               "bottom=%u,row_peak=%u,thr=%u\n",
               now_ms,
               result.edge_detected ? 1 : 0,
               edge_zone_name(result.zone),
               edge_action_name(result.action),
               result.steer,
               result.confidence,
               result.distance_px,
               result.left_hits,
               result.center_hits,
               result.right_hits,
               result.bottom_hits,
               result.row_peak_hits,
               result.gradient_threshold);

        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_EDGE_LOOP_PERIOD_MS));
    }
}
