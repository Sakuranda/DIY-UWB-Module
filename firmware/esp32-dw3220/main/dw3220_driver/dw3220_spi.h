/**
 * @file dw3220_spi.h
 * @brief DW3220 SPI Communication Driver Header
 */

#ifndef DW3220_SPI_H
#define DW3220_SPI_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI Configuration Structure
 */
typedef struct {
    int mosi_pin;           ///< MOSI GPIO pin
    int miso_pin;           ///< MISO GPIO pin
    int sclk_pin;           ///< SCLK GPIO pin
    int cs_pin;             ///< CS GPIO pin
    uint32_t clock_speed_hz; ///< SPI clock speed in Hz (max 36MHz for DW3220)
} dw3220_spi_config_t;

/**
 * @brief Initialize SPI bus for DW3220
 * @param config SPI configuration
 * @return ESP_OK on success
 */
esp_err_t dw3220_spi_init(const dw3220_spi_config_t *config);

/**
 * @brief Deinitialize SPI bus
 */
void dw3220_spi_deinit(void);

/**
 * @brief Read DW3220 register
 * @param reg_addr Register address (16-bit)
 * @param data Buffer to store read data
 * @param len Number of bytes to read
 * @return ESP_OK on success
 */
esp_err_t dw3220_spi_read(uint16_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief Write DW3220 register
 * @param reg_addr Register address (16-bit)
 * @param data Data to write
 * @param len Number of bytes to write
 * @return ESP_OK on success
 */
esp_err_t dw3220_spi_write(uint16_t reg_addr, const uint8_t *data, uint16_t len);

/**
 * @brief Read 32-bit register value
 * @param reg_addr Register address
 * @return Register value
 */
uint32_t dw3220_spi_read32(uint16_t reg_addr);

/**
 * @brief Write 32-bit register value
 * @param reg_addr Register address
 * @param value Value to write
 */
void dw3220_spi_write32(uint16_t reg_addr, uint32_t value);

/**
 * @brief Read 8-bit register value
 * @param reg_addr Register address
 * @return Register value
 */
uint8_t dw3220_spi_read8(uint16_t reg_addr);

/**
 * @brief Write 8-bit register value
 * @param reg_addr Register address
 * @param value Value to write
 */
void dw3220_spi_write8(uint16_t reg_addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // DW3220_SPI_H
