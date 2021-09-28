#ifndef _SUPERCAR_MOTOR_H_
#define _SUPERCAR_MOTOR_H_

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
        /* Expectation configuration */
        float expt_init;                     // Initial expectation
        float expt_max;                      // Max expectation in dynamic mode
        float expt_min;                      // Min expectation in dynamic mode
        float expt_pace;                     // The expection pace. It can change expectation wave period

        /* Other configurations */
        unsigned int ctrl_period;            // Control period
        unsigned int pwm_freq;               // MCPWM output frequency
        mcpwm_unit_t pwm_unit;
        mcpwm_timer_t pwm_timer;
        mcpwm_io_signals_t pwm_signal;
        int pwm_pin;
        int direction_pin;
    } cfg;                                   // Configurations that should be initialized for this example
} supercar_motor_control_t;


void brushed_motor_init(supercar_motor_control_t* motor_ctrl, mcpwm_timer_t pwm_timer, mcpwm_io_signals_t pwm_signal, int pwm_pin, int direction_pin);

void brushed_motor_setup(supercar_motor_control_t* motor_ctrl);

/**
 * @brief Set pwm duty to drive the motor
 *
 * @param duty_cycle PWM duty cycle (100~-100), the motor will go backward if the duty is set to a negative value
 */
void brushed_motor_set_duty(supercar_motor_control_t* motor_ctrl, float duty_cycle);

void brushed_motor_set_direction(supercar_motor_control_t* motor_ctrl, motor_direction_t direction);

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