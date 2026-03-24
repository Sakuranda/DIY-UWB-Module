/**
 * @file fira_responder_adapter.c
 * @brief Minimal Android FiRa responder adapter backed by the DW3220 driver
 */

#include "fira_responder_adapter.h"
#include "dw3220_uwb.h"
#include "esp_log.h"
#include <stdbool.h>
#include <string.h>

static const char *TAG = "FIRA_RESPONDER";
static dw3220_session_config_t g_session = {0};
static bool g_session_configured = false;

uint16_t fira_responder_adapter_get_oob_config(uint8_t *buffer, uint16_t max_len)
{
    return dw3220_get_accessory_config_data(buffer, max_len);
}

esp_err_t fira_responder_adapter_apply_config(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len < 14) {
        ESP_LOGE(TAG, "Invalid Android config length: %u", len);
        return ESP_ERR_INVALID_ARG;
    }

    memset(&g_session, 0, sizeof(g_session));
    g_session.session_id = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
    g_session.preamble_index = data[8];
    g_session.channel = data[9];
    g_session.profile_id = data[10];
    g_session.device_role = data[11];
    g_session.peer_address[0] = data[12];
    g_session.peer_address[1] = data[13];
    dw3220_get_device_address(g_session.device_address);

    g_session_configured = true;

    ESP_LOGI(TAG, "Prepared Android FiRa session: id=%lu channel=%u role=%u",
             g_session.session_id, g_session.channel, g_session.device_role);
    return ESP_OK;
}

esp_err_t fira_responder_adapter_start(void)
{
    if (!g_session_configured) {
        ESP_LOGE(TAG, "No Android FiRa session configured");
        return ESP_ERR_INVALID_STATE;
    }

    return dw3220_start_ranging(&g_session);
}

void fira_responder_adapter_stop(void)
{
    dw3220_stop_ranging();
    g_session_configured = false;
    memset(&g_session, 0, sizeof(g_session));
}
