# Third-Party Notices

本文件用于说明本仓库中涉及的第三方代码来源及其授权边界，便于后续维护、二次分发和公开发布时进行合规核对。

## 1. NXP Android UWBJetpackExample

上游来源：

- <https://github.com/nxp-uwb/UWBJetpackExample>

本地目录：

- `NXP_Android_UWBJetpackExample-main/`

许可证：

- Apache License 2.0

再分发要求：

- 保留上游 `LICENSE.txt`
- 保留原始版权头
- 修改过的文件继续按上游许可证要求处理

## 2. Qorvo Nearby Interaction 样例代码

本地目录：

- `Qorvo_Apple_Nearby_Interaction_v1.3/`

当前可见的授权依据主要包括以下两类：

- 源码文件头里的 Qorvo 许可声明
- 仓库根目录里的
  `Qorvo Object Code OEM Software License Agreement 2021-07-29.pdf`

基于当前可见文本，对 Qorvo 部分的理解如下：

- Swift 样例源码文件头允许以 source / binary 形式再分发，也允许修改后再分发
- 但必须保留版权、条件和免责声明
- 使用范围被限制在 Qorvo IC 或包含 Qorvo IC 的模块
- 不能把派生产品包装成 Qorvo 官方产品

对于根目录 PDF 授权文本，当前理解如下：

- 这份 PDF 更偏向 object-code-only 软件授权
- 它不太像当前这些 Swift 源码文件的主许可证文本
- 所以不能简单拿它去覆盖整个 `Qorvo_Apple_Nearby_Interaction_v1.3/` 目录

因此，更稳妥的仓库处理原则如下：

- 保留所有 Qorvo 源文件头
- 保留随附授权材料
- 公开仓库时，不额外上传来源不清楚的 Qorvo 二进制、预编译库和 object code 包

## 3. 仓库级补充内容

本仓库后续补充的主要内容包括：

- `firmware/`
- `docs/`
- 根目录说明文档
- 仓库级协议说明和整合代码

这些内容的仓库级授权说明见 [LICENSE](C:/Users/hrx/Desktop/UWB/LICENSE)。
