# ESP32-S3 + DW3220 UWB Accessory Firmware

## 当前定位

这不是“已经完整支持 iPhone + Android UWB 测距”的量产固件，而是一个把协议边界拆清楚的参考工程：

- Android 路径：已具备 OOB 协议和 responder 启动入口
- iOS 路径：已具备 OOB 协议、错误返回和接入边界
- Apple 私有协议实现：默认缺失，开源构建下会返回 `apple_stack_missing`

## 关键结构

```text
main/
├── ble_oob/
│   ├── ble_gatt_server.c
│   └── oob_protocol.c
├── platform/
│   ├── apple_ni_adapter.c
│   └── fira_responder_adapter.c
├── session/
│   └── session_manager.c
└── dw3220_driver/
```

## 现在的协议行为

- `Initialize` 改为 `[0x0A, platformId]`
- `platformId = 0x01` 表示 iOS
- `platformId = 0x02` 表示 Android
- 新增 `AccessoryError = [0x04, errorCode]`

## Apple 路径说明

`apple_ni_adapter` 是明确的厂商实现占位层。当前默认实现不会生成 Apple accessory config，也不会启动真实 NI accessory session。

如果你要把 iPhone 真机 UWB 测距做通，就要在这里接入：

- Apple-compatible accessory config 生成逻辑
- Shareable Configuration Data 应用逻辑
- 与 DW3220 配套的真实 UWB session state machine

## Android 路径说明

`fira_responder_adapter` 目前负责：

- 返回 Android `UwbDeviceConfigData`
- 解析 Android `UwbPhoneConfigData`
- 把启动请求转给 `dw3220_start_ranging()`

这意味着它还是“参数桥接层”，不是完整可量产的 responder 栈。

## 构建

需要：

- ESP-IDF
- Python 3.8+

标准命令：

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash
idf.py monitor
```

## 重要说明

- 当前固件模型是“单连接、单会话”
- 不支持同时服务 iPhone 和 Android
- 协议文档请以仓库根 README 和 `docs/ble-oob-protocol.md` 为准
