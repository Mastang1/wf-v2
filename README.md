# wolfBoot 源码与上板实验包：探索者 / STM32F407ZGT6 假设

这是完整学习包，包含锁定的上游源码、可审查的板级修改、Ubuntu 构建与签名脚本、OpenOCD/GDB/VS Code 配置、中文教程与验证记录。**没有连接真实探索者板进行测试。** 请先看 [实际验证记录](evidence/VALIDATION.md)，不要把静态检查当成上板成功。

阅读顺序：

1. [硬件核对、Flash map、编译与签名](docs/01-build.md)
2. [烧录、Reset 到 APP 的调试及实验](docs/02-debug.md)
3. [真实岗位要求与核心问题](docs/03-requirements.md)
4. [从需求到静态架构、接口和数据设计](docs/04-static.md)
5. [启动、认证、交换、确认和恢复的动态设计](docs/05-dynamic.md)
6. [逐文件阅读导航与独立 APP 集成](docs/06-reading.md)
7. [来源、版本与许可证](docs/07-sources.md)

## 从现有 Ubuntu 环境开始

解压到无空格路径，例如 `~/projects/wolfboot-f407-lab`，在该目录执行。无需重新安装 ARM GCC、ST-Link、OpenOCD 或 VS Code。

```bash
bash scripts/build.sh
bash scripts/probe.sh
```

第一条在主机上构建签名工具，生成一次性学习密钥，再交叉编译 Bootloader 和测试 APP，输出 `build/`。第二条连接硬件读取型号/容量并暂停 CPU，不擦写 Flash。若工具缺失，脚本只报错，不自动安装。

**烧录会覆盖已有程序。** 完成第 1 篇的板型/晶振/电压核对并备份后，按第 2 篇执行烧录。不会设置 RDP、WRP、选项字节，也不使用全片擦除。工厂镜像覆盖 S0～S9；S10/S11 保留。首次写入前不应假设这些扇区内没有用户数据。

```bash
export BOARD_CONFIRMED=STM32F407ZGT6-1MiB-8MHz
bash scripts/flash.sh factory build/factory.bin
```

随后用 VS Code 打开本目录，选择 `wolfBoot: attach and reset (no download)`。默认不下载 ELF，避免覆盖带签名头的工厂镜像。

## 包内约定

- `.config` 与 `config/explorer-f407.config` 是同一学习配置；需要同时修改。不要对另一型号芯片复用烧录确认标识。
- `vendor/wolfBoot`、`vendor/wolfBoot-examples` 是 commit 固定的源码快照，不包含 `.git`。wolfssl 依赖按 wolfBoot 子模块 commit 固定；无关子模块未展开。
- `patches/overlay` 保留许可证和完整修改文件；对应 `.patch` 说明差异。`scripts/prepare.py` 校验原始/修改后哈希，再应用；遇到用户自行修改则拒绝覆盖。
- 普通 test-app 为主线；examples 中 STM32F4 measured-boot 是 TPM 扩展阅读，未接 TPM 时不构建。
- 本包不附生产私钥，也不附可直接用于产品的固件。`build.sh` 本地生成学习私钥，之后复用；重建不应换根密钥。
- Mermaid 图保存在 Markdown 中，使用支持 Mermaid 的预览器阅读；源码和操作步骤不依赖图形渲染。

本包在 2026-10-18 按检索到的最新 master 锁定 commit，**不是宣称该 commit 是稳定发行版**。产品采用前需要独立版本评审、安全更新评审和板上验证。
