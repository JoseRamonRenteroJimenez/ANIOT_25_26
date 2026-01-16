idf_component_register(SRCS "muestreador_sthc3.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_timer shtc3
                       REQUIRES "i2c_bus"
                       PRIV_REQUIRES esp_event )
