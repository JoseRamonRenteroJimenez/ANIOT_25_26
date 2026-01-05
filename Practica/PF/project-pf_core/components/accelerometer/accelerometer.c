#include "accelerometer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ACCEL";

ESP_EVENT_DEFINE_BASE(ACCEL_EVENT_BASE);

static esp_event_loop_handle_t accel_loop;

// Task simulando detección
static void accel_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(15000)); // cada 15 s

        ESP_LOGW(TAG, "Perturbation detected!");

        esp_event_post_to(
            accel_loop,
            ACCEL_EVENT_BASE,
            ACCEL_EVENT_PERTURBATION,
            NULL,
            0,
            portMAX_DELAY
        );
    }
}

esp_err_t accelerometer_init(esp_event_loop_handle_t loop_handle)
{
    accel_loop = loop_handle;

    xTaskCreate(
        accel_task,
        "accel_task",
        2048,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "Accelerometer initialized");
    return ESP_OK;
}
