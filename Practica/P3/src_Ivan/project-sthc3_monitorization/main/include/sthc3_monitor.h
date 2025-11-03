
#include "freertos/queue.h"

#define STACK_SIZE 3072
#define STEP_MUESTREO 1000
#define QUEUE_DEF_SIZE 10

//-- Structs
//- Fsm Status
typedef enum{
    INIT = 0,
    OFFLINE_MONITOR,
    WIFI_MONITOR,
    CONSOLE
}fsm_status_t;
const char* fsm_status2str[] = {
 "INIT",
 "OFFLINE_MONITOR",
 "WIFI_MONITOR",
 "CONSOLE"
};
//- Fsm Events (for queue)
typedef enum{
    FSM_SAMPLER_INIT,
    FSM_WIFI_GOT_IP,
    FSM_WIFI_DISCONNECTED,
    FSM_BUTTON_PRESS,
    FSM_MONITOR_COMMAND,

}fsm_events;

//-- Global Vars
fsm_status_t fsm_status;
esp_event_loop_handle_t loop_event_handle;
QueueHandle_t fsmEventsQueue;
QueueHandle_t sensorDataQueue;

//-- Utils
int init_sampler(uint64_t sample_time);

