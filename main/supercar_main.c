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
#include "esp_hidh.h"
#include "esp_hid_host.h"
#include "button.h"
#include "supercar_motor.h"
#include "math.h"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define DEBOUNCE(var, val) (old_xbox.var != xbox.var && xbox.var == val)

static supercar_t supercar;

const char* PROPULSION_MOTOR_NAME = "PROPULSION";
const char* STEERING_MOTOR_NAME = "STEERING";

static const char* TAG = "CAR";

static void supercar_increase_max_speed(supercar_t* car){
    supercar_set_max_speed(car, car->cfg.max_speed + car->cfg.delta_speed);
}

static void supercar_decrease_max_speed(supercar_t* car){
    supercar_set_max_speed(car, car->cfg.max_speed - car->cfg.delta_speed);
}

static void supercar_read_mode(supercar_t* car){
    supercar_set_mode(car, gpio_get_level(car->cfg.mode_input_pin) ? MOTION : SWAY);
}

static void supercar_set_control_type(supercar_t* car, supercar_control_type_t control_type){
    supercar.control_type = control_type;
    if(control_type == LOCAL)
        supercar_read_mode(car);
}

static void supercar_toggle_control_type(supercar_t* car){
    supercar_set_control_type(car, car->control_type == LOCAL ? REMOTE : LOCAL);
    ESP_LOGI(TAG, "Toggling control mode to %s", supercar.control_type == LOCAL ?  "LOCAL" : "REMOTE");
}

static void supercar_apply_mode(supercar_t* car){
    supercar_mode_t current_mode = supercar_get_mode(car);
    ESP_LOGD(TAG, "Applying mode: %s", current_mode == SWAY ? "SWAY" : "MOTION");
    gpio_set_level(car->cfg.mode_output_pin, current_mode == SWAY ? 0 : 1);
    car->applied_mode = current_mode;
}


static void supercar_check_mode(supercar_t* car){
    if(car->propulsion_motor_ctrl.duty_cycle){
        // Apply mode only if the motor is not running to avoid any sudden change of speed
        return;
    }
    supercar_mode_t current_mode = supercar_get_mode(car);

    if(current_mode != car->applied_mode){
        supercar_apply_mode(car);
    }
}

static void supercar_remote_input_thread(void *arg)
{
    xbox_input_event_t old_ev = {0};
    xbox_input_event_t ev;

    while (1) {
        if (xQueueReceive(supercar.remote_events, &ev, 1000/portTICK_PERIOD_MS)) {
            if(ev.type != ESP_HIDH_INPUT_EVENT)
                continue;

            xSemaphoreTake(supercar.mutex, portMAX_DELAY);
            supercar_check_mode(&supercar);
            if(ev.type == ESP_HIDH_CLOSE_EVENT){
                ESP_LOGI(TAG, "Gamepad disconnected, stopping car…");
                supercar_turn(&supercar, STEER_NONE);
                supercar_stop(&supercar);
                supercar_set_mode(&supercar, LOCAL);
                xSemaphoreGive(supercar.mutex);
                continue;
            }

            xbox_input_report_t xbox = ev.report;
            xbox_input_report_t old_xbox = old_ev.report;
                
            if(DEBOUNCE(y, 1)){
                supercar_toggle_control_type(&supercar);
            }
            if(DEBOUNCE(b, 1)){
                supercar_reverse(&supercar);
            }
            if(DEBOUNCE(a, 1)){
                supercar_reverse_mode(&supercar);
            }
            if(xbox.lt || xbox.rt){
                supercar.control_type = REMOTE;
                if(xbox.lt){
                    supercar_throttle(&supercar, -xbox.lt / 1023.0f * 100.0f);
                }
                if(xbox.rt){
                    supercar_throttle(&supercar, xbox.rt / 1023.0f * 100.0f);
                }
            }

            if(DEBOUNCE(dpad, DPAD_LEFT)){
                supercar_turn(&supercar, STEER_LEFT);
            }

            if(DEBOUNCE(dpad, DPAD_RIGHT)){
                supercar_turn(&supercar, STEER_RIGHT);
            }

            if(DEBOUNCE(lb, 1)){
                supercar_decrease_max_speed(&supercar);
            }

            if(DEBOUNCE(rb, 1)){
                supercar_increase_max_speed(&supercar);
            }

            if(xbox.dpad == DPAD_NONE && supercar.steering != STEER_NONE){
                supercar_turn(&supercar, STEER_NONE);
            }

            if(!xbox.lt && !xbox.rt && supercar.control_type == REMOTE){
                //supercar.control_type = LOCAL;
                supercar_stop(&supercar);
            }
            old_ev = ev;
            xSemaphoreGive(supercar.mutex);
        }
    }
}

static void supercar_input_thread(void *arg)
{
    button_event_t ev;
    while (1) {
        if (xQueueReceive(supercar.button_events, &ev, 1000/portTICK_PERIOD_MS)) {
            xSemaphoreTake(supercar.mutex, portMAX_DELAY);
            supercar_check_mode(&supercar);
            /* Accelerator */
            if(supercar.control_type == LOCAL){
                if (ev.pin == GPIO_ACCELERATOR_FWD_IN || ev.pin == GPIO_ACCELERATOR_BWD_IN) {
                    if(ev.event == BUTTON_DOWN){
                        supercar_direction_t direction = ev.pin == GPIO_ACCELERATOR_FWD_IN ? 
                        (supercar.reverse_direction ? DIRECTION_BACKWARD : DIRECTION_FORWARD) : (supercar.reverse_direction ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
                        supercar_start(&supercar, direction);
                    }
                    if(ev.event == BUTTON_UP){
                        supercar_stop(&supercar);
                    }
                }
            

                if(ev.pin == GPIO_MODE_SELECTOR_IN){
                    if(ev.event == BUTTON_DOWN){
                        // Sway
                        supercar_set_mode(&supercar, SWAY);
                    }
                    if(ev.event == BUTTON_UP){
                        supercar_set_mode(&supercar, MOTION);
                    }
                }
            }
            xSemaphoreGive(supercar.mutex);
        }
    }
}

void supercar_init(supercar_t* car){
    ESP_LOGD(TAG, "Car initializing");
    car->power = false;
    car->control_type = LOCAL;
    car->control_type = MOTION;
    car->steering = STEER_NONE;
    car->propulsion_motor_ctrl.name = PROPULSION_MOTOR_NAME;
    car->steering_motor_ctrl.name = STEERING_MOTOR_NAME;
    car->cfg.max_speed = 50;
    car->cfg.delta_speed = 10;
    car->cfg.mode_input_pin = GPIO_MODE_SELECTOR_IN;
    car->cfg.mode_output_pin = GPIO_MODE_SELECTOR_OUT;
    car->cfg.power_output_pin = GPIO_POWER_OUT;
    car->reverse_direction = false;
    car->reverse_mode = false;
    car->running = DIRECTION_NONE;
    car->mutex = xSemaphoreCreateMutex();

    brushed_motor_init(&car->propulsion_motor_ctrl, MCPWM_TIMER_0, MCPWM0A, GPIO_PWM_PROPULSION_OUT, GPIO_DIR_PROPULSION_OUT);
    brushed_motor_init(&car->steering_motor_ctrl, MCPWM_TIMER_1, MCPWM1A, GPIO_PWM_STEERING_OUT, GPIO_DIR_STEERING_OUT);

    car->propulsion_motor_ctrl.cfg.acceleration = 2.0f;
    car->steering_motor_ctrl.cfg.acceleration = 4.0f;

    car->button_events = pulled_button_init(PIN_BIT(GPIO_ACCELERATOR_FWD_IN) | PIN_BIT(GPIO_ACCELERATOR_BWD_IN) | PIN_BIT(GPIO_MODE_SELECTOR_IN), GPIO_PULLUP_ONLY);
    car->remote_events = xQueueCreate(10, sizeof(xbox_input_event_t));

    gpio_config_t config_output = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_DEF_OUTPUT,
        .pin_bit_mask = ((1ULL<<car->cfg.mode_output_pin) | (1ULL<<car->cfg.power_output_pin)),
        .pull_up_en = 1,
        .pull_down_en = 0
    };
    gpio_config(&config_output);    
}

void supercar_setup(supercar_t* car){
    ESP_LOGD(TAG, "Car setting up");
    brushed_motor_setup(&car->propulsion_motor_ctrl);
    brushed_motor_setup(&car->steering_motor_ctrl);

    supercar_read_mode(car);
    supercar_apply_mode(car);

    supercar_power(car, true);
}

void supercar_power(supercar_t* car, bool power){
    gpio_set_level(car->cfg.power_output_pin, !power);
    car->power = power;
}

void supercar_toggle_mode(supercar_t* car){
    if(car->running != DIRECTION_NONE){
        supercar_stop(car);
    }
    supercar_set_mode(car, car->mode == SWAY ? MOTION : SWAY);
}

void supercar_reverse_mode(supercar_t* car){
    ESP_LOGD(TAG, "Car toggling mode");
    car->reverse_mode = !car->reverse_mode;
    supercar_set_mode(car, car->mode);
}

void supercar_reverse(supercar_t* car){
    ESP_LOGD(TAG, "Car toggling direction");
    car->reverse_direction = !car->reverse_direction;
    supercar_motor_control_t* propulsion = &(car->propulsion_motor_ctrl);
    if(propulsion->start_flag){
        ESP_LOGD(TAG, "Applying opposite thrust...");
        brushed_motor_set_speed(propulsion, -propulsion->expt);
    }
}

void supercar_turn(supercar_t* car, supercar_steer_t turn){
    supercar_motor_control_t* steering = &car->steering_motor_ctrl;
    
    car->steering = turn;
    if(turn == STEER_NONE){
        brushed_motor_set_speed(steering, 0);
        brushed_motor_stop(steering);
    }else{
        float steering_speed = turn == STEER_RIGHT ? STEERING_SPEED : -STEERING_SPEED;
        brushed_motor_set_speed(steering, steering_speed);
        brushed_motor_start(steering);
    }
}

supercar_mode_t supercar_get_mode(supercar_t* car){
    if(!car->reverse_mode)
        return car->mode;
    return car->mode == SWAY ? MOTION : SWAY;
}

void supercar_set_mode(supercar_t* car, supercar_mode_t mode){
    car->mode = mode;
    supercar_mode_t new_mode = supercar_get_mode(car);
    ESP_LOGD(TAG, "Setting mode: %s", new_mode == SWAY ? "SWAY" : "MOTION");
}

void supercar_throttle(supercar_t* car, float speed){
    supercar_motor_control_t* propulsion = &car->propulsion_motor_ctrl;
    brushed_motor_set_speed(propulsion, speed);
    if(speed != 0){
        ESP_LOGD(TAG, "Car running");
        car->running = speed > 0 ? DIRECTION_FORWARD : DIRECTION_BACKWARD;
        brushed_motor_start(propulsion);
    }
}

void supercar_start(supercar_t* car, supercar_direction_t direction){
    ESP_LOGD(TAG, "Car starting up...");
    int speed = car->cfg.max_speed;
    supercar_throttle(car, direction == DIRECTION_FORWARD ? speed : -speed);
}

void supercar_stop(supercar_t* car){ 
    supercar_motor_control_t* propulsion = &car->propulsion_motor_ctrl;
    if(car->running != DIRECTION_NONE){
        ESP_LOGD(TAG, "Car stopping");
        car->running = DIRECTION_NONE;
        brushed_motor_stop(propulsion);
    }
}


void supercar_set_max_speed(supercar_t* car, int max_speed){
    int old_max_speed = car->cfg.max_speed;
    car->cfg.max_speed = max(car->cfg.delta_speed, min(max_speed, 100));
    ESP_LOGD(TAG, "Car setting up new max speed : %d -> %d", old_max_speed, car->cfg.max_speed);
    if(car->running != DIRECTION_NONE){
        int new_speed = car->cfg.max_speed;
        if(car->running == DIRECTION_BACKWARD)
            new_speed = -new_speed;
        supercar_throttle(car, new_speed);
    }
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
    
    /* Motor expectation wave generate thread */
    xTaskCreate(supercar_input_thread, "supercar_input_thread", 4096, NULL, 5, NULL);
    xTaskCreate(supercar_remote_input_thread, "supercar_remote_input_thread", 4096, NULL, 5, NULL);

    init_hid_host(&supercar);

    start_http(&supercar);
}