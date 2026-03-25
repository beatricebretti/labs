#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_cpu.h"
#include "esp_system.h"

#ifndef CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ 240
#endif

/*
 * IMPORTANTE:
 * ESP-IDF da un contador de ciclos real con esp_cpu_get_cycle_count().
 * Para "instruction count" no estamos usando un contador hardware portable
 * en este ejemplo, así que usamos una estimación estática por iteración.
 *
 * Puedes refinar este valor mirando el ensamblado de run_full_kernel()
 * con objdump y reemplazar LOOP_INSTR_ESTIMATE.
 */
#define LOOP_INSTR_ESTIMATE 18ULL

static const uint32_t X_VALUES[] = {
    10,
    100,
    1000,
    10000,
    100000,
    1000000
};

typedef struct {
    uint32_t x;
    uint32_t cycles;
    int64_t time_us;
    double time_from_cycles_us;
    uint64_t estimated_instructions;
    double estimated_cpi;
    int pass;
    int32_t result_0;
    int32_t result_1;
    int32_t result_2;
    int32_t result_3;
    float result_4;
} benchmark_result_t;

typedef struct {
    const char *name;
    uint32_t cycles;
    int64_t time_us;
} op_profile_t;

/* ---------------------------------------------------------
   Helpers
--------------------------------------------------------- */

static inline uint32_t cycles_diff(uint32_t start, uint32_t end) {
    return (uint32_t)(end - start);
}

static inline int check_results(
    int32_t result_0,
    int32_t result_1,
    int32_t result_2,
    int32_t result_3,
    float result_4
) {
    const int32_t exp_0 = 361;       // 233 + 128
    const int32_t exp_1 = 243;       // 233 + 10
    const int32_t exp_2 = 105;       // 233 % 128
    const int32_t exp_3 = 29824;     // 233 * 128
    const float   exp_4 = 233.0f / 128.0f;

    if (result_0 != exp_0) return 0;
    if (result_1 != exp_1) return 0;
    if (result_2 != exp_2) return 0;
    if (result_3 != exp_3) return 0;
    if (fabsf(result_4 - exp_4) > 0.0001f) return 0;

    return 1;
}

/* ---------------------------------------------------------
   Kernel principal: replica el pseudocódigo del enunciado
--------------------------------------------------------- */

static benchmark_result_t run_full_kernel(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t var_2 = 128;

    volatile int32_t result_0 = 0;
    volatile int32_t result_1 = 0;
    volatile int32_t result_2 = 0;
    volatile int32_t result_3 = 0;
    volatile float   result_4 = 0.0f;

    benchmark_result_t out = {0};

    const double cpu_hz = (double)CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000000.0;

    /* init_time(), init_cycle_count() */
    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result_0 = var_1 + var_2;
        result_1 = var_1 + 10;
        result_2 = var_1 % var_2;
        result_3 = var_1 * var_2;
        result_4 = (float)var_1 / (float)var_2;
    }

    /* end_time(), end_cycle_count() */
    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    uint32_t total_cycles = cycles_diff(start_cycles, end_cycles);
    int64_t total_time_us = end_time - start_time;

    /*
     * Estimación de instrucciones:
     * - asumimos LOOP_INSTR_ESTIMATE instrucciones máquina por iteración
     * - esto es una aproximación para poder graficar CPI
     * - puedes refinarla con objdump
     */
    uint64_t estimated_instructions = (uint64_t)x * LOOP_INSTR_ESTIMATE;
    double estimated_cpi = 0.0;
    if (estimated_instructions > 0) {
        estimated_cpi = (double)total_cycles / (double)estimated_instructions;
    }

    out.x = x;
    out.cycles = total_cycles;
    out.time_us = total_time_us;
    out.time_from_cycles_us = ((double)total_cycles / cpu_hz) * 1e6;
    out.estimated_instructions = estimated_instructions;
    out.estimated_cpi = estimated_cpi;
    out.pass = check_results(result_0, result_1, result_2, result_3, result_4);
    out.result_0 = result_0;
    out.result_1 = result_1;
    out.result_2 = result_2;
    out.result_3 = result_3;
    out.result_4 = result_4;

    return out;
}

/* ---------------------------------------------------------
   Profiling por operación
--------------------------------------------------------- */

static op_profile_t profile_add_var2(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t var_2 = 128;
    volatile int32_t result = 0;

    op_profile_t out = {.name = "result_0 = var_1 + var_2"};

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result = var_1 + var_2;
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    out.cycles = cycles_diff(start_cycles, end_cycles);
    out.time_us = end_time - start_time;

    (void)result;
    return out;
}

static op_profile_t profile_add_const(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t result = 0;

    op_profile_t out = {.name = "result_1 = var_1 + 10"};

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result = var_1 + 10;
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    out.cycles = cycles_diff(start_cycles, end_cycles);
    out.time_us = end_time - start_time;

    (void)result;
    return out;
}

static op_profile_t profile_mod(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t var_2 = 128;
    volatile int32_t result = 0;

    op_profile_t out = {.name = "result_2 = var_1 % var_2"};

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result = var_1 % var_2;
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    out.cycles = cycles_diff(start_cycles, end_cycles);
    out.time_us = end_time - start_time;

    (void)result;
    return out;
}

static op_profile_t profile_mul(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t var_2 = 128;
    volatile int32_t result = 0;

    op_profile_t out = {.name = "result_3 = var_1 * var_2"};

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result = var_1 * var_2;
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    out.cycles = cycles_diff(start_cycles, end_cycles);
    out.time_us = end_time - start_time;

    (void)result;
    return out;
}

static op_profile_t profile_div(uint32_t x)
{
    volatile int32_t var_1 = 233;
    volatile int32_t var_2 = 128;
    volatile float result = 0.0f;

    op_profile_t out = {.name = "result_4 = var_1 / var_2"};

    int64_t start_time = esp_timer_get_time();
    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (uint32_t i = 0; i < x; i++) {
        result = (float)var_1 / (float)var_2;
    }

    uint32_t end_cycles = esp_cpu_get_cycle_count();
    int64_t end_time = esp_timer_get_time();

    out.cycles = cycles_diff(start_cycles, end_cycles);
    out.time_us = end_time - start_time;

    (void)result;
    return out;
}

static void print_benchmark_header(void)
{
    printf("\n================ FULL BENCHMARK ================\n");
    printf("CPU_FREQ_MHZ,%d\n", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("CSV_HEADER,x,pass,time_us,cycles,time_from_cycles_us,estimated_instructions,estimated_cpi,result_0,result_1,result_2,result_3,result_4\n");
}

static void print_benchmark_result(const benchmark_result_t *r)
{
    printf("CSV_DATA,%" PRIu32 ",%d,%" PRIi64 ",%" PRIu32 ",%.3f,%" PRIu64 ",%.6f,%" PRIi32 ",%" PRIi32 ",%" PRIi32 ",%" PRIi32 ",%.6f\n",
           r->x,
           r->pass,
           r->time_us,
           r->cycles,
           r->time_from_cycles_us,
           r->estimated_instructions,
           r->estimated_cpi,
           r->result_0,
           r->result_1,
           r->result_2,
           r->result_3,
           r->result_4);
}

static void print_profile_header(void)
{
    printf("\n================ OPERATION PROFILE ================\n");
    printf("PROFILE_HEADER,operation,cycles,time_us\n");
}

static void print_profile_result(const op_profile_t *p)
{
    printf("PROFILE_DATA,%s,%" PRIu32 ",%" PRIi64 "\n",
           p->name,
           p->cycles,
           p->time_us);
}

static void benchmark_task(void *arg)
{
    (void)arg;

    vTaskDelay(pdMS_TO_TICKS(1500));

    printf("\nInicio benchmark Ejercicio 3\n");
    printf("Frecuencia configurada: %d MHz\n", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("LOOP_INSTR_ESTIMATE=%llu\n", (unsigned long long)LOOP_INSTR_ESTIMATE);

    /* 1) Barrido de X para tiempo/ciclos/CPI */
    print_benchmark_header();
    for (size_t i = 0; i < sizeof(X_VALUES) / sizeof(X_VALUES[0]); i++) {
        benchmark_result_t r = run_full_kernel(X_VALUES[i]);
        print_benchmark_result(&r);
    }

    /* 2) Profiling de operaciones
       Usa un X grande para que la diferencia sea más visible */
    uint32_t profile_x = 1000000;

    op_profile_t p_add_var2 = profile_add_var2(profile_x);
    op_profile_t p_add_const = profile_add_const(profile_x);
    op_profile_t p_mod      = profile_mod(profile_x);
    op_profile_t p_mul      = profile_mul(profile_x);
    op_profile_t p_div      = profile_div(profile_x);

    print_profile_header();
    print_profile_result(&p_add_var2);
    print_profile_result(&p_add_const);
    print_profile_result(&p_mod);
    print_profile_result(&p_mul);
    print_profile_result(&p_div);

    /* Determinar cuello de botella */
    op_profile_t profiles[] = {
        p_add_var2,
        p_add_const,
        p_mod,
        p_mul,
        p_div
    };

    size_t max_idx = 0;
    for (size_t i = 1; i < sizeof(profiles) / sizeof(profiles[0]); i++) {
        if (profiles[i].cycles > profiles[max_idx].cycles) {
            max_idx = i;
        }
    }

    printf("\nBOTTLENECK,%s,%" PRIu32 " cycles,%" PRIi64 " us\n",
           profiles[max_idx].name,
           profiles[max_idx].cycles,
           profiles[max_idx].time_us);

    printf("\nFin benchmark Ejercicio 3\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(
        benchmark_task,
        "benchmark_task",
        4096,
        NULL,
        5,
        NULL,
        0
    );
}