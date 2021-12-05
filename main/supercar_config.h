#ifndef _SUPERCAR_CONFIG_H_
#define _SUPERCAR_CONFIG_H_

#include "esp_system.h"
#include "supercar_main.h"
#include "cjson.h"

#ifdef __cplusplus
extern "C" {
#endif

void supercar_serialize_motor_config(cJSON* cfg, supercar_motor_control_t* mctl);

void supercar_serialize_config(cJSON* cfg, supercar_t* car);

esp_err_t supercar_config_read(supercar_t* car);
esp_err_t supercar_propulsion_config_read(supercar_t* car);
esp_err_t supercar_steering_config_read(supercar_t* car);
esp_err_t supercar_config_save(supercar_t* car);
esp_err_t supercar_propulsion_config_save(supercar_t* car);
esp_err_t supercar_steering_config_save(supercar_t* car);

void supercar_serialize_motor_config(cJSON* cfg, supercar_motor_control_t* mctl);
void supercar_deserialize_motor_config(cJSON* cfg, supercar_motor_control_t* mctl);
void supercar_serialize_config(cJSON* cfg, supercar_t* car);
void supercar_deserialize_config(cJSON* cfg, supercar_t* car);

void supercar_serialize_propulsion_config(cJSON* node, supercar_t* car);
void supercar_deserialize_propulsion_config(cJSON* node, supercar_t* car);
void supercar_serialize_steering_config(cJSON* node, supercar_t* car);
void supercar_deserialize_steering_config(cJSON* node, supercar_t* car);

#ifdef __cplusplus
}
#endif

#endif
