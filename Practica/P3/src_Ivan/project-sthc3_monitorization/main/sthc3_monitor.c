
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
#include "sthc3_monitor.h"
#include "button.h"

static const char *TAG = "STHC3_Monitor";

// -------------- Utils -------------- //
int init_sampler(uint64_t sample_time){
    // Sampler init
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    int ret = init_i2c(i2c_bus_config);
    sampler_run(loop_event_handle, sample_time);
    return ret;
}


// -------------- Events -------------- //
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    fsm_events fsm_event;

    switch (event_id)
    {
    case NEW_DATA:
        sthc3_data recived_data = *((sthc3_data *)event_data);
        ESP_LOGI(TAG, "New Sampler Data Event -- Sensor Values -- Temp: %f  HUM: %f", 
                recived_data.temp_value, recived_data.hum_value);
        if (xQueueSendToBack(sensorDataQueue, &recived_data, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        break;
    case SAMPLER_INIT:
        ESP_LOGI(TAG, "SAMPLER INIT");
        fsm_event = FSM_SAMPLER_INIT;
        if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        break;
    
    default:
        break;
    }
}


void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    
    fsm_events fsm_event;

    switch (event_id)
    {
    case WIFI_MOCK_EVENT_WIFI_CONNECTED:
        ESP_LOGI(TAG, "Wifi Connected");
        break;
    case WIFI_MOCK_EVENT_WIFI_GOT_IP:
        ESP_LOGI(TAG, "Wifi got IP");
        fsm_event = FSM_WIFI_GOT_IP;
        if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        break;
    case WIFI_MOCK_EVENT_WIFI_DISCONNECTED:
        ESP_LOGI(TAG, "Wifi disconnected");
        fsm_event = FSM_WIFI_DISCONNECTED;
        if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        break;
    default:
        break;
    }
}

void button_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    fsm_events fsm_event;
    switch (id) {
        case BUTTON_EVENT_PRESSED:
            ESP_LOGI(TAG, "Botón PRESIONADO");
            fsm_event = FSM_BUTTON_PRESS;
            xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY);
            break;
        case BUTTON_EVENT_RELEASED:
            ESP_LOGI(TAG, "Botón LIBERADO");
            break;
    }
}


// -------------- Tasks -------------- //
void fsm_control( void * pvParameters ){

    ESP_LOGI(TAG, "FSM Control Started...");
    fsm_status = INIT;
    int sampler_init = 0;
    fsm_events event;
    sthc3_data sensor_data:

    while (1){
        //-- Check for fsm events
        if( xQueueReceive( fsmEventsQueue, &( event ), pdMS_TO_TICKS(100) )){
            ESP_LOGI(TAG, "FSM Status: %s", fsm_status2str[fsm_status]);
            ESP_LOGI(TAG, "FSM Event recived: %d", event);
            //-- Status transition
            switch (fsm_status)
            {
            case INIT:
                // Status change
                if (event == FSM_SAMPLER_INIT){
                    fsm_status = OFFLINE_MONITOR;
                    // Change to OFFLINE_MONITOR inits
                    wifi_mock_init(loop_event_handle);
                    ESP_ERROR_CHECK(wifi_connect());
                }
                break;
            case OFFLINE_MONITOR:
                if (event == FSM_WIFI_GOT_IP){
                    fsm_status = WIFI_MONITOR;
                }
                break;
            case WIFI_MONITOR:
                if (event == FSM_WIFI_DISCONNECTED){
                    fsm_status = OFFLINE_MONITOR;
                }
                else if (event == FSM_BUTTON_PRESS){
                    fsm_status = CONSOLE;
                    ESP_ERROR_CHECK(wifi_disconnect());
                }
                break;
            default:
                break;
            }
        }

        //-- Actions in each status
        switch (fsm_status)
        {
        case INIT:
            // TODO: Add timer
            if (!sampler_init){
                init_sampler(STEP_MUESTREO);
                sampler_init = 1;
            }
            break;
        case OFFLINE_MONITOR:
            if( xQueueReceive( sensorDataQueue, &( sensor_data ), pdMS_TO_TICKS(1000) )){
                ESP_LOGI(TAG, "Save Data in flash memory");
            }
            break;
        case WIFI_MONITOR:
            ESP_LOGI(TAG, "Sending flash data or queue data using wifi");
            break;
        case CONSOLE:
            ESP_LOGI(TAG, "TODO CONSOLE");
            break;
        default:
            break;
        }

    }
}


void app_main(void)
{   

    //-- Queues init
    fsmEventsQueue = xQueueCreate( 10, sizeof( fsm_events ) );
    sensorDataQueue = xQueueCreate( 10, sizeof( sthc3_data ) );

    //-- Events Definition
    //- Event loop init
    esp_event_loop_args_t loop_args = {
        .queue_size = CONFIG_ESP_SYSTEM_EVENT_QUEUE_SIZE,
        .task_name = "monitor_events",
        .task_stack_size = (2304+512),
        .task_priority = 5,
        .task_core_id = 0
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_event_handle));

    //- Wifi event handler
    esp_event_handler_instance_t instance_wifi;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_wifi));

    //- Sampler event handler
    esp_event_handler_instance_t instance_muestreador;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            SAMPLER_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_muestreador));

    esp_event_handler_instance_t instance_button;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            BUTTON_BASE_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &button_event_handler,
                                                            NULL,
                                                            &instance_button));

    // -- Start FSM Tasks
    TaskHandle_t fsmTaskHandle = NULL;
    xTaskCreate(fsm_control, "FSM", STACK_SIZE, (void *)&fsm_control, 5, &fsmTaskHandle);


    // Wifi init and connection
    //wifi_mock_init(loop_event_handle);
    //ESP_ERROR_CHECK(wifi_connect());


    
}
