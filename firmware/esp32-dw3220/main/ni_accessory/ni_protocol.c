/**
 * @file ni_protocol.c
 * @brief Apple NearbyInteraction Accessory Protocol Implementation
 */

#include "ni_protocol.h"
#include "dw3220_uwb.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NI_PROTO";

// Stored Apple configuration
static uint8_t apple_config[256];
static uint16_t apple_config_len = 0;

/**
 * @brief Parse Apple Shareable Configuration Data
 *
 * The Apple Shareable Configuration Data contains UWB session parameters
 * that must be passed to the DW3220 as-is.
 */
bool ni_parse_apple_config(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0 || len > sizeof(apple_config)) {
        ESP_LOGE(TAG, "Invalid Apple config data");
        return false;
    }

    // Store the Apple config data
    memcpy(apple_config, data, len);
    apple_config_len = len;

    ESP_LOGI(TAG, "Stored Apple config: %d bytes", len);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, apple_config, len, ESP_LOG_DEBUG);

    return true;
}

/**
 * @brief Start NI ranging session
 */
bool ni_start_ranging(void)
{
    if (apple_config_len == 0) {
        ESP_LOGE(TAG, "No Apple config available");
        return false;
    }

    ESP_LOGI(TAG, "Starting NI ranging session");

    // Extract session parameters from Apple config
    // Note: The exact format depends on Apple's NI specification
    dw3220_session_config_t session = {
        .session_id = 0,  // Will be extracted from config
        .device_role = UWB_ROLE_RESPONDER,
        .channel = UWB_CHANNEL_9,
        .preamble_index = UWB_PREAMBLE_CODE_10,
        .profile_id = 1,
    };

    // Get device address
    dw3220_get_device_address(session.device_address);

    // Start ranging
    esp_err_t ret = dw3220_start_ranging(&session);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ranging");
        return false;
    }

    return true;
}

/**
 * @brief Stop NI ranging session
 */
void ni_stop_ranging(void)
{
    ESP_LOGI(TAG, "Stopping NI ranging session");
    dw3220_stop_ranging();
    apple_config_len = 0;
}
