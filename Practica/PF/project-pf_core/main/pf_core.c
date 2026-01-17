
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "muestreador_sthc3.h"
#include "button.h"
#include "https_ota.h"
#include "pf_core.h"

#define WIFI_SSID CONFIG_EXAMPLE_WIFI_SSID
#define WIFI_PSW CONFIG_EXAMPLE_WIFI_PASSWORD
#define SAMPLER_PERIOD_MS CONFIG_SAMPLER_PERIOD_MS
#define DEEP_SLEE_TIMER_MS CONFIG_DEPP_SLEEP_TIME_MS
#define DIAGNOSTIC_WIFI_MAX_GET_IP_TIME 100000
#define DIAGNOSTIC_WIFI_DELAY_STEP_TIME 1000
#define COMPONENTS_START_TIMEOUT_ATTEMPS 60
#define COMPONENTS_START_TIMEOUT_ATTEMP_TIME_MS 1000 // 1 seg


//-- Global Vars
static const char *TAG = "PF_CORE";
fsm_status_t fsm_status;
esp_event_loop_handle_t loop_event_handle;
QueueHandle_t fsmEventsQueue;
QueueHandle_t sensorDataQueue;
bool shtc3_sampler_init = false;
bool wifi_init = false;

static esp_timer_handle_t deep_sleep_timer;
static void deep_sleep_timer_callback(void *arg);
int sleep_mode = 0; // active = 0, deep_sleep = 1

// -------------- Utils -------------- //
esp_err_t init_shtc3_sampler(uint64_t sample_time, esp_event_loop_handle_t loop_handle){
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
    sampler_run(loop_handle, sample_time);
    return ret;
}

esp_err_t init_nvs(){
    // NVS Init
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        err = nvs_flash_erase();
        if (!err){
            err = nvs_flash_init();
        }
    }
    return err; 
}

esp_err_t init_wifi(esp_event_loop_handle_t loop_handle){

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK){
        return err;
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK){
        return err;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PSW,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK){
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK){
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK){
        return err;
    }
    err = esp_wifi_connect();
    if (err != ESP_OK){
        return err;
    }

    return err;
}

esp_err_t init_button(esp_event_loop_handle_t loop_handle){
    esp_err_t err = button_init(loop_handle);
    return err;
}

esp_err_t init_accelerometer(esp_event_loop_handle_t loop_handle){
    // TODO
    return ESP_OK;
}

esp_err_t init_components(uint64_t sample_time, esp_event_loop_handle_t loop_handle){

    // Init components
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(init_wifi(loop_handle));
    ESP_ERROR_CHECK(init_button(loop_handle));
    ESP_ERROR_CHECK(init_accelerometer(loop_handle));
    ESP_ERROR_CHECK(init_shtc3_sampler(sample_time, loop_handle));

    // Create deep sleep timer
    const esp_timer_create_args_t deep_sleep_timer_args = {
        .callback = &deep_sleep_timer_callback,
        .arg = NULL,
        .name = "deep_sleep_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&deep_sleep_timer_args, &deep_sleep_timer));
    return ESP_OK;
}

bool check_components(){

    // Check components
    for (int i = 0; i < COMPONENTS_START_TIMEOUT_ATTEMPS; i++){
        // Check components status
        if (shtc3_sampler_init && wifi_init){
            return true;
        }
        vTaskDelay(COMPONENTS_START_TIMEOUT_ATTEMP_TIME_MS / portTICK_PERIOD_MS);
    }
    return false;
}

bool wifi_has_ip(){
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return false;
    }

    return ip_info.ip.addr != 0;
}


bool diagnostic(){

    // Check image validity
    if(! https_ota_is_pending_verify()){
        ESP_LOGI(TAG, "Diagnostic: Image previously validated, diagnostic skipped");
        return true;
    }
    ESP_LOGI(TAG, "Diagnostic: Image downloaded via OTA, starting diagnostic...");

    // Diagnostic (Check wifi_status...)
    int wait_time = 0;
    while (!wifi_has_ip() && wait_time < DIAGNOSTIC_WIFI_MAX_GET_IP_TIME){
        vTaskDelay(DIAGNOSTIC_WIFI_DELAY_STEP_TIME / portTICK_PERIOD_MS);
        wait_time += DIAGNOSTIC_WIFI_DELAY_STEP_TIME;
    }
    if (!wifi_has_ip()) return false;
    ESP_LOGI(TAG, "Diagnostic: Wifi module working (ip getted)");

    // Check if sampler is running
    if (!shtc3_sampler_init) return false;
    ESP_LOGI(TAG, "Diagnostic: Sampler module working");


    return true;
}

// -------------- Callbacks -------------- //
static void deep_sleep_timer_callback(void *arg){

    // Invert mode
    sleep_mode = !sleep_mode;

    // Post change event
    fsm_events fsm_event;
    if (sleep_mode){
        fsm_event = FSM_DEEP_SLEEP_START;
    }
    else{
        fsm_event = FSM_DEEP_SLEEP_STOP;
    }
    if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
        ESP_LOGE(TAG, "Can't write in queue");
    }

    // TODO -> Change to deep sleep

}


// -------------- Events Handlers -------------- //
void shtc3_sampler_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    fsm_events fsm_event;

    switch (event_id)
    {
    case NEW_DATA:
        if(fsm_status == ACTIVE){
            // Sensor data queue
            sensors_data data2send;
            sthc3_data sthc3_recived_data = *((sthc3_data *)event_data);
            data2send.sthc3 = sthc3_recived_data;
            if (xQueueSendToBack(sensorDataQueue, &data2send, portMAX_DELAY ) != pdPASS ){
                ESP_LOGE(TAG, "Can't write in queue");
            }
            // Fsm event queue
            fsm_event = FSM_SHTC3_DATA_IN_SENSOR_QUEUE;
            if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
                ESP_LOGE(TAG, "Can't write in queue");
            }
        }
        break;
    case SAMPLER_INIT:
        ESP_LOGI(TAG, "SAMPLER INIT");
        shtc3_sampler_init = true;
        break;
    
    default:
        break;
    }
}

static void wifi_event_handler(void *arg,esp_event_base_t event_base, int32_t event_id, void *event_data){
    if (event_base == WIFI_EVENT) {
        switch (event_id) {

        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi init, connecting...");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected, retrying....");
            init_wifi(loop_event_handle);
            break;

        default:
            break;
        }
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "IP getted: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_init = true; 
    }
}

void button_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    fsm_events fsm_event;
    switch (id) {
        case BUTTON_EVENT_PRESSED:
            ESP_LOGI(TAG, "Button Pressed");
            fsm_event = FSM_BUTTON_PRESS;
            if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
                ESP_LOGE(TAG, "Can't write in queue");
            }
            break;
        case BUTTON_EVENT_RELEASED:
            ESP_LOGI(TAG, "Button Released");
            break;
    }
}

// -------------- Tasks -------------- //
void fsm_control( void * pvParameters ){

    ESP_LOGI(TAG, "FSM Control Started...");
    fsm_status = INIT;
    fsm_events event;
    fsm_events fsm_event;
    sensors_data sensor_data;
    char sensor_data_str[STHC3_STR_BUFFER_LEN];

    while (1){
        //-- Check for fsm events
        if( xQueueReceive( fsmEventsQueue, &( event ), portMAX_DELAY )){
            #ifdef CONFIG_DEBUG_LOG
            ESP_LOGI(TAG, "FSM Status: %s", fsm_status2str[fsm_status]);
            ESP_LOGI(TAG, "FSM Event recived: %s", fsm_events2str[event]);
            #endif
            //- Status transition
            switch (fsm_status)
            {
            case INIT: {
                if (event == FSM_COMPONENTS_INIT){
                    fsm_status = OTA_VALIDATION;
                }
                break;
            }

            case OTA_VALIDATION: {
                if (event == FSM_VALID_OTA_IMG){
                    ESP_LOGI(TAG, "Diagnostic succesful. Markin image as valid");
                    https_ota_mark_valid();
                    fsm_status = ACTIVE;
                    esp_timer_stop(deep_sleep_timer);
                    esp_timer_start_once(deep_sleep_timer, DEEP_SLEE_TIMER_MS * 1000);
                }
                else if (event == FSM_INVALID_OTA_IMG){
                    ESP_LOGI(TAG, "Diagnostic failed. Marking image as invalid and rebooting...");
                    https_ota_mark_invalid_and_reboot();
                }
                break;
            }

            case ACTIVE: {
                if (event == FSM_BUTTON_PRESS){
                    esp_timer_stop(deep_sleep_timer);
                    fsm_status = OTA_UPDATE;
                }
                else if (event == FSM_DEEP_SLEEP_START){
                    fsm_status = DEEP_SLEEP;
                    esp_timer_stop(deep_sleep_timer);
                    esp_timer_start_once(deep_sleep_timer, DEEP_SLEE_TIMER_MS * 1000);
                }
                break;
            }

            case OTA_UPDATE: {
                if (event == FSM_OTA_FAILURE){
                    ESP_LOGW(TAG, "OTA Failed -> Can't get image from HTTP server, returning to Active mode...");
                    esp_timer_start_once(deep_sleep_timer, DEEP_SLEE_TIMER_MS * 1000);
                    fsm_status = ACTIVE;
                }
                else if (event == FSM_OTA_SUCCES){
                    esp_restart();
                }
                break; 
            }

            case DEEP_SLEEP: {
                if (event == FSM_DEEP_SLEEP_STOP){
                    fsm_status = ACTIVE;
                    esp_timer_stop(deep_sleep_timer);
                    esp_timer_start_once(deep_sleep_timer, DEEP_SLEE_TIMER_MS * 1000);
                }
                break;
            }

            default: {
                break;
            }
            }

            //-- Actions in each status
            switch (fsm_status)
            {
            case INIT: {
                if (event == FSM_START){
                    init_components(SAMPLER_PERIOD_MS, loop_event_handle);
                    bool components_init = check_components();
                    if(components_init){
                        fsm_event = FSM_COMPONENTS_INIT;
                        if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY) != pdPASS){
                            ESP_LOGE(TAG, "Can't write in queue");
                        }
                    }
                    else{
                        ESP_LOGE(TAG, "Error loading components, restarting esp...");
                        esp_restart();
                    }
                }
                break;
            }

            case OTA_VALIDATION: {
                if (event == FSM_COMPONENTS_INIT){
                    bool img_is_ok = diagnostic();
                    fsm_event = img_is_ok ? FSM_VALID_OTA_IMG : FSM_INVALID_OTA_IMG;
                    if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY) != pdPASS){
                        ESP_LOGE(TAG, "Can't write in queue");
                    }
                }
                break;
            }

            case ACTIVE: {
                if (event == FSM_SHTC3_DATA_IN_SENSOR_QUEUE &&
                    xQueueReceive(sensorDataQueue, &sensor_data, 0)){
                    sthc3_data sthc3_sensor_data = sensor_data.sthc3;
                    sthc3_to_string(&sthc3_sensor_data, sensor_data_str, STHC3_STR_BUFFER_LEN);
                    ESP_LOGI(TAG, "Sensor Data: %s", sensor_data_str);
                }
                else if (event == FSM_ACCEL_DATA_IN_SENSOR_QUEUE &&
                    xQueueReceive(sensorDataQueue, &sensor_data, 0)){
                    float accel_sensor_data = sensor_data.accel;
                    ESP_LOGI(TAG, "Accelerometer Sensor Data: %.6f", accel_sensor_data);
                }
                break;
            }

            case OTA_UPDATE: {
                if(event == FSM_BUTTON_PRESS){
                    ESP_LOGI(TAG, "OTA Download");
                    esp_err_t err = https_ota_download(NULL);
                    ESP_LOGI(TAG, "OTA Result: %d",  err);
                    fsm_event = (err == ESP_OK) ? FSM_OTA_SUCCES : FSM_OTA_FAILURE;
                    if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY) != pdPASS){
                        ESP_LOGE(TAG, "Can't write in queue");
                    }
                }
                break;
            }

            case DEEP_SLEEP: {
                if (event == FSM_DEEP_SLEEP_START){
                    ESP_LOGI(TAG, "DEEP SLEEP: TODO");
                }
                break;
            }

            default: {
                break;
            }
        }

        // If FSM_SHTC3_DATA_IN_SENSOR_QUEUE activate FSM but status it's not ACTIVE
        // clean sensorDataQueue associate with the activation
        if (fsm_status != ACTIVE &&
            event == FSM_SHTC3_DATA_IN_SENSOR_QUEUE &&
            xQueueReceive(sensorDataQueue, &sensor_data, 0)){
                #ifdef CONFIG_DEBUG_LOG
                ESP_LOGW(TAG, "sensorDataQueue value discarded");
                #endif
                (void)0;
            }
        }
    }
}


void app_main(void)
{
    ESP_LOGI(TAG, "PF_CORE Start");

    //-- Queues init
    fsmEventsQueue = xQueueCreate( QUEUE_DEF_SIZE, sizeof( fsm_events ) );
    sensorDataQueue = xQueueCreate( QUEUE_DEF_SIZE, sizeof( sensors_data ) );

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
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_instance_t instance_wifi;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_wifi));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_wifi));
    

    //- Sampler event handler
    esp_event_handler_instance_t instance_muestreador;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            SAMPLER_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &shtc3_sampler_event_handler,
                                                            NULL,
                                                            &instance_muestreador));
    
    //- Button event handler
    esp_event_handler_instance_t instance_button;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            BUTTON_BASE_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &button_event_handler,
                                                            NULL,
                                                            &instance_button)); 

    //-- Start FSM Tasks
    TaskHandle_t fsmTaskHandle = NULL;
    xTaskCreate(fsm_control, "FSM", FSM_STACK_SIZE, (void *)&fsm_control, 5, &fsmTaskHandle);

    //-- Send FSM start event to queue
    fsm_events fsm_event = FSM_START;
    if (xQueueSendToBack(fsmEventsQueue, &fsm_event, portMAX_DELAY ) != pdPASS ){
        ESP_LOGE(TAG, "Can't write in queue");
    }
}
