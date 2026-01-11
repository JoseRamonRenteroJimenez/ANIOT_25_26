#ifndef SI7021_SENSOR_H
#define SI7021_SENSOR_H

#include "esp_err.h"
#include "esp_event.h"

#define TEMPERATURE_THRESHOLD 20.0  // Umbral de temperatura (en °C)

typedef struct {
    float temperature;
    float humidity;
} si7021_sensor_data_t;

ESP_EVENT_DECLARE_BASE(SI7021_SENSOR_EVENT);

esp_err_t si7021_sensor_init(esp_event_loop_handle_t event_loop_handle);
esp_err_t si7021_sensor_read(si7021_sensor_data_t *data);
esp_err_t si7021_sensor_check_threshold(si7021_sensor_data_t *data);

#endif // SI7021_SENSOR_H
