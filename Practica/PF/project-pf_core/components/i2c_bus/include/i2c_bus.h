#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "esp_err.h"
#include "driver/i2c_master.h"

// Función que inicializa el bus I²C
esp_err_t i2c_bus_init(void);

// Función para obtener el bus I²C
i2c_master_bus_handle_t i2c_bus_get(void);

#endif // I2C_BUS_H
