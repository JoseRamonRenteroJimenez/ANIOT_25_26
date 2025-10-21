idf_component_register(SRCS "i2c_temp_sensor.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       REQUIRES shtc3)


