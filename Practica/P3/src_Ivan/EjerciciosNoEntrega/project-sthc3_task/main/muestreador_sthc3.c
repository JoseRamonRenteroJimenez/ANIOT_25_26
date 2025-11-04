
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "shtc3.h"

#define STACK_SIZE 3072
#define STEP_MUESTREO 1000

static const char *TAG = "STHC3_Muestreador";

shtc3_t sensor;
i2c_master_bus_handle_t bus_handle;
float temp_value;
float hum_value;

void init_i2c(void) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

   shtc3_init(&sensor, bus_handle, 0x70);
}


void muestreador_task( void * pvParameters ){

    while (true){
        // Get sensor data
        shtc3_get_temp_and_hum(&sensor, &temp_value, &hum_value);
        vTaskDelay(STEP_MUESTREO / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    // I2C init
    init_i2c();

    // Define muestreador task
    TaskHandle_t muestreadorTaskHandle = NULL;
    xTaskCreate(muestreador_task, "Muestreador", STACK_SIZE, NULL, 5, &muestreadorTaskHandle);
    configASSERT( muestreadorTaskHandle );

    // Show sensor value saved
    while (true){
        ESP_LOGI(TAG, "Sensor Values -- Temp: %f  HUM: %f", temp_value, hum_value);
        int slepp_value = rand() % 5000;
        vTaskDelay(slepp_value / portTICK_PERIOD_MS);
    }
}
