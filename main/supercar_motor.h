#ifndef _SUPERCAR_MOTOR_H_
#define _SUPERCAR_MOTOR_H_

#include "freertos/semphr.h"
#include "driver/mcpwm.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    MOTOR_RIGHT = 0,
    MOTOR_LEFT
} motor_direction_t;

typedef struct {
    /* Status */
    unsigned int start_time;                    // Seconds count
    bool start_flag;                         // Motor start flag
    float duty_cycle;
    motor_direction_t direction;
    float expt;
    SemaphoreHandle_t mutex;
    const char* name;
    /* Configurations */
    struct {
        float acceleration;         // Maximum delta per control period
        int ctrl_period;            // Control period
        int pwm_freq;               // MCPWM output frequency
        /* MCPWM Configuration */
        mcpwm_unit_t pwm_unit;
        mcpwm_timer_t pwm_timer;
        mcpwm_io_signals_t pwm_signal;
        int pwm_pin;
        int direction_pin;
    } cfg;                                   // Configurations that should be initialized for this example
} supercar_motor_control_t;


void brushed_motor_init(supercar_motor_control_t* motor_ctrl, mcpwm_timer_t pwm_timer, mcpwm_io_signals_t pwm_signal, int pwm_pin, int direction_pin);

void brushed_motor_setup(supercar_motor_control_t* motor_ctrl);

void brushed_motor_set_speed(supercar_motor_control_t* motor_ctrl, float speed);

/**
 * @brief start motor
 *
 * @param mc supercar_motor_control_t pointer
 */
void brushed_motor_start(supercar_motor_control_t *mc);

/**
 * @brief stop motor
 *
 * @param mc supercar_motor_control_t pointer
 */
void brushed_motor_stop(supercar_motor_control_t *mc);

#ifdef __cplusplus
}
#endif

#endif