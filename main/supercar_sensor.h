#ifndef _SUPERCAR_SENSOR_H_
#define _SUPERCAR_SENSOR_H_

#include "supercar_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
            uint8_t distance;
        } sensor_distance_t;

typedef struct {
    sensor_distance_t a;
    sensor_distance_t b;
    sensor_distance_t c;
    sensor_distance_t d;
} distance_sensor_report_t;


#ifdef __cplusplus
}
#endif

#endif