
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "SI7021_Monitor";

#define I2C_SCL_PIN             CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_SDA_PIN             CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_FREQ_HZ             CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */

#define I2C_DEV_ADDR            0x40
#define RH_MEASUREMENT_COMMAND  0xE5
#define TEMP_READ_COMMAND       0xE0


static float si7021_read_temperature(i2c_master_dev_handle_t dev)
{
    /* We need to call RH_MEASUREMENT_COMMAND before reading, 
       to make sure we take a new temperature value */

    uint8_t recive_buffer[2];
    uint8_t rh_command_byte = RH_MEASUREMENT_COMMAND;
    uint8_t temp_command_byte = TEMP_READ_COMMAND;

    // Request new measurement
    ESP_ERROR_CHECK(i2c_master_transmit(dev, &rh_command_byte, 1, -1));

    // Wait time
    vTaskDelay(pdMS_TO_TICKS(25));

    // Request temp value
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev, &temp_command_byte, 1, recive_buffer, 2, -1));

    // Value conversion
    uint16_t raw = recive_buffer[0] << 8 | recive_buffer[1];
    float temp_c = ((175.72f * raw) / 65536.0f) - 46.85f;
    return temp_c;
}

static float si7021_read_humidity(i2c_master_dev_handle_t dev)
{
    uint8_t recive_buffer[2];
    uint8_t command_byte = RH_MEASUREMENT_COMMAND;

    // Send Command
    ESP_ERROR_CHECK(i2c_master_transmit(dev, &command_byte, 1, -1));

    // Wait time
    vTaskDelay(pdMS_TO_TICKS(25));

    // Recive Data
    ESP_ERROR_CHECK(i2c_master_receive(dev, recive_buffer, 2, -1));

    // Value conversion
    uint16_t raw = recive_buffer[0] << 8 | recive_buffer[1];
    float rh = ((125.0f * raw) / 65536.0f) - 6.0f;
    if (rh > 100.0f) rh = 100.0f;
    if (rh < 0.0f) rh = 0.0f;
    return rh;
}

void app_main(void)
{
    // Configurar el bus I2C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // Añadir el dispositivo Si7021
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEV_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    i2c_master_dev_handle_t si7021;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &si7021));

    // Bucle principal: alternar lecturas cada segundo
    while (1) {
        float temp = si7021_read_temperature(si7021);
        if (temp > -100.0f)
            ESP_LOGI(TAG, " Temperatura: %.2f °C", temp);
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo

        float hum = si7021_read_humidity(si7021);
        if (hum >= 0.0f)
            ESP_LOGI(TAG, " Humedad: %.2f %%", hum);
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo antes del siguiente ciclo
    }

    // Limpieza
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(si7021));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
}