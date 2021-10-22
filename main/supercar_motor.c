
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "driver/mcpwm.h"
#include "supercar_motor.h"
#include "math.h"


#define MOTOR_CTRL_MCPWM_UNIT   MCPWM_UNIT_0
#define MOTOR_CTRL_MCPWM_SIGNAL 

static const char* TAG = "MOTOR";

void brushed_motor_init(supercar_motor_control_t* motor_ctrl, mcpwm_timer_t pwm_timer, mcpwm_io_signals_t pwm_signal, int pwm_pin, int direction_pin){
    ESP_LOGD(TAG, "Initializing motor [%s]", motor_ctrl->name);
    motor_ctrl->duty_cycle = 0;
    motor_ctrl->expt = 0;
    motor_ctrl->direction = MOTOR_RIGHT;
    motor_ctrl->start_flag = false;
    motor_ctrl->start_time = -1;
   
    motor_ctrl->cfg.acceleration = 1.0f;
    motor_ctrl->cfg.ctrl_period = 10;
    motor_ctrl->cfg.pwm_freq = 20000;
    motor_ctrl->cfg.pwm_unit = MOTOR_CTRL_MCPWM_UNIT;
    motor_ctrl->cfg.pwm_timer = pwm_timer;
    motor_ctrl->cfg.pwm_signal = pwm_signal;
    motor_ctrl->cfg.pwm_pin = pwm_pin;
    motor_ctrl->cfg.direction_pin = direction_pin;
    motor_ctrl->mutex = xSemaphoreCreateMutex();
}


/**
 * @brief Set pwm duty to drive the motor
 *
 * @param duty_cycle PWM duty cycle (100~-100), the motor will go backward if the duty is set to a negative value
 */
static void brushed_motor_set_duty(supercar_motor_control_t* motor_ctrl, float duty_cycle)
{
    ESP_LOGD(TAG, "Duty cycle [%s] : %f", motor_ctrl->name, duty_cycle);
    motor_ctrl->duty_cycle = duty_cycle;
    if(duty_cycle){
        if(duty_cycle < 0)
            duty_cycle = -duty_cycle;
        mcpwm_set_duty(motor_ctrl->cfg.pwm_unit, motor_ctrl->cfg.pwm_timer, MCPWM_OPR_A, duty_cycle);
        mcpwm_set_duty_type(motor_ctrl->cfg.pwm_unit, motor_ctrl->cfg.pwm_timer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
    }else{
        mcpwm_set_signal_low(motor_ctrl->cfg.pwm_unit, motor_ctrl->cfg.pwm_timer, MCPWM_OPR_A);
    }
}

static void brushed_motor_set_direction(supercar_motor_control_t* motor_ctrl, motor_direction_t direction){
    ESP_LOGD(TAG, "Direction [%s] : %s", motor_ctrl->name, direction == MOTOR_RIGHT ? "RIGHT" : "LEFT");
    motor_ctrl->direction = direction;
    gpio_set_level(motor_ctrl->cfg.direction_pin, direction);
}

/**
 * @brief Motor control thread
 *
 * @param arg Information pointer transmitted by task creating function
 */
static void brushed_motor_ctrl_thread(void *arg){
    supercar_motor_control_t* motor = (supercar_motor_control_t*)arg;
    ESP_LOGD(TAG, "Initializing motor [%s]", motor->name);
    while (1) {
        xSemaphoreTake(motor->mutex, portMAX_DELAY);
        if(motor->expt != motor->duty_cycle){
            float delta = motor->expt - motor->duty_cycle;
            float acc = motor->cfg.acceleration;
            float new_duty = motor->duty_cycle;
            if(fabs(delta) > acc){
                if(delta > 0)
                    new_duty += acc;
                else 
                    new_duty -= acc;
            }else{
                new_duty = motor->expt;
            }
            ESP_LOGD(TAG, "Duty cycle [%s] expt %f : %f -> %f", motor->name, motor->expt, motor->duty_cycle, new_duty);
            motor_direction_t new_direction = new_duty > 0 ? MOTOR_RIGHT : MOTOR_LEFT;

            if(new_direction != motor->direction){
                brushed_motor_set_direction(motor, new_direction);
            }

            brushed_motor_set_duty(motor, new_duty);
        }
        if(motor->start_flag){
            motor->start_time += motor->cfg.ctrl_period; 
        }
        xSemaphoreGive(motor->mutex);
        vTaskDelay(motor->cfg.ctrl_period / portTICK_PERIOD_MS);
    }
}


void brushed_motor_setup(supercar_motor_control_t* motor_ctrl){
    ESP_LOGD(TAG, "Setting up motor [%s]", motor_ctrl->name);
    /** MCPWM initialization */
    mcpwm_config_t pwm_config;
    pwm_config.frequency = motor_ctrl->cfg.pwm_freq;     //frequency
    pwm_config.cmpr_a = 0;                              //initial duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;                              //initial duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;         //up counting mode
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(motor_ctrl->cfg.pwm_unit, motor_ctrl->cfg.pwm_timer, &pwm_config);    //Configure PWMxA & PWMxB with above settings
    mcpwm_gpio_init(motor_ctrl->cfg.pwm_unit, motor_ctrl->cfg.pwm_signal , motor_ctrl->cfg.pwm_pin);
    /** GPIO config for direction */
    gpio_config_t config_output = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_DEF_OUTPUT,
        .pin_bit_mask = ((1ULL<<motor_ctrl->cfg.direction_pin)),
        .pull_up_en = 0,
        .pull_down_en = 1
    };
    gpio_config(&config_output);
    gpio_set_level(motor_ctrl->cfg.direction_pin, 0);
    /* Motor control thread */
    xTaskCreatePinnedToCore(brushed_motor_ctrl_thread, "mcpwm_brushed_motor_ctrl_thread", 4096, motor_ctrl, 3, NULL, 1);
}

void brushed_motor_set_speed(supercar_motor_control_t* motor_ctrl, float speed){
    motor_ctrl->expt = speed;
}

/**
 * @brief start motor
 *
 * @param mc supercar_motor_control_t pointer
 */
void brushed_motor_start(supercar_motor_control_t *mc){
    ESP_LOGD(TAG, "Motor start [%s]", mc->name);
    mc->start_time = 0;
    mc->start_flag = true;
}

/**
 * @brief stop motor
 *
 * @param mc supercar_motor_control_t pointer
 */
void brushed_motor_stop(supercar_motor_control_t *mc){
    ESP_LOGD(TAG, "Motor stop [%s]", mc->name);
    mc->expt = 0;
    mc->start_time = 0;
    mc->start_flag = false;
}

