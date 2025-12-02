#include "esp_adc/adc_oneshot.h"
#include "esp_event.h"

#define STACK_SIZE 3072

typedef struct{
    adc_unit_t unit_id; 
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
}gp2y0a41sk_cfg_t;

// -- Events definition
ESP_EVENT_DECLARE_BASE(SAMPLER_EVENT);
typedef enum {
    SAMPLER_INIT,
    NEW_DATA
} sampler_event_t;

// Functions
esp_err_t init_sensor(gp2y0a41sk_cfg_t cfg);
esp_err_t sampler_run(esp_event_loop_handle_t event_loop, uint64_t sample_time);
esp_err_t sampler_stop();
esp_err_t get_last_sample_value(double *value);
