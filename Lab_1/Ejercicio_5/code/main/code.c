#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

// --- CONFIGURACIÓN DE HARDWARE ---
#define PIN_IN1 4
#define PIN_IN2 5
#define PIN_IN3 6
#define PIN_IN4 7
#define PIN_ENA 15
#define PIN_ENB 16
#define MIC_ADC_CHAN ADC_CHANNEL_0 

// --- PARÁMETROS DE PROCESAMIENTO (FFT/Frecuencia) ---
#define N_SAMPLES 512          
#define SAMPLING_FREQ 8000     
#define UMBRAL_AMPLITUD 500    

// --- CONFIGURACIÓN PWM ---
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          5000
#define VELOCIDAD_CRUCERO       700   

static adc_oneshot_unit_handle_t adc_handle;

void init_hw() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<PIN_IN1) | (1ULL<<PIN_IN2) | (1ULL<<PIN_IN3) | (1ULL<<PIN_IN4),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t chan_ena = { .speed_mode = LEDC_MODE, .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0,
                                       .intr_type = LEDC_INTR_DISABLE, .gpio_num = PIN_ENA, .duty = 0 };
    ledc_channel_config(&chan_ena);

    ledc_channel_config_t chan_enb = { .speed_mode = LEDC_MODE, .channel = LEDC_CHANNEL_1, .timer_sel = LEDC_TIMER_0,
                                       .intr_type = LEDC_INTR_DISABLE, .gpio_num = PIN_ENB, .duty = 0 };
    ledc_channel_config(&chan_enb);

    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc_handle);
    adc_oneshot_chan_cfg_t adc_config = {.bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12};
    adc_oneshot_config_channel(adc_handle, MIC_ADC_CHAN, &adc_config);
}

void mover(int in1, int in2, int in3, int in4, int vel) {
    gpio_set_level(PIN_IN1, in1); gpio_set_level(PIN_IN2, in2);
    gpio_set_level(PIN_IN3, in3); gpio_set_level(PIN_IN4, in4);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, vel);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, vel);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);
}

float estimar_frecuencia() {
    int muestras[N_SAMPLES];
    int max_val = 0, min_val = 4095;
    
    for (int i = 0; i < N_SAMPLES; i++) {
        adc_oneshot_read(adc_handle, MIC_ADC_CHAN, &muestras[i]);
        if (muestras[i] > max_val) max_val = muestras[i];
        if (muestras[i] < min_val) min_val = muestras[i];
        esp_rom_delay_us(1000000 / SAMPLING_FREQ);
    }

    if ((max_val - min_val) < UMBRAL_AMPLITUD) return 0;

    int cruces = 0;
    int promedio = (max_val + min_val) / 2;
    for (int i = 1; i < N_SAMPLES; i++) {
        if ((muestras[i-1] < promedio && muestras[i] >= promedio) ||
            (muestras[i-1] > promedio && muestras[i] <= promedio)) {
            cruces++;
        }
    }
    return (float)cruces * SAMPLING_FREQ / (2.0 * N_SAMPLES);
}

void app_main(void) {
    init_hw();
    printf("Sistema iniciado. Esperando tonos...\n");

    while (1) {
        float freq = estimar_frecuencia();

        if (freq > 0) {
            printf("Frecuencia: %.2f Hz -> ", freq);

            if (freq >= 400 && freq <= 600) {
                printf("ADELANTE\n");
                mover(1, 0, 1, 0, VELOCIDAD_CRUCERO);
            } else if (freq >= 900 && freq <= 1100) {
                printf("ATRÁS\n");
                mover(0, 1, 0, 1, VELOCIDAD_CRUCERO);
            } else if (freq >= 1400 && freq <= 1600) {
                printf("IZQUIERDA\n");
                mover(0, 1, 1, 0, VELOCIDAD_CRUCERO);
            } else if (freq >= 1900 && freq <= 2100) {
                printf("DERECHA\n");
                mover(1, 0, 0, 1, VELOCIDAD_CRUCERO);
            } else {
                printf("Tono no reconocido\n");
                mover(0, 0, 0, 0, 0);
            }
        } else {
            mover(0, 0, 0, 0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
