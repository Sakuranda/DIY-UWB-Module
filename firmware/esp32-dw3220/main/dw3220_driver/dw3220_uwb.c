/**
 * @file dw3220_uwb.c
 * @brief DW3220 UWB Driver Implementation
 */

#include "dw3220_uwb.h"
#include "dw3220_spi.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "DW3220_UWB";

// Device state
static bool is_initialized = false;
static uint8_t device_address[2] = {0};
static dw3220_session_config_t current_session = {0};
static dw3220_ranging_result_t last_result = {0};

/**
 * @brief Soft reset DW3220
 */
static void dw3220_soft_reset(void)
{
    // Write to PMSC to trigger soft reset
    dw3220_spi_write8(DW3220_REG_PMSC, 0x01);
    vTaskDelay(pdMS_TO_TICKS(5));
    dw3220_spi_write8(DW3220_REG_PMSC, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
}

/**
 * @brief Initialize DW3220 UWB
 */
esp_err_t dw3220_uwb_init(void)
{
    if (is_initialized) {
        return ESP_OK;
    }

    // Soft reset
    dw3220_soft_reset();

    // Read and verify device ID
    uint32_t dev_id = dw3220_read_device_id();
    ESP_LOGI(TAG, "Device ID: 0x%08lX", dev_id);

    // Check if valid DW3000 family device
    if ((dev_id & 0xFFFF0000) != 0xDECA0000) {
        ESP_LOGE(TAG, "Invalid device ID, expected DW3xxx family");
        return ESP_ERR_NOT_FOUND;
    }

    // Generate random device address
    uint32_t rand = esp_random();
    device_address[0] = rand & 0xFF;
    device_address[1] = (rand >> 8) & 0xFF;
    ESP_LOGI(TAG, "Device address: %02X:%02X", device_address[0], device_address[1]);

    // Default configuration
    dw3220_uwb_config_t default_config = {
        .channel = UWB_CHANNEL_9,
        .preamble_code = UWB_PREAMBLE_CODE_10,
        .prf = 64,              // 64 MHz PRF
        .data_rate = 6,         // 6.8 Mbps
        .preamble_length = 64,  // 64 symbols
        .pac_size = 8,
        .sfd_type = 1,          // IEEE 802.15.4z SFD
        .ranging_role = UWB_ROLE_RESPONDER
    };

    esp_err_t ret = dw3220_uwb_configure(&default_config);
    if (ret != ESP_OK) {
        return ret;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "DW3220 initialized successfully");
    return ESP_OK;
}

/**
 * @brief Deinitialize DW3220 UWB
 */
void dw3220_uwb_deinit(void)
{
    if (is_initialized) {
        dw3220_stop_ranging();
        dw3220_soft_reset();
        is_initialized = false;
    }
}

/**
 * @brief Read DW3220 Device ID
 */
uint32_t dw3220_read_device_id(void)
{
    return dw3220_spi_read32(DW3220_REG_DEV_ID);
}

/**
 * @brief Configure UWB parameters
 */
esp_err_t dw3220_uwb_configure(const dw3220_uwb_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Configuring UWB: CH=%d, PRE=%d, ROLE=%d",
             config->channel, config->preamble_code, config->ranging_role);

    // Configure channel
    uint32_t chan_ctrl = 0;
    chan_ctrl |= (config->channel & 0x0F);           // Channel number
    chan_ctrl |= ((config->preamble_code & 0x1F) << 8);  // TX preamble code
    chan_ctrl |= ((config->preamble_code & 0x1F) << 16); // RX preamble code
    dw3220_spi_write32(DW3220_REG_CHAN_CTRL, chan_ctrl);

    // Configure system (enable ranging, etc.)
    uint32_t sys_cfg = dw3220_spi_read32(DW3220_REG_SYS_CFG);
    sys_cfg |= 0x00000040;  // Enable ranging
    dw3220_spi_write32(DW3220_REG_SYS_CFG, sys_cfg);

    return ESP_OK;
}

/**
 * @brief Start ranging session
 */
esp_err_t dw3220_start_ranging(const dw3220_session_config_t *session)
{
    if (session == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Store session config
    memcpy(&current_session, session, sizeof(dw3220_session_config_t));

    ESP_LOGI(TAG, "Starting ranging session: ID=%lu, CH=%d, ROLE=%d",
             session->session_id, session->channel, session->device_role);

    // Configure for session
    dw3220_uwb_config_t config = {
        .channel = session->channel,
        .preamble_code = session->preamble_index,
        .ranging_role = session->device_role
    };
    dw3220_uwb_configure(&config);

    // Enable receiver
    // In real implementation, this would start the ranging state machine

    return ESP_OK;
}

/**
 * @brief Stop ranging session
 */
esp_err_t dw3220_stop_ranging(void)
{
    ESP_LOGI(TAG, "Stopping ranging session");

    // Disable transceiver
    dw3220_spi_write8(DW3220_REG_SYS_CFG, 0x00);

    memset(&current_session, 0, sizeof(dw3220_session_config_t));
    memset(&last_result, 0, sizeof(dw3220_ranging_result_t));

    return ESP_OK;
}

/**
 * @brief Get latest ranging result
 */
esp_err_t dw3220_get_ranging_result(dw3220_ranging_result_t *result)
{
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!last_result.valid) {
        return ESP_ERR_NOT_FOUND;
    }

    memcpy(result, &last_result, sizeof(dw3220_ranging_result_t));
    return ESP_OK;
}

/**
 * @brief Get device short address
 */
void dw3220_get_device_address(uint8_t *address)
{
    if (address != NULL) {
        address[0] = device_address[0];
        address[1] = device_address[1];
    }
}

/**
 * @brief Generate accessory configuration data for OOB
 *
 * Format for iOS (Apple NI Accessory Protocol):
 * - SpecVerMajor (2 bytes)
 * - SpecVerMinor (2 bytes)
 * - UWBConfigDataLength (2 bytes)
 * - UWBConfigData (variable)
 *
 * Format for Android (AOSP OOB):
 * - SpecVerMajor (2 bytes)
 * - SpecVerMinor (2 bytes)
 * - ChipId (2 bytes)
 * - ChipFwVersion (2 bytes)
 * - MwVersion (3 bytes)
 * - SupportedUwbProfileIds (4 bytes)
 * - SupportedDeviceRangingRoles (1 byte)
 * - DeviceMacAddress (2 bytes)
 */
uint16_t dw3220_get_accessory_config_data(uint8_t *buffer, uint16_t max_len)
{
    if (buffer == NULL || max_len < 18) {
        return 0;
    }

    uint16_t offset = 0;

    // Spec version major (little-endian)
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x01;  // Version 1.0

    // Spec version minor
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;

    // Chip ID (DW3220)
    buffer[offset++] = 0x20;
    buffer[offset++] = 0x32;

    // Chip FW version
    buffer[offset++] = 0x01;
    buffer[offset++] = 0x00;

    // MW version
    buffer[offset++] = 0x01;
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;

    // Supported UWB Profile IDs (bitmask)
    // Bit 1: CONFIG_UNICAST_DS_TWR
    buffer[offset++] = 0x02;
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;

    // Supported device ranging roles
    // Bit 0: Controlee, Bit 1: Controller
    buffer[offset++] = 0x03;  // Support both roles

    // Device MAC address
    buffer[offset++] = device_address[0];
    buffer[offset++] = device_address[1];

    return offset;
}
