#include "main_functions.h"
#include "motor_control.h"
#include "web_server.h"
#include "audio_control.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sumo_logic";

// Simulation Image resolution: 96 x 96 (matching Exercise 3 & 4)
#define IMG_COLS 96
#define IMG_ROWS 96
#define BLACK_PIXEL 0x10  // Simulation black border value
#define WHITE_PIXEL 0xF0  // Simulation white ring value
#define BLACK_THRESHOLD 0x80 // Values below this are considered black/dark

// Grayscale simulation buffer
static uint8_t sim_img_buffer[IMG_COLS * IMG_ROWS];

// Audio sampling buffer
static float audio_sample_buffer[AUDIO_FFT_SIZE];

// Autonomous Navigation State Machine states
typedef enum {
    STATE_SUMO_SEARCH = 0, // Seeking/moving forward inside the white ring
    STATE_SUMO_EVADE_BACK, // Detected border, retroceding
    STATE_SUMO_EVADE_TURN  // Turning away from the detected border
} sumo_state_t;

static sumo_state_t g_nav_state = STATE_SUMO_SEARCH;
static int64_t g_state_timer = 0;
static border_zone_t g_evasion_direction = ZONE_NONE;

static const char* zone_name(border_zone_t z) {
    if (z == ZONE_FRONT) return "FRONT";
    if (z == ZONE_LEFT) return "LEFT";
    if (z == ZONE_RIGHT) return "RIGHT";
    return "NONE";
}

// Generates simulated camera pixel values based on active injected simulation state
static void generate_simulated_frame(sim_border_t border_type)
{
    // Default: fill entire image with white pixels (secure zone)
    for (int i = 0; i < IMG_COLS * IMG_ROWS; i++) {
        sim_img_buffer[i] = WHITE_PIXEL;
    }

    if (border_type == SIM_FRONT) {
        for (int r = 0; r < 24; r++) {
            for (int c = 0; c < IMG_COLS; c++) {
                sim_img_buffer[r * IMG_COLS + c] = BLACK_PIXEL;
            }
        }
    } else if (border_type == SIM_LEFT) {
        for (int r = 0; r < IMG_ROWS; r++) {
            for (int c = 0; c < 24; c++) {
                sim_img_buffer[r * IMG_COLS + c] = BLACK_PIXEL;
            }
        }
    } else if (border_type == SIM_RIGHT) {
        for (int r = 0; r < IMG_ROWS; r++) {
            for (int c = 72; c < 96; c++) {
                sim_img_buffer[r * IMG_COLS + c] = BLACK_PIXEL;
            }
        }
    }
}

// Analiza la matriz simulada buscando pixeles negros en las tres zonas de monitoreo
static void analyze_camera_frame(const uint8_t *img, bool &front_dark, bool &left_dark, bool &right_dark)
{
    int front_black_count = 0;
    int left_black_count = 0;
    int right_black_count = 0;

    for (int r = 0; r < 24; r++) {
        for (int c = 0; c < IMG_COLS; c++) {
            if (img[r * IMG_COLS + c] < BLACK_THRESHOLD) {
                front_black_count++;
            }
        }
    }

    for (int r = 24; r < 72; r++) {
        for (int c = 0; c < 24; c++) {
            if (img[r * IMG_COLS + c] < BLACK_THRESHOLD) {
                left_black_count++;
            }
        }
    }

    for (int r = 24; r < 72; r++) {
        for (int c = 72; c < 96; c++) {
            if (img[r * IMG_COLS + c] < BLACK_THRESHOLD) {
                right_black_count++;
            }
        }
    }

    int region_pixel_count = 24 * IMG_COLS; 
    int lateral_pixel_count = 48 * 24;      

    front_dark = (front_black_count > (region_pixel_count * 0.30));
    left_dark = (left_black_count > (lateral_pixel_count * 0.30));
    right_dark = (right_black_count > (lateral_pixel_count * 0.30));
}

void setup(void) {
    motor_init();
    audio_init(); // Initialize ADC for microphone
    ESP_LOGI(TAG, "Sumo autonomous navigator and audio FFT engine initialized.");
}

void loop(void) {
    if (g_sumo_mode == MODE_AUTO) {
        // --- MODE A: AUTONOMOUS SUMO CAMERA EVASION ---
        generate_simulated_frame(g_sim_border);
        run_inference((void *)sim_img_buffer);
        vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz loop
    }
    else if (g_sumo_mode == MODE_AUDIO) {
        // --- MODE B: VOICE CONTROL AUDIO FFT ---
        // Acquire samples either from digital simulation or from physical microphone
        float actual_fs = audio_acquire_samples(audio_sample_buffer, AUDIO_FFT_SIZE, g_sim_audio_freq);
        float rms = audio_preprocess_and_fft(audio_sample_buffer, AUDIO_FFT_SIZE);

        float peak_mag = 0.0f;
        float noise_avg = 0.0f;
        float peak_freq = audio_find_peak_frequency(actual_fs, &peak_mag, &noise_avg);

        // Update state variables for live Web Dashboard readout
        g_audio_peak_freq = peak_freq;
        g_audio_peak_mag = peak_mag;

        motion_cmd_t detected_cmd = audio_classify_command(peak_freq, peak_mag, noise_avg, rms);
        motion_cmd_t stable_cmd = audio_stabilize_command(detected_cmd);

        // Update active audio command
        g_audio_cmd = stable_cmd;

        // Drive motors physically with the audio command
        if (g_current_cmd != stable_cmd) {
            set_motion(stable_cmd, g_speed_percent);
        }

        // Delay 20ms to allow scheduler activity
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    else {
        // --- MODE C: MANUAL REMOTE CONTROL ---
        // Reset audio and border status values for clean web layout
        g_audio_peak_freq = 0.0f;
        g_audio_peak_mag = 0.0f;
        g_audio_cmd = CMD_STOP;
        g_border_detected = false;
        g_detected_zone = ZONE_NONE;

        vTaskDelay(pdMS_TO_TICKS(100)); // Sleep during manual control to save power
    }
}

void run_inference(void *ptr) {
    uint8_t *img = (uint8_t *)ptr;
    bool front_dark = false;
    bool left_dark = false;
    bool right_dark = false;

    analyze_camera_frame(img, front_dark, left_dark, right_dark);

    g_border_detected = (front_dark || left_dark || right_dark);
    if (front_dark) {
        g_detected_zone = ZONE_FRONT;
    } else if (left_dark) {
        g_detected_zone = ZONE_LEFT;
    } else if (right_dark) {
        g_detected_zone = ZONE_RIGHT;
    } else {
        g_detected_zone = ZONE_NONE;
    }

    if (g_sumo_mode != MODE_AUTO) {
        g_nav_state = STATE_SUMO_SEARCH;
        return;
    }

    int64_t current_time_ms = esp_timer_get_time() / 1000;

    switch (g_nav_state) {
        case STATE_SUMO_SEARCH:
            if (g_border_detected) {
                ESP_LOGW(TAG, "!! BORDER DETECTED (zone: %s) !! Reversing motors...", zone_name(g_detected_zone));
                g_evasion_direction = g_detected_zone;
                g_nav_state = STATE_SUMO_EVADE_BACK;
                g_state_timer = current_time_ms + 700; 
                set_motion(CMD_BACKWARD, g_speed_percent);
            } else {
                if (g_current_cmd != CMD_FORWARD) {
                    set_motion(CMD_FORWARD, g_speed_percent);
                }
            }
            break;

        case STATE_SUMO_EVADE_BACK:
            if (current_time_ms >= g_state_timer) {
                motion_cmd_t turn_dir = CMD_RIGHT;
                if (g_evasion_direction == ZONE_RIGHT) {
                    turn_dir = CMD_LEFT;
                }
                
                ESP_LOGI(TAG, "Done reversing. Spinning %s to evade...", (turn_dir == CMD_LEFT) ? "LEFT" : "RIGHT");
                g_nav_state = STATE_SUMO_EVADE_TURN;
                g_state_timer = current_time_ms + 600; 
                set_motion(turn_dir, g_speed_percent);
            } else {
                if (g_current_cmd != CMD_BACKWARD) {
                    set_motion(CMD_BACKWARD, g_speed_percent);
                }
            }
            break;

        case STATE_SUMO_EVADE_TURN:
            if (current_time_ms >= g_state_timer) {
                ESP_LOGI(TAG, "Evasion sequence complete. Resuming forward search.");
                g_nav_state = STATE_SUMO_SEARCH;
                g_evasion_direction = ZONE_NONE;
                set_motion(CMD_FORWARD, g_speed_percent);
            }
            break;
    }
}
