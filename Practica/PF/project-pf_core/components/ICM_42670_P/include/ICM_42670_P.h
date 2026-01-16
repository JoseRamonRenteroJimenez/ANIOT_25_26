#ifndef ICM_42670_P_H
#define ICM_42670_P_H

#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "math.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_bus.h"

// -------- Event base --------
ESP_EVENT_DECLARE_BASE(ACCEL_EVENT_BASE);

// -------- Events --------
typedef enum {
    ACCEL_EVENT_PERTURBATION = 1
} accel_event_id_t;

// -------- API --------
esp_err_t ICM_42670_P_init(esp_event_loop_handle_t loop_handle);

#endif
