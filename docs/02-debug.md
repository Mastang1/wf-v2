# 2　把“能编译”变成可观察的启动和更新

本篇是**待你在真实板卡执行的验收规程**。下面的现象是根据源码推导的预期，不是本包作者已经在探索者上看到的结果。先跑工厂版本，再测更新、确认和回滚，最后测中断恢复。不要同时改密钥、地址和串口。

## 先读、备份，再写

接好 SWDIO、SWCLK、GND、VTref；若用硬件复位，再接 NRST。板由可靠 3.3V 系统供电，避免 ST-Link 与板上电源相互顶电。拔插时检查接线。

```bash
bash scripts/probe.sh
```

保存 `build/logs/probe.log`。预期 `DBGMCU_IDCODE & 0xFFF == 0x413`，Flash-size 半字寄存器 `0x1FFF7A22` 为 `0x0400`（1024KiB）；不同 OpenOCD 的打印可能显示十六进制。这个 ID 匹配并不证明是探索者，也不证明外部晶振频率。

如原有固件有价值，先读取整片：

```bash
mkdir -p backup
bash scripts/openocd.sh -c 'init; reset halt; dump_image backup/before.bin 0x08000000 0x100000; shutdown'
sha256sum backup/before.bin >backup/before.sha256
```

有读保护而读取失败时，停止并确认用途；不要以“解锁”名义自动清 RDP，读保护解除可能导致擦除。不要配置 RDP Level 2。

本包 cfg 来源为上游 `config/openocd/openocd_stm32f4.cfg`，继续使用 OpenOCD 随附的 `interface/stlink.cfg`、`target/stm32f4x.cfg`，没有复制容易版本不兼容的内部目标脚本。保留原 reset-init 调试时钟位设置，移出原文件中的自动 `init/reset/halt`，交由命令行和 Cortex-Debug 调用。默认 `reset_config none`，无需 NRST；接有 NRST 且需复位下连接时，按 cfg 注释修改。旧 OpenOCD 若不认识 `stlink.cfg`，先查本地 `scripts/interface/`，不要随便把多个 ST-Link 接口文件叠加。

构建第 1 篇的产物后：

```bash
export BOARD_CONFIRMED=STM32F407ZGT6-1MiB-8MHz
bash scripts/flash.sh factory build/factory.bin
```

脚本每次写入前读取 ID 和容量，再执行有限地址范围的 `program ... verify`，最后 **reset halt**，不会直接运行。工厂镜像 0xC0000 字节覆盖 S0～S9，包括旧 BOOT/UPDATE 状态和 SWAP；不是只写几个非 FF 字节。任何既有数据在这些扇区都会失去。`BOARD_CONFIRMED` 是你的硬件核对记录，不是软件自动验证板型。

## 从硬复位走到 Bootloader

终端一：

```bash
bash scripts/openocd.sh
```

终端二，在包根目录：

```bash
arm-none-eabi-gdb -x gdb/boot.gdb
```

脚本只载符号、连接、复位暂停，**没有 GDB `load`**。`load build/image.elf` 的思路会只写原始 APP，遗漏或破坏签名头/状态，因此别用默认“编译后下载 APP”的调试模板。

按下面次序逐个临时硬件断点，不要一次设置十几个：

```gdb
x/2wx 0x08000000
info registers pc sp
x/i $pc
thbreak main
continue
thbreak wolfBoot_start
continue
```

`isr_reset` 在 `src/boot_arm.c`，负责复制 `.data`/RAM 函数、清零 `.bss`，随后调用 `main`。当前默认 ARM 构建在 `src/loader.c` 导出 **`main`**；`loader_main` 是 `WOLFBOOT_LOADER_MAIN` 宏的条件分支，不应凭教程标题就假定存在。查 `build/boot-symbols.txt` 或 `info functions loader_main`。`main` 调用 `hal_init()`，再进入 `wolfBoot_start()`。

| 停止位置 | 看什么 | 预期与含义 |
|---|---|---|
| isr_reset | `x/2wx 0x08000000`、SP、PC | 第一个字是栈顶，第二个字是带 Thumb 位的入口；PC 在 Bootloader 地址范围 |
| main | `x/wx 0x40023800` | 尚未执行/正在执行 HAL 时钟初始化；卡 HSE/PLL 就在这里逐步定位 |
| wolfBoot_start | `bootState`、`updateState`（执行赋值之后） | 擦净工厂状态下 `get_partition_state` 可能返回 -1，表示 trailer magic 未建立；不等同于 APP 认证失败 |
| wolfBoot_open_image 返回后 | `boot.hdr`、`fw_base`、`fw_size` | 头 0x08020000，APP 向量表 0x08020100，长度与原始 APP 一致 |
| wolfBoot_verify_integrity | 返回值、`sha_ok` | 成功后为 1；它验证内容摘要 |
| wolfBoot_verify_authenticity | 返回值、`signature_ok` | 成功后为 1；公钥来自编译进去的 keystore |
| do_boot | 参数、MSP、VTOR、入口 | 准备切换向量表/栈并跳入 APP |

在认证函数入口用 `finish` 看返回值；局部变量在调用前或优化后可能不可用，不要把 `<optimized out>` 当程序错误。DEBUG=1 仍不保证所有宏/内联函数有独立符号。

## 把 APP 的入口接到眼前

```gdb
thbreak do_boot
continue
x/2wx 0x08020100
x/wx 0xE000ED08
```

`do_boot` 在 `src/boot_arm.c` 的 Armv6/v7 分支：关闭 MPU，按配置处理 IRQ/VTOR，从 `app_offset[0]` 取 MSP，从 `[1]` 取入口，设置 MSP 后跳转。**目标是向量表，不是直接把 PC 设成 0x08020100**；该地址首字是数据（栈顶），不是指令。向量表第二字要带 Thumb 位。

停在 `do_boot` 后切换符号：

```gdb
source gdb/app.gdb
continue
thbreak main
continue
x/wx 0xE000ED08
info registers sp pc
```

`symbol-file` 替换符号不写 Flash。APP 自己也有 `isr_reset` 和 `main`，用两套 ELF 分阶段切换，避免同名符号混淆。若你用 `add-symbol-file` 同时加载，应按 APP ELF 的真实 section 地址处理，且用源码路径限定同名断点；初学阶段替换更直接。

APP main 做时钟、计时、UART 初始化，后在 UART 收包循环中阻塞。继续运行片刻再暂停：

```gdb
p time_elapsed
p/x (unsigned int)wolfBoot_current_firmware_version()
```

`wolfBoot_current_firmware_version` 是宏，GDB 通常不能直接调用这个宏。实际应使用：

```gdb
p/x (unsigned int)wolfBoot_get_image_version(0)
```

此处 `PART_BOOT=0`，见本 commit 的 `include/wolfboot/wolfboot.h`。目标函数调用要在 APP 已完成初始化、Flash 不在忙写时做。最安全的无执行观察方式是读取镜像头，或让 APP 正常路径记录版本到变量。`time_elapsed` 连续运行后应增长；本包没有点亮板载 LED。

APP 串口默认 115200、8N1、PA9/PA10。官方示例发送的是二进制 `'*'` 加四字节版本，不是整段可读日志；只看到 `*` 或少数字符不能判断为乱码。Bootloader 本包 `DEBUG_UART=0`，其无串口输出是配置选择，使用断点观察。

## VS Code 使用同一条调试链

需要已安装 Cortex-Debug 扩展（推荐列表见 `.vscode/extensions.json`）。打开**解压根目录**，任务 `Build and sign (no flash)` 只构建；`Probe chip (read only)` 只探测。

1. 停止终端 OpenOCD，再选 `wolfBoot: attach and reset (no download)`；扩展会自行启动 OpenOCD。不要两个进程抢 ST-Link。
2. 启动后 CPU 在 reset halt，手动在 `vendor/wolfBoot/src/boot_arm.c` 的 `isr_reset`、`src/loader.c` 的 `main` 设置断点后继续。
3. 到 `do_boot` 时，在 Debug Console 执行 `symbol-file build/image.elf`，再给 APP `isr_reset`/`main` 断点。若扩展的反汇编缓存仍沿用 Bootloader，断开后换第二个 APP attach 配置；它不复位，也不烧录。
4. 第一个配置每次 attach 会复位，第二个配置适合观察已经运行的 APP。两者都不使用 ELF 下载。

GDB 命令在 Cortex-Debug Console 中一般可直接执行；若扩展要求命令前缀，使用 `-exec`，例如 `-exec monitor reset halt`。先以终端 GDB 走通是最易排除 IDE 配置问题的办法。

## 三个版本做升级、确认和回滚

先让 v1 完成启动。官方普通 APP 只对**偶数版本**自动调用 `wolfBoot_success()`；奇数版保留未确认行为。这是测试策略，不是产品健康检查。v1 工厂镜像没有 TESTING trailer，因此不会仅因版本为奇数就自动回滚。

### v1 → v2 → 成功确认

暂停 APP，先断开 GDB/OpenOCD 防止占用探针，再写候选分区：

```bash
bash scripts/flash.sh update build/image_v2_signed.bin
```

写 UPDATE 只写候选镜像，不代表请求升级。脚本结束在 reset halt，BOOT 仍然是 v1。重新启动调试连接，切 APP 符号，继续让 v1 到正常接收循环。暂停后：

```gdb
source gdb/trigger.gdb
symbol-file build/wolfboot.elf
thbreak wolfBoot_start
continue
thbreak wolfBoot_update
continue
```

`wolfBoot_update_trigger()` 会清 UPDATE 状态扇区并标记 UPDATING，`monitor reset halt` 随后让 Bootloader 重启。静态函数 `wolfBoot_update` 在 DEBUG 构建通常可设断点；若编译器内联了它，直接在 `src/update_flash.c` 对应语句下断。

认证通过后进行 SWAP，BOOT 变 v2，状态 TESTING，UPDATE 留下旧 v1。到 APP v2 后，初始化完成且时间计数跨过第一次等待，它会因偶数版本调用 `wolfBoot_success()`。设置该函数断点可看到状态确认，随后复位应继续 v2。

### v2 → v3 → 不确认回滚

同理把 v3 写入 UPDATE，回到 v2 APP 调 `wolfBoot_update_trigger()` 并复位。v3 会启动但因奇数版本不确认。**在 v3 正常运行后再复位**，Bootloader 看到 BOOT=TESTING，调用允许 fallback 的更新路径，将旧 v2 恢复到 BOOT。它不是按计时器自动判断超时；没有 watchdog/用户复位，挂住的 APP 也不会凭空重启。

生产 APP 应在必要自检和关键服务恢复后主动确认，不以版本奇偶为策略。看门狗、复位原因和最大试启动时间属于产品集成责任。

### 中断恢复

从确认的 v2 开始，重新放 v3 并 trigger。通过源码断点按下表取样，每次先恢复相同基线，不要在已经回滚的状态上盲测。

| 注入点 | 操作 | 下一次启动重点 |
|---|---|---|
| UPDATE→SWAP 拷贝后、写 SWAPPING 前 | `monitor reset halt` | 状态仍 NEW，重复该步应安全 |
| SWAPPING 已持久化 | reset | SWAP 持有新块，BOOT 旧块仍可备份 |
| BOOT→UPDATE 后、BACKUP 已持久化 | reset | 用 SWAP 补写 BOOT，不能再误读 UPDATE 为完整新镜像 |
| BOOT 新块完成、UPDATED 写入后 | reset | 跳过已完成块 |
| `wolfBoot_swap_and_final_erase` 的 FINAL_FLAGS 阶段 | reset | 必须观察尾部状态重建路径，不能只测试主循环 |

调试器 reset 只模拟控制流被打断，不等同于真正断电时 Flash 电荷/电压中断。真实断电验证需覆盖擦除中、编程中、写标记前后、重复中断和不稳定供电；记录每次断电相位、复位原因、镜像哈希和最后启动版本。没有这组硬件证据，不能宣称断电安全认证通过。

## 错误镜像的两个不同实验

先从已确认的 v2 做实验，不覆盖 Bootloader。

1. **内容损坏**：复制 v3 签名镜像，修改 256B 头之后的任意一个 payload 字节，写 UPDATE，再从 v2 trigger。预期 integrity 失败，不交换，旧 v2 仍能启动。
2. **真正的错误密钥签名**：在另一个临时目录中运行官方 keygen 生成第二把学习密钥，再用它签同一 `image.bin` 的版本 4。原 Bootloader 不重建、不替换原 keystore。候选中的 Hash 可以正确，但公钥 hint 找不到或签名校验失败。不要在原 wolfBoot 根目录运行第二次 keygen，避免覆盖 trust root。

```bash
mkdir -p work/wrong-key/src
cd work/wrong-key
../../vendor/wolfBoot/tools/keytools/keygen --ed25519 -g wrong.der
../../vendor/wolfBoot/tools/keytools/sign --ed25519 --sha256 ../../build/image.bin wrong.der 4
cd ../..
mv build/image_v4_signed.bin build/wrong_key_v4.bin
bash scripts/flash.sh update build/wrong_key_v4.bin
```

keygen 默认 `src/keystore.c` 在当前目录，所以临时目录需有 `src/`。sign 输出文件随输入 APP 的路径，故得到 `build/image_v4_signed.bin`。实验后删掉第二把学习私钥前确认它只是实验文件。

错误 UPDATE 被拒绝不代表设备拒绝所有启动：仍有效的 BOOT 应继续启动。要观察“没有任何合法镜像时拒绝跳转”，在已有备份与恢复手段的实验板上损坏 BOOT 且清空候选，预期进入 `wolfBoot_panic()`；这不是本教程默认执行的烧录步骤。

## 常见故障按证据排查

| 症状 | 首先核对 | 下一步 |
|---|---|---|
| 找不到 ST-Link | USB 权限、线、供电、同一探针是否被别的程序占用 | 先单独 OpenOCD 探测，必要时按 Ubuntu 现有 udev 配置处理 |
| reset/连接不稳定 | NRST 是否真实接好、SWD 线长度、接口速率 | adapter 降到 100～400kHz，接 NRST 后按 cfg 启用复位下连接 |
| 卡在 hal_init | HSERDY、PLLRDY、VDD、HSE 频率 | 不要直接跳过等待；查板原理图与 RCC 寄存器 |
| 到 APP 后 HardFault | APP 链接地址、向量表、MSP、VTOR、残留 IRQ | 看 SCB CFSR/HFSR/BFAR、反汇编入口；不要把分区起点当代码起点 |
| APP 串口无输出 | PA9/PA10 跳帽、地、电平、115200、PCLK2 | 先用 GDB 确认已到接收循环；看 BRR，不能用 SYSCLK 直接算 UART |
| 固件已写但不更新 | 是否写在 0x08060000、是否含头、是否 trigger | 在 wolfBoot_start 检查 UPDATE trailer 与返回值 |
| 新版本总被拒 | 容量、版本、签名算法、密钥、头大小 | 比对 keystore 和版本清单；当前版本禁止正常路径同版本/降级 |
| 更新一次后下次变旧 | 新版是否 TESTING、是否 success、是否意外复位 | 本例奇数版特意不确认；区分正常回滚与下载错误 |
| 断点打不上 | 载入了哪套 ELF、符号是否存在、硬件断点数 | 分阶段 `thbreak`；优化后的静态函数改用源码行 |
| GDB call 失败 | 是否有函数符号、栈是否健康、CPU 是否 Flash 忙 | 让 APP 正常路径执行 API；不要手写不明 trailer 偏移代替 API |

F407 内部 Flash 是本实验使用的单 Bank 几何。擦写同 Bank 会影响从 Flash 取指/中断响应，即使 HAL 的部分函数被放 RAM，也不代表所有中断、常量、调用链都可同时从 Flash 读取。下载期间评估实时性与 watchdog；本示例不是实时控制产品的完整时序保证。
