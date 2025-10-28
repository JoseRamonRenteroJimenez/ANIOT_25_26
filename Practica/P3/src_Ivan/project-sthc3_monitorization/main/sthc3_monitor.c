
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "mock-flash.h"
#include "mock_wifi.h"
#include "muestreador_sthc3.h"

#define STACK_SIZE 3072
#define STEP_MUESTREO 1000

static const char *TAG = "STHC3_Monitor";


// -------------- Events -------------- //
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    // TODO: Intregrar con maquina de estados

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


void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    switch (event_id)
    {
    case WIFI_MOCK_EVENT_WIFI_CONNECTED:
        ESP_LOGI(TAG, "Wifi Connected");
        break;
    case WIFI_MOCK_EVENT_WIFI_GOT_IP:
        ESP_LOGI(TAG, "Wifi got IP");
        break;
    case WIFI_MOCK_EVENT_WIFI_DISCONNECTED:
        ESP_LOGI(TAG, "Wifi disconnected");
        break;
    default:
        break;
    }
}


void app_main(void)
{
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

    // Wifi event handler
    esp_event_handler_instance_t instance_wifi;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_wifi));

    // Sampler event handler
    esp_event_handler_instance_t instance_muestreador;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            SAMPLER_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_muestreador));

    // Wifi init and connection
    wifi_mock_init(loop_event_handle);
    ESP_ERROR_CHECK(wifi_connect());

    // Sampler init
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    init_i2c(i2c_bus_config);
    sampler_run(loop_event_handle, 1000);

    
}
