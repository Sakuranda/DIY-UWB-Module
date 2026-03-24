/**
 * @file session_manager.h
 * @brief UWB Session Manager
 */

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Session State
 */
typedef enum {
    SESSION_STATE_IDLE,
    SESSION_STATE_CONNECTED,
    SESSION_STATE_INITIALIZED,
    SESSION_STATE_RANGING,
    SESSION_STATE_STOPPED,
    SESSION_STATE_ERROR
} session_state_t;

/**
 * @brief Initialize session manager
 */
void session_manager_init(void);

/**
 * @brief Handle Initialize request from phone
 */
void session_handle_initialize(const uint8_t *data, uint16_t len);

/**
 * @brief Handle Configure and Start request
 * @param data Configuration data
 * @param len Data length
 */
void session_handle_configure_and_start(const uint8_t *data, uint16_t len);

/**
 * @brief Handle Stop request
 */
void session_handle_stop(void);

/**
 * @brief Get current session state
 * @return Current state
 */
session_state_t session_get_state(void);

/**
 * @brief Get the active platform for the current session
 * @return Platform type
 */
uint8_t session_get_active_platform(void);

#ifdef __cplusplus
}
#endif

#endif // SESSION_MANAGER_H
