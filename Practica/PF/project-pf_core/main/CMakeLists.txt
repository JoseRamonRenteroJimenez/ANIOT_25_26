idf_component_register(SRCS "pf_core.c"
                       PRIV_REQUIRES "esp_event" "spi_flash" "shtc3_sampler" "button" "https_ota" "esp_wifi" "ICM_42670_P"
                       "nvs_flash"
                       INCLUDE_DIRS "include")
