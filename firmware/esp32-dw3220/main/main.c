/**
 * @file main.c
 * @brief ESP32-S3 + DW3220 UWB Accessory Firmware Main Entry
 *
 * This firmware enables DW3220 UWB module to work with:
 * - iOS devices via Apple NearbyInteraction Accessory Protocol
 * - Android devices via AOSP Ranging OOB Protocol
 *
 * @author Claude Code
 * @date 2026-01-31
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "dw3220_spi.h"
#include "dw3220_uwb.h"
#include "ble_gatt_server.h"
#include "oob_protocol.h"
#include "session_manager.h"

static const char *TAG = "DW3220_MAIN";

// GPIO Pin Definitions for ESP32-S3 <-> DW3220
#define DW3220_SPI_MOSI     GPIO_NUM_11
#define DW3220_SPI_MISO     GPIO_NUM_13
#define DW3220_SPI_SCLK     GPIO_NUM_12
#define DW3220_SPI_CS       GPIO_NUM_10
#define DW3220_RST          GPIO_NUM_9
#define DW3220_IRQ          GPIO_NUM_14

// Device Name for BLE Advertisement
#define DEVICE_NAME         "DW3220-UWB"

/**
 * @brief Initialize NVS Flash
 */
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief Initialize GPIO pins
 */
static void init_gpio(void)
{
    // Configure DW3220 Reset pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DW3220_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Configure DW3220 IRQ pin
    io_conf.pin_bit_mask = (1ULL << DW3220_IRQ);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "GPIO initialized");
}

/**
 * @brief Hardware reset DW3220
 */
static void reset_dw3220(void)
{
    gpio_set_level(DW3220_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DW3220_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "DW3220 reset complete");
}

/**
 * @brief OOB message callback from BLE
 */
static void oob_message_callback(uint8_t msg_id, uint8_t *data, uint16_t len)
{
    ESP_LOGI(TAG, "OOB message received: ID=0x%02X, len=%d", msg_id, len);

    switch (msg_id) {
        case OOB_MSG_INITIALIZE:
            // Phone requests accessory config data
            ESP_LOGI(TAG, "Initialize request received");
            session_handle_initialize(data, len);
            break;

        case OOB_MSG_CONFIGURE_AND_START:
            // Phone sends config data, start ranging
            ESP_LOGI(TAG, "Configure and start request received");
            session_handle_configure_and_start(data, len);
            break;

        case OOB_MSG_STOP:
            // Stop ranging
            ESP_LOGI(TAG, "Stop request received");
            session_handle_stop();
            break;

        default:
            ESP_LOGW(TAG, "Unknown message ID: 0x%02X", msg_id);
            break;
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "ESP32-S3 + DW3220 UWB Accessory");
    ESP_LOGI(TAG, "=================================");

    // Initialize NVS
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize GPIO
    init_gpio();

    // Reset DW3220
    reset_dw3220();

    // Initialize SPI for DW3220
    dw3220_spi_config_t spi_config = {
        .mosi_pin = DW3220_SPI_MOSI,
        .miso_pin = DW3220_SPI_MISO,
        .sclk_pin = DW3220_SPI_SCLK,
        .cs_pin = DW3220_SPI_CS,
        .clock_speed_hz = 20000000  // 20 MHz initial, can increase to 36 MHz
    };
    ESP_ERROR_CHECK(dw3220_spi_init(&spi_config));
    ESP_LOGI(TAG, "SPI initialized");

    // Initialize DW3220 UWB
    ESP_ERROR_CHECK(dw3220_uwb_init());
    ESP_LOGI(TAG, "DW3220 UWB initialized");

    // Read and display DW3220 device ID
    uint32_t dev_id = dw3220_read_device_id();
    ESP_LOGI(TAG, "DW3220 Device ID: 0x%08lX", dev_id);

    // Initialize Session Manager
    session_manager_init();
    ESP_LOGI(TAG, "Session manager initialized");

    // Initialize BLE with OOB callback
    ble_gatt_config_t ble_config = {
        .device_name = DEVICE_NAME,
        .oob_callback = oob_message_callback
    };
    ESP_ERROR_CHECK(ble_gatt_server_init(&ble_config));
    ESP_LOGI(TAG, "BLE GATT server initialized");

    // Start BLE advertising
    ESP_ERROR_CHECK(ble_gatt_start_advertising());
    ESP_LOGI(TAG, "BLE advertising started");

    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "System ready. Waiting for connection...");
    ESP_LOGI(TAG, "=================================");

    // Main loop - handled by FreeRTOS tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
