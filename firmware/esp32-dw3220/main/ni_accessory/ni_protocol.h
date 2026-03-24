/**
 * @file ni_protocol.h
 * @brief Apple NearbyInteraction Accessory Protocol
 */

#ifndef NI_PROTOCOL_H
#define NI_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse Apple Shareable Configuration Data
 * @param data Raw data from iOS
 * @param len Data length
 * @return true if parsing successful
 */
bool ni_parse_apple_config(const uint8_t *data, uint16_t len);

/**
 * @brief Start NI ranging session
 * @return true if started successfully
 */
bool ni_start_ranging(void);

/**
 * @brief Stop NI ranging session
 */
void ni_stop_ranging(void);

#ifdef __cplusplus
}
#endif

#endif // NI_PROTOCOL_H
