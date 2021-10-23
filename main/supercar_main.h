/* cmd_mcpwm_motor.h

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#ifndef _SUPERCAR_MAIN_H_
#define _SUPERCAR_MAIN_H_
 
#include "supercar_motor.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Outputs **/
// Propulsion motor control pins
#define GPIO_PWM_PROPULSION_OUT 21   //Set GPIO 15 as PWM0A
#define GPIO_DIR_PROPULSION_OUT 17
// Steering motor control pins
#define GPIO_PWM_STEERING_OUT 19
#define GPIO_DIR_STEERING_OUT 18

/** Inputs **/
#define GPIO_ACCELERATOR_FWD_IN 12
#define GPIO_ACCELERATOR_BWD_IN 14

#define GPIO_MODE_SELECTOR_IN 13
#define GPIO_POWER_OUT 0
#define GPIO_MODE_SELECTOR_OUT 4


#define MOTOR_CTRL_MCPWM_TIMER  MCPWM_TIMER_0
#define STEERING_SPEED 100.0f

typedef enum {
    MOTION,
    SWAY
} supercar_mode_t;

typedef enum {
    LOCAL,
    REMOTE
} supercar_control_type_t;

typedef enum {
    FORWARD,
    BACKWARD
} supercar_direction_t;

typedef enum {
    STEER_LEFT,
    STEER_NONE,
    STEER_RIGHT
} supercar_steer_t;

typedef struct {
    bool power;
    supercar_motor_control_t propulsion_motor_ctrl;
    supercar_motor_control_t steering_motor_ctrl;
    supercar_mode_t mode;
    supercar_direction_t direction;
    supercar_control_type_t control_type;
    supercar_steer_t steering;
    bool reverse_direction;
    bool reverse_mode;
    bool running;

    SemaphoreHandle_t mutex;

    /* Handles */
    QueueHandle_t button_events;
    QueueHandle_t remote_events;

    struct {
        int max_speed;
        int delta_speed;
        int mode_input_pin;
        int mode_output_pin;
        int power_output_pin;
    } cfg;

} supercar_t;

void supercar_init(supercar_t* car);

void supercar_setup(supercar_t* car);

void supercar_power(supercar_t* car, bool power);

void supercar_start(supercar_t* car);

void supercar_stop(supercar_t* car);

void supercar_set_max_speed(supercar_t* car, int max_speed);

void supercar_toggle_mode(supercar_t* car);

void supercar_toggle_direction(supercar_t* car);

void supercar_reverse_mode(supercar_t* car);

void supercar_reverse(supercar_t* car);

void supercar_turn(supercar_t* car, supercar_steer_t turn);

supercar_direction_t supercar_get_direction(supercar_t* car);

void supercar_set_direction(supercar_t* car, supercar_direction_t direction);

supercar_mode_t supercar_get_mode(supercar_t* car);

void supercar_set_mode(supercar_t* car, supercar_mode_t mode);

void supercar_throttle(supercar_t* car, float speed);

#ifdef __cplusplus
}
#endif

#endif
