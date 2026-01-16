idf_component_register(SRCS "ICM_42670_P.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "esp_event"
                       REQUIRES "i2c_bus"
                       REQUIRES driver "esp_timer" "esp_driver_gpio")