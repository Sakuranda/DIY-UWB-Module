/**
 * @file session_manager.c
 * @brief UWB Session Manager Implementation
 */

#include "session_manager.h"
#include "oob_protocol.h"
#include "apple_ni_adapter.h"
#include "fira_responder_adapter.h"
#include "ble_gatt_server.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SESSION_MGR";

static session_state_t current_state = SESSION_STATE_IDLE;
static uint8_t active_platform = PLATFORM_UNKNOWN;

static void send_simple_response(uint16_t (*builder)(uint8_t *buffer, uint16_t max_len))
{
    uint8_t response[8];
    uint16_t len = builder(response, sizeof(response));
    if (len > 0) {
        ble_gatt_send_data(response, len);
    }
}

static void send_error_response(uint8_t error_code)
{
    uint8_t response[8];
    uint16_t len = oob_build_error_response(response, sizeof(response), error_code);
    if (len > 0) {
        ble_gatt_send_data(response, len);
    }
}

/**
 * @brief Initialize session manager
 */
void session_manager_init(void)
{
    current_state = SESSION_STATE_IDLE;
    active_platform = PLATFORM_UNKNOWN;
    ESP_LOGI(TAG, "Session manager initialized");
}

/**
 * @brief Handle Initialize request from phone
 */
void session_handle_initialize(const uint8_t *data, uint16_t len)
{
    ESP_LOGI(TAG, "Processing Initialize request");

    oob_initialize_request_t request = {0};
    if (!oob_parse_initialize_request(data, len, &request)) {
        current_state = SESSION_STATE_ERROR;
        send_error_response(OOB_ERROR_INVALID_CONFIG);
        ESP_LOGE(TAG, "Invalid initialize request");
        return;
    }

    if (current_state == SESSION_STATE_RANGING) {
        send_error_response(OOB_ERROR_BUSY);
        ESP_LOGW(TAG, "Ignoring initialize while ranging is active");
        return;
    }

    active_platform = request.platform_id;
    current_state = SESSION_STATE_CONNECTED;

    uint8_t response[128];
    uint16_t response_len = oob_build_config_response(response, sizeof(response), active_platform);
    if (response_len == 0) {
        current_state = SESSION_STATE_ERROR;
        send_error_response(active_platform == PLATFORM_IOS ? OOB_ERROR_APPLE_STACK_MISSING
                                                            : OOB_ERROR_INVALID_CONFIG);
        ESP_LOGE(TAG, "Failed to build config response for %s", oob_platform_name(active_platform));
        return;
    }

    if (ble_gatt_send_data(response, response_len) != ESP_OK) {
        current_state = SESSION_STATE_ERROR;
        ESP_LOGE(TAG, "Failed to send config response");
        return;
    }

    current_state = SESSION_STATE_INITIALIZED;
    ESP_LOGI(TAG, "Sent config response for %s (%d bytes)", oob_platform_name(active_platform), response_len);
}

/**
 * @brief Handle Configure and Start request
 */
void session_handle_configure_and_start(const uint8_t *data, uint16_t len)
{
    ESP_LOGI(TAG, "Processing Configure and Start request: %d bytes", len);

    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Invalid config data");
        current_state = SESSION_STATE_ERROR;
        send_error_response(OOB_ERROR_INVALID_CONFIG);
        return;
    }

    if (active_platform == PLATFORM_UNKNOWN) {
        current_state = SESSION_STATE_ERROR;
        send_error_response(OOB_ERROR_UNSUPPORTED_PLATFORM);
        ESP_LOGE(TAG, "No active platform. Initialize must complete first.");
        return;
    }

    esp_err_t ret = ESP_ERR_INVALID_STATE;

    if (active_platform == PLATFORM_IOS) {
        ret = apple_ni_adapter_apply_shareable_config(data, len);
        if (ret == ESP_OK) {
            ret = apple_ni_adapter_start();
        }
    } else if (active_platform == PLATFORM_ANDROID) {
        ret = fira_responder_adapter_apply_config(data, len);
        if (ret == ESP_OK) {
            ret = fira_responder_adapter_start();
        }
    }

    if (ret == ESP_OK) {
        current_state = SESSION_STATE_RANGING;
        send_simple_response(oob_build_started_response);
        ESP_LOGI(TAG, "Ranging started successfully");
    } else {
        current_state = SESSION_STATE_ERROR;
        send_error_response(active_platform == PLATFORM_IOS ? OOB_ERROR_APPLE_STACK_MISSING
                                                            : OOB_ERROR_INVALID_CONFIG);
        ESP_LOGE(TAG, "Failed to start ranging for %s", oob_platform_name(active_platform));
    }
}

/**
 * @brief Handle Stop request
 */
void session_handle_stop(void)
{
    ESP_LOGI(TAG, "Processing Stop request");

    if (active_platform == PLATFORM_IOS) {
        apple_ni_adapter_stop();
    } else if (active_platform == PLATFORM_ANDROID) {
        fira_responder_adapter_stop();
    }

    current_state = SESSION_STATE_STOPPED;
    send_simple_response(oob_build_stopped_response);
    current_state = SESSION_STATE_IDLE;
    active_platform = PLATFORM_UNKNOWN;

    ESP_LOGI(TAG, "Ranging stopped");
}

/**
 * @brief Get current session state
 */
session_state_t session_get_state(void)
{
    return current_state;
}

uint8_t session_get_active_platform(void)
{
    return active_platform;
}
