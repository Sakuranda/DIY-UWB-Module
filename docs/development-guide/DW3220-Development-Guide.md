# DW3220/DW3210 UWB 模块开发指南

> 版本: 1.0
> 日期: 2026-01-31

> 更新说明:
> - 当前仓库的协议和能力边界，请优先以根 `README.md` 和 `docs/ble-oob-protocol.md` 为准。
> - 尤其是 iPhone 配件测距部分，开源代码当前只保留了接入边界，并不代表已经具备完整的 Apple 私有协议实现。

## 目录

1. [快速入门](#1-快速入门)
2. [固件开发](#2-固件开发)
3. [iOS 集成](#3-ios-集成)
4. [Android 集成](#4-android-集成)
5. [调试技巧](#5-调试技巧)
6. [故障排除](#6-故障排除)
7. [常见问题](#7-常见问题)

---

## 1. 快速入门

### 1.1 开发环境准备

#### ESP32-S3 开发环境

```bash
# 安装 ESP-IDF v5.0+
git clone -b v5.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
source export.sh

# 验证安装
idf.py --version
```

#### iOS 开发环境

- Xcode 14+
- iOS 15+ SDK
- 支持 U1 芯片的 iPhone (11/12/13/14 系列)

#### Android 开发环境

- Android Studio Hedgehog+
- Android SDK 33+
- 支持 UWB 的 Android 设备

### 1.2 硬件准备

| 组件 | 规格 | 数量 |
|------|------|------|
| ESP32-S3 开发板 | DevKitC-1 | 1 |
| DW3220 模块 | Qorvo | 1 |
| 杜邦线 | 母对母 | 若干 |
| USB 线 | Type-C | 1 |

### 1.3 快速测试

```bash
# 克隆项目
cd /path/to/UWB

# 进入固件目录
cd firmware/esp32-dw3220

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash  # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux

# 监控日志
idf.py monitor
```

预期输出:
```
I (xxx) DW3220_MAIN: =================================
I (xxx) DW3220_MAIN: ESP32-S3 + DW3220 UWB Accessory
I (xxx) DW3220_MAIN: =================================
I (xxx) DW3220_MAIN: DW3220 Device ID: 0xDECA0302
I (xxx) BLE_GATT: Advertising started
I (xxx) DW3220_MAIN: System ready. Waiting for connection...
```

---

## 2. 固件开发

### 2.1 项目结构

```
firmware/esp32-dw3220/
├── main/
│   ├── main.c                 # 应用入口
│   ├── dw3220_driver/         # UWB 驱动
│   │   ├── dw3220_spi.c/h     # SPI 通信
│   │   └── dw3220_uwb.c/h     # UWB 功能
│   ├── ble_oob/               # BLE 通信
│   │   ├── ble_gatt_server.c/h
│   │   └── oob_protocol.c/h
│   ├── ni_accessory/          # Apple NI 协议
│   │   └── ni_protocol.c/h
│   └── session/               # 会话管理
│       └── session_manager.c/h
```

### 2.2 SPI 驱动使用

```c
#include "dw3220_spi.h"

// 初始化 SPI
dw3220_spi_config_t config = {
    .mosi_pin = GPIO_NUM_11,
    .miso_pin = GPIO_NUM_13,
    .sclk_pin = GPIO_NUM_12,
    .cs_pin = GPIO_NUM_10,
    .clock_speed_hz = 20000000  // 20 MHz
};
dw3220_spi_init(&config);

// 读取寄存器
uint32_t dev_id = dw3220_spi_read32(0x00);

// 写入寄存器
dw3220_spi_write32(0x04, 0x00000040);
```

### 2.3 UWB 会话管理

```c
#include "dw3220_uwb.h"

// 初始化 UWB
dw3220_uwb_init();

// 配置会话
dw3220_session_config_t session = {
    .session_id = 12345,
    .device_role = UWB_ROLE_RESPONDER,
    .channel = UWB_CHANNEL_9,
    .preamble_index = 10,
    .profile_id = 1
};

// 启动测距
dw3220_start_ranging(&session);

// 获取结果
dw3220_ranging_result_t result;
if (dw3220_get_ranging_result(&result) == ESP_OK) {
    printf("Distance: %.2f m\n", result.distance_m);
    printf("Azimuth: %.1f deg\n", result.azimuth_deg);
}

// 停止测距
dw3220_stop_ranging();
```

### 2.4 BLE OOB 消息处理

```c
#include "oob_protocol.h"

// 消息回调
void oob_callback(uint8_t msg_id, uint8_t *data, uint16_t len) {
    switch (msg_id) {
        case OOB_MSG_INITIALIZE:  // 0x0A
            // 发送配件配置数据
            send_accessory_config();
            break;

        case OOB_MSG_CONFIGURE_AND_START:  // 0x0B
            // 解析配置，启动测距
            start_ranging_with_config(data, len);
            break;

        case OOB_MSG_STOP:  // 0x0C
            // 停止测距
            stop_ranging();
            break;
    }
}
```

---

## 3. iOS 集成

### 3.1 项目配置

在 `Info.plist` 中添加权限:

```xml
<key>NSNearbyInteractionUsageDescription</key>
<string>用于 UWB 测距</string>

<key>NSBluetoothAlwaysUsageDescription</key>
<string>用于发现 UWB 配件</string>

<key>NSCameraUsageDescription</key>
<string>用于改善定位精度</string>
```

### 3.2 BLE 连接

```swift
import CoreBluetooth

// 服务 UUID
let serviceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
let rxUUID = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
let txUUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

// 扫描配件
centralManager.scanForPeripherals(withServices: [serviceUUID])

// 连接后发送初始化
let initData = Data([0x0A])
peripheral.writeValue(initData, for: rxCharacteristic, type: .withResponse)
```

### 3.3 NearbyInteraction 会话

```swift
import NearbyInteraction

// 收到配件配置数据后
func setupNISession(configData: Data) {
    do {
        let config = try NINearbyAccessoryConfiguration(data: configData)
        config.isCameraAssistanceEnabled = true

        niSession = NISession()
        niSession.delegate = self
        niSession.run(config)
    } catch {
        print("配置失败: \(error)")
    }
}

// 代理回调
func session(_ session: NISession, didUpdate nearbyObjects: [NINearbyObject]) {
    guard let object = nearbyObjects.first,
          let distance = object.distance else { return }

    print("距离: \(distance) m")

    if let direction = object.direction {
        let azimuth = atan2(direction.x, direction.z) * 180 / .pi
        print("方位角: \(azimuth)°")
    }
}
```

### 3.4 发送配置数据

```swift
// 收到 Apple Shareable Configuration 后
func session(_ session: NISession,
             didGenerateShareableConfigurationData data: Data,
             for object: NINearbyObject) {
    // 构建消息: 0x0B + Apple 配置数据
    var message = Data([0x0B])
    message.append(data)

    // 通过 BLE 发送给配件
    peripheral.writeValue(message, for: rxCharacteristic, type: .withResponse)
}
```

---

## 4. Android 集成

### 4.1 权限配置

在 `AndroidManifest.xml` 中添加:

```xml
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.UWB_RANGING" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
```

### 4.2 BLE 连接

```java
// 服务 UUID
UUID SERVICE_UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
UUID RX_UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
UUID TX_UUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// 扫描过滤
ScanFilter filter = new ScanFilter.Builder()
    .setServiceUuid(new ParcelUuid(SERVICE_UUID))
    .build();

// 发送初始化 (0x0A, 与 iOS 统一)
byte[] initData = new byte[]{0x0A};
rxCharacteristic.setValue(initData);
bluetoothGatt.writeCharacteristic(rxCharacteristic);
```

### 4.3 UWB Jetpack API

```java
import androidx.core.uwb.*;

// 获取 UWB 管理器
UwbManager uwbManager = UwbManager.createInstance(context);

// 创建会话
Single<UwbControleeSessionScope> sessionSingle =
    UwbManagerRx.controleeSessionScopeSingle(uwbManager);

UwbControleeSessionScope session = sessionSingle.blockingGet();

// 配置测距参数
RangingParameters params = new RangingParameters(
    RangingParameters.CONFIG_UNICAST_DS_TWR,
    sessionId,
    subSessionId,
    sessionKey,
    null,
    new UwbComplexChannel(9, 10),  // Channel 9, Preamble 10
    Arrays.asList(new UwbDevice(new UwbAddress(peerAddress))),
    RangingParameters.RANGING_UPDATE_RATE_AUTOMATIC
);

// 启动测距
Flowable<RangingResult> results =
    UwbClientSessionScopeRx.rangingResultsFlowable(session, params);

results.subscribe(result -> {
    if (result instanceof RangingResult.RangingResultPosition) {
        RangingResult.RangingResultPosition pos =
            (RangingResult.RangingResultPosition) result;
        float distance = pos.getPosition().getDistance().getValue();
        float azimuth = pos.getPosition().getAzimuth().getValue();
        Log.d(TAG, "距离: " + distance + " m, 方位: " + azimuth + "°");
    }
});
```

### 4.4 发送手机配置

```java
// 构建 UwbPhoneConfigData
UwbPhoneConfigData phoneConfig = new UwbPhoneConfigData();
phoneConfig.setSessionId(sessionId);
phoneConfig.setChannel((byte) 9);
phoneConfig.setPreambleId((byte) 10);
phoneConfig.setProfileId((byte) 1);
phoneConfig.setDeviceRangingRole((byte) 1);  // Controller
phoneConfig.setPhoneMacAddress(localAddress);

// 发送: 0x0B + 配置数据
byte[] message = Utils.concat(
    new byte[]{0x0B},
    phoneConfig.toByteArray()
);
rxCharacteristic.setValue(message);
bluetoothGatt.writeCharacteristic(rxCharacteristic);
```

---

## 5. 调试技巧

### 5.1 日志级别配置

```c
// ESP-IDF 中设置日志级别
esp_log_level_set("DW3220_SPI", ESP_LOG_DEBUG);
esp_log_level_set("BLE_GATT", ESP_LOG_INFO);
esp_log_level_set("SESSION_MGR", ESP_LOG_DEBUG);
```

### 5.2 SPI 通信调试

```c
// 验证 DW3220 通信
uint32_t dev_id = dw3220_read_device_id();
if ((dev_id & 0xFFFF0000) != 0xDECA0000) {
    ESP_LOGE(TAG, "SPI 通信失败! 读取 ID: 0x%08lX", dev_id);
    // 检查:
    // 1. 接线是否正确
    // 2. SPI 时钟是否过快
    // 3. DW3220 是否正常供电
}
```

### 5.3 BLE 调试

使用 nRF Connect App 测试:
1. 扫描设备 "DW3220-UWB"
2. 连接后查看服务
3. 订阅 TX 特征 (0x6E400003...)
4. 写入 RX 特征 (0x6E400002...) 发送 `0x0A`
5. 观察 TX 通知返回数据

### 5.4 UWB 测距调试

```c
// 打印测距参数
ESP_LOGI(TAG, "Session: ID=%lu, CH=%d, PRE=%d, ROLE=%d",
         session->session_id,
         session->channel,
         session->preamble_index,
         session->device_role);

// 检查测距状态
dw3220_ranging_result_t result;
esp_err_t err = dw3220_get_ranging_result(&result);
if (err == ESP_OK && result.valid) {
    ESP_LOGI(TAG, "测距成功: %.2f m", result.distance_m);
} else {
    ESP_LOGW(TAG, "测距无效或超时");
}
```

---

## 6. 故障排除

### 6.1 SPI 通信问题

| 症状 | 可能原因 | 解决方案 |
|------|----------|----------|
| Device ID = 0x00000000 | 未供电/接线错误 | 检查电源和连线 |
| Device ID = 0xFFFFFFFF | CS 未拉低/SPI 模式错误 | 检查 CS 引脚和 SPI 模式 |
| 读取不稳定 | 时钟过快/信号完整性 | 降低 SPI 时钟/缩短走线 |

### 6.2 BLE 连接问题

| 症状 | 可能原因 | 解决方案 |
|------|----------|----------|
| 搜索不到设备 | 未广播/广播数据错误 | 检查广播代码 |
| 连接后断开 | MTU 协商失败 | 延长连接参数超时 |
| 数据收发失败 | 特征权限不匹配 | 检查读/写/通知权限 |

### 6.3 UWB 测距问题

| 症状 | 可能原因 | 解决方案 |
|------|----------|----------|
| 无测距结果 | 参数不匹配 | 确认双方使用相同 Channel/Preamble |
| 距离不准确 | 未校准天线延迟 | 执行天线校准 |
| 频繁丢失 | 环境多径/遮挡 | 确保视距/减少障碍物 |

### 6.4 iOS 特定问题

| 症状 | 可能原因 | 解决方案 |
|------|----------|----------|
| NI 配置失败 | 数据格式错误 | 验证配件配置数据格式 |
| 无方向信息 | 设备不支持/未启用 Camera | 检查设备型号/启用 ARKit |
| 会话超时 | 配件响应慢 | 优化配件处理速度 |

### 6.5 Android 特定问题

| 症状 | 可能原因 | 解决方案 |
|------|----------|----------|
| UWB 不支持 | 设备无 UWB 硬件 | 确认设备支持 UWB |
| 权限拒绝 | 未授权 UWB_RANGING | 请求运行时权限 |
| 配置 ID 无效 | Profile 不支持 | 使用 CONFIG_UNICAST_DS_TWR |

---

## 7. 常见问题

### Q1: DW3220 和 DW3210 有什么区别?

A: DW3220 支持双天线 PDoA (相位差到达角)，可提供更精确的角度信息；DW3210 仅支持单天线测距。对于需要方向指示的应用，推荐 DW3220。

### Q2: 为什么 iOS 和 Android 使用不同的消息格式?

A: iOS 使用 Apple 专有的 NearbyInteraction Accessory Protocol，Android 使用 AOSP 定义的 Ranging OOB 格式。我们的固件通过平台检测自动适配两种格式。

### Q3: 测距精度如何优化?

A:
1. 校准天线延迟
2. 使用 Channel 9 (精度更高)
3. 确保视距条件
4. 减少金属遮挡
5. 多次测量取平均

### Q4: 最大通信距离是多少?

A: 视距条件下可达 100m，但实际应用中建议 < 30m 以保证稳定性。室内多径环境下距离会明显缩短。

### Q5: 如何同时支持 iOS 和 Android?

A: 我们的固件设计为双模式支持：
- BLE 服务使用通用的 Nordic UART Service
- 消息 ID 统一 (0x0A/0x0B/0x0C)
- 配件配置数据格式兼容两平台
- 会话管理器自动检测连接平台

### Q6: 需要 MFi 认证吗?

A: 使用 NearbyInteraction Accessory Protocol 与 Apple 设备通信，理论上需要通过 Apple 的互操作认证。但对于开发和测试阶段，可以先进行验证。

### Q7: 功耗如何优化?

A:
1. 空闲时进入 Deep Sleep (1µA)
2. 使用 Delayed TX/RX 减少活动时间
3. 降低测距频率
4. 优化 BLE 连接参数

---

## 附录: 消息协议快速参考

| 方向 | ID | 名称 | 描述 |
|------|-----|------|------|
| Phone → Accessory | `0x0A` | Initialize | 请求配件配置数据 |
| Phone → Accessory | `0x0B` | ConfigureAndStart | 发送配置，启动测距 |
| Phone → Accessory | `0x0C` | Stop | 停止测距 |
| Accessory → Phone | `0x01` | AccessoryConfigData | 配件 UWB 配置 |
| Accessory → Phone | `0x02` | UwbDidStart | 测距已启动 |
| Accessory → Phone | `0x03` | UwbDidStop | 测距已停止 |

---

*由 Claude Code 自动生成*
