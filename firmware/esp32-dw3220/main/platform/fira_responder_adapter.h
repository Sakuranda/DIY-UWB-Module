/**
 * @file fira_responder_adapter.h
 * @brief Android FiRa responder integration boundary
 */

#ifndef FIRA_RESPONDER_ADAPTER_H
#define FIRA_RESPONDER_ADAPTER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t fira_responder_adapter_get_oob_config(uint8_t *buffer, uint16_t max_len);
esp_err_t fira_responder_adapter_apply_config(const uint8_t *data, uint16_t len);
esp_err_t fira_responder_adapter_start(void);
void fira_responder_adapter_stop(void);

#ifdef __cplusplus
}
#endif

#endif // FIRA_RESPONDER_ADAPTER_H
