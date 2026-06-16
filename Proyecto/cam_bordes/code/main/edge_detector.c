#include "edge_detector.h"

#include <string.h>

#include "sdkconfig.h"

#ifndef CONFIG_EDGE_ROI_START_ROW_PERCENT
#define CONFIG_EDGE_ROI_START_ROW_PERCENT 45
#endif
#ifndef CONFIG_EDGE_GRADIENT_THRESHOLD
#define CONFIG_EDGE_GRADIENT_THRESHOLD 110
#endif
#ifndef CONFIG_EDGE_MIN_HITS
#define CONFIG_EDGE_MIN_HITS 35
#endif
#ifndef CONFIG_EDGE_MIN_ROW_HITS
#define CONFIG_EDGE_MIN_ROW_HITS 8
#endif
#ifndef CONFIG_EDGE_BOTTOM_DANGER_ROWS
#define CONFIG_EDGE_BOTTOM_DANGER_ROWS 18
#endif
#ifndef CONFIG_EDGE_DANGER_DISTANCE_PX
#define CONFIG_EDGE_DANGER_DISTANCE_PX 26
#endif

static int abs_i(int value)
{
    return value < 0 ? -value : value;
}

static int clamp_i(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint8_t confidence_from_hits(uint32_t total_hits, uint32_t bottom_hits,
                                    uint32_t row_peak_hits)
{
    uint32_t total_score = (total_hits * 100U) / (CONFIG_EDGE_MIN_HITS * 3U);
    uint32_t bottom_score = (bottom_hits * 100U) / CONFIG_EDGE_MIN_HITS;
    uint32_t row_score = (row_peak_hits * 100U) / (CONFIG_EDGE_MIN_ROW_HITS * 3U);
    uint32_t score = total_score;

    if (bottom_score > score) {
        score = bottom_score;
    }
    if (row_score > score) {
        score = row_score;
    }
    return score > 100U ? 100U : (uint8_t)score;
}

void edge_detector_analyze(const uint8_t *gray, int width, int height,
                           edge_result_t *result)
{
    memset(result, 0, sizeof(*result));
    result->zone = EDGE_ZONE_NONE;
    result->action = EDGE_ACTION_FORWARD;
    result->distance_px = -1;
    result->gradient_threshold = CONFIG_EDGE_GRADIENT_THRESHOLD;

    if (gray == NULL || width < 3 || height < 3) {
        return;
    }

    const int roi_start = clamp_i((height * CONFIG_EDGE_ROI_START_ROW_PERCENT) / 100,
                                  1, height - 2);
    const int bottom_start = clamp_i(height - CONFIG_EDGE_BOTTOM_DANGER_ROWS,
                                     1, height - 2);
    const int left_limit = width / 3;
    const int right_limit = (2 * width) / 3;

    uint32_t left_hits = 0;
    uint32_t center_hits = 0;
    uint32_t right_hits = 0;
    uint32_t bottom_hits = 0;
    uint32_t total_hits = 0;
    uint16_t row_peak_hits = 0;
    int best_row = -1;

    for (int y = roi_start; y < height - 1; ++y) {
        uint16_t row_hits = 0;
        for (int x = 1; x < width - 1; ++x) {
            const int p00 = gray[(y - 1) * width + (x - 1)];
            const int p01 = gray[(y - 1) * width + x];
            const int p02 = gray[(y - 1) * width + (x + 1)];
            const int p10 = gray[y * width + (x - 1)];
            const int p12 = gray[y * width + (x + 1)];
            const int p20 = gray[(y + 1) * width + (x - 1)];
            const int p21 = gray[(y + 1) * width + x];
            const int p22 = gray[(y + 1) * width + (x + 1)];

            const int gx = -p00 - (2 * p10) - p20 + p02 + (2 * p12) + p22;
            const int gy = -p00 - (2 * p01) - p02 + p20 + (2 * p21) + p22;
            const int magnitude = abs_i(gx) + abs_i(gy);

            if (magnitude < CONFIG_EDGE_GRADIENT_THRESHOLD) {
                continue;
            }

            ++total_hits;
            ++row_hits;
            if (y >= bottom_start) {
                ++bottom_hits;
            }
            if (x < left_limit) {
                ++left_hits;
            } else if (x < right_limit) {
                ++center_hits;
            } else {
                ++right_hits;
            }
        }

        if (row_hits > row_peak_hits) {
            row_peak_hits = row_hits;
            best_row = y;
        }
    }

    result->left_hits = left_hits > UINT16_MAX ? UINT16_MAX : (uint16_t)left_hits;
    result->center_hits = center_hits > UINT16_MAX ? UINT16_MAX : (uint16_t)center_hits;
    result->right_hits = right_hits > UINT16_MAX ? UINT16_MAX : (uint16_t)right_hits;
    result->bottom_hits = bottom_hits > UINT16_MAX ? UINT16_MAX : (uint16_t)bottom_hits;
    result->row_peak_hits = row_peak_hits;
    result->confidence = confidence_from_hits(total_hits, bottom_hits, row_peak_hits);

    if (best_row >= 0) {
        result->distance_px = (height - 1) - best_row;
    }

    const bool enough_total_hits = total_hits >= CONFIG_EDGE_MIN_HITS;
    const bool coherent_row = row_peak_hits >= CONFIG_EDGE_MIN_ROW_HITS;
    const bool close_row = result->distance_px >= 0 &&
                           result->distance_px <= CONFIG_EDGE_DANGER_DISTANCE_PX;
    const bool bottom_danger = bottom_hits >= CONFIG_EDGE_MIN_HITS;

    result->edge_detected = enough_total_hits && coherent_row &&
                            (close_row || bottom_danger);
    if (!result->edge_detected) {
        return;
    }

    if (left_hits >= center_hits && left_hits >= right_hits) {
        result->zone = EDGE_ZONE_LEFT;
        result->action = EDGE_ACTION_TURN_RIGHT;
        result->steer = 80;
    } else if (right_hits >= center_hits && right_hits >= left_hits) {
        result->zone = EDGE_ZONE_RIGHT;
        result->action = EDGE_ACTION_TURN_LEFT;
        result->steer = -80;
    } else {
        result->zone = EDGE_ZONE_CENTER;
        result->action = EDGE_ACTION_REVERSE;
        result->steer = 0;
    }
}

const char *edge_zone_name(edge_zone_t zone)
{
    switch (zone) {
    case EDGE_ZONE_LEFT:
        return "left";
    case EDGE_ZONE_CENTER:
        return "center";
    case EDGE_ZONE_RIGHT:
        return "right";
    case EDGE_ZONE_NONE:
    default:
        return "none";
    }
}

const char *edge_action_name(edge_action_t action)
{
    switch (action) {
    case EDGE_ACTION_TURN_LEFT:
        return "turn_left";
    case EDGE_ACTION_TURN_RIGHT:
        return "turn_right";
    case EDGE_ACTION_REVERSE:
        return "reverse";
    case EDGE_ACTION_FORWARD:
    default:
        return "forward";
    }
}
