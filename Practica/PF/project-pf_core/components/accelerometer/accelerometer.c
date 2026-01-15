#include "accelerometer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "math.h"
#include "driver/i2c_master.h"
#include "i2c_bus.h"  // Incluir el componente i2c_bus

//https://deepwiki.com/jamessizeland/esp32c3-devkit-demo/5.1-imu-sensor-(icm42670)

static const char *TAG = "ACCEL";

#define ICM_42670_P_ADDR 0x68

static esp_event_loop_handle_t event_loop;
static esp_timer_handle_t accel_timer;
static i2c_master_dev_handle_t accel_dev;

static void accel_timer_callback(void *arg);

static esp_err_t IMU_write(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return i2c_master_transmit(accel_dev, data, sizeof(data), -1);
}

static esp_err_t IMU_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(accel_dev, &reg, 1, data, len, -1);
}

static void accel_callback(void *arg)
{
    //TODO
}
