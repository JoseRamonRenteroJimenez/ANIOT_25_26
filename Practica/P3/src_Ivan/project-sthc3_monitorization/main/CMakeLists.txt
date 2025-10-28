idf_component_register(SRCS "sthc3_monitor.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES "esp_event"
                       REQUIRES "mock-wifi" "mock-flash" "shtc3_sampler")
