#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"

#define MAX_VECTOR_SIZE 512
#define NUM_TEST_SIZES 6

static const size_t TEST_SIZES[NUM_TEST_SIZES] = {20, 40, 80, 160, 320, 512};

/*
 * Por iteración:
 * - lectura de vector[i]   -> 4 bytes
 * - lectura de num         -> 4 bytes
 * - escritura de result[i] -> 4 bytes
 * Total = 12 bytes por elemento
 */
#define BYTES_PER_ELEMENT_ACCESSED (3 * sizeof(int))

typedef struct {
    const char *memory_name;
    size_t elements;
    size_t bytes_total;
    uint32_t cycles;
    double cycles_per_byte;
    int pass;
} benchmark_result_t;

/* =========================================================
   MEMORIAS ESTATICAS
   ========================================================= */

DRAM_ATTR static volatile int s_vector_dram[MAX_VECTOR_SIZE];
DRAM_ATTR static volatile int s_num_dram = 5;
DRAM_ATTR static volatile int s_result_dram[MAX_VECTOR_SIZE];

RTC_DATA_ATTR static volatile int s_vector_rtc[MAX_VECTOR_SIZE];
RTC_DATA_ATTR static volatile int s_num_rtc = 5;
RTC_DATA_ATTR static volatile int s_result_rtc[MAX_VECTOR_SIZE];

const __attribute__((section(".rodata"))) int s_vector_flash_ext[MAX_VECTOR_SIZE] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20
};
const __attribute__((section(".rodata"))) int s_num_flash_ext = 5;
DRAM_ATTR static volatile int s_result_flash[MAX_VECTOR_SIZE];

/* =========================================================
   HELPERS
   ========================================================= */

static inline uint32_t cycles_diff(uint32_t start, uint32_t end) {
    return (uint32_t)(end - start);
}

static void fill_sequence_volatile(volatile int *vec, size_t n) {
    for (size_t i = 0; i < n; i++) {
        vec[i] = (int)(i + 1);
    }
}

static void fill_sequence_normal(int *vec, size_t n) {
    for (size_t i = 0; i < n; i++) {
        vec[i] = (int)(i + 1);
    }
}

static int check_result_volatile(const volatile int *result, size_t n, int num) {
    for (size_t i = 0; i < n; i++) {
        int expected = (int)(i + 1) * num;
        if (result[i] != expected) {
            return 0;
        }
    }
    return 1;
}

static int check_result_normal(const int *result, size_t n, int num) {
    for (size_t i = 0; i < n; i++) {
        int expected = (int)(i + 1) * num;
        if (result[i] != expected) {
            return 0;
        }
    }
    return 1;
}

static int psram_available(void) {
    void *p = heap_caps_malloc(16, MALLOC_CAP_SPIRAM);
    if (p == NULL) {
        return 0;
    }
    free(p);
    return 1;
}

/* =========================================================
   KERNELS
   ========================================================= */

static uint32_t multiply_static_volatile(
    volatile int *result,
    volatile int *vector,
    volatile int *num,
    size_t n
) {
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (size_t i = 0; i < n; i++) {
        result[i] = vector[i] * (*num);
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    return cycles_diff(start_cycles, end_cycles);
}

static uint32_t multiply_flash_to_dram(
    volatile int *result,
    const int *vector_flash,
    const int *num_flash,
    size_t n
) {
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (size_t i = 0; i < n; i++) {
        result[i] = vector_flash[i] * (*num_flash);
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    return cycles_diff(start_cycles, end_cycles);
}

static uint32_t multiply_dynamic_normal(
    int *result,
    int *vector,
    int *num,
    size_t n
) {
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (size_t i = 0; i < n; i++) {
        result[i] = vector[i] * (*num);
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    return cycles_diff(start_cycles, end_cycles);
}

/* =========================================================
   BENCHMARKS
   ========================================================= */

static benchmark_result_t bench_static_dram(size_t n) {
    benchmark_result_t out = {0};

    fill_sequence_volatile(s_vector_dram, n);
    s_num_dram = 5;
    memset((void *)s_result_dram, 0, n * sizeof(int));

    out.memory_name = "STATIC_DRAM";
    out.elements = n;
    out.bytes_total = n * BYTES_PER_ELEMENT_ACCESSED;
    out.cycles = multiply_static_volatile(s_result_dram, s_vector_dram, &s_num_dram, n);
    out.cycles_per_byte = (double)out.cycles / (double)out.bytes_total;
    out.pass = check_result_volatile(s_result_dram, n, s_num_dram);

    return out;
}

static benchmark_result_t bench_static_rtc(size_t n) {
    benchmark_result_t out = {0};

    fill_sequence_volatile(s_vector_rtc, n);
    s_num_rtc = 5;
    memset((void *)s_result_rtc, 0, n * sizeof(int));

    out.memory_name = "STATIC_RTC";
    out.elements = n;
    out.bytes_total = n * BYTES_PER_ELEMENT_ACCESSED;
    out.cycles = multiply_static_volatile(s_result_rtc, s_vector_rtc, &s_num_rtc, n);
    out.cycles_per_byte = (double)out.cycles / (double)out.bytes_total;
    out.pass = check_result_volatile(s_result_rtc, n, s_num_rtc);

    return out;
}

static benchmark_result_t bench_static_flash(size_t n) {
    benchmark_result_t out = {0};

    memset((void *)s_result_flash, 0, n * sizeof(int));

    out.memory_name = "STATIC_FLASH";
    out.elements = n;
    out.bytes_total = n * BYTES_PER_ELEMENT_ACCESSED;
    out.cycles = multiply_flash_to_dram(s_result_flash, s_vector_flash_ext, &s_num_flash_ext, n);
    out.cycles_per_byte = (double)out.cycles / (double)out.bytes_total;
    out.pass = check_result_volatile(s_result_flash, n, s_num_flash_ext);

    return out;
}

static benchmark_result_t bench_dynamic_dram(size_t n) {
    benchmark_result_t out = {0};

    int *vector = (int *)malloc(n * sizeof(int));
    int *result = (int *)malloc(n * sizeof(int));
    int *num = (int *)malloc(sizeof(int));

    out.memory_name = "DYNAMIC_DRAM";
    out.elements = n;
    out.bytes_total = n * BYTES_PER_ELEMENT_ACCESSED;

    if (!vector || !result || !num) {
        out.cycles = 0;
        out.cycles_per_byte = 0.0;
        out.pass = 0;
        free(vector);
        free(result);
        free(num);
        return out;
    }

    fill_sequence_normal(vector, n);
    memset(result, 0, n * sizeof(int));
    *num = 5;

    out.cycles = multiply_dynamic_normal(result, vector, num, n);
    out.cycles_per_byte = (double)out.cycles / (double)out.bytes_total;
    out.pass = check_result_normal(result, n, *num);

    free(vector);
    free(result);
    free(num);

    return out;
}

static benchmark_result_t bench_dynamic_psram(size_t n) {
    benchmark_result_t out = {0};

    int *vector = (int *)heap_caps_malloc(n * sizeof(int), MALLOC_CAP_SPIRAM);
    int *result = (int *)heap_caps_malloc(n * sizeof(int), MALLOC_CAP_SPIRAM);
    int *num = (int *)heap_caps_malloc(sizeof(int), MALLOC_CAP_SPIRAM);

    out.memory_name = "DYNAMIC_PSRAM";
    out.elements = n;
    out.bytes_total = n * BYTES_PER_ELEMENT_ACCESSED;

    if (!vector || !result || !num) {
        out.cycles = 0;
        out.cycles_per_byte = 0.0;
        out.pass = 0;
        if (vector) free(vector);
        if (result) free(result);
        if (num) free(num);
        return out;
    }

    fill_sequence_normal(vector, n);
    memset(result, 0, n * sizeof(int));
    *num = 5;

    out.cycles = multiply_dynamic_normal(result, vector, num, n);
    out.cycles_per_byte = (double)out.cycles / (double)out.bytes_total;
    out.pass = check_result_normal(result, n, *num);

    free(vector);
    free(result);
    free(num);

    return out;
}

/* =========================================================
   PRINTS
   ========================================================= */

static void print_header(void) {
    printf("\n================ MEMORY ACCESS BENCHMARK ================\n");
    printf("CSV_HEADER,memory_type,elements,total_bytes_accessed,cycles,cycles_per_byte,pass\n");
}

static void print_result(const benchmark_result_t *r) {
    printf("CSV_DATA,%s,%u,%u,%" PRIu32 ",%.6f,%d\n",
           r->memory_name,
           (unsigned)r->elements,
           (unsigned)r->bytes_total,
           r->cycles,
           r->cycles_per_byte,
           r->pass);
}

/* =========================================================
   TASK
   ========================================================= */

static void benchmark_task(void *arg) {
    (void)arg;

    vTaskDelay(pdMS_TO_TICKS(1500));

    printf("\nInicio benchmark Ejercicio 4\n");

    int has_psram = psram_available();

    if (has_psram) {
        printf("PSRAM detectada y usable mediante MALLOC_CAP_SPIRAM.\n");
    } else {
        printf("PSRAM NO disponible o no habilitada.\n");
    }

    print_header();

    for (size_t i = 0; i < NUM_TEST_SIZES; i++) {
        size_t n = TEST_SIZES[i];

        benchmark_result_t r1 = bench_static_dram(n);
        benchmark_result_t r2 = bench_static_rtc(n);
        benchmark_result_t r3 = bench_static_flash(n);
        benchmark_result_t r4 = bench_dynamic_dram(n);

        print_result(&r1);
        print_result(&r2);
        print_result(&r3);
        print_result(&r4);

        if (has_psram) {
            benchmark_result_t r5 = bench_dynamic_psram(n);
            print_result(&r5);
        }
    }

    printf("\nFin benchmark Ejercicio 4\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    xTaskCreatePinnedToCore(
        benchmark_task,
        "benchmark_task",
        8192,
        NULL,
        5,
        NULL,
        0
    );
}