idf_component_register(
    SRCS "ICM_42670_P.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer esp_event i2c_bus icm42670
)