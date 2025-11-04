
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "shtc3.h"
#include "esp_event.h"
#include "muestreador.h"

static const char *TAG = "STHC3_Muestreador";

shtc3_t sensor;
i2c_master_bus_handle_t bus_handle;
esp_event_loop_handle_t loop;

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
static void muestreador_task( void * pvParameters ){

    uint64_t sample_time = *((uint64_t *)pvParameters);
    sthc3_data current_data;
    ESP_LOGI(TAG, "sample: %" PRIu64 "\n", sample_time);

    while (true){
        // Get sensor data
        shtc3_get_temp_and_hum(&sensor, &current_data.temp_value, &current_data.hum_value);
        // Post Event
        esp_event_post_to(loop, SAMPLER_EVENT, NEW_DATA, (void *)&current_data, sizeof(sthc3_data), portMAX_DELAY);
        vTaskDelay(sample_time / portTICK_PERIOD_MS);
    }
}


// ------------------ For Testing ------------------ //
// -------------- Events -------------- //
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    switch (event_id)
    {
    case NEW_DATA:
        sthc3_data recived_data = *((sthc3_data *)event_data);
        ESP_LOGI(TAG, "New Sampler Data Event -- Sensor Values -- Temp: %f  HUM: %f", 
                recived_data.temp_value, recived_data.hum_value);
        break;
    case SPARE:
        ESP_LOGI(TAG, "Spare Event -- Only for testing");
        break;
    
    default:
        break;
    }

}

static void app_main(void)
{

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    init_i2c(i2c_bus_config);


    // Event loop init
    esp_event_loop_args_t loop_args = {
        .queue_size = CONFIG_ESP_SYSTEM_EVENT_QUEUE_SIZE,
        .task_name = "monitor_events",
        .task_stack_size = (2304+512),
        .task_priority = 5,
        .task_core_id = 0
    };
    esp_event_loop_handle_t loop_event_handle;
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_event_handle));

    // Run sampler
    sampler_run(loop_event_handle, 1000);

    // Muestreador event handler
    esp_event_handler_instance_t instance_muestreador;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            SAMPLER_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_muestreador));
}
                                      
