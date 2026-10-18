# 6　逐文件读通当前配置，再换成自己的 APP

“逐文件”在这里指 **STM32F407 内部 Flash SWAP 实验实际相关的构建、启动、HAL、镜像、状态机、主机工具、APP 和 examples 对照文件**。仓库还包含许多其他架构、可选安全模块和测试，它们完整保留在快照中，但本文不声称对所有平台的每一行做完审计。自动生成的 [函数定位索引](source-functions.md) 给出原始快照行号；overlay 后请以符号和实际文件为准。

## 构建路径逐文件

| 文件 | 读什么、怎样复证 |
|---|---|
| `Makefile` | 前部先 include 配置；看 `OBJS` 和更新后端选择；目标 `wolfboot.elf/bin`、`test-app/image.bin`、`factory.bin`；生成 `include/target.h`/`config/target.ld`。上游签名命令某些分支带 `|| true`，本包另行显式 sign 并检查输出，不能只信 make 返回码。 |
| `tools/config.mk` | 默认配置的生效条件，尤其 ARCH 已由 .config 定义时。追踪变量最终值要看 include 顺序，不能把默认块当强制设置。 |
| `arch.mk` | 从 `ifeq ($(ARCH),ARM)` 开始，找 `TARGET=stm32f4`、`CROSS_COMPILE` 和 Cortex-M CPU flags，再看工具变量 CC/LD/SIZE。F4 的实际编译 flags 应可在日志中观察到 Cortex-M4/Thumb。 |
| `options.mk` | `SIGN=ED25519` 分支添加 Ed25519/SHA512 等依赖，`HASH=SHA256` 添加镜像 Hash；看 `CFLAGS_EXTRA`、IMAGE_HEADER_SIZE、VTOR 和 restore clock。选项会影响 Boot 和 APP，不能只改一边。 |
| `config/examples/stm32f407-discovery.config` | 对照官方 128KiB 分区、160MHz 宏和 PLL 参数；本包配置明确偏离的原因见第 1 篇。 |
| `include/target.h.in` | 模板中地址/大小占位符；最终 `include/target.h` 是构建产物。追到 `WOLFBOOT_FIXED_PARTITIONS`、重叠与边界校验。 |
| `hal/stm32f4.ld` | Boot FLASH 起点 0x08000000、长度由 BOOTLOADER_PARTITION_SIZE 替换；普通 RAM 0x1C000；向量 KEEP、data 的加载/运行地址、RAM 代码段、BSS 和栈顶符号。 |
| `test-app/Makefile` | 独立 APP 对象集和链接流程；复用 ../src/libwolfboot.c，生成自己的链接脚本并导出 APP 向量地址。看构建中引用的 HAL 对象是否共享，避免并行构建两种不同配置互相覆盖。 |
| `test-app/ARM.ld` | FLASH = BOOT+头大小；RAM 低 16KiB；data/bss 的启动契约。其默认 LENGTH 没扣完整尾部状态 Sector，本包签名守卫承担额外容量限制。自有 APP 建议直接在链接器中预留状态空间并加 ASSERT。 |

## 启动、硬件与镜像路径逐文件

| 文件 | 核心职责 / 上下游 / 你应回答的问题 |
|---|---|
| `src/boot_arm.c` | `isr_reset` 进行运行时初始化；向量表持有栈与 IRQ 函数；`do_boot` 处理 MPU/VTOR/MSP/入口。上游是 CPU reset 或 FSM，下游是 loader/APP。问：APP 中断向量和栈的物理地址在哪里？ |
| `src/loader.c` | 默认 `main` 调 HAL、可选外设和 `wolfBoot_start`。读条件分支区分 main/loader_main；本包未启用 TPM/UART_FLASH，所以不要把那些调用写入本配置必经流程。 |
| `hal/stm32f4.c` | RCC PLL、Flash wait states、解锁、忙等待、编程、Sector 擦除、prepare_boot。板级 overlay 只改 168MHz 等待周期；物理 Flash 几何沿用官方。问：擦除失败能否返回、写粒度/地址是否对齐、函数/常量在哪个存储器？ |
| `hal/hal.c` | 通用辅助和弱接口；`hal_flash_protect` 默认返回成功但不做保护。问：你的 target 是否提供强实现？不能把弱 stub 当硬件保证。 |
| `include/hal.h` | 统一的地址类型、RAMFUNCTION、Flash 接口契约。看 API 返回值与长度含义，不把芯片 HAL 库的同名 API 参数顺序生搬过来。 |
| `include/image.h` | `wolfBoot_image`、sanity/verification 宏、sector flags、镜像访问包装。大量平台加固分支可先折叠，只看本配置实际预处理结果。 |
| `include/wolfboot/wolfboot.h` | APP 能依赖的接口、magic、TLV、分区 ID、状态码；当前 PART_BOOT=0、PART_UPDATE=1、PART_SWAP=2。以此维护共同镜像契约。 |
| `src/image.c` | `wolfBoot_open_image` 建立视图，`wolfBoot_verify_integrity` 校验摘要，`wolfBoot_verify_authenticity` 查 key slot/权限/算法并调用 wolfCrypt。追一次失败返回直到 FSM，确认没有失败后继续跳转。 |
| `src/libwolfboot.c` | TLV 查询、版本读取、分区 trailer 访问、sector nibble、trigger/success、Flash 访问封装。问：写一个标记前要不要擦整个 Sector？目前 trigger 会擦候选末尾完整 Sector。 |
| `src/update_flash.c` | `wolfBoot_start` 做复位决策；`wolfBoot_update` 候选检查和逐块交换；`wolfBoot_copy_sector` 搬块；`wolfBoot_swap_update_size` 验恢复头；`wolfBoot_swap_and_final_erase` 收尾/恢复。第 5 篇的每条箭头都能在这里找到。 |
| `src/string.c` | 裸机 memcpy/memset 等基础实现；它们可能被 Flash/RAM 搬运调用，不是“和安全无关的杂物”。不同编译选项可能选择工具链实现，最终以链接 map 为准。 |
| `src/keystore.c`（生成）与 `include/keystore.h` | keygen 输出授权公钥及查询接口。不能从包内固定文件推断本地新生成的公钥值。编译出的公钥必须对应签名私钥。 |
| `include/user_settings.h` | 裁剪 wolfCrypt 的平台/算法功能；不是把完整 TLS 客户端塞入 Bootloader。需要修改算法时同时看 options.mk 与此文件条件。 |

## 测试 APP 与主机工具逐文件

| 文件 | 为什么现在读 |
|---|---|
| `test-app/app_stm32f4.c` | 主线应用、UART 下载状态机、page 缓冲、版本奇偶确认、trigger。overlay 改 PA9/PA10 AFH/BRR、计时传入时钟和下载容量上限。它不自动证明自己的重传/超时策略适合产品。 |
| `test-app/system.c` | APP 自己初始化 HSE/PLL、Flash latency；overlay 修正 APB1/APB2 分频。Bootloader restore clock 与 APP 再初始化之间是实际交接契约。 |
| `test-app/timer.c` | TIM2 周期 ISR 增加 time_elapsed、TIM4 PWM；overlay 修正 TIM2 PSC 编码，APP 使用 84MHz timer clock。不用 LED 并不表示不用 TIM2。 |
| `test-app/led.c` | 官方为 PD14/PD15；overlay 将 F4 输出操作置空，避免未经确认的板上引脚冲突。想加 PF9/PF10，用普通 GPIO 心跳更合适，不能只替换 TIM4 PWM 的 pin 数字。 |
| `test-app/system.h`、`timer.h`、`led.h` | WFI、NVIC/计时与 LED 接口；理解 APP main 为什么等待中断，不要单步暂停太久后把时间观察误当真实性能。 |
| `tools/keytools/Makefile` | 主机 gcc 构建签名工具与其 wolfCrypt 依赖；编译器是主机版本，不是 arm-none-eabi-gcc。 |
| `tools/keytools/keygen.c` | RNG 生成学习密钥、导出私钥与 keystore；看输出文件/force/no-overwrite 行为。生产系统应把签名权限放受控主机/HSM/签名服务。 |
| `tools/keytools/sign.c` | 解析命令行、形成头与 TLV、按约定 Hash/签名、写输出。版本参与认证，不是刷完固件后随便改一个字。 |
| `tools/bin-assemble/bin-assemble.c` | 官方按地址合并二进制的实现可用于理解 factory；本包使用 Python 守卫明确 FF 填充及地址上限，不依赖它处理额外工厂空白区。 |
| `config/openocd/openocd_stm32f4.cfg` | 官方 ST-Link/F4 target 和 reset-init 动作；本包 cfg 从它改写并移除自动 init。 |
| `tools/unit-tests/unit-update-flash.c` | 找正常升级/同版本拒绝/回滚/中断/恢复头畸形测试，按不变量理解，不只看测试名称。运行需要另行满足上游主机测试依赖。 |

## wolfCrypt 依赖的阅读深度

`lib/wolfssl/wolfcrypt/src/sha256.c` 负责镜像摘要；`sha512.c`、`ed25519.c`、`fe_operations.c`、`ge_operations.c` 是 Ed25519 路径主要组成，可能另选平台汇编优化。`wc_port.c` 和 settings/types 头决定裸机平台适配、内存/时间/RNG 等能力。你需要能说明 API 输入/输出、错误码、缓冲长度和可信数据来源，不要求先推导椭圆曲线数学。

主机 keygen 需要安全随机数；目标验签不需要每次生成签名私钥。Bootloader 的编译依赖可能还包含 SP math 或通用加固对象，以构建日志/map 为实证。不要根据一个 `.o` 文件存在就判断其所有代码都链接进最终 ELF；section garbage collection 会去掉未用段。

## examples 仓库的 STM32F4 为什么只能作为对照

对照目录 `vendor/wolfBoot-examples/test-app-STM32F4-measured-boot/`：

| 文件/目录 | 已核对内容与本实验取舍 |
|---|---|
| `README.md` | 明确要 STM32F407-Discovery + 外接 TPM2.0；列出 PE0/PB3/PB4/PB5 SPI 与 PB6/PB7 USART1。它不是探索者无 TPM 的普通起步例子。 |
| `measured.wolfboot.config` | 增加 TPM/measurement 相关配置并规定镜像/分区；不能覆盖本包 config 后还声称同一实验。 |
| `Makefile` | 同时编译 Bootloader、wolfTPM 和 APP，生成 factory。还保留旧 Python keytool 回退路径；本包锁定 wolfBoot 已无该旧路径，应使用 C 工具。旧例 Makefile 与最新依赖组合需另行适配。 |
| `src/app_stm32f4.c` | UART 打印 PCR/TPM 交互等应用工作；普通 test-app 的下载/健康确认职责不同。 |
| `src/system.c`、`src/timer.c`、`src/led.c` | 板级初始化和 Discovery 显示/计时；再次说明同是 F407 不能直接套用探索者引脚。 |
| `src/startup_arm.c`、`src/ARM.ld` | 独立 APP 的启动向量和链接契约，与主线 reset/APP 地址迁移方法一致。 |

Secure Boot 用签名决定能否执行，Measured Boot 把度量送入 TPM PCR 供后续判断/证明，两者不是替代关系。本包保留该仓库完整快照和许可证，但其子模块（wolfTPM、各 RTOS 等）未展开，**不宣称可以离线构建所有 examples**。主线所需 wolfssl 已随包提供；如果以后做 TPM 实验，按对应示例 commit 的子模块指针准备依赖并单独复证。

## 换成你的独立 APP

保留你自己的 CMake/Make/CubeIDE 工程，按以下顺序改，不必把整个 APP 搬到 wolfBoot 树里。

1. 在链接器设置 APP Flash 起点 `0x08020100`，有效长度不超过 `0x1FF00`；向量表必须放起点。RAM 按自己工程和 F407 普通 SRAM/CCM 的实际区域规划，栈别落进未连接区域。
2. APP 启动设置/保持 VTOR=0x08020100；许多 STM32 SystemInit 会无条件把 VTOR 改回 0x08000000，需要检查。重新建立时钟和中断环境，不假定全部外设是硬复位状态。
3. 编译你自己的 ELF，用 objcopy 得到 raw binary；用本包 `images.py raw` 检查向量和大小，再用官方同一 sign 工具/私钥签。APP ELF 用于调试，signed.bin 用于分区写入。
4. 需要 trigger/success 时，链接适配本配置的 `src/libwolfboot.c` 与 HAL 或封装对应 API；包含相同 `wolfboot/wolfboot.h`/生成布局。别链接 Bootloader 的整个 `loader.c/update_flash.c` 进入业务 APP。
5. 下载完整签名镜像到 UPDATE；开始会话前处理旧状态，检查长度/Flash 返回值；写完再请求更新，安排 reset。不要从 APP 越界擦 Bootloader 或当前运行镜像。
6. 从新版健康检查完成处调用 success；RTOS 系统安排 watchdog、故障重启、时间窗口和需要保留的用户参数区。
7. 用自己的 APP 再走第 2 篇的认证、跳转、更新、回滚和中断测试，不沿用普通 demo 的测试结论。

可以把镜像地址、算法、头大小、版本作为发布 manifest 同时供两个工程消费。沿用上游 Bootloader Makefile、独立业务 APP、统一发布契约，是一个可维护的组合。
