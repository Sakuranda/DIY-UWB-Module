/**
 * @file ble_gatt_server.c
 * @brief BLE GATT Server Implementation using NimBLE
 */

#include "ble_gatt_server.h"
#include "oob_protocol.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>

static const char *TAG = "BLE_GATT";

// Nordic UART Service UUIDs
static const ble_uuid128_t NUS_SERVICE_UUID =
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

static const ble_uuid128_t NUS_RX_UUID =
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E);

static const ble_uuid128_t NUS_TX_UUID =
    BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                     0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E);

// State
static oob_message_callback_t g_oob_callback = NULL;
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t g_tx_attr_handle = 0;
static bool g_is_advertising = false;

// Forward declarations
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int nus_rx_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg);
static int nus_tx_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg);

// GATT Service Definition
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &NUS_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // RX Characteristic (Phone writes to this)
                .uuid = &NUS_RX_UUID.u,
                .access_cb = nus_rx_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                // TX Characteristic (Accessory notifies via this)
                .uuid = &NUS_TX_UUID.u,
                .access_cb = nus_tx_access,
                .val_handle = &g_tx_attr_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 }  // Terminator
        },
    },
    { 0 }  // Terminator
};

/**
 * @brief RX Characteristic access callback
 */
static int nus_rx_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        uint8_t *data = malloc(len);
        if (data == NULL) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        int rc = ble_hs_mbuf_to_flat(ctxt->om, data, len, NULL);
        if (rc != 0) {
            free(data);
            return BLE_ATT_ERR_UNLIKELY;
        }

        ESP_LOGI(TAG, "RX received %d bytes", len);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, len, ESP_LOG_DEBUG);

        // Process OOB message
        if (len > 0 && g_oob_callback != NULL) {
            uint8_t msg_id = data[0];
            g_oob_callback(msg_id, data + 1, len - 1);
        }

        free(data);
    }
    return 0;
}

/**
 * @brief TX Characteristic access callback
 */
static int nus_tx_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // TX is notify-only, no read/write access needed
    return 0;
}

/**
 * @brief GAP event handler
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                g_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "Connected, handle=%d", g_conn_handle);
            } else {
                ESP_LOGE(TAG, "Connection failed, status=%d", event->connect.status);
                g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                // Restart advertising
                ble_gatt_start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected, reason=%d", event->disconnect.reason);
            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            // Restart advertising
            ble_gatt_start_advertising();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete");
            g_is_advertising = false;
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Subscribe event: attr_handle=%d, cur_notify=%d",
                     event->subscribe.attr_handle, event->subscribe.cur_notify);
            break;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
            break;

        default:
            break;
    }
    return 0;
}

/**
 * @brief NimBLE host task
 */
static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * @brief BLE stack sync callback
 */
static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "BLE stack synced");

    // Get device address
    uint8_t addr[6];
    ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, addr, NULL);
    ESP_LOGI(TAG, "Device address: %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

/**
 * @brief Initialize BLE GATT Server
 */
esp_err_t ble_gatt_server_init(const ble_gatt_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    g_oob_callback = config->oob_callback;

    // Initialize NimBLE
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE port init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure host
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = NULL;

    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Configure GATT services
    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT count cfg failed: %d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT add svcs failed: %d", rc);
        return ESP_FAIL;
    }

    // Set device name
    rc = ble_svc_gap_device_name_set(config->device_name);
    if (rc != 0) {
        ESP_LOGE(TAG, "Set device name failed: %d", rc);
    }

    // Start NimBLE host task
    nimble_port_freertos_init(nimble_host_task);

    ESP_LOGI(TAG, "BLE GATT server initialized");
    return ESP_OK;
}

/**
 * @brief Deinitialize BLE GATT Server
 */
void ble_gatt_server_deinit(void)
{
    nimble_port_stop();
    nimble_port_deinit();
    g_oob_callback = NULL;
    g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
}

/**
 * @brief Start BLE advertising
 */
esp_err_t ble_gatt_start_advertising(void)
{
    if (g_is_advertising) {
        return ESP_OK;
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN,
        .itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX,
    };

    // Set advertising data
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Set adv fields failed: %d", rc);
        return ESP_FAIL;
    }

    // Set scan response with service UUID
    struct ble_hs_adv_fields rsp_fields = {0};
    rsp_fields.uuids128 = &NUS_SERVICE_UUID;
    rsp_fields.num_uuids128 = 1;
    rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Set scan rsp failed: %d", rc);
        return ESP_FAIL;
    }

    // Start advertising
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Start adv failed: %d", rc);
        return ESP_FAIL;
    }

    g_is_advertising = true;
    ESP_LOGI(TAG, "Advertising started");
    return ESP_OK;
}

/**
 * @brief Stop BLE advertising
 */
esp_err_t ble_gatt_stop_advertising(void)
{
    if (!g_is_advertising) {
        return ESP_OK;
    }

    int rc = ble_gap_adv_stop();
    if (rc != 0) {
        ESP_LOGE(TAG, "Stop adv failed: %d", rc);
        return ESP_FAIL;
    }

    g_is_advertising = false;
    ESP_LOGI(TAG, "Advertising stopped");
    return ESP_OK;
}

/**
 * @brief Send data via TX characteristic (notify)
 */
esp_err_t ble_gatt_send_data(const uint8_t *data, uint16_t len)
{
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Not connected, cannot send");
        return ESP_ERR_INVALID_STATE;
    }

    if (g_tx_attr_handle == 0) {
        ESP_LOGE(TAG, "TX handle not set");
        return ESP_ERR_INVALID_STATE;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGE(TAG, "Failed to allocate mbuf");
        return ESP_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, g_tx_attr_handle, om);
    if (rc != 0) {
        ESP_LOGE(TAG, "Notify failed: %d", rc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent %d bytes via notify", len);
    return ESP_OK;
}

/**
 * @brief Check if a device is connected
 */
bool ble_gatt_is_connected(void)
{
    return g_conn_handle != BLE_HS_CONN_HANDLE_NONE;
}
