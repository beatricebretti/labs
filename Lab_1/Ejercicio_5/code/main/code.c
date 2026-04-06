#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// Pines de Motores
#define MOT_IZQ_1  4
#define MOT_IZQ_2  5
#define MOT_DER_1  6
#define MOT_DER_2  7

// Umbral de sonido /////////(Ajustar este valor según el ruido del ambiente)
#define UMBRAL_RUIDO 2200 

void configurar_motores() {
    gpio_reset_pin(MOT_IZQ_1); gpio_set_direction(MOT_IZQ_1, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOT_IZQ_2); gpio_set_direction(MOT_IZQ_2, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOT_DER_1); gpio_set_direction(MOT_DER_1, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOT_DER_2); gpio_set_direction(MOT_DER_2, GPIO_MODE_OUTPUT);
}

void mover_adelante() {
    gpio_set_level(MOT_IZQ_1, 1); gpio_set_level(MOT_IZQ_2, 0);
    gpio_set_level(MOT_DER_1, 1); gpio_set_level(MOT_DER_2, 0);
}

void detener() {
    gpio_set_level(MOT_IZQ_1, 0); gpio_set_level(MOT_IZQ_2, 0);
    gpio_set_level(MOT_DER_1, 0); gpio_set_level(MOT_DER_2, 0);
}

void app_main(void) {
    configurar_motores();

    // Configuración de Micrófonos 
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {.bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12};
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config); // GPIO 1
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &config); // GPIO 2

    int valor_mic_izq, valor_mic_der;

    while (1) {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &valor_mic_izq);
        adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &valor_mic_der);

        printf("Mics -> Izq: %d | Der: %d\n", valor_mic_izq, valor_mic_der);

        if (valor_mic_izq > UMBRAL_RUIDO || valor_mic_der > UMBRAL_RUIDO) {
            printf("¡Sonido detectado! Moviendo...\n");
            mover_adelante();
            vTaskDelay(pdMS_TO_TICKS(500)); // Se mueve medio segundo
        } else {
            detener();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
