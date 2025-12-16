#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "protocol_examples_common.h"
#include "string.h"
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <sys/socket.h>
#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

#include "muestreador_sthc3.h"
#include "esp_timer.h"
#include "button.h"

#define HASH_LEN 32

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
/* The interface name value can refer to if_desc in esp_netif_defaults.h */
#if CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_ETH
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_ETH;
#elif CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_STA
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_STA;
#endif
#endif

static const char *TAG = "sthc3_monitor_ota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256
#define STHC3_STR_BUFFER_LEN 500
#define DIAGNOSTIC_WIFI_MAX_GET_IP_TIME 100000
#define DIAGNOSTIC_WIFI_DELAY_STEP_TIME 1000
//#define STHC3_HUM_EXTRACT // Used to generate two versions to test the OTA features (if defined the version gonna shown humidity and temperature values otherwise only temperature)

esp_event_loop_handle_t loop_event_handle;
static bool ota_in_progress = false;
static bool sampler_running = false;

void simple_ota_example_task(void *pvParameter);

// -------------- Utils -------------- //
int init_sampler(uint64_t sample_time){
    // Sampler init
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    int ret = init_i2c(i2c_bus_config);
    sampler_run(loop_event_handle, sample_time);
    return ret;
}

// -------------- Events -------------- //
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    char sensor_data_str[STHC3_STR_BUFFER_LEN];

    if (event_base == SAMPLER_EVENT){
        switch (event_id)
        {
            case NEW_DATA:
                // Sensor data queue
                sthc3_data recived_data = *((sthc3_data *)event_data);
                #ifdef STHC3_HUM_EXTRACT
                sthc3_to_string(&recived_data, sensor_data_str, STHC3_STR_BUFFER_LEN);
                #else
                snprintf(sensor_data_str, STHC3_STR_BUFFER_LEN,
                        "{ \"temp\": %.2f }",
                        recived_data.temp_value);
                #endif
                ESP_LOGI(TAG, "Data readed: %s", sensor_data_str);
                break;
            case SAMPLER_INIT:
                ESP_LOGI(TAG, "SAMPLER INIT");
                sampler_running = true;
                break;
            default:
                break;
        }
    }
    else if(event_base == BUTTON_BASE_EVENT){
        switch (event_id) 
        {
            case BUTTON_EVENT_PRESSED:
                ESP_LOGI(TAG, "Botón PRESIONADO, tratando de realizar actualziación de firmware OTA...");
                if (!ota_in_progress){
                    ota_in_progress = true;
                    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
                }
                break;
            case BUTTON_EVENT_RELEASED:
                ESP_LOGI(TAG, "Botón LIBERADO");
                break;
            default:
                break;
        }
    }
}

// -------------- OTA -------------- //
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

void simple_ota_example_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA example task");
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
    esp_netif_t *netif = get_example_netif_from_desc(bind_interface_name);
    if (netif == NULL) {
        ESP_LOGE(TAG, "Can't find netif from interface description");
        abort();
    }
    struct ifreq ifr;
    esp_netif_get_netif_impl_name(netif, ifr.ifr_name);
    ESP_LOGI(TAG, "Bind interface name is %s", ifr.ifr_name);
#endif
    esp_http_client_config_t config = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
        .crt_bundle_attach = esp_crt_bundle_attach,
#else
        .cert_pem = (char *)server_cert_pem_start,
#endif /* CONFIG_EXAMPLE_USE_CERT_BUNDLE */
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
        .if_name = &ifr,
#endif
#if CONFIG_EXAMPLE_TLS_DYN_BUF_RX_STATIC
        /* This part applies static buffer strategy for rx dynamic buffer.
         * This is to avoid frequent allocation and deallocation of dynamic buffer.
         */
        .tls_dyn_buf_strategy = HTTP_TLS_DYN_BUF_RX_STATIC,
#endif /* CONFIG_EXAMPLE_TLS_DYN_BUF_RX_STATIC */
    };

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
        ota_in_progress = false;
        vTaskDelete(NULL);
    }
}

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s %s", label, hash_print);
}

bool wifi_has_ip(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return false;
    }

    return ip_info.ip.addr != 0;
}

bool diagnostic(){
    
    // Wait and check if module gets wifi IP
    int wait_time = 0;
    while (!wifi_has_ip() && wait_time < DIAGNOSTIC_WIFI_MAX_GET_IP_TIME){
        vTaskDelay(pdMS_TO_TICKS(DIAGNOSTIC_WIFI_DELAY_STEP_TIME));
        wait_time += DIAGNOSTIC_WIFI_DELAY_STEP_TIME;
    }
    if (!wifi_has_ip()) return false;

    // Check if sampler is running
    if (!sampler_running) return false;

    return true;
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}

void app_main(void)
{   
    //-- OTA init
    ESP_LOGI(TAG, "STHC3 Monitor OTA app_main start");
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    get_sha256_of_partitions();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

    //-- Sampler an button control init
    //- Events Definition
    // Event loop init
    esp_event_loop_args_t loop_args = {
        .queue_size = CONFIG_ESP_SYSTEM_EVENT_QUEUE_SIZE,
        .task_name = "monitor_events",
        .task_stack_size = (2304+512),
        .task_priority = 5,
        .task_core_id = 0
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_event_handle));

    //- Sampler event handler
    esp_event_handler_instance_t instance_muestreador;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            SAMPLER_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_muestreador));
    
    //- Button event handler
    esp_event_handler_instance_t instance_button;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_event_handle,
                                                            BUTTON_BASE_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &instance_button));

    init_sampler(1000);
    button_init(loop_event_handle);


    //-- Check validity of current image (for OTA updates)
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        ESP_LOGI(TAG, "Partition State: %d", ota_state);
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...   
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}
