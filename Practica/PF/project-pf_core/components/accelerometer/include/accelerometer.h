#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "esp_event.h"
#include "esp_err.h"

// -------- Event base --------
ESP_EVENT_DECLARE_BASE(ACCEL_EVENT_BASE);

// -------- Events --------
typedef enum {
    ACCEL_EVENT_PERTURBATION = 1
} accel_event_id_t;

// -------- API --------
esp_err_t accelerometer_init(esp_event_loop_handle_t loop_handle);

#endif
