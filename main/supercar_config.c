#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "supercar_config.h"
#include "math.h"

#define STORAGE_NAMESPACE "storage"
#define TAG "supercar_config"
#define MAX_LEN 2048

static double supercar_get_number(cJSON* cfg, char* name){
    ESP_LOGD(TAG, "Trying to get value of %s", name);
    cJSON* item = cJSON_GetObjectItem(cfg, name);
    if(!item){
        ESP_LOGW(TAG, "Variable %s not found", name);
        return NAN;
    }

    double val = cJSON_GetNumberValue(item);
    return val;
}

static void supercar_update_int(cJSON* cfg, char* name, int* value){
    double val = supercar_get_number(cfg, name);
    if(val == NAN){
        ESP_LOGW(TAG, "Variable %s is not a number", name);
        return;
    }
    int old_value = *value;
    *value = (int) val;
    ESP_LOGD(TAG, "Setting value %s=%d (%d)", name, *value, old_value);
}

static void supercar_update_float(cJSON* cfg, char* name, float* value){
    double val = supercar_get_number(cfg, name);
    if(val || val == NAN)
        return;

    float old_value = *value;
    *value = (float) val;
    ESP_LOGD(TAG, "Setting value %s=%f (%f)", name, *value, old_value);
}

void supercar_serialize_motor_config(cJSON* cfg, supercar_motor_control_t* mctl){
    cJSON_AddNumberToObject(cfg, "acceleration", mctl->cfg.acceleration);
    cJSON_AddNumberToObject(cfg, "ctrl_period", mctl->cfg.ctrl_period);
    cJSON_AddNumberToObject(cfg, "pwm_freq", mctl->cfg.pwm_freq);
    cJSON_AddNumberToObject(cfg, "pwm_pin", mctl->cfg.pwm_pin);
    cJSON_AddNumberToObject(cfg, "direction_pin", mctl->cfg.direction_pin);
}

void supercar_deserialize_motor_config(cJSON* cfg, supercar_motor_control_t* mctl){
    supercar_update_float(cfg, "acceleration", &mctl->cfg.acceleration);
    supercar_update_int(cfg, "ctrl_period", &mctl->cfg.ctrl_period);
    supercar_update_int(cfg, "pwm_freq", &mctl->cfg.pwm_freq);
    supercar_update_int(cfg, "pwm_pin", &mctl->cfg.pwm_pin);
    supercar_update_int(cfg, "direction_pin", &mctl->cfg.direction_pin);
}

void supercar_serialize_config(cJSON* cfg, supercar_t* car){
    cJSON_AddNumberToObject(cfg, "max_speed", car->cfg.max_speed);
    cJSON_AddNumberToObject(cfg, "delta_speed", car->cfg.delta_speed);
    cJSON_AddNumberToObject(cfg, "mode_input_pin", car->cfg.mode_input_pin);
    cJSON_AddNumberToObject(cfg, "mode_output_pin", car->cfg.mode_output_pin);
    cJSON_AddNumberToObject(cfg, "power_output_pin", car->cfg.power_output_pin);
    cJSON_AddNumberToObject(cfg, "distance_threshold_forward", car->cfg.distance_threshold_forward);
    cJSON_AddNumberToObject(cfg, "distance_threshold_backward", car->cfg.distance_threshold_backward);
}

void supercar_deserialize_config(cJSON* cfg, supercar_t* car){
    supercar_update_int(cfg, "max_speed", &car->cfg.max_speed);
    supercar_update_int(cfg, "delta_speed", &car->cfg.delta_speed);
    supercar_update_int(cfg, "mode_input_pin", &car->cfg.mode_input_pin);
    supercar_update_int(cfg, "mode_output_pin", &car->cfg.mode_output_pin);
    supercar_update_int(cfg, "power_output_pin", &car->cfg.power_output_pin);
    supercar_update_int(cfg, "distance_threshold_forward", &car->cfg.distance_threshold_forward);
    supercar_update_int(cfg, "distance_threshold_backward", &car->cfg.distance_threshold_backward);
}

void supercar_serialize_propulsion_config(cJSON* node, supercar_t* car){
    supercar_serialize_motor_config(node, &car->propulsion_motor_ctrl);
}

void supercar_deserialize_propulsion_config(cJSON* node, supercar_t* car){
    supercar_deserialize_motor_config(node, &car->propulsion_motor_ctrl);
}

void supercar_serialize_steering_config(cJSON* node, supercar_t* car){
    supercar_serialize_motor_config(node, &car->steering_motor_ctrl);
}

void supercar_deserialize_steering_config(cJSON* node, supercar_t* car){
    supercar_deserialize_motor_config(node, &car->steering_motor_ctrl);
}

#define MAIN_CONFIG "main"
#define PROPULSION_CONFIG "propulsion"
#define STEERING_CONFIG "steering"

static esp_err_t supercar_nvs_read(supercar_t* car, void (*deserialize)(cJSON*, supercar_t*), const char* name){
    ESP_LOGD(TAG, "Saving configuration");
    nvs_handle_t nvs_h;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_h);
    if (err != ESP_OK) return err;

    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(nvs_h, name, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    
    // Read previously saved blob if available
    char* config_json = malloc(required_size);
    if (required_size > 0) {
        err = nvs_get_blob(nvs_h, name, config_json, &required_size);
        if (err != ESP_OK) {
            free(config_json);
            return err;
        }
        ESP_LOGI(TAG, "Read config : %s", config_json);
        cJSON* cfg = cJSON_Parse(config_json);
        if(cfg == NULL){
            return ESP_FAIL;
        }
        deserialize(cfg, car);
        cJSON_Delete(cfg);
    }else{
        ESP_LOGI(TAG, "No config found");
    }

    free(config_json);
        // Close
    nvs_close(nvs_h);
    return ESP_OK;
}

esp_err_t supercar_config_read(supercar_t* car){
    return supercar_nvs_read(car, supercar_deserialize_config, MAIN_CONFIG);
}

esp_err_t supercar_propulsion_config_read(supercar_t* car){
    return supercar_nvs_read(car, supercar_deserialize_propulsion_config, PROPULSION_CONFIG);
}

esp_err_t supercar_steering_config_read(supercar_t* car){
    return supercar_nvs_read(car, supercar_deserialize_steering_config, STEERING_CONFIG);
}

esp_err_t supercar_nvs_save(supercar_t* car, void (*serialize)(cJSON*, supercar_t*), const char* name){
    ESP_LOGD(TAG, "Saving configuration");
    nvs_handle_t nvs_h;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_h);
    if (err != ESP_OK) return err;

    size_t required_size = 0;  // value will default to 0, if not set yet in NVS

    cJSON* cfg = cJSON_CreateObject();
    serialize(cfg, car);
    char * config_json = cJSON_Print(cfg);
    cJSON_Delete(cfg);

    // Write value including previously saved blob if available
    required_size = strlen(config_json);
    
    err = nvs_set_blob(nvs_h, "main", config_json, required_size);
    free(config_json);

    if (err != ESP_OK) return err;

    // Commit
    err = nvs_commit(nvs_h);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(nvs_h);
    return ESP_OK;
}

esp_err_t supercar_config_save(supercar_t* car){
    return supercar_nvs_save(car, supercar_serialize_config, MAIN_CONFIG);
}

esp_err_t supercar_propulsion_config_save(supercar_t* car){
    return supercar_nvs_save(car, supercar_serialize_propulsion_config, PROPULSION_CONFIG);
}

esp_err_t supercar_steering_config_save(supercar_t* car){
    return supercar_nvs_save(car, supercar_serialize_steering_config, STEERING_CONFIG);
}


