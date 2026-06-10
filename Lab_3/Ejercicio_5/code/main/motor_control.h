#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT,
} motion_cmd_t;

// Safety pin definitions for standard ESP32 DevKit
#define PIN_IN1             GPIO_NUM_12
#define PIN_IN2             GPIO_NUM_13
#define PIN_IN3             GPIO_NUM_14
#define PIN_IN4             GPIO_NUM_27
#define PIN_ENA             GPIO_NUM_25
#define PIN_ENB             GPIO_NUM_26

extern volatile motion_cmd_t g_current_cmd;
extern volatile int g_speed_percent;

void motor_init(void);
void set_motion(motion_cmd_t cmd, int speed_percent);
const char *command_to_string(motion_cmd_t cmd);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_CONTROL_H
