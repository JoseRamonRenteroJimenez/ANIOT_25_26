idf_component_register(SRCS "muestreador_sthc3.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_event 
                       REQUIRES shtc3)
