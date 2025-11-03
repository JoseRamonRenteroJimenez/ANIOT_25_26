
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "muestreador_sthc3.h"

static const char *TAG = "STHC3_Muestreador";

static shtc3_t sensor;
static i2c_master_bus_handle_t bus_handle;
static esp_event_loop_handle_t loop;
static esp_timer_handle_t sampler_timer;
ESP_EVENT_DEFINE_BASE(SAMPLER_EVENT);

static void sampler_timer_callback(void *arg);

int init_i2c(i2c_master_bus_config_t conf) {
    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &bus_handle));
    int ret = shtc3_init(&sensor, bus_handle, 0x70);
    return ret;
}

void sampler_run(esp_event_loop_handle_t event_loop, uint64_t sample_time_ms){
    // Init event post
    loop = event_loop;
    esp_event_post_to(loop, SAMPLER_EVENT, SAMPLER_INIT, NULL, 0, portMAX_DELAY); 
    ESP_LOGI(TAG, "Sampler init -- sample time: %" PRIu64 " ms", sample_time_ms);

    // Timer start
    const esp_timer_create_args_t sampler_timer_args = {
        .callback = &sampler_timer_callback,
        .arg = NULL,
        .name = "sampler_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&sampler_timer_args, &sampler_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sampler_timer, sample_time_ms * 1000));
}

// -------------- Timer Callback -------------- //
static void sampler_timer_callback(void *arg) {
    // Get sensor data
    sthc3_data current_data;
    shtc3_get_temp_and_hum(&sensor, &current_data.temp_value, &current_data.hum_value);

    // Post event
    esp_event_post_to(loop, SAMPLER_EVENT, NEW_DATA, (void *)&current_data, sizeof(sthc3_data), portMAX_DELAY);
}


// -------------- Tasks (Deprecated)-------------- //
/*
void muestreador_task( void * pvParameters ){
    // TODO: TIMERS

    uint64_t sample_time = *((uint64_t *)pvParameters);
    sthc3_data current_data;
    esp_event_post_to(loop, SAMPLER_EVENT, SAMPLER_INIT, NULL, 0, portMAX_DELAY); 
    ESP_LOGI(TAG, "Sampler init -- sample time: %" PRIu64 "", sample_time);

    while (true){
        // Get sensor data
        shtc3_get_temp_and_hum(&sensor, &current_data.temp_value, &current_data.hum_value);
        // Post Event
        esp_event_post_to(loop, SAMPLER_EVENT, NEW_DATA, (void *)&current_data, sizeof(sthc3_data), portMAX_DELAY);
        vTaskDelay(sample_time / portTICK_PERIOD_MS);
    }
}
*/
                         
