# wolfBoot 分区状态分支全景笔记

> 配套 `notes/note-partition.md`（分区/字段定义）、`notes/note_source_code.md`（源码流程）。本文聚焦**状态流转**：每个 `IMG_STATE_*` 是谁写的、`wolfBoot_start` 据此走哪条分支、各核心场景（含失败/回滚/断电恢复）的完整路径。
> 配置：STM32F407 单 Bank NOR、双分区 + SWAP、`WOLFBOOT_SECTOR_SIZE=128KiB`、`PART_BOOT@0x08020000`、`PART_UPDATE@0x08060000`、`PART_SWAP@0x080A0000`。
> 行号对应 `vendor/wolfBoot` 本快照源码。

---

## 第 1 章　分区状态（PART_FLAG）总览

> 字段位置见 `note-partition.md` 第 3.2 节：每个分区末尾 trailer 偏移 1 处的 1 字节。默认极性（`wolfboot.h:645-657`）。

| 状态 | 值 | 含义 | 写入者 | 读取者 |
|------|----|------|--------|--------|
| `IMG_STATE_NEW` | `0xFF` | 擦净态/无有效 trailer（NOR 擦后即是） | 擦除尾扇区 / 从未写 | `wolfBoot_get_partition_state` 返回此值或 -1 |
| `IMG_STATE_UPDATING` | `0x70` | APP 已请求升级 | **APP** 调 `wolfBoot_update_trigger`（`libwolfboot.c:893`） / **BL** 回滚前补写 | `wolfBoot_start` ②分支 |
| `IMG_STATE_FINAL_FLAGS` | `0x30` | 正在收尾最终擦除 | **BL** `swap_and_final_erase`（`update_flash.c:559`） | `swap_and_final_erase(1)` 判重入 |
| `IMG_STATE_TESTING` | `0x10` | BOOT 刚换完待 APP 确认 | **BL** `swap_and_final_erase`（`:629`）/ `wolfBoot_update` 收尾 | `wolfBoot_start` ①分支 |
| `IMG_STATE_SUCCESS` | `0x00` | APP 已确认，稳定运行 | **APP** `wolfBoot_success`（`libwolfboot.c:964`） / **BL** 回滚后直接认定 | `wolfBoot_start` ③分支（非 TESTING） |

**NOR 物理约束**：只能 `1→0`，擦除才回 `1`。故状态值按 `0xFF → 0x70 → 0x30 → 0x10 → 0x00` 单调"清位"前进；回 `NEW(0xFF)` 必须擦整个状态扇区（128KiB）。这就是 trigger/success/final_erase 都要 `hal_flash_erase(尾扇区)` 的根因。

**扇区进度 nibble（`SECT_FLAG_*`，仅 UPDATE 用）**：逐块对换时 `NEW(0x0F)→SWAPPING(0x07)→BACKUP(0x03)→UPDATED(0x00)`，断电后据此续传（`image.h:46-54`，`update_flash.c:1292`）。

**镜像头状态（独立于 PART_FLAG）**：`hdr_ok`/`sha_ok`/`signature_ok` 三道门，任一失败 → 紧急回滚。

---

## 第 2 章　分区状态转移总图

```mermaid
stateDiagram-v2
    direction LR
    [*] --> NEW : 擦净 / 首启无 trailer

    state "BOOT: NEW/SUCCESS\nUPDATE: NEW" as Stable {
        [*] --> 走 wolfBoot_start ③\n直接认证 BOOT → do_boot
    }

    NEW --> UPDATING : APP: wolfBoot_update_trigger\n(写 UPDATE 尾扇区)

    state "UPDATE: UPDATING" as Upd1
    Upd1 --> wolfBoot_update_0 : 复位进 wolfBoot_start ②\nfallback_allowed=0 拒同版/降级

    state "逐块对换中" as Swapping {
        [*] --> SECT_FLAG_NEW
        SECT_FLAG_NEW --> SECT_FLAG_SWAPPING : BOOT→SWAP 备份
        SECT_FLAG_SWAPPING --> SECT_FLAG_BACKUP : UPDATE→BOOT 新块就位
        SECT_FLAG_BACKUP --> SECT_FLAG_UPDATED : SWAP→UPDATE 旧块挪走
        note right of Swapping
            每块写完立刻标 nibble
            断电后看 nibble 续传
        end note
    }
    wolfBoot_update_0 --> Swapping
    Swapping --> FINAL_FLAGS : swap_and_final_erase(0)\n备份 staging sector 到 SWAP\nUPDATE: → FINAL_FLAGS

    state "最终擦除中" as FinalErase
    FINAL_FLAGS --> FinalErase : 擦 BOOT 尾扇区\nBOOT: → TESTING\n擦 UPDATE 尾扇区 → UPDATE: NEW
    note right of FinalErase
        staging sector 写 MAGIC_TRAIL
        断电后 swap_and_final_erase(1) 重入
    end note

    FinalErase --> Testing : BOOT=TESTING, UPDATE=NEW

    state "BOOT: TESTING" as Test1
    Test1 --> do_boot_APP_new : wolfBoot_start ③ 认证 BOOT\n(此时 UPDATE 已 NEW, ③不命中①②)
    do_boot_APP_new --> APP_runs
    state "APP 运行新镜像" as APP_runs
    APP_runs --> SUCCESS : APP 调 wolfBoot_success\n(业务自检通过)
    APP_runs --> 断电或看门狗 : APP 未调 success
    断电或看门狗 --> Rollback : 复位进 wolfBoot_start ①\nBOOT==TESTING → wolfBoot_update(1)\nfallback_allowed=1 允许旧版

    state "回滚中" as Rollback
    Rollback --> Swapping2 : 用 UPDATE 里的旧版恢复 BOOT\n(同一套逐块对换机制)
    Swapping2 --> FinalErase2 : swap_and_final_erase
    FinalErase2 --> SUCCESS : rollback_needed 时直接\nBOOT: → SUCCESS\n(认定回滚后稳定, 不再 TESTING)

    SUCCESS --> Stable
```

> 图中 "BL" = Bootloader，"APP" = 应用固件。`wolfBoot_update_0`/`wolfBoot_update_1` 是同一函数 `wolfBoot_update(fallback_allowed)` 的两种调用（`update_flash.c:1046`），区别仅在是否跳过"upd_ver > cur_ver"版本检查。

---

## 第 3 章　`wolfBoot_start` 入口分支决策

> 源码 `update_flash.c:1697`。本配置走 `双分区+SWAP` 分支（无 `SELF_UPDATE_MONOLITHIC`/`DISABLE_BACKUP`/`CUSTOM_PARTITION_TRAILER`）。

```mermaid
flowchart TD
    A[wolfBoot_start] --> B[读 BOOT/UPDATE 的 PART_FLAG<br/>bootState, updateState<br/>bootRet, updateRet]
    B --> C[swap_and_final_erase 1<br/>恢复中断的最终擦除<br/>resumedFinalErase]
    C --> D{resumedFinalErase!=0<br/>或 BOOT==TESTING?}
    D -- 否 --> M[③ 直接认证 BOOT]
    D -- 是 --> E{BOOT==TESTING?}
    E -- 是 --> F[① 回滚 wolfBoot_update 1<br/>fallback_allowed=1 允许旧版<br/>UPDATE trailer 无效则先置 UPDATING]
    E -- 否 --> G{UPDATE==UPDATING?}
    G -- 是 --> H[② 正常升级 wolfBoot_update 0<br/>fallback_allowed=0<br/>拒同版/降级 无 ALLOW_DOWNGRADE]
    G -- 否 --> M
    F --> M
    H --> M
    M --> I[wolfBoot_open_image BOOT<br/>解析头 MAGIC+TLV]
    I --> J[wolfBoot_verify_integrity<br/>重算 SHA256 比 HDR_SHA256]
    J --> K[wolfBoot_verify_authenticity<br/>HDR_PUBKEY hint 查 keystore<br/>Ed25519 验 HDR_SIGNATURE]
    K --> L{校验通过?}
    L -- 否 --> N[紧急回滚 wolfBoot_update 1]
    N --> O{回滚成功?}
    O -- 否 --> P[wolfBoot_panic<br/>无可引导项 → 死]
    O -- 是 --> Q[重新 open+integrity+authenticity BOOT<br/>BOOT 内容已被回滚改写]
    Q --> R{再次校验通过?}
    R -- 否 --> P
    R -- 是 --> S
    L -- 是 --> S[hal_prepare_boot<br/>do_boot fw_base 0x08020100]
    S --> T([跳进 APP])
```

**三个入口分支判据**：
1. **① BOOT==TESTING → 回滚**：上次 BOOT 换了新镜像但 APP 没确认就复位 → 回到 UPDATE 留的旧版。
2. **② UPDATE==UPDATING → 正常升级**：APP 已 trigger → 校验 UPDATE 通过后逐块对换进 BOOT。
3. **③ 都不是 → 直接认证 BOOT**：系统稳定，UPDATE 是 NEW/空。

> 外层 `resumedFinalErase!=0` 守卫：即使刚恢复完中断、BOOT 已被收尾设成 TESTING 但 UPDATE 已清成 NEW，也进块；块内 ①②都不命中就静默 fall-through 到 ③。

---

## 第 4 章　核心场景流程图

### 4.1 场景一：首启 / 稳定运行（BOOT=SUCCESS, UPDATE=NEW）

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant G as get_partition_state
    participant V as verify_**
    participant B as do_boot
    participant APP as APP

    R->>S: reset
    S->>G: get_partition_state(PART_BOOT, &b)
    G-->>S: bootRet=0, bootState=SUCCESS
    S->>G: get_partition_state(PART_UPDATE, &u)
    G-->>S: updateRet=0, updateState=NEW
    S->>S: swap_and_final_erase(1) → -1 (无中断)
    Note over S: resumedFinalErase==0 且 BOOT!=TESTING<br/>① ② 都不命中 → ③ 直接认证
    S->>V: open_image(BOOT) → integrity → authenticity
    V-->>S: 全通过
    S->>B: do_boot(boot.fw_base)
    B->>APP: 跳进 APP @ 0x08020100
    Note over APP: 业务自检通过 → wolfBoot_success()<br/>BOOT 保持 SUCCESS (无变化)
```

### 4.2 场景二：APP 触发升级（trigger，不执行升级）

```mermaid
sequenceDiagram
    autonumber
    participant APP as APP v1 (BOOT=SUCCESS)
    participant TR as wolfBoot_update_trigger<br/>libwolfboot.c:893
    participant F as Flash UPDATE 尾扇区

    APP->>APP: UART 收包 → 逐页写 UPDATE@0x08060000
    Note over APP: 收齐后 (next_seq>=tot_len)
    APP->>TR: wolfBoot_update_trigger()
    TR->>F: hal_flash_unlock + erase UPDATE 尾扇区
    TR->>F: set_partition_state(PART_UPDATE, IMG_STATE_UPDATING)
    TR->>F: hal_flash_lock
    TR-->>APP: 返回
    Note over APP: UPDATE: NEW → UPDATING<br/>trigger 只标记, 不校验 UPDATE 内容<br/>真正升级在下次复位 BL 里
    APP->>APP: while(1) 等复位 (watchdog/手动)
```

### 4.3 场景三：正常升级 v1→v2 并确认（核心成功路径）

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant U as wolfBoot_update 0
    participant FE as swap_and_final_erase 0
    participant F as Flash
    participant APP as APP v2 (新)
    participant SU as wolfBoot_success

    Note over F: BOOT=v1(SUCCESS), UPDATE=v2(UPDATING)
    R->>S: reset
    S->>S: bootState=SUCCESS, updateState=UPDATING → ②
    S->>U: wolfBoot_update(0)
    U->>U: open UPDATE/BOOT, verify UPDATE (integrity+authenticity)
    U->>U: upd_ver(2) > cur_ver(1)? 是 → 继续
    U->>U: get_update_sector_flag(0)=SECT_FLAG_NEW → 开始对换
    loop 逐 sector block (0..N-1)
        U->>F: BOOT[i]→SWAP (备份旧块)
        U->>F: UPDATE[i]→BOOT (新块就位)
        U->>F: SWAP[old]→UPDATE (旧块挪走)
        U->>F: set_update_sector_flag(i, UPDATED)
    end
    U->>U: 擦剩余扇区
    U->>FE: swap_and_final_erase(0)
    FE->>F: backup staging sector → SWAP
    FE->>F: UPDATE: → FINAL_FLAGS
    FE->>F: 擦 BOOT 尾扇区 → BOOT: → TESTING
    FE->>F: 擦 UPDATE 尾扇区 → UPDATE: → NEW
    FE-->>U: 0
    U-->>S: 0
    S->>S: open BOOT(已是 v2) → integrity → authenticity → 通过
    S->>F: do_boot(boot.fw_base) → APP v2 @ 0x08020100
    APP->>APP: 时钟/Timer/UART 初始化, 业务自检
    Note over APP: v2 偶数版 = 健康
    APP->>SU: wolfBoot_success()
    SU->>F: BOOT: TESTING → SUCCESS
    Note over F: 稳定: BOOT=v2(SUCCESS), UPDATE=v1(NEW)
```

### 4.4 场景四：不确认回滚（v2→v3 不确认 → 回 v2）

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant U as wolfBoot_update 1
    participant FE as swap_and_final_erase
    participant F as Flash
    participant APP as APP v2 (恢复后)

    Note over F: v2 已 SUCCESS, UPDATE 存 v3, trigger 过<br/>本次升级 v2→v3 已完成对换:<br/>BOOT=v3(TESTING), UPDATE=v2(NEW)
    R->>S: reset (v3 跑起来但没确认)
    S->>S: bootState=TESTING, updateState=NEW → ①
    Note over S: UPDATE trailer 仍有效 (magic 在)<br/>不需要补写 UPDATING
    S->>U: wolfBoot_update(1) fallback_allowed=1
    U->>U: open UPDATE(=v2)/BOOT(=v3)<br/>bootStateRet=0, bootState=TESTING<br/>cur_ver(3) >= upd_ver(2) → rollback_needed=1
    U->>U: 校验 UPDATE (v2) integrity+authenticity
    U->>U: 版本检查: fallback_allowed=1 跳过 upd>cur 限制
    loop 逐 sector block
        U->>F: BOOT[i]→SWAP, UPDATE[i]→BOOT, SWAP→UPDATE
    end
    U->>FE: swap_and_final_erase(0)
    FE->>F: BOOT: → TESTING, UPDATE: → NEW
    U->>U: rollback_needed==1 → BOOT: → SUCCESS (直接认定稳定)
    Note over F: BOOT=v2(SUCCESS), UPDATE=v3(NEW)
    S->>S: open BOOT(v2) → integrity → authenticity → 通过
    S->>F: do_boot → APP v2
    Note over APP: v2 偶数版健康 → wolfBoot_success (BOOT 已 SUCCESS, 无变化)
```
> 关键：回滚用 UPDATE 里留的**旧版**恢复 BOOT。`rollback_needed` 标志让收尾直接把 BOOT 设成 `SUCCESS` 而非 `TESTING`——因为回滚本身就是"放弃新版本、回到已验证的旧版本"，没必要再让 APP 确认一次（否则会陷入"回滚→TESTING→再回滚"的死循环）。

### 4.5 场景五：BOOT 认证失败 → 紧急回滚 → 再失败 panic

```mermaid
flowchart TD
    A[wolfBoot_start ③] --> B[open_image BOOT]
    B --> C{hdr_ok?}
    C -- 否 --> X1[bootRet<0]
    C -- 是 --> D[verify_integrity SHA256]
    D --> E{sha_ok?}
    E -- 否 --> X1
    E -- 是 --> F[verify_authenticity Ed25519]
    F --> G{signature_ok?}
    G -- 否 --> X1
    G -- 是 --> OK[do_boot APP]
    X1 --> H[wolfBoot_update 1 紧急回滚]
    H --> H1{回滚成功?}
    H1 -- 否 --> P[wolfBoot_panic<br/>无可引导项 → 死循环/停机]
    H1 -- 是 --> I[BOOT 已被回滚改写<br/>重新 open+integrity+authenticity]
    I --> J{再次校验通过?}
    J -- 否 --> P
    J -- 是 --> OK
```
> 紧急回滚与场景四走的是同一个 `wolfBoot_update(1)`，区别在触发点：场景四是"状态机①主动回滚"，场景五是"认证三道门失败后被动回滚"。回滚成功后**必须再 open+verify 一次**，因为 BOOT 内容刚被 update 改写过（`update_flash.c:1822-1825`）。两次都失败才 panic——这是"最后一道防线"。

### 4.6 场景六：升级中逐块对换断电恢复

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant U as wolfBoot_update 0
    participant F as Flash

    Note over F: 升级 v1→v2 中断在第 i 块对换<br/>UPDATE=UPDATING, SECT_FLAG[i]=SWAPPING/BACKUP
    R->>S: reset
    S->>S: bootState=SUCCESS(v1), updateState=UPDATING → ②
    S->>U: wolfBoot_update(0)
    U->>U: open UPDATE/BOOT (可能头已换, 容错)
    U->>U: get_update_sector_flag(0..N) 逐块查 nibble
    Note over U: flag=UPDATED 的块跳过<br/>flag=SWAPPING 的块从 SWAPPING 步继续<br/>flag=NEW 的块从头开始
    U->>U: 校验 UPDATE (重新 verify, 头可能已换位)
    U->>U: 版本检查 (upd>cur, v2>v1)
    loop 从断点块继续到 N-1
        U->>F: BOOT[i]→SWAP, UPDATE[i]→BOOT, SWAP→UPDATE
        U->>F: set_update_sector_flag(i, UPDATED)
    end
    U->>U: swap_and_final_erase(0) 收尾 → BOOT=TESTING, UPDATE=NEW
    S->>S: 认证 BOOT(v2) → do_boot → APP v2 → success
```
> 续传依据是 `SECT_FLAG_*` nibble（`update_flash.c:1292-1345`）：每个 case 标号对应断电时已完成的阶段，`FALL THROUGH` 继续往下做。`sector==1` 后会重新 `open_image` 重算 `fw_size`/`total_size`，因为第一块换完后头就互换了（`:1366-1377`）。

### 4.7 场景七：最终擦除中断电恢复（swap_and_final_erase 重入）

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant FE1 as swap_and_final_erase 1
    participant F as Flash

    Note over F: 逐块对换全部完成<br/>收尾 swap_and_final_erase(0) 中断<br/>UPDATE=FINAL_FLAGS, staging sector 已写 MAGIC_TRAIL
    R->>S: reset
    S->>S: 读 bootState/updateState (可能值不定)
    S->>FE1: swap_and_final_erase(1) resume=1
    FE1->>F: 读 BOOT staging sector @ tmpBootPos
    F-->>FE1: tmpBuffer[TRAILER_OFFSET_WORDS]==MAGIC_TRAIL → swapDone=1
    FE1->>F: get_partition_state(UPDATE) → updateState
    alt updateState == FINAL_FLAGS (第一次收尾断电)
        FE1->>F: 已备份 staging → 跳过 backup 步
    else updateState != FINAL_FLAGS (更早断电)
        FE1->>F: backup_last_boot_sector → SWAP
        FE1->>F: UPDATE: → FINAL_FLAGS
    end
    FE1->>F: 擦 BOOT 尾扇区 → BOOT: → TESTING
    FE1->>F: 擦 UPDATE 尾扇区 → UPDATE: → NEW
    FE1->>F: 从 SWAP 还原 staging sector (若 tmpBootPos 在 fw 范围内)
    FE1-->>S: resumedFinalErase != 0 (表示确实恢复过)
    Note over S: BOOT==TESTING → ① 回滚? 否——<br/>此时 UPDATE 已 NEW, ①②都不命中<br/>静默 fall-through 到 ③ 认证 BOOT(已是新镜像)
    S->>S: 认证 BOOT → do_boot → APP 新版
```
> `swap_and_final_erase` 的可重入性靠三个信号：① staging sector 里的 `MAGIC_TRAIL`（表示对换已完成、正在收尾）；② UPDATE 的 `FINAL_FLAGS`（表示 backup 步已做，重入时跳过 backup）；③ SWAP 里的 staging sector 备份（收尾中擦 BOOT 尾扇区前先把 staging 存走，收尾末尾再还原）。三者保证收尾阶段任意点断电都能恢复到"BOOT=新镜像(待确认), UPDATE=NEW"的一致态。

### 4.8 场景八：回滚中断电恢复

```mermaid
sequenceDiagram
    autonumber
    participant R as 复位
    participant S as wolfBoot_start
    participant U as wolfBoot_update 1
    participant F as Flash

    Note over F: BOOT=TESTING(v3 新), UPDATE 存 v2<br/>回滚 v3→v2 中断在第 i 块
    R->>S: reset
    S->>S: bootState=TESTING, updateState=NEW/不定 → ①
    S->>U: wolfBoot_update(1) fallback_allowed=1
    U->>U: open UPDATE(v2)/BOOT(v3)
    U->>U: bootState=TESTING && cur_ver(3)>=upd_ver(2) → rollback_needed=1
    Note over U: 回滚也是逐块对换, 同样靠 SECT_FLAG 续传<br/>断电后看 nibble 从断点继续
    loop 从断点块继续
        U->>F: BOOT[i]→SWAP, UPDATE[i]→BOOT, SWAP→UPDATE
    end
    U->>U: swap_and_final_erase(0) 收尾<br/>(本身断电可重入, 见场景七)
    U->>U: rollback_needed → BOOT: → SUCCESS (直接稳定)
    Note over F: BOOT=v2(SUCCESS), UPDATE=v3(NEW)
    S->>S: 认证 BOOT(v2) → do_boot → APP v2
```
> 回滚与升级共用同一套逐块对换 + `swap_and_final_erase` 机制，因此**断电恢复能力相同**。区别仅在：① `fallback_allowed=1` 跳过版本检查允许旧版；② `rollback_needed` 标志让收尾直接把 BOOT 设 `SUCCESS` 而非 `TESTING`，避免"回滚后又要 APP 确认"的死循环。

---

## 第 5 章　失败/异常路径汇总

| 触发点 | 现象 | 分支 | 结果 |
|--------|------|------|------|
| 首启无 trailer | `get_partition_state` 返回 -1 | `bootRet/updateRet != 0`，① ② 都不进 → ③ 认证 BOOT | 若 BOOT 镜像合法 → `do_boot`；不合法 → 紧急回滚（场景五） |
| `update_trigger` 时 UPDATE 内容坏 | trigger 只写状态不校验 → UPDATE=UPDATING | 下次复位 ② `wolfBoot_update(0)` 校验失败 → return -1 | BOOT 不变，UPDATE 状态保持 UPDATING（下次还会再试，需 APP 重写或擦 UPDATE） |
| 升级中逐块断电 | `SECT_FLAG[i]` 卡在 SWAPPING/BACKUP | ② `wolfBoot_update(0)` 看 nibble 续传（场景六） | 续传完成 → 收尾 → BOOT=新(TESTING) |
| 最终擦除中断电 | staging sector 有 `MAGIC_TRAIL`，UPDATE=FINAL_FLAGS | `swap_and_final_erase(1)` 重入（场景七） | 恢复到 BOOT=新(TESTING), UPDATE=NEW → ③ 认证 BOOT |
| 新镜像跑起来不确认 | APP 不调 `wolfBoot_success` 就复位 | BOOT=TESTING → ① 回滚（场景四） | BOOT 回到旧版，UPDATE 存被放弃的新版 |
| BOOT 镜像被篡改/损坏 | 认证三道门失败 | 紧急回滚（场景五） | 回滚成功 → 重认证 → `do_boot`；回滚失败 → `wolfBoot_panic` |
| 回滚中断电 | 同升级中断电 | ① 重入 `wolfBoot_update(1)`，靠 nibble 续传（场景八） | BOOT=旧(SUCCESS) |
| 回滚后 BOOT 仍校验不过 | 回滚成功但新写入的旧版也坏 | 二次 open+verify 失败 | `wolfBoot_panic`（无可引导项） |
| `wolfBoot_panic` | 无可引导镜像 | 死循环/停机（`loader.c` 末尾） | 设备 brick，需外部工具重烧 |
| 版本降级（无 ALLOW_DOWNGRADE） | upd_ver <= cur_ver | ② `wolfBoot_update(0)` 版本检查 return -1 | 拒绝升级，BOOT 不变；UPDATE 保持 UPDATING |
| 版本回滚（fallback_allowed=1） | upd_ver < cur_ver 但 BOOT=TESTING | ① `wolfBoot_update(1)` 跳过版本检查 | 允许用旧版恢复 BOOT |

**关键不变量**（断电安全的基础）：
1. **任何时候 BOOT 至少存有一份可引导镜像**：要么 BOOT 自身合法（SUCCESS/TESTING），要么 UPDATE 里有备份可恢复。
2. **逐块对换前必先备份到 SWAP**：BOOT[i]→SWAP 在 UPDATE[i]→BOOT 之前，所以"新块写一半断电"不会丢旧块。
3. **状态字段单调清位**：NOR 只能 1→0，状态只能往"更小的值"走，不会因断电回到中间态。
4. **trailer MAGIC 双校验**：读状态前先验 `WOLFBOOT_MAGIC_TRAIL`，MAGIC 不在则当"无 trailer"处理（返回 -1），不会误用未初始化字节。
5. **回滚用 SUCCESS 而非 TESTING 终态**：避免"回滚→TESTING→未确认→再回滚"死循环。

---

## 附：与其它笔记的衔接

- `notes/note-partition.md`：分区/字段**定义**（trailer 在哪、PART_FLAG 是什么、IMG_STATE_* 取值表）。本文是其**动态行为**展开。
- `notes/note_source_code.md`：源码**流程伪代码**与函数地图。本文的 mermaid 图是其流程的可视化补充。
- `notes/note_debug.md`：**怎么调试**这些分支（断点表、`get_partition_state` 返回 -1 的现场观察）。
- 关键函数行号：`wolfBoot_start`(`update_flash.c:1697`)、`wolfBoot_update`(`:1046`)、`swap_and_final_erase`(`:488`)、`wolfBoot_update_trigger`(`libwolfboot.c:893`)、`wolfBoot_success`(`libwolfboot.c:964`)、`get_partition_state`(`libwolfboot.c:687`)、`set_update_sector_flag`(`libwolfboot.c:764`)。

---

## 一句话总结

`wolfBoot_start` 用 **BOOT 的 PART_FLAG** 判"要不要回滚"、用 **UPDATE 的 PART_FLAG** 判"要不要升级"、用 **BOOT 末尾 staging 的 MAGIC_TRAIL** 判"上次是否断在最终擦除"；三个判据组合出**回滚 / 升级 / 直接认证**三条入口分支，认证失败再叠一层**紧急回滚**。所有分支最终都汇聚到"认证 BOOT → `do_boot`"，认证不过且回滚也失败才 `panic`。逐块对换靠 `SECT_FLAG_*` nibble 续传、最终擦除靠 staging `MAGIC_TRAIL` + `FINAL_FLAGS` 重入，二者共同保证"任何断电点都能恢复到一致态"。
