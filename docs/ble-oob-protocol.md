# BLE OOB 协议规范文档

> 版本: 2.0
> 状态: 与当前代码实现保持一致

## 1. 设计原则

本仓库现在不再依赖“收到 `0x0B` 后猜平台”的做法，而是采用显式平台字段：

- `Initialize = [0x0A, platformId]`
- 固件在初始化阶段就决定返回 iOS 还是 Android 配置
- 固件可以显式返回错误，而不是让客户端在后续会话阶段隐式失败

## 2. BLE 服务

### 2.1 主服务

自制硬件主路径使用 `Nordic UART Service (NUS)`：

| 名称 | UUID |
|---|---|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

### 2.2 可选兼容服务

Qorvo NI Service 在客户端侧保留兼容逻辑，但不是自制硬件主路径所必需。

## 3. 消息格式

统一格式：

```text
[msgId, payload...]
```

### 3.1 消息 ID

| 方向 | ID | 名称 |
|---|---|---|
| Accessory -> Phone | `0x01` | `AccessoryConfigData` |
| Accessory -> Phone | `0x02` | `UwbDidStart` |
| Accessory -> Phone | `0x03` | `UwbDidStop` |
| Accessory -> Phone | `0x04` | `AccessoryError` |
| Phone -> Accessory | `0x0A` | `Initialize` |
| Phone -> Accessory | `0x0B` | `ConfigureAndStart` |
| Phone -> Accessory | `0x0C` | `Stop` |

### 3.2 Initialize

```text
[0x0A, platformId]
```

| `platformId` | 平台 |
|---|---|
| `0x01` | iOS |
| `0x02` | Android |

### 3.3 AccessoryError

```text
[0x04, errorCode]
```

| `errorCode` | 名称 | 含义 |
|---|---|---|
| `0x01` | `unsupported_platform` | 平台不支持 |
| `0x02` | `apple_stack_missing` | 缺少 Apple 私有协议实现 |
| `0x03` | `invalid_config` | 配置 payload 无效 |
| `0x04` | `busy` | 配件已有活跃会话 |

## 4. 平台配置负载

### 4.1 iOS

`AccessoryConfigData` payload 必须是 Apple 可接受的 Nearby Interaction accessory 配置字节。

当前开源构建中：

- 固件没有这份真实 payload 的生成能力
- 固件会返回 `AccessoryError(apple_stack_missing)`
- 代码接入点在 `apple_ni_adapter`

### 4.2 Android

`AccessoryConfigData` payload 使用 `UwbDeviceConfigData`：

```text
specVerMajor(2)
specVerMinor(2)
chipId(2)
chipFwVersion(2)
mwVersion(3)
supportedUwbProfileIds(4)
supportedDeviceRangingRoles(1)
deviceMacAddress(2)
```

`ConfigureAndStart` payload 使用 `UwbPhoneConfigData`：

```text
specVerMajor(2)
specVerMinor(2)
sessionId(4)
preambleId(1)
channel(1)
profileId(1)
deviceRangingRole(1)
phoneMacAddress(2)
```

## 5. 时序

### 5.1 iOS

1. App 连接配件
2. 发送 `[0x0A, 0x01]`
3. 固件尝试走 `apple_ni_adapter`
4. 若无私有实现，返回 `[0x04, 0x02]`
5. App 明确提示“缺少 Apple NI vendor stack”

### 5.2 Android

1. App 连接配件
2. 发送 `[0x0A, 0x02]`
3. 固件返回 `[0x01, androidConfig...]`
4. App 发送 `[0x0B, phoneConfig...]`
5. 固件通过 `fira_responder_adapter` 启动 responder
6. 固件返回 `[0x02]`

## 6. 固件接入边界

| 文件 | 当前作用 |
|---|---|
| `platform/apple_ni_adapter.c` | Apple NI 私有实现占位层 |
| `platform/fira_responder_adapter.c` | Android responder 参数装配与启动入口 |
| `session/session_manager.c` | 协议状态机与错误码分发 |

## 7. 注意事项

- 这个协议现在明确面向“单硬件、单手机、单会话”
- 不支持两个手机同时在线
- 不再宣称开源固件已经自动支持完整 iOS 配件测距
