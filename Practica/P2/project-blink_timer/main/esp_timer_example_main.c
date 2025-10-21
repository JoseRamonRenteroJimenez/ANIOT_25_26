/* esp_timer (high resolution timer) example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

#define BLINK_TIME_STEP 500000
#define BLINK_GPIO 7

static const char* TAG = "example";
static uint8_t s_led_state = 0;

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    s_led_state = !s_led_state;
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void blink_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Blink timer called, time since boot: %lld us", time_since_boot);
    blink_led();
}

void app_main(void)
{
    // Led config
    configure_led();

    // Timer init
    const esp_timer_create_args_t blink_timer_args = {
            .callback = &blink_timer_callback,
            .name = "blink_timer"
    };

    esp_timer_handle_t blink_timer;
    ESP_ERROR_CHECK(esp_timer_create(&blink_timer_args, &blink_timer));


    // Start the timer 
    ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, BLINK_TIME_STEP));
    ESP_LOGI(TAG, "Started blink timer, time since boot: %lld us", esp_timer_get_time());

    /* Let the timer run for a little bit more */
    usleep(2000000);

    /* Clean up and finish the example */
    ESP_ERROR_CHECK(esp_timer_stop(blink_timer));
    ESP_ERROR_CHECK(esp_timer_delete(blink_timer));
    ESP_LOGI(TAG, "Stopped and deleted timers");
}
