/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sensor_gp2y0a41sk.h"

const static char *TAG = "Distance Measurenment";
#define N CONFIG_N
#define SAMPLER_PERIOD_MS CONFIG_SAMPLER_PERIOD_MS
#define ADC_UNIT CONFIG_ADC_UNIT
#define ADC_CHANNEL CONFIG_ADC_CHANNEL
#define ADC_ATTENUATION CONFIG_ADC_ATTENUATION
#define ADC_BITHWIDTH CONFIG_ADC_BITHWIDTH

void sampler_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base == SAMPLER_EVENT) {
        switch (id) {
        case SAMPLER_INIT:
            ESP_LOGI(TAG, "Sampler initialized");
            break;

        case NEW_DATA: {
            double distance;
            get_last_sample_value(&distance);
            ESP_LOGI(TAG, "Sampler new distance: %.2f cm", distance);
            break;
        }

        default:
            ESP_LOGW(TAG, "Unknown sampler event");
            break;
        }
    }
}

void app_main(void)
{
    // ---------------------- CONFIG SENSOR ----------------------
    gp2y0a41sk_cfg_t config = {
        .unit_id = ADC_UNIT,
        .channel = ADC_CHANNEL,
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITHWIDTH,
        .n=N,
    };

    ESP_ERROR_CHECK(init_sensor(config));
    ESP_LOGI(TAG, "Sensor initialized!");

    // ---------------------- CREATE EVENT LOOP ----------------------
    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "sampler_loop",
        .task_priority = 5,
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };
    esp_event_loop_handle_t loop_event_handle;
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_event_handle));

    // Register our handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
        loop_event_handle,
        SAMPLER_EVENT,
        ESP_EVENT_ANY_ID,
        sampler_event_handler,
        NULL,
        NULL));

    ESP_LOGI(TAG, "Event loop created.");

    // ---------------------- START SAMPLER ----------------------
    uint64_t sample_time_ms = SAMPLER_PERIOD_MS;
    sampler_run(loop_event_handle, sample_time_ms);

    ESP_LOGI(TAG, "Sampler running every %llu ms", sample_time_ms);
}