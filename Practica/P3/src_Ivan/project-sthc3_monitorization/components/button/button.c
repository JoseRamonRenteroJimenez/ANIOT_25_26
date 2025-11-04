#include "button.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BUTTON";

ESP_EVENT_DEFINE_BASE(BUTTON_BASE_EVENT);

static esp_event_loop_handle_t button_event_loop;
static volatile button_state_t last_state = BUTTON_NOT_PRESSED;

static esp_timer_handle_t debounce_timer = NULL;
static volatile bool debounce_active = false;

#define DEBOUNCE_TIME_US 200000  // 200 ms

static void IRAM_ATTR button_handler(void *arg)
{
    if (!debounce_active) {
        debounce_active = true;
        esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_US);
    }
}

static void debounce_timer_callback(void *arg)
{
    debounce_active = false;
    int level = gpio_get_level(BUTTON_GPIO);

#if BUTTON_ACTIVE_LOW
    bool pressed = (level == 0);
#else
    bool pressed = (level == 1);
#endif

    if (pressed) {
        ESP_LOGI(TAG, "Botón presionado");
        esp_event_post_to(button_event_loop, BUTTON_BASE_EVENT, BUTTON_EVENT_PRESSED, NULL, 0, portMAX_DELAY);
    }
}

esp_err_t button_init(esp_event_loop_handle_t loop)
{
    button_event_loop = loop;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),  //Con este bit mask elegimos cuál de los pines configurar
        .mode = GPIO_MODE_INPUT,                //Entrada digital 
#if BUTTON_ACTIVE_LOW
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,         /*!< GPIO interrupt type : falling edge*/
#else
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,         /*!< GPIO interrupt type : rising edge*/
#endif
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    const esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer"
    };
    esp_timer_create(&debounce_timer_args, &debounce_timer);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_handler, NULL);

    ESP_LOGI(TAG, "Botón inicializado en GPIO %d", BUTTON_GPIO);
    return ESP_OK;
}