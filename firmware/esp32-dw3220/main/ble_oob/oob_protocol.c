/**
 * @file oob_protocol.c
 * @brief OOB Protocol Implementation
 */

#include "oob_protocol.h"
#include "apple_ni_adapter.h"
#include "fira_responder_adapter.h"
#include "dw3220_uwb.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "OOB_PROTO";

const char *oob_platform_name(uint8_t platform_id)
{
    switch (platform_id) {
        case PLATFORM_IOS:
            return "iOS";
        case PLATFORM_ANDROID:
            return "Android";
        default:
            return "Unknown";
    }
}

bool oob_parse_initialize_request(const uint8_t *data, uint16_t len, oob_initialize_request_t *request)
{
    if (data == NULL || request == NULL || len < 1) {
        return false;
    }

    request->platform_id = data[0];
    return request->platform_id == PLATFORM_IOS || request->platform_id == PLATFORM_ANDROID;
}

/**
 * @brief Detect platform from config data
 */
uint8_t oob_detect_platform(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len < 4) {
        return PLATFORM_UNKNOWN;
    }

    // iOS Apple Shareable Config typically has specific header
    // Android config has different structure
    // This is a heuristic based on data patterns

    // Check for Apple-specific patterns (version bytes)
    uint16_t ver_major = data[0] | (data[1] << 8);
    uint16_t ver_minor = data[2] | (data[3] << 8);

    // Apple NI typically uses version 0x0100 or similar
    if (ver_major == 0x0100 && len > 10) {
        ESP_LOGI(TAG, "Detected iOS platform");
        return PLATFORM_IOS;
    }

    // Android AOSP OOB has different structure
    if (len >= 12) {
        ESP_LOGI(TAG, "Detected Android platform");
        return PLATFORM_ANDROID;
    }

    return PLATFORM_UNKNOWN;
}

/**
 * @brief Build accessory config response
 */
uint16_t oob_build_config_response(uint8_t *buffer, uint16_t max_len, uint8_t platform_id)
{
    if (buffer == NULL || max_len < 20) {
        return 0;
    }

    uint16_t offset = 0;
    uint16_t config_len = 0;

    // Message ID
    buffer[offset++] = OOB_MSG_ACCESSORY_CONFIG_DATA;

    if (platform_id == PLATFORM_ANDROID) {
        config_len = fira_responder_adapter_get_oob_config(buffer + offset, max_len - offset);
    } else if (platform_id == PLATFORM_IOS) {
        uint16_t apple_len = 0;
        if (apple_ni_adapter_get_accessory_config(buffer + offset, max_len - offset, &apple_len) == ESP_OK) {
            config_len = apple_len;
        }
    }

    offset += config_len;

    ESP_LOGI(TAG, "Built config response for %s: %d bytes", oob_platform_name(platform_id), offset);
    return offset;
}

uint16_t oob_build_error_response(uint8_t *buffer, uint16_t max_len, uint8_t error_code)
{
    if (buffer == NULL || max_len < 2) {
        return 0;
    }

    buffer[0] = OOB_MSG_ACCESSORY_ERROR;
    buffer[1] = error_code;
    return 2;
}

/**
 * @brief Build UWB started response
 */
uint16_t oob_build_started_response(uint8_t *buffer, uint16_t max_len)
{
    if (buffer == NULL || max_len < 1) {
        return 0;
    }

    buffer[0] = OOB_MSG_UWB_DID_START;
    return 1;
}

/**
 * @brief Build UWB stopped response
 */
uint16_t oob_build_stopped_response(uint8_t *buffer, uint16_t max_len)
{
    if (buffer == NULL || max_len < 1) {
        return 0;
    }

    buffer[0] = OOB_MSG_UWB_DID_STOP;
    return 1;
}
