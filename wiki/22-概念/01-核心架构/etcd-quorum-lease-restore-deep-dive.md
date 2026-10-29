---
sources:
- "wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md"
- "wiki/23-实体/02-K8s核心组件/etcd.md"
- "wiki/22-概念/11-交叉分析/etcd-×-节点诊断.md"
- "wiki/10-平台工程/02-运维/15-lease-leader-election.md"
- "wiki/12-可靠性/01-备份恢复/01-etcd-backup-restore.md"
- "对话沉淀: 2026-10-20 etcd 面试复习 quorum/Lease/恢复实务 讨论"
- "kubernetes.io kube-controller-manager 参考（node-monitor-grace-period 默认 50s，1.32 起，2026-10 核实）"
- "kubernetes.io/blog 2024-08-15 Accelerating Cluster Performance with Consistent Reads from Cache"
- "github.com/etcd-io/etcd CHANGELOG-3.5/3.6（snapshot restore --bump-revision/--mark-compacted，2026-10 核实）"
title: etcd 深挖：quorum 数学、写路径、MVCC 维护、Lease 两层语义与恢复实务
summary: 面向面试追问场景的 etcd 五个深挖点——quorum 交集定理与节点拓扑取舍、写请求全路径与两段磁盘延迟、compact 的真正执行者与 Watch too-old 机理、K8s Lease 与 etcd 原生 Lease 的两层语义、备份恢复的不对称自动化策略与 revision 回退防护。
category: concepts
tags:
- etcd
- raft
- quorum
- mvcc
- compaction
- defrag
- watch
- resource-version
- lease
- node-heartbeat
- leader-election
- snapshot-restore
- linearizable-read
tier: core
created: 2026-10-20
updated: 2026-10-20
last_updated: 2026-10
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 25min
intent_queries:
- quorum 为什么必须过半、为什么奇数节点
- etcd 一次写请求的完整路径和瓶颈
- etcd 的 compact 是 apiserver 做的还是 etcd 自己做
- too old resource version 是什么原理
- K8s 的 Lease 和 etcd 原生 Lease 有什么区别
- kubelet 停心跳后 Lease 会被删吗
- etcd 全量恢复是脚本还是逐台手动
- 恢复后客户端 resourceVersion 倒退怎么办
trigger_keywords:
- quorum
- 多数派
- ReadIndex
- 线性一致读
- compact
- defrag
- renewTime
- kube-node-lease
- event-ttl
- snapshot restore
- bump-revision
- too old resource version
k8s_versions:
- '1.14'
- '1.31'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。etcd 侧操作涉及控制面数据存储：`etcdctl defrag`、`member remove`、`snapshot restore` 属于 🔴 高风险（可能造成服务中断或数据丢失），执行前必须备份、走变更审批并准备回退路径；查询类命令为 🟢 低风险。

# etcd 深挖：quorum 数学、写路径、MVCC 维护、Lease 两层语义与恢复实务

架构全景、参数表、命令手册见 [[wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]] 与 [[wiki/22-概念/01-核心架构/etcd-operational-reference.md|etcd Operational Reference]]，本文不重复；聚焦五个"面试一追问就掉坑"的点：

1. quorum 到底是什么——交集定理，而不是"过半"两个字；
2. 一次写请求的全路径——瓶颈为什么是磁盘 fsync；
3. MVCC 维护——**compact 在 K8s 集群里默认是 kube-apiserver 做的**，以及 Watch "too old resource version" 的完整机理；
4. Lease 的两层语义——K8s Lease 对象（心跳/选主）与 etcd 原生 Lease（Events TTL）毫无关系的两层；
5. 恢复实务——备份全自动、恢复半手动的不对称，以及恢复后 revision 回退的新解法。

---

## 一、quorum：交集定理，不是"过半"两个字

### 1.1 定义与为什么"过半"能保单一真相

quorum（法定多数派）= ⌊N/2⌋ + 1。写要落到多数派才算提交，选主要拿到多数派选票。它真正起作用的是**交集定理**：任意两个多数派集合必有至少一个公共节点。由此推出两条 Raft 核心保证：

- **选举安全性**：一个 term 内最多一个 Leader（两方都当选则必然有公共投票者，矛盾）；
- **Leader 完备性**：已提交的条目必然存在于任何后续 Leader 的日志里（新 Leader 的选票来自多数派，其中至少一人有该条目，日志比较会淘汰没有它的候选者）。

| 集群规模 N | quorum | 容忍故障 f |
|---|---|---|
| 1 | 1 | 0（无高可用） |
| 2 | 2 | 0（**比 1 节点没有任何好处**） |
| 3 | 2 | 1 |
| 4 | 3 | 1 |
| 5 | 3 | 2 |
| 7 | 4 | 3 |

### 1.2 奇数与规模选择的判断线

- **偶数不加容错**：4 节点 quorum=3 只容忍 1，和 3 节点一样，还多一台复制开销；且偶数在选举中更容易平票拖长不可用时间。
- **3 还是 5**：5 节点容忍 2——允许"一台计划内维护 + 一台计划外故障"同时发生，代价是每次写要等 3 台落盘、peer 流量和提交延迟上升。绝大多数集群 3 节点够；跨 3 AZ、SLA 苛刻的大集群上 5。
- **跨 AZ 可以，跨 Region 不要**：默认 heartbeat 100ms / election timeout 1000ms 是按低 RTT 假设给的。跨 Region RTT 30–100ms 时心跳丢包触发重选风暴、提交延迟被 quorum 内最慢节点的 RTT+fsync 主导。正确做法：etcd 留在单 Region 内按 AZ 展开（3 节点 1-1-1，5 节点 2-2-1），确需高 RTT 网络时才调 `--heartbeat-interval` / `--election-timeout`（后者 ≥ 10× 前者）。
- **加分细节**：etcd 3.5 起 `--pre-vote` 默认开启（预选举阶段先确认自己能拿到多数派才 bump term 参选），网络分区恢复的节点不会一回来就凭高 term 顶掉健康 Leader；`etcdctl move-leader` 用于计划内转移；**Learner 成员不参与投票、不计入 quorum**。

---

## 二、一次写请求的全路径与两段磁盘延迟

```
Client ──Put──▶ etcd（Follower 会转发给 Leader）
                  │
                  ▼
        ① Leader 追加 WAL 并 fdatasync      ← etcd_disk_wal_fsync_duration_seconds（p99 > 10ms 告警）
                  │
                  ▼
        ② 并行复制 AppendEntries 给 Follower（各自 WAL fsync）
                  │
                  ▼
        ③ 多数派确认 → commit index 推进     ← etcd_network_peer_round_trip_time_seconds（p99 > 50ms 告警）
                  │
                  ▼
        ④ apply 协程异步应用：写 MVCC（treeIndex + bbolt）
           bbolt 事务批量提交（默认 100ms / 1 万条一批）← etcd_disk_backend_commit_duration_seconds（p99 > 25ms 告警）
                  │
                  ▼
        ⑤ 应答客户端（线性一致读的请求会等 applied ≥ readIndex）
```

要点与坑：

- **两段延迟对应两种病**：`wal_fsync` 高 = 磁盘顺序写慢（换 NVMe/独立盘）；`backend_commit` 高 = bbolt 落盘慢（数据量大/批量积压）。只看 QPS 定位不了。
- **commit ≠ apply**：多数派落盘即 commit，apply 是异步的；`etcd_server_proposals_pending` 高、日志 `apply entries took too long` 说明 apply 跟不上，表现为读延迟和 Watch 事件滞后。
- **磁盘选型红线**：本地 NVMe/SSD、不用网络盘/共享盘（对象存储、NFS 一票否决）；WAL 可用 `--wal-dir` 指到独立盘；上线前用 fio 实测 fsync——etcd 官方口径 `fio --rw=write --ioengine=sync --fdatasync=1 --size=22m --bs=2300 --name=test` 的 99 分位应 < 10ms。
- **"Leader 单点写"不是缺陷**：Raft 通过唯一 Leader 化解并发写冲突，Leader 挂了多数派选出新 Leader 继续写——是以秒级不可用换一致性。

---

## 三、MVCC 维护：谁在 compact、空间什么时候回来、Watch 为什么 too old

### 3.1 revision 模型 30 秒版

每次写（含删）全局 `main revision` 单调 +1；每个 key 在内存 treeIndex（B 树）里维护 `create_rev / mod_rev / version`，bbolt 里按 `(key 的历史 revision → value)` 存储；删除写 tombstone。K8s 对象的 `resourceVersion` 就是 etcd 的 `mod_revision`。**保留全部历史版本**是 Watch 能从任意旧 revision 回放的前提，也是 db 无限膨胀的原因。

### 3.2 compact 的执行者：默认是 kube-apiserver（高频错答）

- **K8s 集群默认链路**：kube-apiserver 内置 compactor，`--etcd-compaction-interval` 默认 **5 分钟**发一次 `etcdctl compact`；多 apiserver 副本通过 etcd 里的一个协调 key（`compact_rev_key`）轮询，同一窗口只有一个副本真正执行。
- etcd 自身的 `--auto-compaction-retention`（periodic/revision 模式）是**另一条独立保险**，kubeadm 默认不配、靠 apiserver 兜底；生产建议两条都留（先到先压，互不冲突）。
- **compact 只清理指定 revision 之前的旧版本**（每个 key 保留 compact 点位时的最新值），清出来的页进入 bbolt freelist 标记可复用——**文件不缩小**。
- **defrag 才把空间还给文件系统**：重写 db 文件，期间该节点不可服务；必须**逐台串行**（一把 `--cluster` 可能同时打掉多数派）；quota 告警（NOSPACE）还要 `alarm disarm` 才恢复写。完整处置顺序见沉淀 Q10。
- 判断碎片还是真大：`etcd_debugging_mvcc_db_total_size_in_bytes`（文件大小）vs `_in_use_in_bytes`（实际使用），差距大 → compact+defrag 有效；两者都贴 quota → 数据真大，调 quota 或治理大对象（Events、超大 ConfigMap/CR）。

### 3.3 Watch "too old resource version" 的完整链路

1. 客户端（Informer 的 Reflector / watch cache）带 `resourceVersion=N` 向 apiserver 续订 Watch；
2. apiserver 的 **watch cache（Cacher）** 对每种资源维护一份内存事件滚动窗口，集群里所有 client watch 都挂在 cache 上，**etcd 只看到每资源每 apiserver 一条 watch**；
3. cache 窗口也接不上（或直接打 etcd）且 N 已被 compact → etcd 返回 `ErrCompacted` → apiserver 转成 **410 Gone "too old resource version"**；
4. 客户端只能**重新 List 全量**再续 Watch——所以 compact 过激/事件风暴后常见一波 list 风暴，大集群要靠 APF 限流挡住。
5. 缓解：Watch bookmark（bookmarks 事件让客户端及时前移 RV，减少 410）；1.27+ WatchList（流式 list）进一步减少全量 list。

### 3.4 resourceVersion 三种语义与 ConsistentListFromCache

| 请求 RV | 含义 | 读路径 |
|---|---|---|
| `""`（默认 get/list） | 最新（线性一致） | 历史上直打 etcd quorum read |
| `"0"` | 任意旧值即可 | apiserver watch cache（可能略旧） |
| `"N"` | 不旧于 N | watch cache，追不上报 410/超时 |

- **ConsistentListFromCache**（KEP-2340，1.30 Alpha、**1.31 起 Beta 默认开启**）：RV="" 的 list 也从 watch cache 服务，但先用 etcd 的 **progress notify** 确认 cache 已追上 etcd 当前 revision 再返回，保住线性一致的语义，同时把大 list 从 etcd 挪到 apiserver 内存——大型集群 etcd 减负的关键特性（GA 在 1.34/1.35 推进中）。
- 面试落点：**"kubectl get 读的是 etcd 还是缓存"取决于 RV 语义**——默认 `get` 要最新（ quorum read，1.31+ 默认从 cache 服务但等价一致），`--resource-version=0` 是"有就行"。

---

## 四、读一致性：ReadIndex 与串行读

- **线性一致读（默认）**：Leader 把当前 commitIndex 记为 readIndex → 带着它走一轮心跳向多数派**确认自己仍是 Leader** → 等 appliedIndex ≥ readIndex → 读 bbolt。代价是一次 RTT，**不需要写盘**（Follower 上读要先向 Leader 要 readIndex）。
- **串行读**：`etcdctl get --consistency=s`（客户端 `WithSerializable`）直接读本地状态，可能旧值，零网络开销——适合监控轮询等不敏感场景。
- 相关指标：`etcd_server_slow_read_indexes_total`、`read_indexes_failed_total`（都应 ≈ 0，持续增长说明网络/apply 有问题）。
- 与 3.4 节合起来的完整口径：**K8s 读链路的一致性由 "apiserver cache + etcd progress notify + ReadIndex" 三者拼出来**，这也是近年大集群可扩展性的主线（把读从 etcd 挪走，但不牺牲语义）。

---

## 五、Lease 的两层语义（最容易混的一组概念）

### 5.1 K8s Lease 对象 · 节点心跳

Lease 是普通 API 对象（`coordination.k8s.io/v1`），和 Pod 一样经 apiserver 存进 etcd，key 形如 `/registry/leases/kube-node-lease/<node>`——**节点租约在 `kube-node-lease` namespace，一个节点一个，名字同节点名**。

时间线（默认值）：

```
kubelet 每 10s（leaseDuration 40s 的 1/4）更新一次 renewTime
  NodeLifecycleController watch Lease，renewTime 停更超过
  --node-monitor-grace-period（默认 40s，K8s 1.32 起改为 50s）
    → Node Ready=Unknown，打 node.kubernetes.io/unreachable taint
    → Taint 管理器按 Pod tolerationSeconds（默认 300s）驱逐
    → 工作负载控制器在其他节点重建
节点恢复：kubelet 续同一个 Lease（对象从没被删过），taint 移除
```

- **KEP-0009（NodeLease，1.14 GA）的动机是写入放大**：旧机制 kubelet 每 10s 全量更新 Node status（conditions/volumes/镜像列表，几十 KB），5000 节点集群把 apiserver/etcd 写穿；Lease 只有几十字节，status 降为 5 分钟一次或变更时上报。租约或 status 任一更新都算心跳。
- 因果链：**etcd 磁盘慢 → renewTime 写不进去 → 被误判 NotReady**——心跳故障的第一嫌疑人可能不是网络，见 [[wiki/22-概念/11-交叉分析/etcd-×-节点诊断.md|etcd × 节点诊断]]。
- 现场核验：`kubectl -n kube-node-lease get lease <node> -o yaml` 看 `renewTime`。

### 5.2 K8s Lease 对象 · 组件选主

同一对象机制的第二个用法：kube-scheduler / KCM / controller-runtime operator 在 `kube-system` 持 Lease（如 `kube-system/kube-scheduler`），`holderIdentity` 写自己。参数默认 **leaseDuration 15s / renewDeadline 10s / retryPeriod 2s**；抢占靠 Lease 对象的 resourceVersion 乐观锁（Update CAS），旧 Leader 续不上就自杀停写，备副本接管。1.14 之前的选主锁放在 Endpoints/ConfigMap 注解里，已属历史考点。细节见 [[wiki/10-平台工程/02-运维/15-lease-leader-election.md|Lease 与 Leader 选举]]。

### 5.3 etcd 原生 Lease · Events 的 TTL

etcd 自己的 Lease 是另一回事：`LeaseGrant(TTL)` → `Put` 时 `WithLease` 绑定 → 客户端 KeepAlive 流续约 → **到期由 etcd 服务端自动删除绑定的 key**。K8s 里真用它的是 **Events**：kube-apiserver 的 `--event-ttl`（默认 1h）到期后事件自动消失——不是 controller 删的，是 etcd 存储层（etcd3 store 的 leaseManager）把带 TTL 的对象挂到原生 Lease 上实现的；leaseManager 还会把 TTL 相近的对象**复用同一个 Lease**（默认 60s 复用窗口）以控制 Lease 数量。可用 `etcdctl lease list` / `lease timetolive` 查看。

### 5.4 为什么节点心跳不用 etcd 的 TTL 自动过期

1. **控制器模式**：过期后的动作（打 taint、按 tolerationSeconds 分级驱逐、恢复回滚）是业务语义，etcd TTL 只会"删 key"；
2. **声明式不搞隐式删除**：状态变更收敛到控制器做判定，数据不悄悄消失，节点恢复还要续同一个 Lease；
3. **参数语义**：宽限期（40/50s）、驱逐时长（300s）都是 K8s 层可调的运维参数；
4. **时钟归属**：TTL 由 etcd 服务端计时，控制器方案里判定逻辑和观测（renewTime 字段）都在同一层，好排障。

### 5.5 两层对照表

| | K8s Lease 对象 | etcd 原生 Lease |
|---|---|---|
| 本体 | coordination.k8s.io 对象（存 etcd） | etcd gRPC 服务端原语 |
| "过期"谁判定 | 控制器比对 renewTime（KCM / 选主等待者） | etcd 服务端 TTL 到期 |
| 过期动作 | 置 NotReady+驱逐 / 选主转移 | 自动删除绑定的 key |
| K8s 用途 | 节点心跳、组件选主 | Events TTL（--event-ttl） |
| 会不会删对象 | 永不（只是 renewTime 变旧） | 会（删 attach 的 key） |

---

## 六、恢复实务：备份全自动、恢复半手动

### 6.1 自动化程度为什么不对称

- **备份**每小时甚至每 5–15 分钟一次，生命周期内成千上万次——必须 CronJob/系统定时器全自动，且要监控备份任务本身、定期验证快照（`etcdutl snapshot status` 看 REV/KEYS）、跨 AZ/介质存放。
- **全量恢复**生命周期可能只发生一两次、不可逆、每台参数不同、中间有必须人工验收的门禁——**runbook 驱动半手动**：脚本只负责按成员清单生成每台的命令，人逐台执行；可靠性靠季度演练验证，不靠"写好了恢复脚本"。

### 6.2 全量恢复 runbook（kubeadm/静态 Pod 版，含门禁）

```
门禁 0  变更窗口+双人复核；确认快照一致性（三台用同一份、snapshot status 验 REV/KEYS）
① 冻结写入：挪走 /etc/kubernetes/manifests/{kube-apiserver,kube-controller-manager,kube-scheduler}.yaml
   （静态 Pod 被删即停）；若 apiserver 尚存活，先做一次最新 snapshot
② 留退路：每台 mv 旧 data-dir（如 /var/lib/etcd → /var/lib/etcd.bak-<ts>），绝不原地删
③ 逐台 restore（etcd 3.5 用 etcdutl；注意是"新 data-dir、新 cluster ID"）：
   etcdutl snapshot restore /backup/etcd-<ts>.db \
     --name etcd-<i> --data-dir /var/lib/etcd \
     --initial-cluster etcd-1=https://n1:2380,etcd-2=https://n2:2380,etcd-3=https://n3:2380 \
     --initial-advertise-peer-urls https://n<i>:2380
   （三台的 --name/--peer-urls 各不相同；3.5.x 后期起可用 --bump-revision/--mark-compacted，
    见 6.3）
④ 门禁：逐台启动 etcd（manifest 放回），第一台成为单成员 Leader、后两台加入；
   etcdctl endpoint health --cluster + endpoint status 确认 quorum、revision 与快照一致
⑤ 恢复 apiserver 等组件 manifest，确认 /healthz、kubectl get all -A 抽查各 ns
⑥ 观察窗口：informer 全量 relist、控制器收敛、事件与 Watch 无 410 风暴
```

### 6.3 恢复后的 revision 回退问题（进阶加分点）

快照恢复会把 revision 拨回快照时刻——比故障前**小**。apiserver watch cache、各 Informer 手里还握着更高的 RV，续订会撞上"revision 倒退"，轻则 410 风暴重则读到不一致视图。防护两招：

- 流程兜底：恢复后**重启所有 apiserver 与依赖 informer 的控制器**（KCM/scheduler/自研 operator），强制全量 relist；
- 版本解法：etcd 3.5 后期补丁起（CHANGELOG-3.5 记录，3.6 移入 etcdutl）`snapshot restore` 提供 **`--bump-revision N`**（把 revision 抬到高于故障前，如 +10⁹）与 **`--mark-compacted`**（把旧区间标为已 compact，旧 RV 客户端直接收到 410 触发 relist）。

### 6.4 单节点故障：换节点，不是恢复

3 节点坏 1，quorum 仍在、服务不受影响，但再坏一台就失 quorum——**时间窗口要快**，且修复路径是成员替换而非快照恢复：

```
etcdctl member remove <bad-id>                 # 先摘，避免同名/同 peer-urls 冲突
etcdctl member add etcd-new --learner --peer-urls=https://new:2380
# 新节点空 data-dir + --initial-cluster-state=existing 启动，追平日志
etcdctl member promote <new-id>                # Learner 不计票，追平后转正
```

**Learner（etcd 3.4+）存在的意义**：新成员追日志期间若立即计票，会拉低多数派的有效性（大集群快照传输还可能拖垮 Leader），先做非投票成员是安全阀。3 节点同时坏 2 的全量灾难路径见沉淀 Q1（`--force-new-cluster` 红线 + RPO 交代）。

---

## 七、部署拓扑与参数速查

- **stacked etcd（kubeadm 默认）**：etcd 与控制面同节点，省机器，但控制面变更/资源争用直接波及 etcd；**external etcd**：独立集群，隔离好、可单独扩，多一套 TLS 与主机成本。云上托管版（如 ACK 托管集群）etcd 由云侧运维，客户侧考点转为备份策略与配额观测。
- 拓扑铁律：奇数节点、单 Region 跨 AZ 展开（3→1-1-1，5→2-2-1）、独占磁盘、不与其他重 IO 负载同盘。
- 高 RTT 网络才动 `--heartbeat-interval`/`--election-timeout`（默认 100ms/1000ms，>10ms RTT 建议 500ms/5000ms，跨 Region 1000ms/10000ms——但优先别跨 Region）。
- 空间规划：`--quota-backend-bytes` 默认 2GB、生产 8GB（官方建议上限）；db 达 quota 的 80% 告警。规模估算见 [[wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]] §3.3。

---

## 八、面试口径映射（沉淀卡索引）

| 主题 | 沉淀卡 |
|---|---|
| 3 节点挂 2 失 quorum、force-new-cluster 红线、RPO | Q1 |
| NOSPACE 三步处置（compact→defrag→disarm） | Q10 |
| quorum 交集定理、奇偶、3 vs 5、跨 Region | Q116 |
| 写路径全流程、两段磁盘延迟、fio 验盘 | Q117 |
| compact 执行者（apiserver 5m）、碎片判断 | Q118 |
| too old resource version、watch cache、RV 三语义 | Q119 |
| Lease 存哪、停心跳不删、40s→50s 版本线 | Q120 |
| K8s Lease vs etcd 原生 Lease、Events TTL | Q121 |
| 线性读 ReadIndex vs 串行读、ConsistentListFromCache | Q122 |
| 恢复实务、revision 回退、bump-revision | Q123 |
| 单节点换成员、Learner、member 操作红线 | Q124 |
| stacked vs external、跨 AZ 拓扑、参数调优 | Q125 |

---

## 九、相关概念

- [[wiki/23-实体/02-K8s核心组件/etcd.md|etcd 实体总览]]
- [[wiki/01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]] — 架构/参数/运维手册全景
- [[wiki/01-集群基础/02-设计原则/08-distributed-consensus-etcd.md|分布式共识与 etcd]] — Raft 协议细节
- [[wiki/22-概念/01-核心架构/etcd-operational-reference.md|etcd Operational Reference]] — 运维口径
- [[wiki/22-概念/01-核心架构/watch-mechanism.md|Watch 机制]] 与 [[wiki/01-集群基础/02-设计原则/05-watch-list-mechanism.md|List-Watch 机制深度解析]]
- [[wiki/01-集群基础/02-设计原则/07-resource-version-control.md|资源版本控制]] — RV/乐观并发/410 治理
- [[wiki/10-平台工程/02-运维/15-lease-leader-election.md|Lease 与 Leader 选举]] — 选主参数与抢占流程
- [[wiki/22-概念/11-交叉分析/etcd-×-节点诊断.md|etcd × 节点诊断]] — 心跳误判案例
- [[wiki/22-概念/07-调度与资源/node-taint.md|Node Taint 与污点驱逐]]
- [[wiki/12-可靠性/01-备份恢复/01-etcd-backup-restore.md|etcd 备份与恢复实战手册]]
- [[wiki/12-可靠性/02-灾难恢复/14-etcd-corruption-recovery-playbook.md|etcd 损坏恢复 Playbook]]
- [[wiki/13-生产运维/05-工单案例/ticket-case-009-etcd-disk-full-apiserver-slow.md|工单案例 009：etcd 磁盘满与 apiserver 变慢]]
- [[wiki/23-实体/02-K8s核心组件/kube-controller-manager.md|kube-controller-manager]] — NodeLifecycleController 宿主
