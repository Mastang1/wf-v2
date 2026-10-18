# 板级适配

原始快照保持在 vendor。scripts/prepare.py 按 manifest.json 核对哈希后应用 overlay，不覆盖未知编辑。各 .patch 便于人工审查。5 个修改文件保留原有上游版权与许可证；改动原因见 docs/07-sources.md。

没有修改 update_flash.c 的交换算法；没有声称这些板级修改已在探索者实测。普通 APP 的 UART 下载 demo 仍有产品化局限（超时、重试会话、Flash 错误传播等），实操主线使用 OpenOCD 写候选分区并通过 APP API 触发。
