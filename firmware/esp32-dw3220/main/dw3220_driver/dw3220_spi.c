/**
 * @file dw3220_spi.c
 * @brief DW3220 SPI Communication Driver Implementation
 *
 * Implements SPI communication with DW3220 UWB transceiver.
 * Based on DW3000 family SPI protocol specification.
 */

#include "dw3220_spi.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DW3220_SPI";

// SPI Host
#define DW3220_SPI_HOST     SPI2_HOST

// DW3220 SPI transaction format
#define DW3220_SPI_WR_BIT   0x80    // Write bit in first byte
#define DW3220_SPI_RD_BIT   0x00    // Read bit (no bit set)

static spi_device_handle_t spi_handle = NULL;
static int cs_pin_num = -1;

/**
 * @brief Initialize SPI bus for DW3220
 */
esp_err_t dw3220_spi_init(const dw3220_spi_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cs_pin_num = config->cs_pin;

    // Configure CS pin as GPIO (manual control for better timing)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->cs_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(config->cs_pin, 1);  // CS high (inactive)

    // SPI bus configuration
    spi_bus_config_t bus_config = {
        .mosi_io_num = config->mosi_pin,
        .miso_io_num = config->miso_pin,
        .sclk_io_num = config->sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
        .flags = SPICOMMON_BUSFLAG_MASTER
    };

    esp_err_t ret = spi_bus_initialize(DW3220_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // SPI device configuration
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = config->clock_speed_hz,
        .mode = 0,                  // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = -1,         // Manual CS control
        .queue_size = 7,
        .pre_cb = NULL,
        .post_cb = NULL
    };

    ret = spi_bus_add_device(DW3220_SPI_HOST, &dev_config, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        spi_bus_free(DW3220_SPI_HOST);
        return ret;
    }

    ESP_LOGI(TAG, "SPI initialized at %lu Hz", config->clock_speed_hz);
    return ESP_OK;
}

/**
 * @brief Deinitialize SPI bus
 */
void dw3220_spi_deinit(void)
{
    if (spi_handle != NULL) {
        spi_bus_remove_device(spi_handle);
        spi_bus_free(DW3220_SPI_HOST);
        spi_handle = NULL;
    }
}

/**
 * @brief Build SPI header for DW3220
 * @param reg_addr Register address
 * @param is_write True for write, false for read
 * @param header Buffer to store header (at least 2 bytes)
 * @return Header length in bytes
 */
static uint8_t build_spi_header(uint16_t reg_addr, bool is_write, uint8_t *header)
{
    // DW3220 uses variable length header:
    // - Short header (1 byte): for addresses 0x00-0x3F
    // - Long header (2 bytes): for addresses 0x40+

    if (reg_addr <= 0x3F) {
        // Short header: [R/W:1][ADDR:6][0:1]
        header[0] = (is_write ? DW3220_SPI_WR_BIT : DW3220_SPI_RD_BIT) | ((reg_addr & 0x3F) << 1);
        return 1;
    } else {
        // Long header: [R/W:1][1:1][ADDR_H:6] [ADDR_L:8]
        header[0] = (is_write ? DW3220_SPI_WR_BIT : DW3220_SPI_RD_BIT) | 0x40 | ((reg_addr >> 8) & 0x3F);
        header[1] = reg_addr & 0xFF;
        return 2;
    }
}

/**
 * @brief Read DW3220 register
 */
esp_err_t dw3220_spi_read(uint16_t reg_addr, uint8_t *data, uint16_t len)
{
    if (spi_handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t header[2];
    uint8_t header_len = build_spi_header(reg_addr, false, header);

    // Prepare transaction
    uint8_t *tx_buf = heap_caps_malloc(header_len + len, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_malloc(header_len + len, MALLOC_CAP_DMA);
    if (tx_buf == NULL || rx_buf == NULL) {
        free(tx_buf);
        free(rx_buf);
        return ESP_ERR_NO_MEM;
    }

    memcpy(tx_buf, header, header_len);
    memset(tx_buf + header_len, 0, len);  // Dummy bytes for read

    spi_transaction_t trans = {
        .length = (header_len + len) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf
    };

    // Execute transaction
    gpio_set_level(cs_pin_num, 0);  // CS low
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    gpio_set_level(cs_pin_num, 1);  // CS high

    if (ret == ESP_OK) {
        memcpy(data, rx_buf + header_len, len);
    }

    free(tx_buf);
    free(rx_buf);
    return ret;
}

/**
 * @brief Write DW3220 register
 */
esp_err_t dw3220_spi_write(uint16_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (spi_handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t header[2];
    uint8_t header_len = build_spi_header(reg_addr, true, header);

    // Prepare transaction
    uint8_t *tx_buf = heap_caps_malloc(header_len + len, MALLOC_CAP_DMA);
    if (tx_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(tx_buf, header, header_len);
    memcpy(tx_buf + header_len, data, len);

    spi_transaction_t trans = {
        .length = (header_len + len) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = NULL
    };

    // Execute transaction
    gpio_set_level(cs_pin_num, 0);  // CS low
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    gpio_set_level(cs_pin_num, 1);  // CS high

    free(tx_buf);
    return ret;
}

/**
 * @brief Read 32-bit register value
 */
uint32_t dw3220_spi_read32(uint16_t reg_addr)
{
    uint8_t data[4];
    if (dw3220_spi_read(reg_addr, data, 4) != ESP_OK) {
        return 0;
    }
    // Little-endian
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

/**
 * @brief Write 32-bit register value
 */
void dw3220_spi_write32(uint16_t reg_addr, uint32_t value)
{
    uint8_t data[4] = {
        value & 0xFF,
        (value >> 8) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 24) & 0xFF
    };
    dw3220_spi_write(reg_addr, data, 4);
}

/**
 * @brief Read 8-bit register value
 */
uint8_t dw3220_spi_read8(uint16_t reg_addr)
{
    uint8_t data;
    if (dw3220_spi_read(reg_addr, &data, 1) != ESP_OK) {
        return 0;
    }
    return data;
}

/**
 * @brief Write 8-bit register value
 */
void dw3220_spi_write8(uint16_t reg_addr, uint8_t value)
{
    dw3220_spi_write(reg_addr, &value, 1);
}
