---
title: Ceph 分布式存储深度拆解：CRUSH 故障域、四维水位与稳定性保障
description: Ceph 的架构原理与生产治理全景——RADOS 组件职责、CRUSH 故障域与 PG 映射、RBD/CephFS/RGW 三种接口、BlueStore 写入路径，进而按"容量/性能/配额/冗余"四维水位拆解 OSD/Pool/PG 的观测与阈值，覆盖心跳与降级判定、慢 OSD 治理、恢复风暴控制、scrub 策略、脑裂与数据保护，并给出故障分级预案与演练清单。
summary: Ceph 不是一块大硬盘，而是一台"数据放置引擎"——稳定性取决于故障域是否隔离、PG 是否 active+clean、以及 OSD 水位是否逼近 nearfull/full 三级阈值；本文把这三件事讲成可观测、可告警、可演练的工程体系。
category: storage
tags:
- ceph
- rook
- rados
- crush
- pg
- osd
- bluestore
- distributed-storage
- capacity-planning
- reliability
- deep-dive
tier: core
created: '2026-11-09'
last_updated: 2026-11
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 架构师
- 平台工程师
estimated_read_time: 40min
intent_queries:
- Ceph 架构原理 CRUSH PG 是什么
- Ceph 集群水位怎么看
- Ceph OSD 写满了怎么办
- Ceph nearfull full backfillfull 阈值
- Ceph 慢 OSD 怎么治理
- Ceph 恢复风暴怎么控制
- Ceph 故障域怎么规划
- Ceph 稳定性保障方案
trigger_keywords:
- Ceph
- CRUSH
- PG
- OSD
- BlueStore
- nearfull
- backfillfull
- slow ops
- scrub
- 故障域
- Ceph 水位
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Ceph 分布式存储深度拆解：CRUSH 故障域、四维水位与稳定性保障

> **版本基线（2026-11 核对）**：Ceph 官方活跃版本为 **Umbrella (21.2) / Tentacle (20.2) / Squid (19.2)**；**Reef (18.2) 与 Quincy (17.2) 已归档**，不再接收补齐。新集群选活跃版本线，存量集群规划升级。
>
> **定位**：部署与日常操作看 [[wiki/06-存储/03-分布式存储/02-rook-ceph-production|Rook-Ceph 生产部署与运维]]；本文回答"为什么这么设计、什么时候会出事、怎么观测和兜住"。

---

## 1. 一句话理解 Ceph

**Ceph 不是一块大硬盘，而是一台"数据放置引擎"。**

传统存储先分配卷、再往里写；Ceph 反过来——你先告诉它"这份数据要几副本、放在哪类故障域"，它把每个 4 MiB 对象切成小对象、算哈希、用 CRUSH 算法直接算出该落在哪几个 OSD 上，**没有中心索引表**。

这个设计带来三个直接后果，也是所有稳定性问题的根源：

| 设计后果 | 好处 | 代价 |
|---------|------|------|
| **无中心元数据** | 无单点、近乎无限扩展 | PG/OSD map 要在全网同步，规模越大 map 越大、客户端更新越慢 |
| **数据放置由算法决定** | 加盘自动再平衡 | 算法输入（权重/故障域/PG 数）配错，会持续失衡且难以察觉 |
| **一致性靠副本 + 读修复** | 任一副本坏可自愈 | 自愈本身消耗 I/O，**恢复风暴**可能拖垮业务 |

---

## 2. 架构拆解

### 2.1 六大组件与职责边界

```text
                  ┌──────────────────────────────────────────┐
   接口层          │  RBD(块)      CephFS(文件)     RGW(对象)  │
                  └──────────────┬───────────────────────────┘
                                 ▼
                  ┌──────────────────────────────────────────┐
   RADOS 核心      │  OSD ×N  ——  数据 + 副本 + 恢复 + 心跳    │
                  │  MON ×3/5 ——  Cluster Map 权威（Paxos）   │
                  │  MGR ×2   ——  指标/PG 自动调整/仪表盘      │
                  └──────────────────────────────────────────┘
                      MDS（仅 CephFS） / CRUSH（算法，非进程）
```

| 组件 | 数量建议 | 核心职责 | 挂了会怎样 |
|------|---------|---------|-----------|
| **MON** | 3 或 5（奇数） | 维护 Cluster Map（MON/OSD/PG/CRUSH map） | 多数派存活不影响读写；**失去 quorum 集群不可操作** |
| **OSD** | 一盘一 OSD | 存数据、处理副本、参与恢复、心跳 | 少量 down 可降级运行；超故障域容忍度则数据不可用 |
| **MGR** | 2（active + standby） | 导出 Prometheus 指标、PG 自动伸缩、Dashboard | 不影响数据面，但**监控失明** |
| **MDS** | CephFS 场景 2+ | 管理目录/文件元数据 | 全部 down 时 CephFS 不可挂载（RBD 不受影响） |
| **CRUSH** | 不是进程 | 决定"数据放哪"的**算法** | 无进程可挂，但**配置错误会持续影响放置** |
| **Client/CSI** | 随负载 | 直连 OSD 读写，周期同步 map | map 过期会导致请求重试出错 |

> **交流要点**：客户常以为"MON 挂了集群就挂"。实际相反——**MON 短暂少数派失联，集群照常读写**，只是不能提交 map 变更（如新建 Pool、标记 OSD 状态）。真正的红线是**失去 quorum**。

### 2.2 数据路径：一次写入经历了什么

以三副本（`size=3, min_size=2`）写一个 RBD 对象为例：

```text
客户端写 4 MiB
   │
   ├─ ① 按对象名做 CRUSH 哈希 → 得到 PG id（如 2.1f3）
   │
   ├─ ② 查 PG map → 得到该 PG 的 [主 OSD, 备 OSD1, 备 OSD2]
   │
   ├─ ③ 写入主 OSD → 主 OSD 落 BlueStore 后，并行转发给两个备
   │
   ├─ ④ 两个备 OSD 落盘并返回 ACK
   │
   └─ ⑤ 主 OSD 向客户端返回成功（此时 ≥ min_size 副本已在盘上）
```

关键推论：

- **写延迟 ≈ 最慢的那个副本**。3 副本写延迟由**尾副本**决定，所以一块慢盘会拖慢整个 PG 的写响应；
- **`min_size=2` 是可用性的最后闸门**：一个 PG 的可用副本低于 `min_size` 时，该 PG 进入 **inactive**，**该 PG 上的所有 I/O 直接阻塞**（不是变慢，是卡住）；
- **size=3 + min_size=2 意味着可容忍 1 个副本不可用**。若故障域只到 host 而在同一 host 上放了 2 个副本，一次宕机就可能直接击穿。

### 2.3 BlueStore 写入路径

BlueStore 取代 FileStore 后，数据**直接写裸盘**（绕过文件系统），元数据存 RocksDB：

```text
写请求
  ├─ 小写(< min_alloc_size，默认 4K)：写进 WAL(内存) → 合并后落 RocksDB
  ├─ 大块 (>max_alloc_size，默认 4M)：跳过 WAL 直接写数据区
  └─ 中等：走 WAL → 数据区 → 元数据入 RocksDB
```

| 参数 | 默认值 | 影响 |
|------|-------|------|
| `bluestore_min_alloc_size` | 4 KiB | 小于它的写进入 WAL，小文件/小 IO 多时元数据压力大 |
| `bluestore_max_alloc_size` | 4 MiB | 大 IO 直写，避免 WAL 放大 |
| `bluestore_cache_size_hdd/ssd` | 动态（约 1~4 GiB 起） | 元数据缓存，命中率决定小 IO 延迟 |

> **典型事故**：全闪集群跑大量小 IO 时 `bluestore_cache_size` 不足 → RocksDB 频繁读盘 → OSD 延迟飙升 → 表现为"集群不健康但磁盘没坏"。内存规划见 [[wiki/06-存储/03-分布式存储/02-rook-ceph-production|Rook-Ceph 生产部署]]。

---

## 3. CRUSH 与故障域：稳定性第一性原理

### 3.1 故障域层级

CRUSH map 是一棵树，CRUSH 规则通过 `choose` 在不同层级选不同数量的 OSD：

```text
root
 ├── datacenter (dc1)
 │    ├── rack (rack-a)
 │    │    ├── host (node-01) ─┬─ osd.0 (weight 1)
 │    │    │                   └─ osd.1 (weight 1)
 │    │    └── host (node-02) ─┬─ osd.2
 │    │                        └─ osd.3
 │    └── rack (rack-b) ...
 └── datacenter (dc2) ...
```

**故障域（failure domain）** 就是 `rule` 中 `step choose firstn 0 type <X>` 的 `<X>`：

| 故障域 | 容忍 | 适用 |
|-------|------|------|
| `osd` | 单盘故障 | ❌ 几乎不用于生产（一台机器多盘同时坏概率高） |
| `host` | 整机故障 | **最常用**，前提是每台机器只放 1 个副本 |
| `rack` | 整机柜（含交换机）故障 | 有独立机柜/交换机时 |
| `datacenter` | 单机房故障 | 跨机房部署，副本跨 DC |

### 3.2 三副本的有效容量账

```text
逻辑容量 = 物理裸容量 × 副本数(3) × 水位上限(约 0.85 留余量)
         ≈ 裸容量 × 3 / 0.85

例：20 台 × 12 盘 × 8 TiB = 1,920 TiB 裸容量
    可用逻辑容量 ≈ 1,920 ÷ 3 × 0.85 ≈ 544 TiB
```

> **交流要点**：客户按"买了多少盘"理解容量，实际**可用只有裸容量的 1/3 左右**。EC 编码可把开销降到 1.5x，但代价是恢复开销与性能敏感性更高——见第 5A 节。

### 3.3 三个致命配置错误

| 错误 | 后果 | 识别方法 |
|------|------|---------|
| **副本未跨 host** | 一台机器宕机即多副本同时丢 | `ceph osd tree` 检查 OSD 分布；`ceph osd crush rule dump` 看 `type` 是否为 host |
| **CRUSH weight 与实际容量不符** | 大盘小用、小盘超载，持续失衡 | 比较 `ceph osd df` 的 `SIZE` 与 `%USE` |
| **PG 数不足** | 单 PG 过大，恢复时 I/O 集中、迁移慢 | `ceph osd pool autoscale-status` 看 `NEW PG_NUM` |

```bash
# 🟢 低风险/只读：核对故障域与副本分布
ceph osd tree
ceph osd crush rule dump --format json | jq '.[] | {rule_name, steps}'
ceph osd df tree
```

---

## 4. 四维水位：Ceph 专属落地

本文沿用全库统一的**四维水位**框架（容量 / 性能 / 配额 / 冗余），并映射到 Ceph 的真实对象与指标。

### 4.1 四维水位总览

| 维度 | Ceph 观测对象 | 核心观测点 | 越界后果 |
|------|--------------|-----------|---------|
| **① 容量水位** | OSD、Pool | 单 OSD 使用率 vs **nearfull/backfillfull/full** 三级阈值 | 达 full → **集群拒绝写入** |
| **② 性能水位** | OSD、网络 | apply/commit latency、slow ops、恢复占用 | 慢盘拖垮 PG，业务 P99 劣化 |
| **③ 配额水位** | Pool | `max_bytes` / `max_objects`、PG 自动伸缩 | 单 Pool 写满拖垮整个集群 |
| **④ 冗余水位** | PG、故障域 | `active+clean` 比例、可用副本数 vs min_size | 冗余耗尽 → PG inactive → I/O 阻塞 |

### 4.2 容量水位：三级阈值是硬闸门

Ceph 的容量不是"接近 100% 才危险"，而是**三级递进硬阈值**（官方默认值）：

| 阈值 | 参数 | 默认 | 行为 |
|------|------|------|------|
| 接近满 | `mon_osd_nearfull_ratio` | **0.75** | 告警 `OSD_NEARFULL`，仍可写 |
| 回填满 | `mon_osd_backfillfull_ratio` | **0.85** | 停止向该 OSD 回填，**恢复会卡住** |
| 写满 | `mon_osd_full_ratio` | **0.95** | `OSD_FULL`，**相关 Pool 写入被阻塞** |

> 🔴 **这是 Ceph 最重要的稳定性知识**：接近 full 时 Ceph 会主动停止读写以保护数据。所以 **0.85 是必须开始扩容的红线，而不是 0.95**——到了 0.85，回填已经停了，集群失去自愈能力。

**工程红线建议**：单 OSD 使用率 **> 0.75 预警、> 0.80 必须扩容**，即比官方 nearfull 还提前，为扩容留出恢复窗口。

```bash
# 🟢 低风险/只读：查看全局与单盘水位
ceph df
ceph osd df tree          # 看 SIZE / RAW USE / %USE / VAR
ceph osd dump --format json | jq '{full: .full_ratio, backfillfull: .backfillfull_ratio, nearfull: .nearfull_ratio}'
```

### 4.3 Pool 配额水位：防止单池吃掉全集群

Ceph 的一个特殊风险：**任一 OSD 达到 full，受影响的 Pool 全部阻塞**（即使别的 Pool 还有空间）。因此必须用 Pool 配额做隔离：

```bash
# 🟡 中风险：设置 Pool 配额（先测算再设，设错会导致业务写入失败）
# 官方语法为 set-quota，非 pool set
ceph osd pool set-quota <pool> max_bytes $((5*1024*1024*1024*1024))   # 5 TiB
ceph osd pool set-quota <pool> max_objects 10000000
# 取消配额：值设为 0
ceph osd pool set-quota <pool> max_bytes 0

# 🟢 低风险/只读：查看配额与使用量
ceph osd pool ls detail
ceph df detail
```

### 4.4 性能水位：延迟与 slow ops

Ceph 的性能水位主张**看延迟而不是看 IOPS**，因为 IOPS 上限取决于盘型和规模，而延迟劣化是先兆。

| 指标 | 含义 | 阈值建议 |
|------|------|---------|
| `ceph_osd_apply_latency_ms` | OSD 应用延迟 | 持续 > 50 ms 排查 |
| `ceph_osd_commit_latency_ms` | OSD 提交延迟 | 持续 > 50 ms 排查 |
| `ceph_health_detail{name="SLOW_OPS"}` | 慢请求超过阈值 | 出现即处理 |
| `ceph_pg_total - ceph_pg_active` | Inactive PG 数 | **> 0 即 P0** |

```bash
# 🟢 低风险/只读：定位慢 OSD
ceph osd perf                # apply/commit latency 按 OSD 排列
ceph health detail | grep -i slow
ceph tell osd.* dump_blocked_ops 2>/dev/null | head -50
```

### 4.5 冗余水位：PG 状态机

PG 状态是 Ceph 冗余水位的**唯一真相**。看懂状态码比看任何图表都有用：

| 状态 | 含义 | 风险等级 |
|------|------|---------|
| `active+clean` | 健康，全部副本一致 | ✅ |
| `active+degraded` | 可用但副本不足，正在恢复 | ⚠️ 可读写，无冗余 |
| `active+recovery` | 正在回填数据 | ⚠️ 关注恢复速率 |
| `active+undersized` | 副本数低于 `size` | ⚠️ 冗余下降 |
| **`inactive`** | 可用副本 < `min_size` | 🔴 **I/O 阻塞** |
| `incomplete` | 无法完成 peering | 🔴 严重 |
| `stale` | 主 OSD 无响应 | 🔴 严重 |

```bash
# 🟢 低风险/只读：PG 状态全景
ceph pg stat
ceph pg dump_stuck                 # 卡住的 PG
ceph pg dump_stuck inactive
ceph pg dump_stuck unclean
ceph pg dump_stuck stale
```

> **交流要点**：**`degraded` 不可怕，`inactive` 才致命。** degraded 表示"还能读写，只是少了一份保障"；inactive 表示"数据凑不齐，业务直接卡死"。告警矩阵要把两者分开定级。

### 4.6 四维水位落地告警映射

| 维度 | 触发条件 | 级别 | 动作 |
|------|---------|------|------|
| 容量 | `OSD_NEARFULL`（0.75） | 警告 | 排期扩容 |
| 容量 | 自算 OSD 使用率 > 0.80 | **严重** | 立即扩容 |
| 容量 | `OSD_FULL`（0.95） | **P0** | 战情室，禁止变更 |
| 性能 | `SLOW_OPS` 持续 5 分钟 | 严重 | 定位慢盘 |
| 性能 | apply latency > 50ms / 30 分钟 | 警告 | 排查 |
| 冗余 | `PG_AVAILABILITY`（含不含 OSD_DOWN 判定） | **P0** | I/O 阻塞处置 |
| 冗余 | degraded > 15 分钟 | 警告 | 看恢复进度 |
| 配额 | Pool 使用率 > `max_bytes` 的 80% | 警告 | 提配额或扩容 |
| 配额 | `TOO_MANY_PGS` / PG 失衡 > 30% | 警告 | 调整 autoscaler |

官方告警规则中最值得注意的一条是 `CephPGUnavailableBlockingIO`，它用 `(PG_AVAILABILITY==1) - (OSD_DOWN)` 判断"可用性问题是否由 OSD down 引起"，避免重复告警——**建议直接导入 Rook 的 `localrules.yaml`，而不是手写规则**。

---

## 5. 稳定性保障体系

### 5.1 五层保障模型

```text
┌──────────────────────────────────────────────────────────┐
│ L5 业务连续性：多集群/跨地域灾备、RTO/RPO 演练             │
├──────────────────────────────────────────────────────────┤
│ L4 数据保护：快照、RBD mirror、RGW 多站点、备份导出         │
├──────────────────────────────────────────────────────────┤
│ L3 自愈能力：OSD 替换、回填限速、PG 修复、scrub             │
├──────────────────────────────────────────────────────────┤
│ L2 故障域：CRUSH 规则、host/rack/DC 隔离、副本数            │
├──────────────────────────────────────────────────────────┤
│ L1 水位与配额：三级阈值、Pool 配额、PG 数、容量规划          │
├──────────────────────────────────────────────────────────┤
│ L0 基础硬件与网络：磁盘健康、双网、时钟同步、MTU             │
└──────────────────────────────────────────────────────────┘
```

> **Ceph 的稳定性不由云厂商承诺，全部由使用者自己设计。** 这与托管云存储最大的差别——Ceph 把能力交给你，也把责任交给你。

### 5.2 心跳与降级判定：为什么会"假宕机"

OSD 是否 `down` 不是由 MON 直接探活，而是**由其他 OSD 投票报告**：

| 参数 | 默认 | 含义 |
|------|------|------|
| `osd_heartbeat_interval` | 6 s | OSD 之间心跳间隔 |
| `osd_heartbeat_grace` | 20 s | 超过该时间无心跳判 down |
| `mon_osd_min_down_reporters` | 2 | 至少 2 个 OSD 报告才认定 down |
| `mon_osd_reporter_subtree_level` | host | 报告者须来自不同 host |
| `mon_osd_down_out_interval` | 600 s(10min) | down 后多久标记 **out** |
| `mon_osd_down_out_subtree_limit` | rack | 大于该级故障域不自动 out |
| `mon_osd_report_timeout` | 900 s | OSD 未上报 MON 的最长时间 |

**关键推论：**

- **down ≠ out**。down 只是"联系不上"，数据仍在盘上；标记 **out** 才会触发数据重分布。默认 10 分钟窗口就是留给运维反应的时间；
- **网络抖动/BGP 闪断会导致整批 OSD 同时 down**，此时若立即 out，会引发大规模回填——这正是很多"小抖动变大事"的根因；
- `mon_osd_down_out_subtree_limit=rack` 保护了"整个机柜掉线时不自动 out"，避免连锁重分布。

> 🔴 **运维铁律：故障排查期间，先 `set noout`，再动手。**

```bash
# 🟡 中风险：暂停自动 out（维护窗口标准动作）
ceph osd set noout
ceph osd set norebalance
ceph osd set norecover
ceph osd set nobackfill
# 🟡 中风险：维护结束务必恢复
ceph osd unset noout
ceph osd unset norebalance
ceph osd unset norecover
ceph osd unset nobackfill
```

### 5.3 慢 OSD 治理

**慢盘是 Ceph 最常见的稳定性杀手**——一块盘变慢，会拖慢它所在的 PG，进而拖垮依赖这些 PG 的全部业务。

| 现象 | 定位手段 | 处置 |
|------|---------|------|
| apply/commit latency 单盘偏高 | `ceph osd perf`、`ceph osd df` | 换盘或调整 mClock |
| slow ops 持续 | `ceph health detail`、blocked_ops | 先限流恢复，再换盘 |
| 同一 host 多盘同时慢 | 节点硬件/背板/控制器 | 整机下线维护 |
| 网络丢包导致重传 | `osd_slow_ping_time_*` | 查交换机/链路 |

```bash
# 🟢 低风险/只读：按 commit 延迟排序找出最慢的 OSD
# ceph osd perf 输出为 [ID COMMIT_LAT(ms) APPLY_LAT(ms)]，按第 2 列倒序
ceph osd perf | awk 'NR>1 {print $0}' | sort -k2 -n -r | head -10
```

**mClock 调度器**（Quincy 起引入并成为默认）是控制"业务 vs 恢复"资源分配的核心机制。内置三种 profile：**`balanced`（默认）**、`high_client_ops`（业务优先）、`high_recovery_ops`（恢复优先），也可自定义 reservation/limit/weight 但官方不建议除非有专业调优团队。详见第 5.4 节。

### 5.4 恢复风暴控制

OSD 故障后触发的回填是 **Ceph 最危险的时刻**——恢复流量可能挤占业务带宽，造成级联故障。

| 控制手段 | 命令 | 说明 |
|---------|------|------|
| 全局限速 | `osd_max_backfills`、`osd_recovery_max_active` | 传统方式，粗粒度 |
| 睡眠参数 | `osd_recovery_sleep`、`osd_snap_trim_sleep` | 给每个回填任务加延迟 |
| **mClock** | `osd_mclock_profile`（`balanced`/`high_client_ops`/`high_recovery_ops`） | **推荐方式**：按 QoS 分配 |
| 按时段限速 | 定时任务在业务低谷调高恢复并发 | 需配合监控 |

```bash
# 🟢 低风险/只读：查看当前恢复进度与流量
ceph -s | grep -E "recovery|backfill"
ceph osd pool stats

# 🟡 中风险：切到"客户端优先"profile，牺牲恢复速度保业务
ceph config set osd osd_mclock_profile high_client_ops

# 🟡 中风险：传统限速方式（mClock 关闭时使用）
ceph config set osd osd_max_backfills 1
ceph config set osd osd_recovery_max_active 1
```

> **交流要点**：**"恢复不是越快越好。"** 全速恢复看似积极，实则最容易把业务打死。生产应默认"业务优先"，在业务低谷窗口再放开恢复。

### 5.5 数据一致性：scrub 与 deep-scrub

| 类型 | 频率 | 检查内容 | 资源开销 |
|------|------|---------|---------|
| **scrub**（轻度） | 默认每日 | 比对元数据与校验和 | 低 |
| **deep-scrub**（深度） | 默认每周 | 逐字节读取校验 | 高，可能显著影响性能 |

```bash
# 🟢 低风险/只读：查看 scrub 状态
ceph pg dump | grep -E "scrubbing|deep"
ceph osd pool get <pool> scrub_min_interval

# 🟡 中风险：业务高峰暂停 scrub
ceph osd set nodeep-scrub
ceph osd set noscrub
```

> **稳定性取舍**：deep-scrub 是发现静默数据损坏（bit rot）的唯一手段，**不能长期关闭**。正确做法是设定低谷时段（`osd_scrub_begin_hour/end_hour`）而不是永久关闭。

### 5.6 数据保护与容灾

| 手段 | 保护对象 | 特点 |
|------|---------|------|
| RBD 快照 | 块卷 | 秒级、COW、**依赖集群本身** |
| **RBD mirror** | 跨集群块卷 | **journal 模式**：实时复制（RPO 秒级）；**snapshot 模式**：基于快照增量复制，崩溃一致（RPO 取决于快照周期） |
| CephFS 快照 | 目录 | 依赖 MDS |
| **RGW 多站点（Multisite）** | 对象桶 | 跨集群同步，读写可分区域 |
| 导出备份（rbd export / s3 sync） | 全量 | 最强兜底，**可脱离集群恢复** |

> 🔴 **最常见的架构缺陷**：只做快照，不做导出。快照与数据在同一集群，**集群级损坏时快照一起没**。Ceph 必须有"一份能脱离 Ceph 恢复的备份"。

---

## 5A. 纠删码（EC）与副本：选型账

| 维度 | 三副本 | EC（如 k=4, m=2） |
|------|-------|------------------|
| 空间开销 | 3.0x | 1.5x |
| 可用容量 | 33% 裸容量 | 约 67% 裸容量 |
| 写放大 | 中等 | 大（需分块+编码） |
| 恢复开销 | 整盘复制 | 需多盘联合重建，读放大高 |
| 小 IO 性能 | 好 | 差（默认 4K 分块需条带化） |
| 适用 | 数据库、低延迟、小 IO | 大文件、冷数据、归档、备份 |

**决策要点**：

- **EC 不支持部分 RBD 特性**（历史上不支持某些操作，需按版本核对官方矩阵）；
- EC Pool **不能直接给需要覆盖写/小 IO 的业务**用，通常按"缓存层（副本）+ 归档层（EC）"分层；
- EC 集群容量水位要留**更大余量**，因为重建期间读放大显著。

> EC 的精确特性支持矩阵（哪些 RBD/RGW/CephFS 操作可用）随版本变化，**必须以所选版本线的官方文档为准**，不要按记忆配置。详见 <https://docs.ceph.com/en/latest/rados/operations/erasure-code/>。

---

## 6. 故障分级与处置预案

### 6.1 故障分级

| 级别 | 判定 | 典型场景 | 响应 |
|------|------|---------|------|
| **P0** | PG inactive / OSD FULL / 失去 quorum | 多副本同时不可用、写满 | 立即战情室，`noout` 止血 |
| **P1** | degraded 长期不收敛、恢复停滞、批量 slow ops | 回填卡在 backfillfull、慢盘扩散 | 30 分钟内缓解 |
| **P2** | 单 OSD down、scrub 错误、单 Pool 偏高 | 单盘故障、校验和错误 | 2 小时内处理 |
| **P3** | 水位预警、PG 轻度失衡、MDS 掉 standby | nearfull、PG 数不足 | 排期处理 |

### 6.2 场景 A：OSD 写满 / 拒绝写入

```bash
# 🟢 低风险/只读：确认是否有 OSD 达到 full
ceph health detail | grep -E "OSD_FULL|OSD_NEARFULL|no space"
# 按使用率找最满的 OSD（VAR 列显示偏离均值幅度，优先处理 VAR 大的盘）
ceph osd df tree | awk 'NR>1 && $2 ~ /^[0-9]+$/ {print}' | sort -k5 -n -r | head -10
```

处置顺序：

1. **先止血**：`ceph osd set noout`，避免任何进一步的数据搬迁放大用量；
2. **释放空间**：删除过期快照、清理废弃 Pool、降低副本（谨慎）；
3. **扩容**：加盘/加节点；
4. ⚠️ **官方建议的"临时提高 full 阈值"是应急手段**，只能小幅上调且必须尽快恢复，否则会破坏数据可用性设计。

> 🔴 高风险：**不要**在集群 full 状态下新建 Pool 或调整 CRUSH weight，会触发大规模搬迁直接雪崩。

### 6.3 场景 B：PG inactive（业务 I/O 阻塞）

```bash
ceph pg dump_stuck inactive
ceph osd tree
ceph -s
```

原因通常是 `min_size` 无法满足。处置：**优先恢复 down 的 OSD**（而非标记 out）；若数据确实不在，需从快照/备份恢复该 PG 或整个 Pool。

> 🔴 **红线**：`ceph osd out` 是不可逆的重分布动作。在确认盘上数据可恢复前，**绝不主动 out**。

### 6.4 场景 C：恢复停滞（backfillfull）

现象：`degraded` 长期不降，`ceph -s` 显示 `backfillfull`。根因：部分 OSD 已达 0.85，**无法接收回填数据**。

处置：先扩容腾出空间，再放开 `nobackfill`。

### 6.5 场景 D：MON 失去 quorum

```bash
ceph -s          # 若命令直接失败，说明客户端连不上 MON
ceph mon stat    # 在 MON 节点上执行
```

处置原则：**先判断是网络分区还是进程崩溃**。网络分区时强行重启 MON 可能导致 MON 存储分裂，恢复时需按官方流程挑选数据最完整的 MON 重建。

### 6.6 演练清单

| 编号 | 演练项 | 通过标准 |
|------|-------|---------|
| C1 | 单 OSD 故障替换 | 自动降级不中断，替换后 `active+clean` 收敛 |
| C2 | 单节点（host）宕机 | 不触发 inactive，恢复在窗口内收敛 |
| C3 | 模拟 OSD 到达 0.80 | 告警到达，扩容流程可执行 |
| C4 | 恢复风暴注入 | 业务 P99 劣化在可接受范围，mClock 生效 |
| C5 | deep-scrub 全量执行 | 无业务感知，发现并修复错误 |
| C6 | 快照恢复 + 导出的脱集群恢复 | 能脱离原集群恢复数据 |
| C7 | MON 单点故障 | quorum 保持，无业务影响 |

---

## 7. 与云存储的定位对比

Ceph 与托管云存储不是竞争关系，而是不同责任的承担者：

| 维度 | Ceph（自建） | 阿里云 OSS/EBS/NAS（托管） |
|------|-------------|---------------------------|
| 运维责任 | **全部自负**：升级、故障、容量、性能 | 云厂商负责平台，客户负责配额与水位 |
| 容量单价 | 硬件成本 + 人力，规模大时更优 | 按量计费，无前期投入 |
| 弹性 | 加盘需采购与上架，天级 | 分钟级 |
| 可控性 | 可改 CRUSH、QoS、参数 | 仅配额与性能档 |
| 故障域 | 自行设计 | 多 AZ 已内置 |
| 适用 | 专有云、大规模、数据不出域、成本敏感 | 弹性、免运维、中小规模 |

> 客户交流话术：**"选 Ceph 不是选更便宜的存储，是选'自己当存储厂商'。"** 决策依据应是"是否有专职存储团队"，而不是单价对比。

---

## 8. 官方参考

| 主题 | 链接 |
|------|------|
| Ceph 版本线（活跃/归档） | <https://docs.ceph.com/en/latest/releases/> |
| MON/OSD 交互与心跳参数默认值 | <https://docs.ceph.com/en/latest/rados/configuration/mon-osd-interaction/> |
| MON 配置与 nearfull/backfillfull/full 阈值 | <https://docs.ceph.com/en/latest/rados/configuration/mon-config-ref/> |
| 健康检查项说明（OSD_FULL / PG_AVAILABILITY 等） | <https://docs.ceph.com/en/latest/rados/operations/health-checks/> |
| Rook 官方 Prometheus 告警规则（localrules.yaml） | <https://github.com/rook/rook/blob/master/deploy/examples/monitoring/localrules.yaml> |
| Rook 外部集群告警规则（externalrules.yaml） | <https://github.com/rook/rook/blob/master/deploy/examples/monitoring/externalrules.yaml> |

## See Also

- [[wiki/06-存储/03-分布式存储/02-rook-ceph-production|Rook-Ceph 生产部署与运维]]（部署手册）
- [[wiki/06-存储/03-分布式存储/03-longhorn-production|Longhorn 生产部署]]
- [[wiki/06-存储/03-分布式存储/04-openebs-production|OpenEBS 生产部署]]
- [[wiki/06-存储/03-分布式存储/06-nfs-csi-production-guide|NFS CSI 生产指南]]

## Related

- [[wiki/18-云厂商/01-阿里云/公有云-ACK/09-alicloud-storage-deep-dive|阿里云存储产品深度拆解]]（四维水位框架来源）
- [[wiki/12-可靠性/02-灾难恢复/19-storage-backend-failure-playbook|存储后端故障手册]]
- [[wiki/12-可靠性/03-容量规划/07-ai-driven-capacity-planning-cost-optimization-2025|容量规划与成本优化]]
- [[wiki/06-存储/06-云存储对比/01-cloud-storage-comparison|云存储全面对比]]
- [[wiki/26-标签/02-网络与存储/storage|storage 标签枢纽]]

<!-- risk-assessed -->