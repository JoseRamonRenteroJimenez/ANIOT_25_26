#ifndef MOCK_WIFI_H_
#define MOCK_WIFI_H_

#include "esp_event.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

enum mock_wifi_state
{
    NOT_INITIALIZED,
    INITIALIZED,
    CONNECTED,
    CONNECTED_WITH_IP,
    DISCONNECTED
};

ESP_EVENT_DECLARE_BASE(WIFI_MOCK);
enum
{
    WIFI_MOCK_EVENT_WIFI_CONNECTED,
    WIFI_MOCK_EVENT_WIFI_GOT_IP,
    WIFI_MOCK_EVENT_WIFI_DISCONNECTED
};

void wifi_mock_init(esp_event_loop_handle_t event_loop);
esp_err_t wifi_connect(void);
esp_err_t wifi_disconnect(void);
esp_err_t send_data_wifi(void *data, size_t size);

#endif // #ifndef MOCK_WIFI_H_
