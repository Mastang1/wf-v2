# 7　来源、版本、修改范围与许可证

检索/制作日期：2026-10-18（Asia/Shanghai）。两个仓库以检索时最新 master 的具体 commit 锁定，不使用浮动 `master` 作为复现实验的版本号。wolfBoot 的版本头写 2.10.0，但这里没有把 master 快照称为 2.10.0 稳定发行包。

| 内容 | 固定版本 / 链接 | 本包处理 |
|---|---|---|
| wolfBoot | [`0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f`](https://github.com/wolfSSL/wolfBoot/tree/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f)，提交时间 2026-10-16 UTC | `vendor/wolfBoot` 完整主仓库源文件；必要修改由 overlay 显式管理 |
| wolfBoot-examples | [`0c5e0ab54f9b422f704520bb66d26e81c765a338`](https://github.com/wolfSSL/wolfBoot-examples/tree/0c5e0ab54f9b422f704520bb66d26e81c765a338) | 完整主仓库快照；未展开 RTOS/TPM 等无关子模块 |
| wolfssl / wolfCrypt | [`f99453320ff306fe5bfb10788117f18c9bfeaf6b`](https://github.com/wolfSSL/wolfssl/tree/f99453320ff306fe5bfb10788117f18c9bfeaf6b) | 由上述 wolfBoot 的 `lib/wolfssl` gitlink 读取，非随意选最新版；源文件在相应子目录 |
| 官方 F4 配置 | [stm32f407-discovery.config](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/config/examples/stm32f407-discovery.config) | 作为起点和差异核对依据，未宣称等于探索者 |
| 官方 OpenOCD cfg | [openocd_stm32f4.cfg](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/config/openocd/openocd_stm32f4.cfg) | 继承 ST-Link/F4 target/reset-init；本包自定义生命周期、速率、复位方式 |
| 官方普通 APP | [app_stm32f4.c](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/app_stm32f4.c) | 主线复用，见 patches |
| 官方 TPM 对照 | [STM32F4 measured-boot](https://github.com/wolfSSL/wolfBoot-examples/tree/0c5e0ab54f9b422f704520bb66d26e81c765a338/test-app-STM32F4-measured-boot) | 解释为什么无 TPM 的探索者不能直接照搬 |

官方资料是解释补充；与锁定源码有差异时，本教程以实际代码为准，尤其是尾部状态扇区、恢复流程和函数名。

- [wolfBoot 手册：HAL](https://www.wolfssl.com/documentation/manuals/wolfboot/chapter04.html)：Flash 抽象接口与链接脚本。
- [wolfBoot 手册：Flash partitions](https://www.wolfssl.com/documentation/manuals/wolfboot/chapter05.html)：分区和持久状态。
- [wolfBoot 手册：Features](https://www.wolfssl.com/documentation/manuals/wolfboot/chapter06.html)：镜像头、APP API、签名与更新能力。注意手册中固定头大小说明不应脱离算法配置使用。
- 快照内 `docs/Signing.md`、`docs/firmware_update.md` 及相关构建/端口说明，可离线阅读，具体存在文件见目录。
- [ST：STM32F407ZG](https://www.st.com/en/microcontrollers-microprocessors/stm32f407zg.html)、[数据手册](https://www.st.com/resource/en/datasheet/stm32f407zg.pdf)、[RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)：芯片容量、地址和 Flash/时钟约束。板级原理图仍需按你的探索者修订版查证。
- [OpenOCD：Debug Adapter Configuration](https://openocd.org/doc/html/Debug-Adapter-Configuration.html)、[Configuration Guidelines](https://openocd.org/doc/html/Config-File-Guidelines.html)：ST-Link、target、reset 的分层使用。
- [Cortex-Debug 官方属性说明](https://github.com/Marus/cortex-debug/blob/master/debug_attributes.md)：attach、executable、configFiles、postAttachCommands 等。本包 `.vscode` 是按其文档编写的专用配置，没有冒称下载了某个可直接适配探索者的第三方项目。
- JD 来源及事实/推理区分见 [第 3 篇](03-requirements.md)。仅摘要并链接公开招聘页，不收录完整受版权保护的招聘文案。

## 本包修改了哪些上游文件

| overlay | 修改原因 |
|---|---|
| `hal/stm32f4.c` | 168MHz、3.3V 假设下 Flash latency 改 5 |
| `test-app/system.c` | APB1 /4、APB2 /2，与 F407 限值/Boot HAL 一致 |
| `test-app/app_stm32f4.c` | USART1 PA9/PA10、AFH、PCLK2 BRR；84MHz timer；镜像容量保留完整状态 Sector |
| `test-app/led.c` | F4 LED GPIO 操作禁用，避免 Discovery 管脚套用到未知修订探索者 |
| `test-app/timer.c` | TIM2 PSC 写值对应 PSC+1 的真实分频规则 |

这些是为本实验准备的源码适配，依据寄存器/代码审查，**未经探索者实板确认**。差异文件、完整 overlay 与修改前后 SHA256 在 `patches/`。原始上游内容可用 commit URL 逐行复核；`prepare.py` 只接受记录中的原始或已适配哈希，保护用户局部编辑。

## 许可证与分发边界

wolfBoot 根 `LICENSE` 是 GPLv3，源文件通常写 GPLv3-or-later；wolfssl 的 `COPYING` 是 GPLv3，另有商业许可渠道；wolfBoot-examples 根 `LICENSE` 是 GPLv2。各示例、依赖及单文件声明可能另有条件，以原文件为准。本包保留源文件版权、许可证及随附声明，overlay 延续被修改文件的原许可，**不将其改授 MIT**。

原创教程、独立 wrapper 和布局检查脚本的许可见 `LICENSE-TUTORIAL.txt`。本包没有重新分发 ST 手册全文、OpenOCD 安装程序、VS Code 扩展或 ARM 编译器；cfg 引用用户现有 OpenOCD 脚本。构建验证用的临时主机工具也不进入 ZIP。

产品使用/闭源分发前，应由项目负责人按实际采用的源文件与交付形式评估 GPL 或商业授权需求；这不是本包已经替你取得商业授权。学习包保留完整必要源码以供审查和复现。
