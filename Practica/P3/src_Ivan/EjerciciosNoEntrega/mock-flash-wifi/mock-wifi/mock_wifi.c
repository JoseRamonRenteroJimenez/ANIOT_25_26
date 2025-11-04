#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event_base.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "mock_wifi.h"

static const char *TAG = "MOCK_WIFI";

esp_event_loop_handle_t loop_connect;

// Define event base
ESP_EVENT_DEFINE_BASE(WIFI_MOCK);

// Input params from menuconfig
#define CONNECT_DELAY CONFIG_CONNECT_DELAY
#define IP_DELAY CONFIG_IP_DELAY
#define DISCONNECT_DELAY CONFIG_DISCONNECT_DELAY

// Define timers
esp_timer_handle_t conn_timer;
esp_timer_handle_t ip_timer;
esp_timer_handle_t disconnection_timer;

uint64_t us_connect_delay = CONNECT_DELAY * 1000000;
uint64_t us_ip_delay = IP_DELAY * 1000000;
uint64_t us_disconnect_delay = DISCONNECT_DELAY * 1000000;

enum mock_wifi_state wifi_state = NOT_INITIALIZED;

const char *stateNames[] = {"NOT_INITIALIZED", "INITIALIZED", "CONNECTED", "CONNECTED_WITH_IP", "DISCONNECTED"};

static void conn_timer_callback(void *arg);
static void ip_timer_callback(void *arg);
static void disconnection_timer_callback(void *arg);

void wifi_mock_init(esp_event_loop_handle_t loop)
{
    loop_connect = loop;
    wifi_state = INITIALIZED;

    const esp_timer_create_args_t conn_timer_args = {
        .callback = &conn_timer_callback,
        .name = "conn"};
    esp_timer_create(&conn_timer_args, &conn_timer);

    const esp_timer_create_args_t ip_timer_args = {
        .callback = &ip_timer_callback,
        .name = "ip"};
    esp_timer_create(&ip_timer_args, &ip_timer);

    const esp_timer_create_args_t disconnection_timer_args = {
        .callback = &disconnection_timer_callback,
        .name = "disconnection"};
    esp_timer_create(&disconnection_timer_args, &disconnection_timer);

    ESP_LOGI(TAG, "Wifi Initialized");
}

esp_err_t wifi_connect(void)
{
    esp_timer_start_once(conn_timer, us_connect_delay);
    return ESP_OK;
}

esp_err_t wifi_disconnect(void)
{
    wifi_state = DISCONNECTED;
    esp_timer_stop(conn_timer);
    esp_timer_stop(ip_timer);
    esp_timer_stop(disconnection_timer);
    ESP_LOGI(TAG, "Wifi Disconnected, call wifi_connect() to reconnect");
    esp_event_post_to(loop_connect, WIFI_MOCK, WIFI_MOCK_EVENT_WIFI_DISCONNECTED, NULL, 0, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t send_data_wifi(void *data, size_t size)
{
    if (wifi_state != CONNECTED_WITH_IP)
    {
        ESP_LOGI(TAG, "Error sending data, invalid state -> %s", stateNames[wifi_state]);
        return ESP_ERR_INVALID_STATE;
    }
    else
    {
        ESP_LOGI(TAG, "Data '%.6f' sent successfully", *((float *)data));
        return ESP_OK;
    }
}

static void conn_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "Wifi Connected");
    wifi_state = CONNECTED;
    esp_event_post_to(loop_connect, WIFI_MOCK, WIFI_MOCK_EVENT_WIFI_CONNECTED, NULL, 0, portMAX_DELAY);
    esp_timer_start_once(ip_timer, us_ip_delay);
}

static void ip_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "Wifi got IP");
    wifi_state = CONNECTED_WITH_IP;
    esp_event_post_to(loop_connect, WIFI_MOCK, WIFI_MOCK_EVENT_WIFI_GOT_IP, NULL, 0, portMAX_DELAY);
    esp_timer_start_once(disconnection_timer, us_disconnect_delay);

}

static void disconnection_timer_callback(void *arg)
{
    wifi_disconnect();
}