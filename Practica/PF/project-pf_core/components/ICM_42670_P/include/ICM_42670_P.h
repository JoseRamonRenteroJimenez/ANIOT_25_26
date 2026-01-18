
#include "esp_event.h"
#include "esp_err.h"

ESP_EVENT_DECLARE_BASE(ACCEL_EVENT_BASE);

typedef enum {
    ACCEL_EVENT_PERTURBATION = 1,
} accel_event_id_t;

typedef struct {
    float accel_magnitude;
} imu_data_t;

esp_err_t ICM_42670_P_init(esp_event_loop_handle_t loop);