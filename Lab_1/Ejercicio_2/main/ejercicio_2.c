#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "se2_1_image.h"

#define OUT_WIDTH  96
#define OUT_HEIGHT 96

// Sobel Kernels
const int Gx[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

const int Gy[3][3] = {
    { 1,  2,  1},
    { 0,  0,  0},
    {-1, -2, -1}
};

void nearest_neighbor_resize(const uint8_t *input, int in_w, int in_h, uint8_t *output, int out_w, int out_h) {
    for (int y = 0; y < out_h; y++) {
        for (int x = 0; x < out_w; x++) {
            int src_x = (x * in_w) / out_w;
            int src_y = (y * in_h) / out_h;
            output[y * out_w + x] = input[src_y * in_w + src_x];
        }
    }
}

void apply_sobel(const uint8_t *input, uint8_t *output, int width, int height) {
    // Start from 1 and end at width/height - 1 to avoid out-of-bounds on the image borders
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0;
            int sumY = 0;

            // Apply the 3x3 kernels
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int pixel = input[(y + i) * width + (x + j)];
                    sumX += pixel * Gx[i + 1][j + 1];
                    sumY += pixel * Gy[i + 1][j + 1];
                }
            }

            // Calculate the approximate magnitude (faster than sqrt for microcontrollers)
            int magnitude = abs(sumX) + abs(sumY);

            // Clamp the value to a standard 8-bit grayscale range (0-255)
            if (magnitude > 255) {
                magnitude = 255;
            } else if (magnitude < 0) {
                magnitude = 0;
            }

            output[y * width + x] = (uint8_t)magnitude;
        }
    }
}

void app_main(void)
{
    printf("Resizing SE2-1 image from %dx%d to %dx%d...\n", IMG_WIDTH, IMG_HEIGHT, OUT_WIDTH, OUT_HEIGHT);

    // Allocate memory for the resized input image
    uint8_t *resized_image = (uint8_t *)calloc(OUT_WIDTH * OUT_HEIGHT, sizeof(uint8_t));
    if (resized_image == NULL) {
        printf("Failed to allocate memory for resized image!\n");
        return;
    }

    nearest_neighbor_resize(se2_1_image, IMG_WIDTH, IMG_HEIGHT, resized_image, OUT_WIDTH, OUT_HEIGHT);

    // Allocate memory for our output image
    uint8_t *output_image = (uint8_t *)calloc(OUT_WIDTH * OUT_HEIGHT, sizeof(uint8_t));
    if (output_image == NULL) {
        printf("Failed to allocate memory for output image!\n");
        free(resized_image);
        return;
    }

    // Process the resized image
    apply_sobel(resized_image, output_image, OUT_WIDTH, OUT_HEIGHT);

    printf("Processing complete!\n");
    printf("Output Image (Sobel Filtered):\n");
    for (int y = 0; y < OUT_HEIGHT; y++) {
        for (int x = 0; x < OUT_WIDTH; x++) {
            printf("%3d ", output_image[y * OUT_WIDTH + x]);
        }
        printf("\n");
        
        // Give the serial buffer time to flush to prevent dropping characters
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Clean up
    free(resized_image);
    free(output_image);
}
