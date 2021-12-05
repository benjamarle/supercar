/* IR protocols example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "esp_log.h"
#include "driver/rmt.h"
#include "supercar_sensor.h"
#include "freertos/semphr.h"

static const char* TAG = "sensor";

static const rmt_channel_t RX_CHANNEL = RMT_CHANNEL_0;

static void sensor_rx_task(void *arg)
{
    supercar_t* supercar = (supercar_t*) arg;
    size_t length = 0;
    RingbufHandle_t rb = NULL;
    rmt_item32_t *items = NULL;
    distance_sensor_report_t report;
    sensor_distance_t* array = (sensor_distance_t*) &report;

    rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(GPIO_DISTANCE_SENSOR_IN, RX_CHANNEL);
    rmt_rx_config.rx_config.filter_en = false;
    rmt_rx_config.rx_config.idle_threshold = 2000;

    ESP_LOGD(TAG, "Configuring RMT…");
    rmt_config(&rmt_rx_config);
    ESP_LOGD(TAG, "Installing RMT driver…");
    rmt_driver_install(RX_CHANNEL, 1000, 0);


    ESP_LOGD(TAG, "Getting RMT ring buffer handle…");
    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(RX_CHANNEL, &rb);
    assert(rb != NULL);
    // Start receive
    ESP_LOGD(TAG, "Starting RMT RX channel…");
    rmt_rx_start(RX_CHANNEL, true);
    int s = 0;
    while (1) {
        memset(&report, 0, sizeof(distance_sensor_report_t));
        items = (rmt_item32_t *) xRingbufferReceive(rb, &length, portMAX_DELAY);
        //ESP_LOGI(TAG, "Buffer received %d", length);
        if (items) {
            length /= 4; // one RMT = 4 Bytes
            
            rmt_item32_t * current = items;
            current += 2;
            for(int i = 2; i < length-1; i++){
                //ESP_LOGI(TAG, "Item d0=%d l0=%d d1=%d l1=%d val=%d", current->duration0, current->level0, current->duration1, current->level1, current->val);
                if(current->duration0 > 150){
                    int r = i-2;
                    int sensor_index = r / 8;
                    int bit_index = (7 - (r % 8));
                    array[sensor_index].distance |= (1 << bit_index);
                }
                
                current++;
            }

            xQueueSend(supercar->distance_events, &report, 100 / portTICK_PERIOD_MS);
            if(!(s++ % 8))
                ESP_LOGD(TAG, "Distance report %d %d %d %d", array[0].distance, array[1].distance, array[2].distance, array[3].distance);
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void *) items);
        }
    }
    
    ESP_LOGD(TAG, "Uninstalling RMT driver…");
    rmt_driver_uninstall(RX_CHANNEL);
    ESP_LOGD(TAG, "End on RMT task");
    vTaskDelete(NULL);
}

void init_distance_sensor_rx(supercar_t* car)
{
    xTaskCreatePinnedToCore(sensor_rx_task, "sensor_rx_task", 2048, car, 10, NULL, 1);
}
