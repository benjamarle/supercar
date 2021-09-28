/* brushed dc motor control example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use MCPWM module to control brushed dc motor.
 * This code is tested with L298 motor driver.
 * User may need to make changes according to the motor driver they use.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "esp_log.h"

#include "driver/mcpwm.h"

#include "supercar_main.h"
#include "esp_hid_host.h"
#include "button.h"
#include "supercar_motor.h"

/* The global infomation structure */
static supercar_t supercar;

const char* PROPULSION_MOTOR_NAME = "PROPULSION";
const char* STEERING_MOTOR_NAME = "STEERING";

static const char* TAG = "CAR";

static void supercar_input_thread(void *arg)
{
    button_event_t ev;
    while (1) {
        if (xQueueReceive(supercar.button_events, &ev, 1000/portTICK_PERIOD_MS)) {
            /* Accelerator */
            if(supercar.control_type == LOCAL){
                if (ev.pin == GPIO_ACCELERATOR_FWD_IN || ev.pin == GPIO_ACCELERATOR_BWD_IN) {
                    if(ev.event == BUTTON_DOWN){
                        supercar_set_direction(&supercar, ev.pin == GPIO_ACCELERATOR_FWD_IN ? FORWARD : BACKWARD);
                        supercar_start(&supercar);
                    }
                    if(ev.event == BUTTON_UP){
                        supercar_stop(&supercar);
                    }
                }
            }
        }
    }
}

#define DEBOUNCE(var, val) (old_ev.var != ev.var && ev.var == val)

static void supercar_remote_input_thread(void *arg)
{
    xbox_input_report_t old_ev = {
        0
    };
    xbox_input_report_t ev;
    while (1) {
        if (xQueueReceive(supercar.remote_events, &ev, 1000/portTICK_PERIOD_MS)) {
            if(DEBOUNCE(y, 1)){
                supercar.control_type = supercar.control_type == LOCAL ? REMOTE : LOCAL;
            }
            if(DEBOUNCE(b, 1)){
                supercar_reverse(&supercar);
            }
            if(ev.lt || ev.rt){
                supercar.control_type = REMOTE;
                if(ev.lt){
                    supercar_set_direction(&supercar, BACKWARD);
                    supercar_throttle(&supercar, ev.lt / 1023.0f * 100.0f);
                }
                if(ev.rt){
                    supercar_set_direction(&supercar, FORWARD);
                    supercar_throttle(&supercar, ev.rt / 1023.0f * 100.0f);
                }
            }

            if(DEBOUNCE(dpad, DPAD_LEFT)){
                supercar_turn(&supercar, STEER_LEFT);
            }

            if(DEBOUNCE(dpad, DPAD_RIGHT)){
                supercar_turn(&supercar, STEER_RIGHT);
            }

            if(ev.dpad == DPAD_NONE){
                supercar_turn(&supercar, STEER_NONE);
            }

            if(!ev.lt && !ev.rt && supercar.control_type == REMOTE){
                supercar.control_type = LOCAL;
                supercar_stop(&supercar);
            }
            old_ev = ev;
        }
    }
}

void supercar_init(supercar_t* car){
    ESP_LOGD(TAG, "Car initializing");
    car->control_type = LOCAL;
    car->control_type = MOTION;
    car->direction = FORWARD;
    car->propulsion_motor_ctrl.name = PROPULSION_MOTOR_NAME;
    car->steering_motor_ctrl.name = STEERING_MOTOR_NAME;
    car->cfg.max_speed = 50;
    brushed_motor_init(&car->propulsion_motor_ctrl, MCPWM_TIMER_0, MCPWM0A, GPIO_PWM_PROPULSION_OUT, GPIO_DIR_PROPULSION_OUT);
    brushed_motor_init(&car->steering_motor_ctrl, MCPWM_TIMER_1, MCPWM1A, GPIO_PWM_STEERING_OUT, GPIO_DIR_STEERING_OUT);

    car->button_events = pulled_button_init(PIN_BIT(GPIO_ACCELERATOR_FWD_IN), GPIO_PULLUP_ONLY);
    car->remote_events = xQueueCreate(10, sizeof(xbox_input_report_t));
}

void supercar_setup(supercar_t* car){
    ESP_LOGD(TAG, "Car setting up");
    brushed_motor_setup(&car->propulsion_motor_ctrl);
    brushed_motor_setup(&car->steering_motor_ctrl);
}

void supercar_reverse(supercar_t* car){
    supercar_set_direction(car, car->direction == FORWARD ? BACKWARD : FORWARD);
}

void supercar_turn(supercar_t* car, supercar_steer_t turn){
    supercar_motor_control_t* steering = &car->steering_motor_ctrl;
    xSemaphoreTake(steering->mutex, portMAX_DELAY);
    if(turn == STEER_NONE){
        brushed_motor_set_speed(steering, 0);
        brushed_motor_stop(steering);
    }else{
        brushed_motor_set_speed(steering, STEERING_SPEED);
        brushed_motor_start(steering);
        brushed_motor_set_direction(steering, turn == STEER_RIGHT ? MOTOR_RIGHT : MOTOR_LEFT);
    }
    xSemaphoreGive(steering->mutex);
}

void supercar_set_direction(supercar_t* car, supercar_direction_t direction){
    supercar_motor_control_t* propulsion = &car->propulsion_motor_ctrl;
    xSemaphoreTake(propulsion->mutex, portMAX_DELAY);
    car->direction = direction;
    brushed_motor_set_direction(propulsion, car->direction == FORWARD ? MOTOR_RIGHT : MOTOR_LEFT);
    xSemaphoreGive(propulsion->mutex);
}

void supercar_throttle(supercar_t* car, float speed){
    supercar_motor_control_t* propulsion = &car->propulsion_motor_ctrl;
    xSemaphoreTake(propulsion->mutex, portMAX_DELAY);
    brushed_motor_set_speed(propulsion, speed);
    if(!propulsion->start_flag)
        brushed_motor_start(propulsion);
    xSemaphoreGive(propulsion->mutex);
}

void supercar_start(supercar_t* car){
    ESP_LOGD(TAG, "Car starting up");
    supercar_throttle(car, car->cfg.max_speed);
}

void supercar_stop(supercar_t* car){
    ESP_LOGD(TAG, "Car stopping");
    supercar_motor_control_t* propulsion = &car->propulsion_motor_ctrl;
    xSemaphoreTake(propulsion->mutex, portMAX_DELAY);
    brushed_motor_set_speed(propulsion, 0);
    brushed_motor_stop(propulsion);
    xSemaphoreGive(propulsion->mutex);
}

/**
 * @brief The main entry of this example
 */
void app_main(void)
{
    printf("Super car starting...\n");
    /* Initialize peripherals and modules */
    supercar_init(&supercar);
    supercar_setup(&supercar);

    /* Initialize the console */
    cmd_mcpwm_motor_init(&(supercar.propulsion_motor_ctrl));
    
    /* Motor expectation wave generate thread */
    xTaskCreate(supercar_input_thread, "supercar_input_thread", 4096, NULL, 5, NULL);
    xTaskCreate(supercar_remote_input_thread, "supercar_remote_input_thread", 4096, NULL, 5, NULL);

    init_hid_host(&supercar);
}