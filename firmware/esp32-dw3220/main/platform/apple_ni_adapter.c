/**
 * @file apple_ni_adapter.c
 * @brief Default open-source Apple NI adapter placeholder
 */

#include "apple_ni_adapter.h"
#include "esp_log.h"

static const char *TAG = "APPLE_NI_ADAPTER";

esp_err_t apple_ni_adapter_get_accessory_config(uint8_t *buffer, uint16_t max_len, uint16_t *out_len)
{
    (void) buffer;
    (void) max_len;
    if (out_len != NULL) {
        *out_len = 0;
    }
    ESP_LOGW(TAG, "Apple NI accessory config is unavailable in the open-source build");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t apple_ni_adapter_apply_shareable_config(const uint8_t *data, uint16_t len)
{
    (void) data;
    (void) len;
    ESP_LOGW(TAG, "Apple NI shareable config cannot be applied without a vendor implementation");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t apple_ni_adapter_start(void)
{
    ESP_LOGW(TAG, "Apple NI ranging cannot start without a vendor implementation");
    return ESP_ERR_NOT_SUPPORTED;
}

void apple_ni_adapter_stop(void)
{
    ESP_LOGI(TAG, "Apple NI adapter stop requested");
}

bool apple_ni_adapter_is_available(void)
{
    return false;
}
