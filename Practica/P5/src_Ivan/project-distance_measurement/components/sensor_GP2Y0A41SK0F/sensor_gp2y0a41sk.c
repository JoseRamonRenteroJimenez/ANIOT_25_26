#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "sensor_gp2y0a41sk.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

const static char *TAG = "Sensor GP2Y0A41SK";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_6
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_chan0_handle = NULL;
static bool do_calibration1_chan0;
static esp_event_loop_handle_t loop;
static esp_timer_handle_t sampler_timer;
static SemaphoreHandle_t value_mutex;
static double last_distance_value;
static int N = 1;
ESP_EVENT_DEFINE_BASE(SAMPLER_EVENT);

static void sampler_timer_callback(void *arg);

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
}


static double volts2distance(int measured_voltage){

    double volts = ((double)measured_voltage / 1000);
    double distance = 0.1187 + 11.0277/volts;
    return distance;
}

esp_err_t init_sensor(gp2y0a41sk_cfg_t cfg){

    esp_err_t ret = ESP_OK; 

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = cfg.unit_id,
    };
    ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = cfg.atten,
        .bitwidth = cfg.bitwidth,
    };
    ret = adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config);

    //-------------ADC1 Calibration Init---------------//
    do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);

    //-------------Mutex used by get_last_sample_value()---------------//
    value_mutex = xSemaphoreCreateMutex();
    if (value_mutex == NULL) {
        ret = ESP_FAIL;
    }

    N = cfg.n;

    return ret;
}

esp_err_t sampler_run(esp_event_loop_handle_t event_loop, uint64_t sample_time_ms){
    // Init event post
    loop = event_loop;
    esp_event_post_to(loop, SAMPLER_EVENT, SAMPLER_INIT, NULL, 0, portMAX_DELAY); 
    ESP_LOGI(TAG, "Sampler init -- sample time: %" PRIu64 " ms", sample_time_ms);

    // Timer start
    const esp_timer_create_args_t sampler_timer_args = {
        .callback = &sampler_timer_callback,
        .arg = NULL,
        .name = "sampler_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&sampler_timer_args, &sampler_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sampler_timer, sample_time_ms * 1000));

    return ESP_OK;
}

esp_err_t sampler_stop(void) {

    esp_err_t ret = esp_timer_stop(sampler_timer);

    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_timer_delete(sampler_timer);
    sampler_timer = NULL;
    return ret;
}


esp_err_t get_last_sample_value(double *value){

    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(value_mutex, portMAX_DELAY)) {
        *value = last_distance_value;
        xSemaphoreGive(value_mutex);
    }
    else{
        ret = ESP_FAIL;
    }

    return ret; 
}


// -------------- Timer Callback -------------- //
static void sampler_timer_callback(void *arg) {
    // Get sensor data
    int adc_raw;
    int voltage;
    double distance;

    for (int i = 0; i < N; i++){
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &voltage));
        distance += volts2distance(voltage);
    }
    distance /= N;

    // Write in shared var
    if (xSemaphoreTake(value_mutex, portMAX_DELAY)) {
        last_distance_value = distance;
        xSemaphoreGive(value_mutex);
    }

    // Post event
    esp_event_post_to(loop, SAMPLER_EVENT, NEW_DATA, (void *)&distance, sizeof(double), portMAX_DELAY);
}
