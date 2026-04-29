#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_random.h"

static const char *TAG = "LAB2_BENCH";


#define REPEAT_FLOAT_MACS 200000
#define REPEAT_INT8_MACS  400000
#define MEM_BYTES         (128 * 128 * 3)   // tamaño de una imagen RGB 128x128x3 = 49152 bytes
#define MEM_REPEAT        2000

volatile float sink_f32 = 0.0f;
volatile int32_t sink_i32 = 0;

typedef struct {
    const char *name;
    int params;
    int estimated_macs;
    int tflite_int8_bytes;
    float acc_presencia;
    float acc_cuadrante;
} model_info_t;

static model_info_t models[] = {
    {"red_1_mlp_pequena",   1573061,  1573024, 1577352, 0.896226f, 0.537736f},
    {"red_2_mlp_profunda",  6302085,  6301856, 6312552, 0.896226f, 0.537736f},
    {"red_3_cnn_tiny",        1749,  1179984,    7200, 0.896226f, 0.537736f},
    {"red_4_cnn_small",      17877, 14746784,   26776, 0.905660f, 0.594340f},
    {"red_5_cnn_depthwise",   4357,  1375480,   16408, 0.896226f, 0.537736f},
};

static void fill_float(float *a, int n)
{
    for (int i = 0; i < n; i++) {
        a[i] = (float)((i % 127) - 63) / 64.0f;
    }
}

static void fill_int8(int8_t *a, int n)
{
    for (int i = 0; i < n; i++) {
        a[i] = (int8_t)((i % 127) - 63);
    }
}

static double benchmark_float_mac_seconds_per_mac(void)
{
    float a = 1.2345f;
    float b = 2.3456f;
    float acc = 0.0f;

    int64_t t0 = esp_timer_get_time();

    for (int i = 0; i < REPEAT_FLOAT_MACS; i++) {
        acc += a * b;
        a += 0.000001f;
        b -= 0.000001f;
    }

    int64_t t1 = esp_timer_get_time();

    sink_f32 = acc;

    double elapsed_s = (double)(t1 - t0) / 1000000.0;
    return elapsed_s / (double)REPEAT_FLOAT_MACS;
}

static double benchmark_int8_mac_seconds_per_mac(void)
{
    int8_t a = 13;
    int8_t b = -7;
    int32_t acc = 0;

    int64_t t0 = esp_timer_get_time();

    for (int i = 0; i < REPEAT_INT8_MACS; i++) {
        acc += (int32_t)a * (int32_t)b;
        a = (int8_t)(a + 1);
        b = (int8_t)(b - 1);
    }

    int64_t t1 = esp_timer_get_time();

    sink_i32 = acc;

    double elapsed_s = (double)(t1 - t0) / 1000000.0;
    return elapsed_s / (double)REPEAT_INT8_MACS;
}

static double benchmark_memcpy_seconds_per_byte(void)
{
    uint8_t *src = heap_caps_malloc(MEM_BYTES, MALLOC_CAP_8BIT);
    uint8_t *dst = heap_caps_malloc(MEM_BYTES, MALLOC_CAP_8BIT);

    if (!src || !dst) {
        ESP_LOGE(TAG, "No hay memoria para benchmark_memcpy");
        if (src) free(src);
        if (dst) free(dst);
        return -1.0;
    }

    for (int i = 0; i < MEM_BYTES; i++) {
        src[i] = (uint8_t)(i & 0xFF);
    }

    int64_t t0 = esp_timer_get_time();

    for (int r = 0; r < MEM_REPEAT; r++) {
        memcpy(dst, src, MEM_BYTES);
    }

    int64_t t1 = esp_timer_get_time();

    sink_i32 = dst[0] + dst[MEM_BYTES - 1];

    free(src);
    free(dst);

    double elapsed_s = (double)(t1 - t0) / 1000000.0;
    double total_bytes = (double)MEM_BYTES * (double)MEM_REPEAT;
    return elapsed_s / total_bytes;
}

static double benchmark_mem_rw_seconds_per_byte(void)
{
    uint8_t *buf = heap_caps_malloc(MEM_BYTES, MALLOC_CAP_8BIT);

    if (!buf) {
        ESP_LOGE(TAG, "No hay memoria para benchmark_mem_rw");
        return -1.0;
    }

    memset(buf, 0, MEM_BYTES);

    int64_t t0 = esp_timer_get_time();

    uint32_t acc = 0;
    for (int r = 0; r < MEM_REPEAT; r++) {
        for (int i = 0; i < MEM_BYTES; i++) {
            buf[i] = (uint8_t)(buf[i] + 1);
            acc += buf[i];
        }
    }

    int64_t t1 = esp_timer_get_time();

    sink_i32 = acc;

    free(buf);

    double elapsed_s = (double)(t1 - t0) / 1000000.0;
    // Cada iteración hace lectura + escritura.
    double total_bytes_moved = (double)MEM_BYTES * (double)MEM_REPEAT * 2.0;
    return elapsed_s / total_bytes_moved;
}

static void print_model_estimations(double sec_per_mac_int8, double sec_per_byte_mem, float voltage, float current_active)
{
    printf("\n\n================ ESTIMACION POR RED ================\n");
    printf("Supuestos:\n");
    printf("- Se usa tiempo por MAC int8 medido en ESP32.\n");
    printf("- Movimiento de memoria aproximado = entrada + salida + pesos int8.\n");
    printf("- Entrada RGB 128x128x3 = %d bytes.\n", MEM_BYTES);
    printf("- Potencia activa estimada = V * I = %.2f V * %.3f A = %.3f W.\n",
           voltage, current_active, voltage * current_active);
    printf("\n");

    printf("%-24s,%12s,%12s,%12s,%12s,%12s,%12s,%12s\n",
           "modelo", "macs", "int8_bytes", "lat_mac_ms", "lat_mem_ms", "lat_total_ms", "fps", "energia_mJ");

    for (int i = 0; i < (int)(sizeof(models) / sizeof(models[0])); i++) {
        model_info_t m = models[i];

        double input_bytes = (double)MEM_BYTES;
        double output_bytes = 5.0; // 1 salida presencia + 4 salidas cuadrante aprox.
        double weight_bytes = (double)m.tflite_int8_bytes;
        double bytes_moved = input_bytes + output_bytes + weight_bytes;

        double lat_mac_s = (double)m.estimated_macs * sec_per_mac_int8;
        double lat_mem_s = bytes_moved * sec_per_byte_mem;
        double lat_total_s = lat_mac_s + lat_mem_s;

        double fps = 1.0 / lat_total_s;
        double power_w = voltage * current_active;
        double energy_j = power_w * lat_total_s;

        printf("%-24s,%12d,%12d,%12.3f,%12.3f,%12.3f,%12.2f,%12.3f\n",
               m.name,
               m.estimated_macs,
               m.tflite_int8_bytes,
               lat_mac_s * 1000.0,
               lat_mem_s * 1000.0,
               lat_total_s * 1000.0,
               fps,
               energy_j * 1000.0);
    }

}

void app_main(void)
{
    printf("\n\n=============================================\n");
    printf("LAB 2 - EJERCICIO 2.3 - BENCHMARK ESP32\n");
    printf("=============================================\n\n");

    float voltage = 3.3f;
    float current_active = 0.120f; // 120 mA como supuesto inicial

    ESP_LOGI(TAG, "Heap libre antes: %lu bytes", (unsigned long)esp_get_free_heap_size());

    double sec_per_mac_f32 = benchmark_float_mac_seconds_per_mac();
    double sec_per_mac_i8 = benchmark_int8_mac_seconds_per_mac();
    double sec_per_byte_memcpy = benchmark_memcpy_seconds_per_byte();
    double sec_per_byte_rw = benchmark_mem_rw_seconds_per_byte();

    printf("\n================ MEDICIONES BASE ================\n");
    printf("float32 MAC: %.9f us/MAC\n", sec_per_mac_f32 * 1000000.0);
    printf("int8 MAC:    %.9f us/MAC\n", sec_per_mac_i8 * 1000000.0);
    printf("memcpy:      %.9f us/byte\n", sec_per_byte_memcpy * 1000000.0);
    printf("mem R+W:     %.9f us/byte moved\n", sec_per_byte_rw * 1000000.0);

    printf("\nRendimiento aproximado:\n");
    printf("float32 MAC/s: %.2f\n", 1.0 / sec_per_mac_f32);
    printf("int8 MAC/s:    %.2f\n", 1.0 / sec_per_mac_i8);
    printf("memcpy MB/s:   %.2f\n", 1.0 / sec_per_byte_memcpy / (1024.0 * 1024.0));
    printf("mem R+W MB/s:  %.2f\n", 1.0 / sec_per_byte_rw / (1024.0 * 1024.0));

    print_model_estimations(sec_per_mac_i8, sec_per_byte_rw, voltage, current_active);

    ESP_LOGI(TAG, "Heap libre despues: %lu bytes", (unsigned long)esp_get_free_heap_size());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
