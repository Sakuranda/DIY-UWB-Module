/**
 * @file dw3220_uwb.h
 * @brief DW3220 UWB Driver Header
 */

#ifndef DW3220_UWB_H
#define DW3220_UWB_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// DW3220 Register Addresses (DW3000 family compatible)
#define DW3220_REG_DEV_ID           0x00    // Device ID register
#define DW3220_REG_SYS_CFG          0x04    // System configuration
#define DW3220_REG_TX_FCTRL         0x08    // TX frame control
#define DW3220_REG_DX_TIME          0x0A    // Delayed TX/RX time
#define DW3220_REG_SYS_STATUS       0x44    // System status
#define DW3220_REG_RX_FINFO         0x10    // RX frame info
#define DW3220_REG_RX_BUFFER        0x12    // RX buffer
#define DW3220_REG_TX_BUFFER        0x14    // TX buffer
#define DW3220_REG_CHAN_CTRL        0x1F    // Channel control
#define DW3220_REG_PMSC             0x36    // Power management

// Device ID expected values
#define DW3220_DEV_ID               0xDECA0302  // DW3220 device ID

// UWB Channel definitions
#define UWB_CHANNEL_5               5
#define UWB_CHANNEL_9               9

// Preamble codes
#define UWB_PREAMBLE_CODE_9         9
#define UWB_PREAMBLE_CODE_10        10
#define UWB_PREAMBLE_CODE_11        11
#define UWB_PREAMBLE_CODE_12        12

// Ranging roles
#define UWB_ROLE_RESPONDER          0x00
#define UWB_ROLE_INITIATOR          0x01
#define UWB_ROLE_CONTROLEE          0x00
#define UWB_ROLE_CONTROLLER         0x01

/**
 * @brief UWB Configuration Structure
 */
typedef struct {
    uint8_t channel;            // UWB channel (5 or 9)
    uint8_t preamble_code;      // Preamble code
    uint8_t prf;                // Pulse repetition frequency
    uint8_t data_rate;          // Data rate
    uint8_t preamble_length;    // Preamble length
    uint8_t pac_size;           // Preamble acquisition chunk size
    uint8_t sfd_type;           // SFD type
    uint8_t ranging_role;       // Ranging role (initiator/responder)
} dw3220_uwb_config_t;

/**
 * @brief Ranging Session Configuration
 */
typedef struct {
    uint32_t session_id;        // Session ID
    uint8_t device_role;        // Device role in session
    uint8_t channel;            // UWB channel
    uint8_t preamble_index;     // Preamble index
    uint8_t profile_id;         // UWB profile ID
    uint8_t device_address[2];  // Device short address
    uint8_t peer_address[2];    // Peer short address
} dw3220_session_config_t;

/**
 * @brief Ranging Result Structure
 */
typedef struct {
    float distance_m;           // Distance in meters
    float azimuth_deg;          // Azimuth angle in degrees
    float elevation_deg;        // Elevation angle in degrees
    int8_t rssi;                // Received signal strength
    uint8_t los_confidence;     // Line-of-sight confidence (0-100)
    bool valid;                 // Result validity flag
} dw3220_ranging_result_t;

/**
 * @brief Initialize DW3220 UWB
 * @return ESP_OK on success
 */
esp_err_t dw3220_uwb_init(void);

/**
 * @brief Deinitialize DW3220 UWB
 */
void dw3220_uwb_deinit(void);

/**
 * @brief Read DW3220 Device ID
 * @return Device ID (should be DW3220_DEV_ID)
 */
uint32_t dw3220_read_device_id(void);

/**
 * @brief Configure UWB parameters
 * @param config UWB configuration
 * @return ESP_OK on success
 */
esp_err_t dw3220_uwb_configure(const dw3220_uwb_config_t *config);

/**
 * @brief Start ranging session
 * @param session Session configuration
 * @return ESP_OK on success
 */
esp_err_t dw3220_start_ranging(const dw3220_session_config_t *session);

/**
 * @brief Stop ranging session
 * @return ESP_OK on success
 */
esp_err_t dw3220_stop_ranging(void);

/**
 * @brief Get latest ranging result
 * @param result Pointer to store result
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no result available
 */
esp_err_t dw3220_get_ranging_result(dw3220_ranging_result_t *result);

/**
 * @brief Get device short address
 * @param address Buffer to store address (2 bytes)
 */
void dw3220_get_device_address(uint8_t *address);

/**
 * @brief Generate accessory configuration data for OOB
 * @param buffer Buffer to store config data
 * @param max_len Maximum buffer length
 * @return Actual data length, or 0 on error
 */
uint16_t dw3220_get_accessory_config_data(uint8_t *buffer, uint16_t max_len);

#ifdef __cplusplus
}
#endif

#endif // DW3220_UWB_H
