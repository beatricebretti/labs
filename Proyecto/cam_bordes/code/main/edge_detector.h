#ifndef ROBOT_SUMO_EDGE_DETECTOR_H_
#define ROBOT_SUMO_EDGE_DETECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EDGE_ZONE_NONE = 0,
    EDGE_ZONE_LEFT,
    EDGE_ZONE_CENTER,
    EDGE_ZONE_RIGHT,
} edge_zone_t;

typedef enum {
    EDGE_ACTION_FORWARD = 0,
    EDGE_ACTION_TURN_LEFT,
    EDGE_ACTION_TURN_RIGHT,
    EDGE_ACTION_REVERSE,
} edge_action_t;

typedef struct {
    bool edge_detected;
    edge_zone_t zone;
    edge_action_t action;
    uint8_t confidence;
    int8_t steer;
    int16_t distance_px;
    uint16_t left_hits;
    uint16_t center_hits;
    uint16_t right_hits;
    uint16_t bottom_hits;
    uint16_t row_peak_hits;
    uint16_t gradient_threshold;
} edge_result_t;

void edge_detector_analyze(const uint8_t *gray, int width, int height,
                           edge_result_t *result);
const char *edge_zone_name(edge_zone_t zone);
const char *edge_action_name(edge_action_t action);

#ifdef __cplusplus
}
#endif

#endif
