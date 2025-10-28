
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "muestreador_sthc3.h"

static const char *TAG = "STHC3_Muestreador";

shtc3_t sensor;
i2c_master_bus_handle_t bus_handle;
esp_event_loop_handle_t loop;
ESP_EVENT_DEFINE_BASE(SAMPLER_EVENT);

void init_i2c(i2c_master_bus_config_t conf) {
    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &bus_handle));
   shtc3_init(&sensor, bus_handle, 0x70);
}


void sampler_run(esp_event_loop_handle_t event_loop, uint64_t sample_time){
    // Define sampler task
    loop = event_loop;
    TaskHandle_t muestreadorTaskHandle = NULL;
    xTaskCreate(muestreador_task, "Muestreador", STACK_SIZE, (void *)&sample_time, 5, &muestreadorTaskHandle);
    configASSERT( muestreadorTaskHandle );
}

// -------------- Tasks -------------- //
void muestreador_task( void * pvParameters ){

    uint64_t sample_time = *((uint64_t *)pvParameters);
    sthc3_data current_data;
    ESP_LOGI(TAG, "Sampler init -- sample time: %" PRIu64 "\n", sample_time);

    while (true){
        // Get sensor data
        shtc3_get_temp_and_hum(&sensor, &current_data.temp_value, &current_data.hum_value);
        // Post Event
        esp_event_post_to(loop, SAMPLER_EVENT, NEW_DATA, (void *)&current_data, sizeof(sthc3_data), portMAX_DELAY);
        vTaskDelay(sample_time / portTICK_PERIOD_MS);
    }
}
                         
