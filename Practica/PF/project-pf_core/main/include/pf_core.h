
#define FSM_STACK_SIZE 8192
#define QUEUE_DEF_SIZE 20
#define STHC3_STR_BUFFER_LEN 500
#define SAMPLER_PERIOD_MS CONFIG_SAMPLER_PERIOD_MS

//-- Structs
//- Fsm Status
typedef enum{
    INIT = 0,
    OTA_VALIDATION,
    ACTIVE,
    OTA_UPDATE,
    DEEP_SLEEP
}fsm_status_t;
const char* fsm_status2str[] = {
 "INIT",
 "OTA_VALIDATION",
 "ACTIVE",
 "OTA_UPDATE",
 "DEEP_SLEEP"
};

//- Fsm Events (for queue)
typedef enum{
    FSM_START,
    FSM_COMPONENTS_INIT,
    FSM_VALID_OTA_IMG,
    FSM_INVALID_OTA_IMG,
    FSM_OTA_SUCCES,
    FSM_OTA_FAILURE,
    FSM_BUTTON_PRESS,
    FSM_DEEP_SLEEP_START,
    FSM_DEEP_SLEEP_STOP,
    FSM_SHTC3_DATA_IN_SENSOR_QUEUE,
    FSM_ACCEL_DATA_IN_SENSOR_QUEUE
}fsm_events;
const char* fsm_events2str[] = {
 "FSM_START",
 "FSM_COMPONENTS_INIT",
 "FSM_VALID_OTA_IMG",
 "FSM_INVALID_OTA_IMG",
 "FSM_OTA_SUCCES",
 "FSM_OTA_FAILURE",
 "FSM_BUTTON_PRESS",
 "FSM_DEEP_SLEEP_START",
 "FSM_DEEP_SLEEP_STOP",
 "FSM_SHTC3_DATA_IN_SENSOR_QUEUE",
 "FSM_ACCEL_DATA_IN_SENSOR_QUEUE"
};

// Sensors Data
typedef struct{
    sthc3_data sthc3;
    float accel;
}sensors_data;

//-- Utils
esp_err_t init_shtc3_sampler(uint64_t sample_time, esp_event_loop_handle_t loop_handle);
esp_err_t init_nvs();
esp_err_t init_wifi(esp_event_loop_handle_t loop_handle);
esp_err_t init_button(esp_event_loop_handle_t loop_handle);
esp_err_t init_accelerometer(esp_event_loop_handle_t loop_handle);
esp_err_t init_components(uint64_t sample_time, esp_event_loop_handle_t loop_handle);
bool check_components(); // Blocking call
bool wifi_has_ip();
bool diagnostic();

