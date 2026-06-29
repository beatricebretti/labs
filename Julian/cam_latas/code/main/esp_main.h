// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"

// ESP32-P4 doesn't support camera yet, so use CLI_ONLY_INFERENCE mode
#if CONFIG_IDF_TARGET_ESP32P4
// Can be defined for other targets as well, to skip camera usage
#define CLI_ONLY_INFERENCE 1
#endif

// Set to 1 for static-image inference, or 0 for live ESP32-CAM inference.
#define CLI_ONLY_INFERENCE 0

// Enable this to get cpu stats. Disabled for the project firmware to keep
// the live-camera build focused on behavior and telemetry.
// #define COLLECT_CPU_STATS 1

#if !CLI_ONLY_INFERENCE
// Enable display support if BSP is enabled in menuconfig
#if (CONFIG_TFLITE_USE_BSP)
#define DISPLAY_SUPPORT 1
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void run_inference(void *ptr);
#ifdef __cplusplus
}
#endif
