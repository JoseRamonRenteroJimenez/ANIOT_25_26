#include "ICM_42670_P.h"

static const char *TAG = "ICM_42670_P";

#define ICM_42670_P_ADDR 0x68

#define REG_DEVICE_CONFIG 0x11
#define REG_ACCEL_CONFIG0 0x50
#define REG_ACCEL_DATA_X1 0x1F

#define ACCEL_THRESHOLD 20000
#define TIMER_PERIOD_US (100 * 1000)

ESP_EVENT_DEFINE_BASE(ACCEL_EVENT_BASE);

static esp_event_loop_handle_t loop;
static esp_timer_handle_t accel_timer;
static i2c_master_dev_handle_t accel_dev;

static esp_err_t imu_write(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(accel_dev, data, sizeof(data), -1);
}

static esp_err_t imu_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(accel_dev, &reg, 1, data, len, -1);
}

static void accel_timer_callback(void *arg)
{
    uint8_t raw[6];
    int16_t ax, ay, az;

    if (imu_read(REG_ACCEL_DATA_X1, raw, sizeof(raw)) != ESP_OK)
    {
        return;
    }

    ax = (raw[0] << 8) | raw[1];
    ay = (raw[2] << 8) | raw[3];
    az = (raw[4] << 8) | raw[5];

    int32_t magnitude = ax * ax + ay * ay + az * az;

    if (magnitude > ACCEL_THRESHOLD)
    {
        imu_data_t imu_data = {
            .accel_magnitude = (float)magnitude};

        esp_event_post_to(
            loop,
            ACCEL_EVENT_BASE,
            ACCEL_EVENT_PERTURBATION,
            &imu_data,
            sizeof(imu_data_t),
            portMAX_DELAY);
    }
}

esp_err_t ICM_42670_P_init(esp_event_loop_handle_t loop_handle)
{
    loop = loop_handle;

    i2c_master_bus_handle_t bus = i2c_bus_get();
    if (bus == NULL)
    {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_FAIL;
    }

    i2c_device_config_t dev_cfg = {
        .device_address = ICM_42670_P_ADDR,
        .scl_speed_hz = 400000};

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &accel_dev));

    ESP_ERROR_CHECK(imu_write(REG_DEVICE_CONFIG, 0x01));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(imu_write(REG_ACCEL_CONFIG0, 0x0F));

    const esp_timer_create_args_t timer_args = {
        .callback = accel_timer_callback,
        .name = "accel_timer"};

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &accel_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(accel_timer, TIMER_PERIOD_US));

    ESP_LOGI(TAG, "ICM-42670-P initialized");
    return ESP_OK;
}
