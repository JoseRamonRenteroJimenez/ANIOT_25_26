
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
#include "esp_timer.h"
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

int init_wifi(){
    wifi_mock_init(loop_event_handle);
    int ret = wifi_connect();
    return ret;
}
// -------------- Events -------------- //
void sampler_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    fsm_events fsm_event;

    switch (event_id)
    {
    case NEW_DATA:
        // Sensor data queue
        sthc3_data recived_data = *((sthc3_data *)event_data);
        if (xQueueSendToBack(sensorDataQueue, &recived_data, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        // Fsm event queue
        fsm_event = FSM_DATA_IN_SENSOR_QUEUE;
        if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
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
    fsm_events event;
    sthc3_data sensor_data;
    char sensor_data_str[STHC3_STR_BUFFER_LEN];

    while (1){
        //-- Check for fsm events
        if( xQueueReceive( fsmEventsQueue, &( event ), portMAX_DELAY )){
            //-- Transition Table
            ESP_LOGI(TAG, "FSM Status: %s", fsm_status2str[fsm_status]);
            ESP_LOGI(TAG, "FSM Event recived: %s", fsm_events2str[event]);
            //- Status transition
            switch (fsm_status)
            {
            case INIT:
                if (event == FSM_SAMPLER_INIT){
                    fsm_status = OFFLINE_MONITOR;
                    init_wifi();
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
                    init_wifi();
                }
                else if (event == FSM_BUTTON_PRESS){
                    fsm_status = CONSOLE;
                    ESP_ERROR_CHECK(wifi_disconnect());
                }
                break;
            case CONSOLE:
                if (event == FSM_MONITOR_COMMAND){
                    fsm_status = OFFLINE_MONITOR;
                    init_wifi();
                }
            default:
                break;
            }
        
            //-- Actions in each status
            switch (fsm_status)
            {
            case INIT:
                init_sampler(SAMPLER_PERIOD_MS);
                ESP_ERROR_CHECK(mock_flash_init(MOCK_FLASH_SIZE));
                break;
            case OFFLINE_MONITOR:
                if( event == FSM_DATA_IN_SENSOR_QUEUE && 
                    xQueueReceive( sensorDataQueue, &( sensor_data ), portMAX_DELAY )){
                    if(writeToFlash((void *) &sensor_data, sizeof(sthc3_data)) != ESP_OK){
                        ESP_LOGW(TAG, "Error saving data in Flash Memory, discarding value");
                    }
                }
                break;
            case WIFI_MONITOR:
                //-- Check flash data and send it
                while(getDataLeft() != 0){
                    sensor_data = *(sthc3_data *) readFromFlash(sizeof(sthc3_data));
                    sthc3_to_string(&sensor_data, sensor_data_str, STHC3_STR_BUFFER_LEN);
                    if(send_data_wifi((void *) &sensor_data_str, STHC3_STR_BUFFER_LEN) != ESP_OK){
                        ESP_LOGW(TAG, "Error sending wifi data, data writen to Flash Memory");
                        if(writeToFlash((void *) &sensor_data, sizeof(sthc3_data)) != ESP_OK){
                            ESP_LOGW(TAG, "Error saving data in Flash Memory, discarding value");
                        }
                        
                    }
                }
                //-- Send queue data
                if( event == FSM_DATA_IN_SENSOR_QUEUE && 
                    xQueueReceive( sensorDataQueue, &( sensor_data ), portMAX_DELAY )){
                    sthc3_to_string(&sensor_data, sensor_data_str, STHC3_STR_BUFFER_LEN);
                    if(send_data_wifi((void *) &sensor_data_str, STHC3_STR_BUFFER_LEN) != ESP_OK){
                        ESP_LOGW(TAG, "Error sending wifi data, data writen to Flash Memory");
                        if(writeToFlash((void *) &sensor_data, sizeof(sthc3_data)) != ESP_OK){
                            ESP_LOGW(TAG, "Error saving data in Flash Memory, discarding value");
                        }
                    }
                }
                break;
            case CONSOLE:
                ESP_LOGI(TAG, "TODO CONSOLE");
                break;
            default:
                break;
            }
            
        }
    }
}


void app_main(void)
{   

    //-- Queues init
    fsmEventsQueue = xQueueCreate( QUEUE_DEF_SIZE, sizeof( fsm_events ) );
    sensorDataQueue = xQueueCreate( QUEUE_DEF_SIZE, sizeof( sthc3_data ) );

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
                                                            &sampler_event_handler,
                                                            NULL,
                                                            &instance_muestreador));

    //-- Start FSM Tasks
    TaskHandle_t fsmTaskHandle = NULL;
    xTaskCreate(fsm_control, "FSM", STACK_SIZE, (void *)&fsm_control, 5, &fsmTaskHandle);

    //-- Send FSM start event to queue
    fsm_events fsm_event = FSM_START;
    if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
        ESP_LOGE(TAG, "Can't write in queue");
    }
    
}
