/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "detection_responder.h"
#include "model_settings.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "esp_log.h"

#include <cstdio>

#include "esp_main.h"
#if DISPLAY_SUPPORT
#include "image_provider.h"
#include "bsp/esp-bsp.h"

#define IMG_WD (96 * 2)
#define IMG_HT (96 * 2)

static lv_obj_t *camera_canvas = NULL;
static lv_obj_t *can_indicator = NULL;
static lv_obj_t *label = NULL;

void create_gui(void)
{
  bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = {
      .task_priority = CONFIG_BSP_DISPLAY_LVGL_TASK_PRIORITY,
      .task_stack = 6144,
      .task_affinity = 1,
      .task_max_sleep_ms = CONFIG_BSP_DISPLAY_LVGL_MAX_SLEEP,
      .timer_period_ms = CONFIG_BSP_DISPLAY_LVGL_TICK,
    },
    .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
    .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
    .flags = {
      .buff_dma = false,
      .buff_spiram = true,
    }
  };
  bsp_display_start_with_config(&cfg);
  bsp_display_backlight_on();

  bsp_display_lock(0);
  camera_canvas = lv_canvas_create(lv_scr_act());
  assert(camera_canvas);
  lv_obj_align(camera_canvas, LV_ALIGN_TOP_MID, 0, 0);

  can_indicator = lv_led_create(lv_scr_act());
  assert(can_indicator);
  lv_obj_align(can_indicator, LV_ALIGN_BOTTOM_MID, -70, 0);
  lv_led_set_color(can_indicator, lv_palette_main(LV_PALETTE_GREEN));

  label = lv_label_create(lv_scr_act());
  assert(label);
  lv_label_set_text_static(label, "Can detected");
  lv_obj_align_to(label, can_indicator, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
  bsp_display_unlock();
}
#endif // DISPLAY_SUPPORT

static const char *TAG = "can_detect";
static bool last_can_detected = false;

namespace {
constexpr int kCanThresholdPercent = 65;

int ScoreToPercent(float score) {
  int score_int = (score * 100.0f) + 0.5f;
  if (score_int < 0) {
    return 0;
  }
  if (score_int > 100) {
    return 100;
  }
  return score_int;
}
}  // namespace

void RespondToDetection(float can_score, float no_can_score) {
  const int can_score_int = ScoreToPercent(can_score);
  const int no_can_score_int = ScoreToPercent(no_can_score);
  const bool can_detected = can_score_int >= kCanThresholdPercent &&
                            can_score_int >= no_can_score_int;

#if DISPLAY_SUPPORT
  if (!camera_canvas) {
    create_gui();
  }

  uint16_t *buf = (uint16_t *) image_provider_get_display_buf();

  bsp_display_lock(0);
  if (!can_detected) {
    lv_led_off(can_indicator);
  } else {
    lv_led_on(can_indicator);
  }

  lv_canvas_set_buffer(camera_canvas, buf, IMG_WD, IMG_HT, LV_COLOR_FORMAT_RGB565);
  bsp_display_unlock();
#endif // DISPLAY_SUPPORT

  if (can_detected && !last_can_detected) {
    ESP_LOGI(TAG, "CAN DETECTED! score=%d%%", can_score_int);
  } else if (!can_detected && last_can_detected) {
    ESP_LOGI(TAG, "No can detected (no_can=%d%% can=%d%%)",
             no_can_score_int, can_score_int);
  }
  last_can_detected = can_detected;

  printf("CAN,detected=%d,can_score=%d,no_can_score=%d,threshold=%d\n",
         can_detected ? 1 : 0,
         can_score_int,
         no_can_score_int,
         kCanThresholdPercent);

  MicroPrintf("can:%d%%, no_can:%d%%", can_score_int, no_can_score_int);
}
