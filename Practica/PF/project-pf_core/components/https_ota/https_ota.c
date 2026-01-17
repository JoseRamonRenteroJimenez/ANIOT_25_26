#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#include "https_ota.h"
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif
#if CONFIG_EXAMPLE_HTTPS_OTA_TEST
#include "esp_wifi.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#endif

static const char *TAG = "https_ota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_err_t https_ota_download(const https_ota_config_t *config)
{
    // Default config
    https_ota_config_t cfg = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        .cert_pem = (const char *)server_cert_pem_start,
        .skip_common_name_check = true,
        .reboot_after_update = false,
        .timeout_ms = 100000,
    };

    // USer config
    if (config != NULL) {
        cfg = *config;
    }

    esp_http_client_config_t http_cfg = {
        .url = cfg.url,
        .cert_pem = cfg.cert_pem,
        .timeout_ms = cfg.timeout_ms > 0 ? cfg.timeout_ms : 10000,
        .skip_cert_common_name_check = cfg.skip_common_name_check,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    ESP_LOGI(TAG, "Attempting to download update from %s", cfg.url);

    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK) {
        if (cfg.reboot_after_update) {
            ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
            esp_restart();
        } else {
            ESP_LOGI(TAG, "OTA Succeed, reboot later");
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "Firmware upgrade failed");
    return ESP_FAIL;
}

bool https_ota_is_pending_verify(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_img_states_t state;
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        return state == ESP_OTA_IMG_PENDING_VERIFY;
    }
#endif
    return false;
}

void https_ota_mark_valid(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_valid_cancel_rollback();
#endif
}

void https_ota_mark_invalid_and_reboot(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_invalid_rollback_and_reboot();
#endif
}

#if CONFIG_EXAMPLE_HTTPS_OTA_TEST
void app_main(void)
{  

    // NVS Init
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

    //-- Version info
    ESP_LOGI(TAG, "OTA HTTPS Component test V1.0");

    //-- Wifi init
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //ESP_ERROR_CHECK(example_connect());

    // Verify
    if(https_ota_is_pending_verify()){
        ESP_LOGI(TAG, "Current Image is pending of validation, diagnostic() call..");
        bool image_ok = true; // diagnostic()
        if(image_ok){
            https_ota_mark_valid();
        }
        else{
            https_ota_mark_invalid_and_reboot();
        }
    }

    // Wait and make a OTA request again
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    // OTA config
    https_ota_config_t cfg = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        .cert_pem = (const char *)server_cert_pem_start,
        .skip_common_name_check = true,
        .reboot_after_update = true,
        .timeout_ms = 100000,
    };
    ESP_ERROR_CHECK(https_ota_download(&cfg));
}
#endif
