idf_component_register(SRCS "si7021_sensor.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "esp_event"
                       REQUIRES driver "esp_timer" "esp_driver_gpio")