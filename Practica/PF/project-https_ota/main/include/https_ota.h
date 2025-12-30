#include "esp_err.h"

typedef struct {
    const char *url;
    const char *cert_pem;
    bool skip_common_name_check;
    bool reboot_after_update;
    int timeout_ms;
} https_ota_config_t;


// Main functions
esp_err_t https_ota_download(const https_ota_config_t *config);

// Rollback Utils
bool https_ota_is_pending_verify(void);
void https_ota_mark_valid(void);
void https_ota_mark_invalid_and_reboot(void);