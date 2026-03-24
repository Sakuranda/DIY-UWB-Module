/**
 * @file apple_ni_adapter.h
 * @brief Integration boundary for Apple Nearby Interaction accessory support
 */

#ifndef APPLE_NI_ADAPTER_H
#define APPLE_NI_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t apple_ni_adapter_get_accessory_config(uint8_t *buffer, uint16_t max_len, uint16_t *out_len);
esp_err_t apple_ni_adapter_apply_shareable_config(const uint8_t *data, uint16_t len);
esp_err_t apple_ni_adapter_start(void);
void apple_ni_adapter_stop(void);
bool apple_ni_adapter_is_available(void);

#ifdef __cplusplus
}
#endif

#endif // APPLE_NI_ADAPTER_H
