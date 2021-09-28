/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef _ESP_HID_HOST_H_
#define _ESP_HID_HOST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_event.h"

#define SCAN_DURATION_SECONDS 5

typedef enum {
   DPAD_NONE = 0,
   DPAD_UP,
   DPAD_UP_RIGHT,
   DPAD_RIGHT,
   DPAD_DOWN_RIGHT,
   DPAD_DOWN,
   DPAD_DOWN_LEFT,
   DPAD_LEFT,
   DPAD_UP_LEFT
} dpad_input_t;


typedef struct {
   uint16_t lx;
   uint16_t ly;
   uint16_t rx; 
   uint16_t ry;
   uint16_t lt;
   uint16_t rt;
   /*uint8_t up: 1;
   uint8_t right: 1;
   uint8_t down: 1;
   uint8_t left: 1;*/
   uint8_t dpad;
   uint8_t a:1;
   uint8_t b:1;
   uint8_t x:1;
   uint8_t y:1;
   uint8_t lb:1;
   uint8_t rb:1;
   uint8_t select:1;
   uint8_t menu:1;
   uint8_t extra;
} xbox_input_report_t;



void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

void hid_task(void* param);

void init_hid_host(supercar_t* car);

#endif