/**
 * @file oob_protocol.h
 * @brief OOB Protocol Definitions for iOS/Android UWB
 */

#ifndef OOB_PROTOCOL_H
#define OOB_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// OOB Message IDs (unified for iOS and Android)
#define OOB_MSG_ACCESSORY_CONFIG_DATA   0x01    // Accessory -> Phone: Config data
#define OOB_MSG_UWB_DID_START           0x02    // Accessory -> Phone: Ranging started
#define OOB_MSG_UWB_DID_STOP            0x03    // Accessory -> Phone: Ranging stopped
#define OOB_MSG_ACCESSORY_ERROR         0x04    // Accessory -> Phone: Error response
#define OOB_MSG_INITIALIZE              0x0A    // Phone -> Accessory: Request config
#define OOB_MSG_CONFIGURE_AND_START     0x0B    // Phone -> Accessory: Start ranging
#define OOB_MSG_STOP                    0x0C    // Phone -> Accessory: Stop ranging

// Platform detection
#define PLATFORM_UNKNOWN                0
#define PLATFORM_IOS                    1
#define PLATFORM_ANDROID                2

// Accessory error codes
#define OOB_ERROR_UNSUPPORTED_PLATFORM  0x01
#define OOB_ERROR_APPLE_STACK_MISSING   0x02
#define OOB_ERROR_INVALID_CONFIG        0x03
#define OOB_ERROR_BUSY                  0x04

typedef struct {
    uint8_t platform_id;
} oob_initialize_request_t;

/**
 * @brief Detect platform from config data
 * @param data Configuration data received
 * @param len Data length
 * @return Platform type (PLATFORM_IOS, PLATFORM_ANDROID, or PLATFORM_UNKNOWN)
 */
uint8_t oob_detect_platform(const uint8_t *data, uint16_t len);

/**
 * @brief Parse initialize request payload
 * @param data Raw payload without message ID
 * @param len Payload length
 * @param request Parsed request output
 * @return true if request is valid
 */
bool oob_parse_initialize_request(const uint8_t *data, uint16_t len, oob_initialize_request_t *request);

/**
 * @brief Build accessory config response for the requested platform
 * @param buffer Output buffer
 * @param max_len Maximum buffer length
 * @param platform_id Platform requested by the phone
 * @return Actual data length
 */
uint16_t oob_build_config_response(uint8_t *buffer, uint16_t max_len, uint8_t platform_id);

/**
 * @brief Build accessory error response
 * @param buffer Output buffer
 * @param max_len Maximum buffer length
 * @param error_code Error code payload
 * @return Actual data length
 */
uint16_t oob_build_error_response(uint8_t *buffer, uint16_t max_len, uint8_t error_code);

/**
 * @brief Resolve human-readable platform name
 * @param platform_id Platform identifier
 * @return Static platform name string
 */
const char *oob_platform_name(uint8_t platform_id);

/**
 * @brief Build UWB started response
 * @param buffer Output buffer
 * @param max_len Maximum buffer length
 * @return Actual data length
 */
uint16_t oob_build_started_response(uint8_t *buffer, uint16_t max_len);

/**
 * @brief Build UWB stopped response
 * @param buffer Output buffer
 * @param max_len Maximum buffer length
 * @return Actual data length
 */
uint16_t oob_build_stopped_response(uint8_t *buffer, uint16_t max_len);

#ifdef __cplusplus
}
#endif

#endif // OOB_PROTOCOL_H
