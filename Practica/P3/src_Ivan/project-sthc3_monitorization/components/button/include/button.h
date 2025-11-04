#ifndef BUTTON_H
#define BUTTON_H

#include "esp_event.h"
#include "esp_timer.h"

#define BUTTON_GPIO       9        // GPIO 0 es el botón de Boot       
#define BUTTON_ACTIVE_LOW 1       

typedef enum {
    BUTTON_NOT_PRESSED = 0,
    BUTTON_PRESSED
} button_state_t;

ESP_EVENT_DECLARE_BASE(BUTTON_BASE_EVENT);

typedef enum {
    BUTTON_EVENT_PRESSED,     
    BUTTON_EVENT_RELEASED     
} button_event_id_t;

esp_err_t button_init(esp_event_loop_handle_t loop);

#endif // BUTTON_H