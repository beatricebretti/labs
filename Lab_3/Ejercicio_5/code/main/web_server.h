#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"
#include "motor_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Robot Mode enum
typedef enum {
    MODE_MANUAL = 0,
    MODE_AUTO,
    MODE_AUDIO
} sumo_mode_t;

// Simulated border enum
typedef enum {
    SIM_NONE = 0,
    SIM_FRONT,
    SIM_LEFT,
    SIM_RIGHT
} sim_border_t;

typedef enum {
    ZONE_NONE = 0,
    ZONE_FRONT,
    ZONE_LEFT,
    ZONE_RIGHT
} border_zone_t;

// Simulated audio enum
typedef enum {
    SIM_AUDIO_NONE = 0,
    SIM_AUDIO_650,
    SIM_AUDIO_900,
    SIM_AUDIO_1150,
    SIM_AUDIO_1400
} sim_audio_t;

extern volatile sumo_mode_t g_sumo_mode;
extern volatile sim_border_t g_sim_border;
extern volatile bool g_border_detected;
extern volatile border_zone_t g_detected_zone;

// Audio variables
extern volatile sim_audio_t g_sim_audio;
extern volatile float g_sim_audio_freq;

httpd_handle_t start_webserver(void);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H
