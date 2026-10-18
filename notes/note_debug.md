# VS Code + Cortex-Debug + OpenOCD 调试步骤笔记

> 目标芯片：STM32F407ZGT6（探索者假设，1MiB Flash / 8MHz HSE / 3.3V）。
> 调试链：VS Code + `marus25.cortex-debug` 插件 + OpenOCD 0.12.0（ST-Link + stm32f4x）+ Arm GNU Toolchain（`arm-none-eabi-gcc 14.3` / `gdb 15.2`）。
> 配套文件：`.vscode/launch.json`、`.vscode/tasks.json`、`.vscode/extensions.json`、`config/openocd-explorer.cfg`、`config/STM32F407.svd`、`gdb/*.gdb`、`scripts/*.sh`。

---

## 0. 工具链与文件就绪检查（一次性）

| 检查项 | 命令 | 预期 |
|--------|------|------|
| Arm GCC | `arm-none-eabi-gcc --version` | 14.3.1，在 `/opt/arm-gnu-toolchain/bin` |
| Arm GDB | `arm-none-eabi-gdb --version` | 15.2.90，在 PATH 中 |
| OpenOCD | `openocd --version` | 0.12.0，`/usr/bin/openocd` |
| SVD | `ls config/STM32F407.svd` | 存在，`<name>STM32F407</name>` |
| OpenOCD 脚本 | `ls /usr/share/openocd/scripts/interface/stlink.cfg /usr/share/openocd/scripts/target/stm32f4x.cfg` | 两者都在 |
| 调试产物 | `ls build/wolfboot.elf build/image.elf` | 都在，`file` 显示 `with debug_info, not stripped` |

VS Code 装插件：`marus25.cortex-debug`（`extensions.json` 已推荐）+ `ms-vscode.cpptools`。

---

## 1. 接线与硬件核对（写之前先读）

接线：`SWDIO / SWCLK / GND / VTref`；需要硬件复位时再接 `NRST`。探索者由 3.3V 系统供电，避免 ST-Link 与板上电源互相顶电。

```bash
bash scripts/probe.sh
```

只读：核对 `DBGMCU_IDCODE & 0xFFF == 0x413`、Flash-size 寄存器 `0x1FFF7A22 == 0x0400`（1024KiB）。结果存 `build/logs/probe.log`，不擦写 Flash。

如原有固件有价值，先备份整片：

```bash
mkdir -p backup
bash scripts/openocd.sh -c 'init; reset halt; dump_image backup/before.bin 0x08000000 0x100000; shutdown'
sha256sum backup/before.bin > backup/before.sha256
```

> 不要以"解锁"名义自动清 RDP；读保护解除可能导致擦除，也不要配置 RDP Level 2。

---

## 2. 构建与烧录（调试前必做）

```bash
bash scripts/build.sh
```

产物：`build/wolfboot.elf`（带符号）、`build/image.elf`（带符号）、`build/wolfboot.bin`、`build/image.bin`、`build/factory.bin`、`build/image_v1..v3_signed.bin`、`build/boot-symbols.txt`、`build/app-symbols.txt`。

烧录工厂镜像：

```bash
export BOARD_CONFIRMED=STM32F407ZGT6-1MiB-8MHz
bash scripts/flash.sh factory build/factory.bin
```

`flash.sh` 每次写入前重读 ID + 容量，再做有限地址范围 `program ... verify`，最后 `reset halt`（不直接运行）。工厂镜像覆盖 S0~S9，这些扇区原有数据全部丢失。`BOARD_CONFIRMED` 是你的硬件核对记录，不是软件自动校验板型。

---

## 3. launch.json 的两个调试配置（已就绪）

两个配置都是 `cortex-debug` + `servertype: openocd` + `gdbPath: arm-none-eabi-gdb`，都带 `svdFile: ${workspaceFolder}/config/STM32F407.svd`。

| 配置名 | request | executable | reset 行为 | 用途 |
|--------|---------|------------|------------|------|
| `wolfBoot: attach and reset (no download)` | attach | `build/wolfboot.elf` | attach 后 `monitor reset halt` | 调 Bootloader 启动阶段（复位 → `do_boot`） |
| `APP: attach existing running image (no reset)` | attach | `build/image.elf` | **不复位** | 调 Bootloader 跳转后已在运行的 APP |

> ⚠️ Cortex-Debug 会**自己**拉起 OpenOCD（GDB 端口 3333）。**不要同时手动跑 `scripts/openocd.sh`**，否则端口冲突。
> ⚠️ 两个配置都是 `attach`，**不要用 GDB `load`**——`load build/image.elf` 只写裸 APP，会破坏签名头/状态。固件靠 `flash.sh` 写。

## 4. 阶段 A —— 调试 Bootloader 启动

1. VS Code 左侧「运行和调试」面板，下拉选 **`wolfBoot: attach and reset (no download)`**。
2. 按 **F5**。Cortex-Debug 拉起 OpenOCD → `arm-none-eabi-gdb` 连 3333 → `monitor reset halt` → 载入 `build/wolfboot.elf` 符号。
3. 逐个下临时断点（Cortex-M4 只有 **6 个硬件断点**，别一次堆十几个；`thbreak` 用完即消）：

   | 停止点 | 源文件 | 看什么 |
   |--------|--------|--------|
   | `isr_reset` | `vendor/wolfBoot/src/boot_arm.c` | `x/2wx 0x08000000`、SP、PC；首字是栈顶，次字带 Thumb 位 |
   | `main` | `vendor/wolfBoot/src/loader.c` | `x/wx 0x40023800`（RCC CR）；卡 HSE/PLL 在此定位 |
   | `wolfBoot_start` | `vendor/wolfBoot/src/loader.c` | `bootState`、`updateState`；擦净工厂状态可能返回 -1 |
   | `wolfBoot_open_image` 返回后 | `vendor/wolfBoot/src/loader.c` | `boot.hdr`、`fw_base`、`fw_size`；头 0x08020000 |
   | `wolfBoot_verify_integrity` | `vendor/wolfBoot/src/loader.c` | `finish` 看返回值；成功为 1 |
   | `wolfBoot_verify_authenticity` | `vendor/wolfBoot/src/loader.c` | 同上；公钥来自编译进去的 keystore |
   | `do_boot` | `vendor/wolfBoot/src/boot_arm.c` | 关 MPU、设 MSP、跳向量表前最后一步 |

4. 打开左侧 **PERIPHERALS（外设寄存器）视图**：已带 `config/STM32F407.svd`，可实时看/改 RCC、GPIO、Flash、USART 等寄存器（卡 `hal_init` 时看 `RCC->CR` 的 `HSERDY/PLLRDY`）。
5. 认证函数入口用 `finish` 看返回值；局部变量可能 `<optimized out>`，不是程序错误（`DEBUG=1` 仍不保证所有内联/宏有独立符号）。
6. 停在 `do_boot` 后，**继续 (F5) 让 Bootloader 跳进 APP**。

---

## 5. 阶段 B —— 调试已运行的 APP

> 这一步**不复位**，保留 Bootloader 已设好的时钟/UART 状态，直接 attach 到已在 APP 收包循环里的目标。

1. 阶段 A 让 Bootloader 跳进 APP，APP 跑完时钟/计时/UART 初始化，进入 UART 收包循环。
2. **停止**阶段 A 的调试会话（红框停止按钮）——Cortex-Debug 关闭它自己拉起的 OpenOCD，板子保持运行（不复位）。
3. 下拉切到 **`APP: attach existing running image (no reset)`**，按 **F5**。重新拉起 OpenOCD、用 `build/image.elf` 载符号、**不做 reset**，attach 到正在跑的 APP。
4. 在 APP 源码下断点：`isr_reset` / `main`（已跑过则断在循环或 `time_elapsed`）；`p time_elapsed` 看计时。
5. 触发升级：在 APP 收包循环里暂停后，在 Debug Console 执行 `call (void)wolfBoot_update_trigger()`（等价 `gdb/trigger.gdb` 的 `call`），再 `monitor reset halt` 让 Bootloader 重启处理 UPDATE；此时切回 **wolfBoot 配置**、载 `build/wolfboot.elf` 符号，在 `wolfBoot_start` / `wolfBoot_update` 下断点继续调。

---

## 6. 符号切换的注意事项

- 终端手动流程（`docs/02-debug.md`）在**同一个 GDB 会话**里用 `symbol-file build/image.elf` 切符号；
- VS Code/Cortex-Debug 每个 launch 配置绑定**一个 `executable`**，切 APP 符号 = 切到第二个配置重启调试会话。两套等价，别在 APP 配置里硬 `symbol-file` wolfboot 符号。
- Bootloader 与 APP 都有 `isr_reset`、`main` 同名符号——靠分阶段用不同 ELF 区分（配置名带 `wolfBoot` / `APP` 前缀就是这个意思）。
- 静态函数可能被内联，断点打不上就改在 `src/*.c` 对应**源码行**下断。

## 7. 一句话流程

```
build.sh → probe.sh → flash.sh factory
  → F5 选 "wolfBoot: attach and reset" 调 Bootloader 到 do_boot
  → 继续让它跳进 APP → 停会话
  → F5 选 "APP: attach (no reset)" 调 APP
  → 需要看更新流程再切回 wolfBoot 配置
```

---

## 8. 常见故障速查

| 症状 | 首先核对 | 下一步 |
|------|---------|--------|
| 找不到 ST-Link | USB 权限、线、供电、是否被别的程序占用 | 单独 OpenOCD 探测；按 udev 配置处理 |
| reset/连接不稳 | NRST、SWD 线长、接口速率 | `adapter speed` 降到 100~400kHz；接 NRST 后按 cfg 注释启用复位下连接 |
| 断点跳转源文件打不开 | **ELF 里 DWARF 调试路径是编译时的绝对路径，项目移动后失效**（见第 9 节） | 重新 `build.sh`，或配 `sourceFileMap` / GDB `substitute-path` |
| 卡在 hal_init | HSERDY、PLLRDY、VDD、HSE 频率 | 查 RCC 寄存器；别直接跳过等待 |
| 到 APP 后 HardFault | APP 链接地址、向量表、MSP、VTOR、残留 IRQ | 看 SCB CFSR/HFSR/BFAR；别把分区起点当代码起点 |
| APP 串口无输出 | PA9/PA10 跳帽、电平、115200、PCLK2 | 先 GDB 确认已到接收循环；看 BRR，不能用 SYSCLK 算 UART |
| 固件已写但不更新 | 是否写在 0x08060000、是否含头、是否 trigger | 在 `wolfBoot_start` 检查 UPDATE trailer 与返回值 |
| 断点打不上 | 载入了哪套 ELF、符号是否存在、硬件断点数 | 分阶段 `thbreak`；内联函数改用源码行 |
| GDB `call` 失败 | 是否有函数符号、栈是否健康、CPU 是否 Flash 忙 | 让 APP 正常路径执行 API；别手写不明 trailer 偏移 |

---

## 9. 重点：调试时"跳转的源文件打不开"的原因与修复

> **状态（2026-10-19）：已修复。** 清掉 `vendor/wolfBoot/tools/keytools/` 下残留的 stale `.d/.o` 后重跑 `bash scripts/build.sh`，构建通过；`build/*.elf` 的 DWARF 路径已更新为当前 `wb_pro_2` 路径，`strings build/wolfboot.elf | grep -c outputs/wolfboot-f407-lab` 返回 0。下面保留诊断与方案作为知识记录。

### 现象
在 VS Code 调试里命中断点后，Cortex-Debug 提示找不到源文件 / 路径为空 / 打开的是空白页，源码无法定位。

### 根因
`build/wolfboot.elf`、`build/image.elf` 里的 **DWARF 调试信息（`DW_AT_comp_dir` + `DW_AT_name`）记录的是"编译时所在的绝对路径"**，不是相对路径。

实测当前 ELF 里嵌入的路径前缀是：

```
/home/niki/workspace/open_src/bootloaders/wfbl-notes/outputs/wolfboot-f407-lab/vendor/wolfBoot/...
```

而项目**现在的实际位置**是：

```
/home/niki/workspace/open_src/bootloaders/wfbl-notes/wb_pro_2/wolfboot-f407-lab/vendor/wolfBoot/...
```

两段路径中间从 `outputs` 变成了 `wb_pro_2`——项目被移动/重命名过，但 `build/*.elf` 没有重新编译。Cortex-Debug/GDB 按记录的绝对路径去 `.../outputs/.../loader.c` 找文件，那个位置现在不存在，于是源文件打不开。

> 验证命令：`strings build/wolfboot.elf | grep -E '^/(home|root)' | sort -u`
> 会看到 ELF 嵌入的 `.../outputs/wolfboot-f407-lab/vendor/wolfBoot/...` 路径，和当前工作目录对不上。

### 修复方案（按推荐度排序）

1. **重新编译（推荐，一劳永逸）**：在当前 `wb_pro_2/wolfboot-f407-lab` 目录重新跑 `bash scripts/build.sh`，让 DWARF 重新记录当前绝对路径。`build.sh` 已有保护：已存在的学习私钥 `wolfboot_signing_private_key.der` 不会重新生成（trust root 不轮换）。重新编译后 `build/*.elf` 的调试路径就和当前路径一致，断点跳转即可正常打开源文件。

2. **不改 ELF，加路径映射（不想重新编译时）**：在 `launch.json` 两个配置里加 `sourceFileMap`，把旧路径段映射到当前路径段：

   ```jsonc
   "sourceFileMap": {
     "/home/niki/workspace/open_src/bootloaders/wfbl-notes/outputs/wolfboot-f407-lab":
     "/home/niki/workspace/open_src/bootloaders/wfbl-notes/wb_pro_2/wolfboot-f407-lab"
   }
   ```

   等价的 GDB 命令是：`set substitute-path .../outputs/wolfboot-f407-lab .../wb_pro_2/wolfboot-f407-lab`。

   > 缺点：映射写死绝对路径，换机器/换目录又得改；不如重新编译干净。

### 预防
- 以后移动/重命名项目目录后，**先重新 `build.sh` 再调试**。
- 或在 `scripts/build.sh` 开头加一行校验：若当前路径与 ELF 记录的 comp_dir 不一致则提示重编译。