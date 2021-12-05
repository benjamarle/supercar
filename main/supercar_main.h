/* cmd_mcpwm_motor.h

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#ifndef _SUPERCAR_MAIN_H_
#define _SUPERCAR_MAIN_H_
 
#include "supercar_motor.h"
#include "freertos/semphr.h"
#include "supercar_sensor.h"

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

#define GPIO_DISTANCE_SENSOR_IN 22

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
    DIRECTION_FORWARD,
    DIRECTION_BACKWARD, 
    DIRECTION_NONE
} supercar_direction_t;

typedef enum {
    STEER_LEFT,
    STEER_NONE,
    STEER_RIGHT
} supercar_steer_t;

typedef struct {
    uint8_t front_left;
    uint8_t front_right;
    uint8_t back_left;
    uint8_t back_right;
} supercar_distance_sensor_t;

typedef struct {
        int max_speed;
        int delta_speed;
        int mode_input_pin;
        int mode_output_pin;
        int power_output_pin;
        int distance_threshold_forward;
        int distance_threshold_backward;
} supercar_config_t;

typedef struct {
    bool power;
    supercar_motor_control_t propulsion_motor_ctrl;
    supercar_motor_control_t steering_motor_ctrl;
    supercar_mode_t mode;
    supercar_mode_t applied_mode;
    supercar_control_type_t control_type;
    supercar_steer_t steering;
    bool reverse_direction;
    bool reverse_mode;
    supercar_direction_t running;
    supercar_distance_sensor_t distance;


    SemaphoreHandle_t mutex;

    /* Handles */
    QueueHandle_t button_events;
    QueueHandle_t remote_events;
    QueueHandle_t distance_events;

    supercar_config_t cfg;

} supercar_t;

void supercar_init(supercar_t* car);

void supercar_setup(supercar_t* car);

void supercar_power(supercar_t* car, bool power);

void supercar_start(supercar_t* car, supercar_direction_t direction);

void supercar_stop(supercar_t* car);

void supercar_set_max_speed(supercar_t* car, int max_speed);

void supercar_toggle_mode(supercar_t* car);

void supercar_reverse_mode(supercar_t* car);

void supercar_reverse(supercar_t* car);

void supercar_turn(supercar_t* car, supercar_steer_t turn);

supercar_mode_t supercar_get_mode(supercar_t* car);

void supercar_set_mode(supercar_t* car, supercar_mode_t mode);

void supercar_throttle(supercar_t* car, float speed);

extern void start_rest_main(supercar_t* car);

extern void init_distance_sensor_rx(supercar_t* car);

#ifdef __cplusplus
}
#endif

#endif
