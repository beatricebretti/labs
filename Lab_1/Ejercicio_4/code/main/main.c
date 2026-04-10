#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"

#define VECTOR_SIZE 20 
#define TOTAL_BYTES_PROCESSED (VECTOR_SIZE * 3 * sizeof(int))

/* =========================================================
   MEMORIAS ESTÁTICAS (Static)
   ========================================================= */
DRAM_ATTR static int v_dram_s[VECTOR_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
DRAM_ATTR static int r_dram_s[VECTOR_SIZE];

static int v_iram_s[VECTOR_SIZE] __attribute__((section(".iram1.data"))) = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
DRAM_ATTR static int r_iram_s[VECTOR_SIZE];

RTC_DATA_ATTR static int v_rtc_s[VECTOR_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
RTC_DATA_ATTR static int r_rtc_s[VECTOR_SIZE];

const __attribute__((section(".rodata"))) int v_flash_s[VECTOR_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
DRAM_ATTR static int r_flash_s[VECTOR_SIZE];

/* =========================================================
   KERNEL
   ========================================================= */
void IRAM_ATTR multiply_kernel(int *res, const int *vec, int num) {
    for (int i = 0; i < VECTOR_SIZE; i++) {
        res[i] = vec[i] * num;
    }
}

void print_bench(const char* name, uint32_t start, uint32_t end) {
    uint32_t cycles = end - start;
    float cpb = (float)cycles / TOTAL_BYTES_PROCESSED;
    printf("%-15s | %-15lu | %-15.4f\n", name, cycles, cpb);
}

void benchmark_task(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(1500));
    printf("\n--- INICIO BENCHMARK COMPLETO ---\n\n");

    // MEMORIAS DINÁMICAS
    int *v_dram_d = (int *)malloc(VECTOR_SIZE * sizeof(int));
    int *r_dram_d = (int *)malloc(VECTOR_SIZE * sizeof(int));
    
    // IRAM DINÁMICA
    int *v_iram_d = (int *)heap_caps_malloc(VECTOR_SIZE * sizeof(int), MALLOC_CAP_INTERNAL | MALLOC_CAP_EXEC);
    int *r_iram_d = (int *)heap_caps_malloc(VECTOR_SIZE * sizeof(int), MALLOC_CAP_INTERNAL | MALLOC_CAP_EXEC);

    // PSRAM DINÁMICA
    int *v_psram_d = (int *)heap_caps_malloc(VECTOR_SIZE * sizeof(int), MALLOC_CAP_SPIRAM);
    int *r_psram_d = (int *)heap_caps_malloc(VECTOR_SIZE * sizeof(int), MALLOC_CAP_SPIRAM);

    // Inicializar dinámicas
    if(v_dram_d) for(int i=0; i<VECTOR_SIZE; i++) v_dram_d[i] = i+1;
    if(v_iram_d) for(int i=0; i<VECTOR_SIZE; i++) v_iram_d[i] = i+1;
    if(v_psram_d) for(int i=0; i<VECTOR_SIZE; i++) v_psram_d[i] = i+1;

    uint32_t s, e;
    printf("%-15s | %-15s | %-15s\n", "Tipo Memoria", "Ciclos Totales", "Ciclos/Byte");
    printf("----------------|-----------------|-----------------\n");

    // Pruebas Estáticas
    s = esp_cpu_get_cycle_count(); multiply_kernel(r_dram_s, v_dram_s, 5); e = esp_cpu_get_cycle_count();
    print_bench("STATIC_DRAM", s, e);

    s = esp_cpu_get_cycle_count(); multiply_kernel(r_iram_s, v_iram_s, 5); e = esp_cpu_get_cycle_count();
    print_bench("STATIC_IRAM", s, e);

    s = esp_cpu_get_cycle_count(); multiply_kernel(r_rtc_s, v_rtc_s, 5); e = esp_cpu_get_cycle_count();
    print_bench("STATIC_RTC", s, e);

    s = esp_cpu_get_cycle_count(); multiply_kernel(r_flash_s, v_flash_s, 5); e = esp_cpu_get_cycle_count();
    print_bench("STATIC_FLASH", s, e);

    // Pruebas Dinámicas
    if(v_dram_d) {
        s = esp_cpu_get_cycle_count(); multiply_kernel(r_dram_d, v_dram_d, 5); e = esp_cpu_get_cycle_count();
        print_bench("DYNAMIC_DRAM", s, e);
    }
    if(v_iram_d) {
        s = esp_cpu_get_cycle_count(); multiply_kernel(r_iram_d, v_iram_d, 5); e = esp_cpu_get_cycle_count();
        print_bench("DYNAMIC_IRAM", s, e);
    }
    if(v_psram_d) {
        s = esp_cpu_get_cycle_count(); multiply_kernel(r_psram_d, v_psram_d, 5); e = esp_cpu_get_cycle_count();
        print_bench("DYNAMIC_PSRAM", s, e);
    }

    // Libera
    free(v_dram_d); free(r_dram_d);
    heap_caps_free(v_iram_d); heap_caps_free(r_iram_d);
    heap_caps_free(v_psram_d); heap_caps_free(r_psram_d);

    printf("\n--- FIN BENCHMARK ---\n");
    vTaskDelete(NULL);
}

void app_main(void) {
    xTaskCreate(benchmark_task, "bench", 8192, NULL, 5, NULL);
}