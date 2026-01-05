idf_component_register(SRCS "accelerometer.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "esp_event"
                       REQUIRES driver "esp_timer" "esp_driver_gpio")