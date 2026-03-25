#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_cpu.h"

void app_main(void)
{
    volatile int var_1 = 233;
    volatile int var_2 = 128;
    volatile int result_0 = 0;
    volatile int result_1 = 0;
    volatile int result_2 = 0;
    volatile int result_3 = 0;
    volatile float result_4 = 0.0f;

    const int X = 1000000;

    int64_t start_time, end_time;
    uint32_t start_cycles, end_cycles;
    uint32_t total_cycles;
    double elapsed_us;
    double time_using_cycles_us;

    start_time = esp_timer_get_time();
    start_cycles = esp_cpu_get_cycle_count();

    for (int i = 0; i < X; i++) {
        result_0 = var_1 + var_2;
        result_1 = var_1 + 10;
        result_2 = var_1 % var_2;
        result_3 = var_1 * var_2;
        result_4 = (float)var_1 / (float)var_2;
    }

    end_cycles = esp_cpu_get_cycle_count();
    end_time = esp_timer_get_time();

    total_cycles = end_cycles - start_cycles;
    elapsed_us = (double)(end_time - start_time);

    // Ojo: ajusta esta frecuencia según tu config real del ESP32
    double cpu_freq_hz = 240000000.0;
    time_using_cycles_us = ((double)total_cycles / cpu_freq_hz) * 1e6;

    int pass = 1;
    if (result_0 != 361) pass = 0;
    if (result_1 != 243) pass = 0;
    if (result_2 != 105) pass = 0;
    if (result_3 != 29824) pass = 0;
    if (result_4 < 1.8203f || result_4 > 1.8204f) pass = 0;

    printf("Check results: %s\n", pass ? "PASS" : "FAIL");
    printf("Time measured with esp_timer: %.3f us\n", elapsed_us);
    printf("Total cycles: %" PRIu32 "\n", total_cycles);
    printf("Time using cycles: %.3f us\n", time_using_cycles_us);

    // CPI e instruction count:
    // No está calculado aquí porque necesitas una forma real de medir instrucciones.
    // Eso depende de cómo hagan el profiling en su curso/herramientas.
    printf("CPI: pendiente de instrumentacion de instruction count\n");
}