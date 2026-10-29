---
sources:
- "wiki/01-集群基础/03-控制平面/12-apiserver-deep-dive.md"
- "wiki/01-集群基础/03-控制平面/18-api-priority-fairness.md"
- "wiki/22-概念/01-核心架构/etcd-operational-reference.md"
- "对话沉淀: 2026-10-22 apiserver 整体变慢排查复习（面试卡 Q9–Q12 / Q126–Q127）"
title: apiserver 延迟定界与 etcd 第一嫌疑人排查法 (API Server Latency Triage)
summary: 系统化回答"apiserver 整体变慢怎么查"：为什么 etcd 是第一嫌疑人（写、quorum LIST、watch 三条链路汇合），etcd 侧 fsync/db size/Leader 三件证据的判读红线，etcd 洗清嫌疑后用指标与审计时间戳把延迟按段切给 APF 排队/准入 webhook/存储层，附五个高频故障模式 runbook 与告警 PromQL。
category: concepts
tags:
- apiserver
- etcd
- latency
- fsync
- watch-cache
- api-priority-fairness
- troubleshooting
- performance
tier: core
created: 2026-10-22
updated: 2026-10-22
last_updated: 2026-10
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- apiserver 整体变慢排查顺序是什么
- 为什么 apiserver 慢先查 etcd
- etcd wal fsync 延迟多少算不健康
- 怎么定位一个写请求慢在 apiserver 哪一段
- apiserver 429 是故障还是设计
trigger_keywords:
- apiserver 变慢
- etcd 第一嫌疑人
- wal_fsync
- db size
- leader changes
- watch cache
- resourceVersion
- APF 429
- 慢 webhook
- compact defrag
k8s_versions:
- '1.24'
- '1.28'
- '1.31'
- '1.34'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与资源是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 特别提醒：etcd 的 compact / defrag / alarm 操作直接作用于集群唯一真相源。defrag 期间节点不可写，必须逐台执行；本文所有 etcdctl 命令默认已带 `--endpoints/--cacert/--cert/--key`，为省篇幅从略。

# apiserver 延迟定界与 etcd 第一嫌疑人排查法 (API Server Latency Triage)

"apiserver 整体变慢"是控制面最常见也最容易答歪的故障题。本文给一条可背诵、可在现场执行的主线：**先查 etcd（fsync 延迟、db size、Leader 状态）——etcd 是 apiserver 延迟的第一嫌疑人；etcd 洗清嫌疑后，再按请求经过的站点把延迟切给 APF 排队、准入 webhook 或 apiserver 自身**。

本文与相邻笔记的分工：[[wiki/01-集群基础/03-控制平面/12-apiserver-deep-dive.md|apiserver deep-dive]] 讲架构与认证授权机制，[[wiki/01-集群基础/03-控制平面/17-apiserver-tuning.md|apiserver 调优]] 讲参数清单，[[wiki/01-集群基础/03-控制平面/18-api-priority-fairness.md|API Priority and Fairness]] 讲限流机制本体；本文聚焦**延迟故障的定界方法**——每个候选根因配什么证据、什么红线、什么动作。面试对应卡：[[maintaining/04-报告/k8s-高级面试快问快答-沉淀.md|快问快答沉淀]] Q9–Q12、Q126–Q127（etcd 侧机制卡另见 Q116–Q125 etcd 专题批）。

---

## 一、为什么 etcd 是第一嫌疑人：三条链路都在它身上汇合

### 1.1 写路径：每个写请求都要过 Raft 多数派落盘

```
kubectl apply / controller update
  → apiserver（authn → APF 排队 → authz → mutating 准入 → 校验 → validating 准入）
  → 存储层：对 etcd 发 Txn（以 mod_revision 做 compare-and-swap 乐观并发）
  → etcd leader 追加 Raft 日志 → 复制给 follower → 多数派 WAL fsync 落盘
  → apply 到状态机 → 响应返回 apiserver → 写事件进 watch 链路
```

任何一环慢——WAL fsync 慢、leader 换人、网络 RTT 大——**所有写请求一起慢**，没有例外。这也是"扩 apiserver 副本治不了写延迟"的原因：副本分摊的是连接和缓存读，写还是全压同一个 etcd leader（控制面 HA 分工详见 [[wiki/01-集群基础/03-控制平面/03-plane-high-availability.md|控制面高可用]] 与 Q17 卡）。

### 1.2 读路径：三种 resourceVersion 语义，两种打 etcd

| 请求形态 | 谁服务 | 打不打 etcd | 典型来源 |
|---|---|---|---|
| LIST/GET 不带 `resourceVersion` | etcd quorum 读（linearizable） | **打** | `kubectl get` 默认、多数裸脚本 |
| `resourceVersion=0` | apiserver 内存 watch cache | 不打 | informer/reflector 默认 |
| `resourceVersion=N`（NotOlderThan） | watch cache 等到 N，不够回源 | 视情况 | 分页续读、严格一致性场景 |

生产含义：**不带 RV 的全量 LIST 是最贵的一类请求**，大集群一次 `kubectl get pods -A` 可能拉几十万对象。1.31 起 `ConsistentListFromCache` 进入 beta 并默认开启（KEP-2340），允许"必须最新"的 LIST 先向 etcd 发 progress notify 确认缓存不落后、再从缓存出数——但它依赖 etcd ≥ 3.4.31 / 3.5.13 的 progress notify 能力，旧版本集群仍在走 quorum 读。resourceVersion 与 409/410 的完整语义见 [[wiki/01-集群基础/02-设计原则/07-resource-version-control.md|资源版本控制]]；watch/compact 视角的 410 原理另见面试卡 Q119。

### 1.3 watch 链路：etcd 慢会伪装成"控制器失灵"

etcd 的 watch 事件 → apiserver watch cache（cacher）→ 分发给每个客户端 watcher。etcd 落盘慢，事件就晚发出，**所有控制器的反应都变慢**——表现为 Deployment 改了 Pod 迟迟不建、节点心跳延迟上报告警，看起来像 controller-manager 出问题，实际源头在存储。机制细节见 [[wiki/22-概念/01-核心架构/watch-mechanism.md|Watch 机制]]。

**推论（排查顺序的第一性原理）**：写、默认 LIST、watch 三条最重的链路全部经过 etcd，且 etcd 是 Raft 单 leader 写、无法用 apiserver 横向扩容分摊——所以 etcd 排第一嫌疑人。

---

## 二、etcd 侧三件证据：fsync 延迟、db size、Leader 状态

### 2.1 fsync 延迟：慢盘的第一证据

etcd 有两个独立的磁盘延迟指标，含义不同：

| 指标 | 含义 | 基线 |
|---|---|---|
| `etcd_disk_wal_fsync_duration_seconds` | Raft 日志（WAL）每次提案强制落盘，**每个写请求都要经历** | p99 < **10ms**（官方性能基线，SSD 正常 1–5ms） |
| `etcd_disk_backend_commit_duration_seconds` | boltdb 后端事务批量提交（默认 100ms 攒一批） | p99 < **25ms**（etcd 官方告警规则阈值 0.25s） |

注意两套数字的用途不同：**10ms/25ms 是"盘健不健康"的体检线**；etcd 自带告警规则（`etcdHighFsyncDurations` 0.5s、`etcdHighCommitDurations` 0.25s）是"已经烂到要告警"的兜底线——面试时能分清这两套，比背一个数字高一个档次。

从 apiserver 视角先看第一手证据（不用登 etcd 节点）：

```bash
# 🟢 apiserver 视角的 etcd 操作延迟（operation 分 get/list/create/update...）
kubectl get --raw /metrics | grep etcd_request_duration_seconds
```

登 etcd 节点确认三件事：

```bash
# 🟢 一屏看 DB SIZE / IS LEADER / RAFT TERM
etcdctl endpoint status --cluster -w table

# 🟢 fsync / backend commit 直方图
curl -s localhost:2381/metrics | grep -E "etcd_disk_(wal_fsync|backend_commit)_duration"

# 🟢 etcd 日志签名（慢盘三连）
journalctl -u etcd --since "-30m" | grep -E "apply request took too long|failed to send out heartbeat on time|slow fdatasync"
```

`apply request took too long`（expected-duration 100ms）和 `failed to send out heartbeat on time` 是慢盘的经典日志签名——后者还会引发选举，把问题升级成 Leader 抖动（见 2.3）。

**fsync 飙高的三大来源与取证**：

1. **共盘**：etcd 数据盘和容器日志、镜像层、监控 TSDB 同盘。`lsblk`/`df` 确认挂载关系，`iotop`/`iostat -xz 1` 看 `w_await` 与 aqu-sz 同时段飙升。
2. **HDD / 网络盘**：etcd 官方明确不建议网络存储（EBS 类网络盘 fsync 抖动大）。
3. **CPU/IO 抢占**：同节点重负载挤压 etcd 进程。

**验盘标准动作**（官方 fio 配方，模拟 etcd 写模式）：

```bash
# 🟡 会写一个测试文件，放到数据盘目录执行；看 fdatasync 99th percentile < 10ms
fio --rw=write --ioengine=sync --fdatasync=1 --directory=/var/lib/etcd/fio-test \
    --size=22m --bs=2300 --name=etcd-fsync-test
# 粗筛也可用 etcdctl check perf（会注入负载，🟡 生产慎用，低峰执行）
```

**治法**：独立本地 SSD/NVMe；调大 `heartbeat-interval`/`election-timeout`（跨 AZ 部署时官方建议按 RTT 调大，如 200ms/2000ms）；给 etcd 进程提 IO 优先级 `ionice -c2 -n0 -p $(pgrep etcd)`；大集群把 **Events 分库**：`--etcd-servers-overrides=/events#https://<独立etcd>:2379`。etcd 写路径内部的两段延迟机制（WAL fsync 与 bbolt 批量提交、commit≠apply）拆解见面试卡 Q117 与 [[wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd deep-dive]]。

### 2.2 db size：quota、碎片与空间大户

- quota：`--quota-backend-bytes` **默认 2GiB，官方建议上限 8GiB**。打满后 `etcdctl alarm list` 出 `NOSPACE`，etcd **拒写不拒读**，写请求报 `etcdserver: mvcc: database space exceeded`。
- 两个尺寸指标：`etcd_mvcc_db_total_size_in_bytes`（文件大小，旧版本在 `etcd_debugging_` 前缀下）与 `etcd_mvcc_db_total_size_in_use_in_bytes`（实际有效数据）。**total 大、in use 小 = 碎片**，compact + defrag 可救；in use 也贴顶 = 数据真的大。
- k8s 集群的空间大户：**Events**（`--event-ttl` 默认 1h，大集群每秒几百条）、**高翻新对象的 revision 堆积**（每个节点每 10s 续一次 Lease——5000 节点仅心跳就是 ~500 写/s 的 revision 流）、超大 CR / ConfigMap（单对象上限 `--max-request-bytes` 1.5MiB）。
- compaction 的真实分工：**k8s 场景下 compaction 由 apiserver 驱动**——`--etcd-compaction-interval` 默认 5m，apiserver 周期性请求 etcd 压缩历史；etcd 自身的 `--auto-compaction-retention` 反而常是关的（原生默认 0）。**但 defrag 永远没有自动机制**，例行维护必须自己做。

NOSPACE 恢复三步（每步都不能省，原理与红线见 Q10 卡）：

```bash
# 1. 🟡 compact：清当前 revision 之前的旧版本
rev=$(etcdctl endpoint status -w json | jq -r '.[0].Status.header.revision')
etcdctl compact "$rev"
# 2. 🟡 defrag：逐台执行！--cluster 一把梭会打掉 quorum
etcdctl --endpoints=<m1> defrag   # 等该节点健康再下一台
# 3. 🟡 解告警：quota 恢复不会自动消 alarm
etcdctl alarm disarm
```

### 2.3 Leader 状态：选举与提案失败

```bash
# 🟢 endpoint status 里看三列
#   IS LEADER：leader 在哪台；RAFT TERM：涨了 = 发生过选举
etcdctl endpoint status --cluster -w table

# 🟢 Leader 稳定性与提案健康
curl -s localhost:2381/metrics | grep -E "etcd_server_(leader_changes_seen_total|has_leader|proposals_(failed|pending|committed|applied)_total)"
```

判读：

- `leader_changes_seen_total` 近期 > 0 = 磁盘抖动或网络分区触发过选举；**每次选举所有写停摆一个选举窗口**（默认 election-timeout 1000ms 起步），apiserver 表现为批量请求超时。
- `has_leader == 0` = 当前无 leader，集群不可写，最高优先级。
- `proposals_failed_total` 增长 = 丢 leader/丢 quorum/提交超时；`committed - applied` 差值持续大 = apply 跟不上（典型慢盘）。
- 跨 AZ 部署时对照 `etcd_network_peer_round_trip_time_seconds`，RTT 大先调大心跳/选举超时再谈别的。

**etcd 三件证据的判断线**：fsync p99 > 10ms 或有慢盘日志签名 → 盘；db size 贴 quota / NOSPACE → 空间；leader changes / term 涨 → 稳定性。三者任一命中，先治 etcd，别动 apiserver。

---

## 三、etcd 洗清嫌疑之后：按段定界 apiserver 自身

### 3.1 请求在 apiserver 里经过哪几站

```
TLS → 认证(authn) → 审计 RequestReceived → APF 排队/拒绝 → 鉴权(authz)
    → 变更准入(mutating admission, 含 webhook) → schema 校验
    → 验证准入(validating admission, 含 webhook) → 存储层编码 → etcd Txn → 响应
```

### 3.2 指标对照表：每段一把尺子

| 段 | 指标 | 判读 |
|---|---|---|
| 全程 | `apiserver_request_duration_seconds{verb,resource}` | **必须按 verb 拆**：写慢回 etcd/准入，GET/LIST 慢看缓存与 APF |
| APF 排队 | `apiserver_flowcontrol_request_wait_duration_seconds`、`apiserver_flowcontrol_rejected_requests_total`、`current_inqueue_requests` | wait 高 = 座位不够/被大户挤；reject = 429 来源（座位模型见 Q127） |
| 准入 webhook | `apiserver_admission_webhook_admission_duration_seconds{name,type,rejected}` | 单个 name 高 → 慢 webhook；rejected=1 → 拦截写 |
| 存储 | `etcd_request_duration_seconds`、`apiserver_storage_list_total` vs `apiserver_cache_list_total` | list 总量里回源 etcd 的占比高 = 缓存没接住 |
| 响应体 | `apiserver_response_sizes` | LIST 响应巨大 = 大 LIST 客户端 |
| 进程 | `process_cpu_seconds_total`、Go GC 指标 | CPU 打满/GC 频繁 = apiserver 自身容量 |

### 3.3 审计日志时间戳定界法（按人、按 UA 找慢请求）

audit event 每条带 `requestReceivedTimestamp` 与各 stage 的 `stageTimestamp`（RequestReceived → ResponseComplete）。两者之差 = 该请求在 apiserver+etcd 的全程耗时，且带 `user/userAgent/objectRef/responseSize`——是唯一能把"慢"精确归因到客户端身份的证据源：

```bash
# 🟢 找最慢的 10 个请求（jq 计算毫秒差，按 user 和 UA 汇总）
jq -r 'select(.stage=="ResponseComplete")
  | [ ((.stageTimestamp|fromdateiso8601) - (.requestReceivedTimestamp|fromdateiso8601)) * 1000 | floor,
      .verb, .objectRef.resource, .user.username, .userAgent, .responseStatus.code ] | @tsv' \
  audit.log | sort -rn | head
```

### 3.4 pprof 与遗留指标

- `kubectl get --raw '/debug/pprof/profile?seconds=30'`（需 `--profiling` 未关，加固集群常关）抓 CPU 火焰图，定位 apiserver 自身热点（如加密 provider、序列化）。
- `apiserver_current_inflight_requests` 是 APF 之前的遗留指标；开启 APF（1.29 GA，现为唯一机制）后以 flowcontrol 系列为准。
- 定界决策树：

```
写 P99 高
├─ etcd_request_duration ≈ 总延迟        → etcd（回第二章三件证据）
├─ webhook admission_duration 某 name 高 → 慢 webhook（见 4.5）
├─ flowcontrol wait_duration 高          → APF 排队（见 Q127/[[18-api-priority-fairness]]）
└─ 三者都低但总延迟高                    → apiserver 自身 CPU/GC/加密 → pprof
GET/LIST 也慢且伴 429                    → 大 LIST 客户端（见 4.3）
只有 watch 事件延迟大                    → 回看 etcd fsync（事件链路伪装成控制器失灵）
```

---

## 四、五个高频故障模式与处置

### 4.1 慢盘 / 共盘 → fsync 飙高

症状：全集群写慢 + 控制器反应慢；etcd 日志三连签名。确认：2.1 的指标与 fio。处置：独立本地 SSD、events 分库、ionice、跨 AZ 调大心跳/选举超时。**红线：❌ 把 etcd 放网络盘；❌ 和容器日志/镜像同盘。**

### 4.2 etcd NOSPACE → 拒写

症状：所有写报 `mvcc: database space exceeded`，读正常。处置：compact → **逐台** defrag → `alarm disarm`（缺一步都恢复不了）。预防：例行 defrag 计划任务、events 治理（`--event-ttl`、EventRateLimit）、quota 上调到 8GiB 前先瘦身。**红线：❌ defrag `--cluster` 一把梭；❌ 删数据目录重建。**

### 4.3 大 LIST 与 event 洪泛 → 429、内存尖峰

定位三刀：`apiserver_storage_objects` 看哪个资源对象最多（events 十有八九第一）；audit 按 responseSize 找最大 LIST 的 userAgent；`apiserver_flowcontrol_rejected_requests_total` 看 429 集中在哪个 priorityLevel。治法：客户端一律 informer（List+Watch + watch cache，`resourceVersion=0`）；真要大 LIST 用 `limit+continue` 分页；event 用 `fieldSelector=involvedObject.name` 精确查。**红线：❌ 429 就扩 apiserver——先揪出打爆它的客户端。**

### 4.4 apiserver 重启 → LIST 风暴

机制：watch 全断 → reflector 先 LIST 全量再 watch，而此时 watch cache 冷、第一批 LIST 回源 etcd。缓解：逐台滚动且间隔拉够；WatchBookmark（GA）让客户端 RV 保鲜、减少被迫全量 LIST；APF 保控制面优先级；新版 WatchList（KEP-3157，1.32 前后 beta）用流式 watch 替代初始 LIST 是根治方向。判断线：抖动窗口 = 缓存暖机时间，超 5 分钟还在 429 是容量问题不是余波。

### 4.5 慢 admission webhook → 写路径隐形台阶

webhook 在每个写请求的 mutating/validating 两步各串行一次：`timeoutSeconds` 默认 **10s**，`failurePolicy` 默认 **Fail**——慢 webhook 不只是慢，超时+Fail 会直接把写变成 5xx。定位：`apiserver_admission_webhook_admission_duration_seconds` 按 name 排序。治法：webhook 部署独立于业务命名空间（防自锁死循环）、配 `objectSelector/namespaceSelector` 缩拦截面、`timeoutSeconds` 调到 1–2s、幂等且 `failurePolicy` 经评估后选择。**红线：❌ failurePolicy=Fail 又不给短超时；❌ webhook 拦自己所在命名空间的资源。**

---

## 五、容量红线与告警（PromQL 速查）

```promql
# 写延迟 P99（按 verb 拆，关注 POST/PUT/PATCH/DELETE）
histogram_quantile(0.99, sum by (verb, le) (rate(apiserver_request_duration_seconds_bucket{verb=~"POST|PUT|PATCH|DELETE"}[5m]))) > 1

# etcd fsync p99 > 10ms（体检线；0.5s 是 etcd 官方告警兜底线）
histogram_quantile(0.99, sum by (le) (rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m]))) > 0.01

# leader 变更（10m 内 > 0 即异常）
increase(etcd_server_leader_changes_seen_total[10m]) > 0

# db size / quota 比值（默认 2GiB，按实际 quota 换算）
max(etcd_mvcc_db_total_size_in_bytes) > 8e9 * 0.8

# APF 拒绝（429）速率
sum by (priority_level) (rate(apiserver_flowcontrol_rejected_requests_total[5m])) > 0

# 慢 webhook
histogram_quantile(0.99, sum by (name, le) (rate(apiserver_admission_webhook_admission_duration_seconds_bucket[5m]))) > 1

# events 对象数（空间大户水位）
apiserver_storage_objects{resource="events"} > 100000
```

---

## 六、与相邻概念的关系

- **[[wiki/01-集群基础/03-控制平面/12-apiserver-deep-dive.md|apiserver deep-dive]]**：请求处理链路、认证授权与审计机制本体；本文只取与延迟有关的断面。
- **[[wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd deep-dive]] / [[wiki/01-集群基础/03-控制平面/19-etcd-operations.md|etcd 运维]]**：Raft 机制与备份恢复、集群生命周期操作；本文聚焦"延迟故障现场"的三件证据。
- **[[wiki/22-概念/01-核心架构/etcd-operational-reference.md|etcd Operational Reference]]**：etcd 生产参数与 FTA 故障模式的参考手册，本文 2.x 节是其延迟视角的展开。
- **[[wiki/22-概念/01-核心架构/watch-mechanism.md|Watch 机制]]**：watch/cache/事件分发的机制本体，解释"etcd 慢伪装成控制器失灵"。
- **[[wiki/01-集群基础/02-设计原则/07-resource-version-control.md|资源版本控制]]**：resourceVersion 语义与 409/410 治理，本文 1.2 节读路径的上游。
- **[[wiki/01-集群基础/03-控制平面/18-api-priority-fairness.md|API Priority and Fairness]]**：APF 机制与配置全解；本文 3.2/4.3 只取延迟与 429 的判读。
- **[[wiki/22-概念/11-交叉分析/apiserver-×-Prometheus.md|apiserver × Prometheus]]**：指标采集与告警配置的落地视角；本文第五章给出定界用的最小指标集。
- **[[wiki/19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]] / [[wiki/19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]**：通用排障 playbook（含完全不可用分支）；本文专注"慢"这一支。
- **[[maintaining/04-报告/k8s-高级面试快问快答-沉淀.md|面试快问快答沉淀]]**：本主题的面试卡 Q9–Q12（排查顺序/NOSPACE/大 LIST/重启风暴）、Q117–Q119（etcd 写路径与磁盘证据/compact 分工/watch 410 与读路径，etcd 内部视角）与 Q126–Q127（apiserver 按段定界/APF 座位模型，apiserver 侧视角）。

## 七、一页速记

| 问题 | 第一证据 | 红线数字 | 动作 |
|---|---|---|---|
| apiserver 整体变慢先查谁 | `etcd_request_duration_seconds`（apiserver 视角） | — | etcd 三件证据 → 再拆 apiserver |
| 盘慢 | `wal_fsync p99` + 日志三连签名 | **10ms**；告警兜底 0.5s | 独立 SSD、events 分库、ionice |
| 空间满 | db total vs in use；`alarm list` | quota **2GiB** 默认 / **8GiB** 上限 | compact → 逐台 defrag → alarm disarm |
| Leader 抖 | `leader_changes_seen_total` / RAFT TERM | 选举窗口内全停写 | 查磁盘/网络 RTT，调心跳超时 |
| 请求慢在哪一段 | 拆 verb + webhook/wait/etcd 三把尺子 | webhook 超时默认 **10s** | audit 时间戳归因到人 |
| 429 是什么 | `flowcontrol_rejected` + 谁在 LIST | 总座位 600（400+200） | 揪大 LIST 客户端，不是扩容 |
| 重启后抖 | LIST 回源 etcd 的尖峰窗口 | >5min = 容量问题 | 分批滚动 + WatchBookmark |
