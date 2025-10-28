idf_component_register(SRCS "mock_wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES "driver" "esp_event" "esp_timer")