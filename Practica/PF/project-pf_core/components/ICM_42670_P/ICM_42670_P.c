#include "ICM_42670_P.h"
#include "icm42670.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "i2c_bus.h"

static const char *TAG = "ICM_42670_P";

#define TIMER_PERIOD_US   (100 * 1000)   // 100 ms
#define ACCEL_THRESHOLD_G 1.5f

ESP_EVENT_DEFINE_BASE(ACCEL_EVENT_BASE);

static esp_event_loop_handle_t loop;
static esp_timer_handle_t accel_timer;
static icm42670_handle_t icm42670_h;

/* ---------------------------------------------------------- */

static void accel_timer_callback(void *arg)
{
    icm42670_value_t acc;
    esp_err_t ret = icm42670_get_acce_value(icm42670_h, &acc);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read accel");
        return;
    }

    float magnitude = (float)acc.z;

    if (magnitude >= ACCEL_THRESHOLD_G)
    {
        imu_data_t imu_data = {
            .accel_magnitude = magnitude
        };

        esp_event_post_to(
            loop,
            ACCEL_EVENT_BASE,
            ACCEL_EVENT_PERTURBATION,
            &imu_data,
            sizeof(imu_data),
            portMAX_DELAY);
    }
}

/* ---------------------------------------------------------- */

static esp_err_t icm42670_init(i2c_master_bus_handle_t bus)
{
    esp_err_t ret;

    ret = icm42670_create(bus, ICM42670_I2C_ADDRESS, &icm42670_h);
    if (ret != ESP_OK) return ret;

    const icm42670_cfg_t cfg = {
        .acce_fs  = ACCE_FS_2G,
        .acce_odr = ACCE_ODR_400HZ,
        .gyro_fs  = GYRO_FS_2000DPS,
        .gyro_odr = GYRO_ODR_400HZ,
    };

    ret = icm42670_config(icm42670_h, &cfg);
    if (ret != ESP_OK) return ret;

    ret = icm42670_acce_set_pwr(icm42670_h, ACCE_PWR_LOWNOISE);
    if (ret != ESP_OK) return ret;

    ret = icm42670_gyro_set_pwr(icm42670_h, GYRO_PWR_LOWNOISE);
    return ret;
}

/* ---------------------------------------------------------- */

esp_err_t ICM_42670_P_init(esp_event_loop_handle_t loop_handle)
{
    loop = loop_handle;

    i2c_master_bus_handle_t bus = i2c_bus_get();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(icm42670_init(bus));

    const esp_timer_create_args_t timer_args = {
        .callback = accel_timer_callback,
        .name = "accel_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &accel_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(accel_timer, TIMER_PERIOD_US));

    ESP_LOGI(TAG, "ICM-42670-P initialized");
    return ESP_OK;
}
