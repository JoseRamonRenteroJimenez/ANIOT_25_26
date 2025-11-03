
#include "shtc3.h"
#define STACK_SIZE 3072
#define STEP_MUESTREO 1000

typedef struct{
    float temp_value;
    float hum_value;
}sthc3_data;

// -- Events definition
ESP_EVENT_DECLARE_BASE(SAMPLER_EVENT);
typedef enum {
    SAMPLER_INIT,
    NEW_DATA
} sampler_event_t;

// Functions
int init_i2c(i2c_master_bus_config_t conf);
void sampler_run(esp_event_loop_handle_t event_loop, uint64_t sample_time);


