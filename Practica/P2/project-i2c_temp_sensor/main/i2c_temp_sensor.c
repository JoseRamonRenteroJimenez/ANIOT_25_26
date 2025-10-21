/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "shtc3.h"

shtc3_t tempSensor;
i2c_master_bus_handle_t bus_handle;


void init_i2c(void) {
    uint16_t id;
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

   shtc3_init(&tempSensor, bus_handle, 0x70);
}

void app_main(void)
{
   
    // I2C init
    init_i2c();

    // Get temp
    float temp_val;
    float hum_val;
    shtc3_get_temp_and_hum(&tempSensor, &temp_val, &hum_val);
    printf("Current Temp: %f", temp_val);
}
