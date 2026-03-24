/**
 * @file ble_gatt_server.h
 * @brief BLE GATT Server for UWB OOB Communication
 */

#ifndef BLE_GATT_SERVER_H
#define BLE_GATT_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OOB Message callback type
 */
typedef void (*oob_message_callback_t)(uint8_t msg_id, uint8_t *data, uint16_t len);

/**
 * @brief BLE GATT Configuration
 */
typedef struct {
    const char *device_name;            ///< Device name for advertising
    oob_message_callback_t oob_callback; ///< Callback for received OOB messages
} ble_gatt_config_t;

/**
 * @brief Initialize BLE GATT Server
 * @param config Configuration
 * @return ESP_OK on success
 */
esp_err_t ble_gatt_server_init(const ble_gatt_config_t *config);

/**
 * @brief Deinitialize BLE GATT Server
 */
void ble_gatt_server_deinit(void);

/**
 * @brief Start BLE advertising
 * @return ESP_OK on success
 */
esp_err_t ble_gatt_start_advertising(void);

/**
 * @brief Stop BLE advertising
 * @return ESP_OK on success
 */
esp_err_t ble_gatt_stop_advertising(void);

/**
 * @brief Send data via TX characteristic (notify)
 * @param data Data to send
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t ble_gatt_send_data(const uint8_t *data, uint16_t len);

/**
 * @brief Check if a device is connected
 * @return true if connected
 */
bool ble_gatt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_GATT_SERVER_H
