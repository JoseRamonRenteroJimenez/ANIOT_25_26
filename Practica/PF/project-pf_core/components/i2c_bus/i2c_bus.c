#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";

static i2c_master_bus_handle_t bus_handle = NULL;

esp_err_t i2c_bus_init(void)
{
    if (bus_handle != NULL) {
        ESP_LOGI(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    // Configuración del bus I²C
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 21,   // Asegúrate de usar los pines correctos
        .scl_io_num = 22,   // Asegúrate de usar los pines correctos
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    // Crear el bus I²C
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized");
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_bus_get(void)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return NULL;
    }
    return bus_handle;
}
