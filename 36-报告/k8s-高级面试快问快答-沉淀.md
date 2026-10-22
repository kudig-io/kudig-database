# K8s 高级面试 · 快问快答沉淀

> 来源：`k8s-ops-quick-qa` 快问快答会话。每问答完即沉淀一条，格式同答案卡。
> 开始日期：2026-10-13

---

## Q1. etcd 3 节点挂 2，失去 quorum 怎么办？

**一句话**｜Raft 需多数派，3 挂 2 已无 quorum，etcd 拒写直至读超时——绝不能 `--force-new-cluster` 硬拉。

**30 秒答**｜先 `etcdctl endpoint status --cluster -w table` 看 IS LEADER / RAFT TERM / DB SIZE，`etcdctl member list -w table` 看成员状态；幸存节点日志出现 `lost quorum`、apiserver 报 `etcdserver: request timed out` 即确认。强制单节点启动会让旧数据成为唯一真相，等另两台回来时更高 revision 被覆盖 → 数据不可逆丢失。正确路径：先 snapshot 备份现有数据目录，再二选一——(a) 单台数据完好且业务接受 RPO → 用它的 snapshot `etcdctl snapshot restore` 到新集群；(b) 有周期性快照 → restore 到最近备份，丢失窗口内变更靠 GitOps 重放。恢复后 `kubectl get all -A` 逐集群校验。判断线：这是"数据完整性 vs 可用性"的选择题，先问业务能接受丢多少。

**弹药**
- `ETCDCTL_API=3 etcdctl --endpoints=... --cacert=... --cert=... snapshot save /tmp/esnap.db`
- `etcdctl snapshot status /tmp/esnap.db -w table`（先验 REV/TOTAL KEY，快照不是空的）
- `snapshot restore` 出来的是新 data dir、新集群 ID，不能原地塞回旧目录
- 备份频率口径：关键集群每 5–15 分钟一次 + 变更前手动一次

**红线**｜❌ 赌 `--force-new-cluster`；❌ 没有事前 snapshot 还答"我们有每小时备份"（这题及格前提就是备份频率说得出口）。

**下一问**｜丢了 12 分钟 etcd 数据，怎么算 RPO、怎么向业务交代？

---

## Q2. OOMKilled，但容器 limit 根本没打满，为什么？（负载）

**一句话**｜先分清是 cgroup OOM（容器超了自己的 limit）还是节点 OOM（内核挑倒霉蛋），90% 候选人栽在没分这两层。

**30 秒答**｜`kubectl describe pod` 看 `lastState.terminated.reason`，是 `OOMKilled` 才落在 cgroup 层。limit 1Gi、RSS 才 600Mi 就被杀，三种典型原因：①page cache 被算进 cgroup 用量，RSS 低但 cache 把 limit 顶满；②瞬时尖峰，监控 30s 采样采不到秒级毛刺；③节点级压力驱逐——节点没内存时 kubelet 按 requests 优先级挑受害者，表现是 `Evicted` 而不是 `OOMKilled`。判断线：节点 OOM 查 `dmesg -T | grep -i oom` 和 `MemAvailable`；Pod 只有 Evicted 没有 OOMKilled → 一定是节点层，别再看容器了。

**弹药**
- `dmesg -T | grep -i "killed process"` — 节点 OOM 铁证
- `kubectl exec <p> -- cat /sys/fs/cgroup/memory.current` 对比 `memory.stat` 里的 `file`（cache 占用）
- Java：`-Xmx` 必须 < limit，堆外（direct buffer / metaspace）不在 -Xmx 内，最容易超
- `kubectl get events --field-selector reason=Evicted` 区分驱逐 vs OOM

**红线**｜❌ "把 limit 调大就行"——掩盖问题，下次搞挂节点；❌ 分不清 cgroup OOM 和 node OOM。

**下一问**｜怎么自动识别"节点 OOM 但 cgroup 没到"这种场景？（→ 关联节点 MemAvailable 与 Pod 事件）

### Q2 深挖：RSS 是什么 & 为什么 RSS 远小于 limit 还被 OOMKilled

**RSS（Resident Set Size）**= 进程此刻实际占用物理内存的页总和。层次：
`VSZ（虚拟，无参考意义） ⊃ RSS（常驻物理页） ⊃ USS/PSS（扣掉共享库页，才是容器独占）`。
RSS = 匿名页（heap/stack，换不出去）+ 文件页（so 库、文件读写缓存）。

**cgroup 的账本比 RSS 大**：
```
memory.current = anon + file(page cache，charge 给发起容器) + kernel(内核栈/slab/socket buffer) + 共享页
```
所以 RSS 600Mi 时 `memory.current` 可能早已逼近 1Gi。三个吃余量的口子：
① 容器内文件读写（写日志、读大文件、/dev/shm）产生的 page cache 全记账在容器头上；
② 内核开销：线程内核栈(16KiB×线程数)、slab(dentry/inode)、socket 队列，Java 高并发轻松几十 Mi；
③ 30s 采样监控抓不到的秒级 anon 尖峰。

**触发链**：memory.current 撞 limit → 内核先 reclaim（踢 cache）→ 回收不动（涨的是 anon）→ cgroup OOM killer 杀最大进程 → exit 137，kubelet 记 OOMKilled。

**三层账本对照**：
| 层 | 账本 | 命令 | 超限表现 |
|----|------|------|---------|
| 进程 | RSS | `ps -o rss` / top | 无直接表现 |
| cgroup | memory.current | `cat /sys/fs/cgroup/memory.current` + `memory.stat`(anon/file/kernel 分账) | **OOMKilled** (137) |
| 节点 | MemAvailable | `free -m`; `dmesg -T \| grep -i oom` | **Evicted** / kernel OOM |

**现场三步**：① `describe pod` 看 lastState 是 OOMKilled 还是 Evicted；② 容器内拆 `memory.current` 与 `memory.stat`（file 大=cache 吃的，anon 大=应用真涨）；③ `dmesg` + `free -m` 排除节点层。

**根治**：`-Xmx` < limit（堆外 metaspace/direct buffer 不在 -Xmx 内）；日志走 stdout 或独立 emptyDir；limit 给 1.5× requests 且 ≤ 节点可分配；对 `memory.current/limit` 建 >85% 告警（比看 RSS 早）。

---

## Q3. Service 不通，怎么查？（网络）

**一句话**｜按 `Service → Endpoints → Pod → kube-proxy → NetworkPolicy → DNS` 六层往下切，每层都有独立证据。

**30 秒答**｜先分清**是访问不通还是解析不通**：能解析出 ClusterIP 说明 DNS 这层过了。第一步 `kubectl get svc` 看 type/端口/targetPort，第二步 `kubectl get endpoints <svc>`——**Endpoints 为空，问题一定在 Service 侧，不用再看网络**。Endpoints 有内容，就从同集群另一个 Pod 里 `curl <podIP>:<containerPort>` 绕开 Service 验证：Pod IP 通、Service IP 不通 → kube-proxy/Service 网络编程层；再不通 → NetworkPolicy；最后才怀疑 CNI。判断线：始终用 Pod IP 做对照实验，有对照就能把"应用挂了"和"网络挂了"分开。

**弹药**
- `kubectl get endpoints <svc>` — 最快的一刀
- `kubectl exec -it <dbg> -- curl -sv <podIP>:<port>` — 绕开 Service 的对照实验
- `kubectl get svc -o wide` — 对 selector 和 Pod label
- 节点上 `ipvsadm -Ln | grep <ClusterIP>`；iptables 模式看 `iptables -t nat -L KUBE-SERVICES -n`

**红线**｜❌ "重启 CoreDNS 试试"——Endpoints 为空跟 DNS 一点关系都没有，先拿证据再动手。

**下一问**｜Endpoints 是空的，为什么？（→ selector 不匹配 / Pod 没过 readiness；只有 readiness 影响 Endpoints，liveness 挂的是重启）

---

## Q4. 线上出故障，你回滚还是继续排查？（变更决策）

**一句话**｜先止血再定位——回滚是第一止血手段，除非回滚本身有风险。

**30 秒答**｜判断顺序三步：（1）故障发生在变更后（或变更后 30 分钟内）吗？是 → **默认回滚**，不要辩护"这个变更不可能引起"；（2）回滚有没有副作用？**有状态变更（DB schema、数据迁移、外部方已消费的消息）不能简单回滚** → 改用开关（feature flag）、扩容、降级、限流止血；（3）不可回滚又没有开关 → 前滚修复，但要立刻拉人、把时间预算说清楚。关键原则：**恢复优先于根因**，MTTR 是结果指标，根因留给复盘。判断线：能说出"我不确定是不是这个变更引起的，但回滚成本比排查成本低，所以先回滚"——这就是资深 SRE 的判断。

**弹药**
- `kubectl rollout undo deployment/<x> --to-revision=<n>`（先 `rollout history` 确认目标版本可用）
- GitOps 回滚 = 回滚 Git 提交 + 同步，别 `kubectl edit` 手改（与仓库漂移，下次同步被覆盖）
- 回滚后保留现场：cordon 节点、别删故障 Pod（`logs --previous` / heapdump）
- 止血工具箱：降级、限流、扩容、切流（跨 AZ/集群容灾）

**红线**｜❌ 故障中 `kubectl edit` 改线上——无记录、无回滚点、和 GitOps 冲突；❌ 没搞清依赖就回滚数据库（新版已写入不兼容数据，回滚 DB = 数据损坏）。

**下一问**｜回滚之后告警还在，怎么判断是第二个故障还是余波？

---

## Q5. 节点 DiskPressure，Pod 开始被驱逐，怎么办？（节点/存储）

**一句话**｜先分清是根盘满还是 imagefs（容器运行时空间）满——90% 是后者，且是镜像和日志没人清。

**30 秒答**｜kubelet eviction 阈值针对 `nodefs.available`、`imagefs.available`、`memory.available`；imagefs 就是 containerd 根目录（`/var/lib/containerd`），**镜像 GC 只在磁盘吃紧时才激进回收**，平时能堆到上百 GB。确认：`kubectl describe node` 的 Conditions 里 `DiskPressure=True`，Events 有 `FreeDiskSpaceFailed`。处置顺序：① `crictl rmi --prune` 清未使用镜像（最见效）→ ② 清容器日志（`/var/log/pods`，查 `json.log.*` 是否有 size limit）→ ③ 清已退出容器（`crictl rm $(crictl ps -a -q --state Exited)`）→ ④ 再考虑扩盘。判断线：`df` 满但 `du` 加起来对不上 → **有进程持有已删除文件的句柄**，`lsof +L1` 找出来重启它。

**弹药**
- `df -h / /var/lib/containerd /var/lib/kubelet` 分别看（三个可能不同盘）
- `du -sh /var/lib/containerd/* /var/log/* | sort -h | tail`；`crictl imagefsinfo`
- kubelet：`--image-gc-high-threshold`(默认85) / `--image-gc-low-threshold`(默认80)；日志 `containerLogMaxSize`(默认10MB) + `containerLogMaxFiles`
- 驱逐是软隔离，不保证业务无损——节点级故障靠提前告警，不靠等驱逐

**红线**｜❌ `rm -rf /var/lib/containerd/*`——containerd 元数据与实际不一致，节点直接废掉，必须用 `crictl rmi`。

**下一问**｜驱逐（eviction）和 OOMKilled 是同一回事吗？（→ 驱逐是 kubelet 按策略主动丢 Pod，OOM 是内核 cgroup 杀进程）

---

## Q6. Prometheus 重启后丢了 6 小时数据，为什么？（可观测）

**一句话**｜Prometheus 本地 TSDB 是可重建的临时缓存，不是存储系统——把它当长期存储用才是根因。

**30 秒答**｜机制：数据先写 WAL（`wal` 目录，2h 一段切 block，产生 checkpoint），重启时 replay WAL 恢复未 compaction 的数据。丢 6 小时的可能：①**WAL 目录和 storage 不同盘 / 挂载丢了（用了 emptyDir 而不是 PVC）**；②block 损坏；③WAL 超出 replay 上限；④进程被 OOM 杀时正在写没落盘。先确认事实：`kubectl get pod -o jsonpath='{.spec.volumes}'` 看 `--storage.tsdb.path` 挂的是不是持久卷——**"重启后数据没了"的第一嫌疑就是没挂 PVC**。缓解：从双副本的另一实例查、recording rule 重算可推导指标、从 remote write 下游（Mimir/Thanos）回捞。长期：remote write → 长期存储 + 对象存储归档，本地只留 2–15 天热数据。

**弹药**
- `promtool tsdb analyze /data/prometheus` — 看 block 和 WAL 结构
- `--storage.tsdb.retention.time`（默认 15d）；指标 `prometheus_wal_written_bytes_total`、`prometheus_remote_storage_samples_failed_total`
- 高可用标准做法：**规则相同的双副本**（prometheus-operator `replicas=2`），不是靠存得久
- remote write 下游挂了不会立刻丢（WAL 重放兜底），但有上限

**红线**｜❌ "我们存一年所以没问题"——存得久 ≠ 不丢，单副本 + 本地盘 = 必然丢；❌ remote write 不做失败监控，要用时才发现链路早断了。

**下一问**｜remote write 持续失败，怎么在丢数据之前知道？

---

## Q7. 1000 节点集群，5 分钟内 200 节点同时 NotReady，怎么查？（控制面/节点）

**一句话**｜先判断这是集群级公共依赖故障而不是 200 次独立故障——2 分钟内必须定出"控制面 / 网络 / kubelet"哪一层。

**30 秒答**（步骤清单式）：
```
1. 拿故障节点名单，看共同点
   kubectl get nodes | grep NotReady
   → 名单跟机房/机架对齐 → underlay/CNI
   → 名单跟版本批次对齐 → 最近变更
   → 完全随机 → 往下

2. 确认控制面活着（必查）
   kubectl get --raw /readyz
   etcdctl endpoint health --cluster
   → 控制面挂 = 所有 kubelet 心跳同时失联，表现和节点挂一模一样

3. 随便挑一台故障节点看 kubelet 日志
   journalctl -u kubelet --since "-10m"
   → PLEG is not healthy       → 本机问题
   → node status update failed → 控制面问题（回第 2 步）
   → 证书错误                   → 查证书过期
```
判断线：200 台"同时"挂不可能是巧合——先找共同点（机房/版本/控制面），再看单台。

**弹药**
- `kubectl describe node <n> | grep -A10 Conditions`（Ready=False 的 reason：`KubeletNotReady` / `NodeStatusUnknown`）
- 心跳口径：kubelet 默认 **10s 一报**，controller-manager `node-monitor-grace-period` 默认 **40s**——"Unknown" 至少 40s 没心跳，**不代表 kubelet 死了**（幻象 Pod：Pod 还在跑但控制面认为节点没了）
- NotReady 后 taint manager 计时 `node.kubernetes.io/unreachable:NoExecute` **默认 300s** 开始驱逐
- `kubectl get events -A --field-selector reason=NodeNotReady --sort-by=.lastTimestamp | tail -30`

**红线**｜❌ 上来批量 `kubectl delete node` 或重启 kubelet——驱逐+重建同时打爆调度器和镜像仓库，制造第二场故障。

**下一问**｜200 节点恢复后大量 Pod 同时重建，你担心什么？（→ 调度器排队 + 镜像集中拉取打爆仓库/磁盘 + 应用连接风暴）

---

## Q8. Pod / PVC 卡在 Terminating，怎么办？（节点/存储）

**一句话**｜Terminating 不是"慢"，是**有人不放行**：kubelet 清理失败（卷卸载不掉）或 finalizer 没人摘。

**30 秒答**（步骤清单式）：
```
1. 先分清卡的是 Pod 还是 PVC（处理路径完全不同）

2. Pod 卡 Terminating → 查节点侧清理
   节点上 ps -eo stat,cmd | grep '^D'   → D 状态 = IO 挂死
   dmesg | tail                          → NFS not responding / I/O error
   journalctl -u kubelet | grep -i unmount
   → 常见根因：卷 unmount/detach 失败（NFS/云盘 IO 挂住）
     或容器进程收了 SIGKILL 也不退

3. PVC/PV 卡 Terminating → 查 finalizer
   kubectl get pvc <p> -o jsonpath='{.metadata.finalizers}'
   → kubernetes.io/pvc-protection = 还有 Pod 在用
   → kubernetes.io/pv-protection  = provisioner 没删完
   → 先解决"谁还在用"，别急着摘

4. 实在要手摘 finalizer → 先确认底层资源
   必须记录卷 ID、人工确认后端云盘已删
   → 不然云盘变孤儿（继续扣钱）或 provisioner 永远忘记它
```
判断线：**finalizer 能不能手摘，取决于摘了之后底层资源会不会泄漏**。

**弹药**
- `kubectl get pod <p> -o jsonpath='{.metadata.deletionTimestamp} {.status.phase}'`（有 deletionTimestamp 但没进 Succeeded/Failed = 卡住）
- 节点侧：`mount | grep <volume-handle>`、`lsblk`
- CSI 侧：`kubectl logs -n <csi-ns> <csi-attacher pod>`；attach/detach 由 external-attacher 驱动

**红线**｜❌ 把 `kubectl delete --force --grace-period=0` 当日常手段——只绕过 apiserver，节点上的卷和挂载还在，之后出更恶心的问题。

**下一问**｜什么时候允许自己用 `--force`？（→ 节点确认已隔离/下线，且业务能容忍 Pod 可能还在跑）

---

## Q9. apiserver 整体变慢，排查顺序？（控制面/apiserver）

**一句话**｜etcd 是第一嫌疑人——所有写和 quorum 读都过它——先看 etcd 的 fsync 延迟、db size、Leader 状态，最后才看 apiserver 自身的队列和慢 webhook。

**30 秒答**（排查顺序清单式）：
```
为什么 etcd 排第一：apiserver 每个写请求都要过 Raft 多数派落盘，
不带 resourceVersion 的 LIST（kubectl 默认）也是 quorum 读直打 etcd；
etcd 慢 = 写慢 + LIST 慢 + watch 事件晚发 = 全集群控制器一起慢。

1. 不用登 etcd 节点，先从 apiserver metrics 拿第一手证据
   kubectl get --raw /metrics | grep etcd_request_duration_seconds

2. 登 etcd 一屏看三件事
   etcdctl endpoint status --cluster -w table
   → DB SIZE（逼近 quota？） / IS LEADER（leader 在哪台） / RAFT TERM（涨 = 在选举）

3. fsync 延迟（官方红线 10ms，SSD 正常 1–5ms）
   etcd_disk_wal_fsync_duration_seconds P99 > 10ms = 慢盘
   和容器/日志共盘、HDD、网络盘，是 fsync 飙高的三大来源

4. Leader 稳定性
   etcd_server_leader_changes_seen_total 近期 >0 = 磁盘抖/网络分区
   leader 一换，所有写停摆一小段，apiserver 表现为批量超时

5. etcd 干净了再看 apiserver 自身
   apiserver_request_duration_seconds 按 verb 拆：写+LIST 慢 → 回 etcd；
   GET 也慢、429 频发 → APF 队列 / 内存 / admission webhook
   apiserver_admission_webhook_admission_duration_seconds 找慢 webhook
```
判断线：**写和 LIST 慢 → etcd；缓存读也慢且伴 429 → apiserver 自身过载；只有个别资源慢 → 查那个资源的准入 webhook/CR 转换**。

**弹药**
- `histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) by (verb)` — 拆 verb 是定界的第一动作
- `etcd_server_proposals_failed_total` 涨 = 丢 leader/丢 quorum/提交超时
- 扩 apiserver 副本只分摊连接和缓存读，写还是全压同一个 etcd leader——etcd 没病时加副本才有意义
- watch 事件延迟会伪装成"控制器失灵"：Deployment 等 Pod 不出，先看 `etcd_request_duration` 再查 controller-manager

**红线**｜❌ 一变慢就扩 apiserver 副本——写瓶颈在 etcd，加多少副本都白加；❌ 不拆 verb 看总 P99 就下结论——写慢和读慢的根因在两个组件；❌ etcd 没查就重启 apiserver——把最可能的线索抹了。

**下一问**｜etcd db size 逼近 quota，报 `mvcc: database space exceeded`，怎么救？（→ compact + defrag + alarm disarm 三步，缺一步都恢复不了）

---

## Q10. etcd 报 `mvcc: database space exceeded`，怎么救？（控制面/etcd）

**一句话**｜db size 撞 quota（`--quota-backend-bytes` 默认 2GB，官方建议上限 8GB）→ etcd 拒写；救法固定三步：compact 清旧 revision → 逐台 defrag 还空间 → `alarm disarm` 解除告警。

**30 秒答**（步骤清单式）：
```
1. 确认状态
   etcdctl alarm list        → NOSPACE（此时只拒写，读还能用）
   etcdctl endpoint status --cluster -w table   → DB SIZE 贴着 quota

2. 看是真大还是碎片
   etcd_debugging_mvcc_db_total_size_in_bytes      vs  _in_use
   → in use 小：compact+defrag 就能救；in use 也贴顶：数据真的大

3. compact：清掉老 revision（k8s 里大户是 events + 删掉对象的残留版本）
   rev=$(etcdctl endpoint status -w json | jq -r '.[0].Status.header.revision')
   etcdctl compact $rev

4. defrag：必须逐台做，等一台健康再下一台
   etcdctl --endpoints=<m1> defrag
   → defrag 期间该节点不可写，--cluster 一把梭会打掉 quorum

5. 解告警：quota 恢复不会自动消 alarm，不清掉继续拒写
   etcdctl alarm disarm
```
判断线：in use 也贴顶时，先删事件洪流（`--event-ttl` 默认 1h、开 EventRateLimit），再考虑上调 quota 到 8GB——**先瘦身再扩容，否则 8GB 只是缓刑**。

**弹药**
- `--auto-compaction-retention`（原生 etcd 默认 0 = 关）管 revision 堆积；k8s 里 compaction 实际由 apiserver 的 `--etcd-compaction-interval`（默认 5m）驱动（Q118 展开），但 **compact 只清版本不缩文件**——defrag 永远没有自动机制，例行维护必须自己做
- `etcdctl compact <rev>` 的 rev 用当前 revision 即可，compact 掉的只是"此之前的旧版本"，不影响现存对象
- watch 客户端停在 compact 掉的 revision 会收到 410 `too old resource version`，被迫重新 LIST——compact 后的 LIST 小尖峰是正常余波
- `--max-request-bytes` 默认 1.5MB，防单对象过大写爆 db

**红线**｜❌ compact 后不做 defrag 就宣布修好——文件不缩，很快再撞；❌ defrag `--cluster` 一起做——三台同时不可写，直接丢 quorum；❌ 撞 NOSPACE 后删数据目录重建——把存储故障升级成数据丢失事故。

**下一问**｜apiserver 打 429、内存尖峰，怎么定位是哪个客户端干的？（→ 大 LIST 和 event 洪泛）

---

## Q11. apiserver 打 429、内存尖峰，怎么定位是哪个客户端干的？（控制面/apiserver）

**一句话**｜九成是大 LIST——全表扫、不分页、不开 watch cache 的客户端；用 audit log 的 responseSize 和 `apiserver_storage_objects` 找大户。

**30 秒答**｜先定性：429 来自 APF（API Priority and Fairness）限流，说明有客户端把队列占满了，最常见的占队列姿势就是**全量 LIST 大资源**（几十万 events/Pod）。定位三刀：① `apiserver_storage_objects` 看哪个资源对象最多——events 十有八九第一，一条 `kubectl get events -A` 不分页就是全表；② audit log 按 responseSize 排最大的 LIST，userAgent 直接给出是谁；③ `apiserver_flowcontrol_rejected_requests_total` 看 429 集中在哪个 priorityLevel/flowSchema。治法：客户端一律上 informer（List+Watch + watch cache），禁止裸轮询全表；真要大 LIST 用 `limit+continue` 分页；查 event 用 fieldSelector `involvedObject.name` 精确查而不是拉全表。

**弹药**
- `kubectl get --raw /metrics | grep '^apiserver_storage_objects' | awk '{print $2, $1}' | sort -rn | head`
- 读路径语义：`resourceVersion=0`（informer 默认）走 apiserver 内存 watch cache，不打 etcd；不带的 LIST 是 quorum 读直打 etcd
- `--event-ttl`（默认 1h）+ EventRateLimit admission 插件（默认不开，要显式启用）控 event 洪泛
- APF 默认把 system/exempt 级留给 kubelet 和控制面组件，429 先打的是低优先级的 CI/监控类客户端——这是设计，不是故障

**红线**｜❌ 把 429 归罪于"apiserver 不够"就扩容——不揪出打爆它的客户端，扩多少都被一个全表扫描循环吃掉；❌ 生产放开 `get/list -A` 的 RBAC 给 CI 和巡检脚本。

**下一问**｜apiserver 滚动重启，为什么整个集群会跟着抖一下？（→ LIST 风暴）

---

## Q12. apiserver 滚动重启，为什么整个集群会跟着抖一下？（控制面/apiserver）

**一句话**｜所有客户端的 watch 同时断开 → reflector 全量重新 LIST → 几千个 LIST 同一秒打进来，apiserver 的 watch cache 还是冷的，第一批全落到 etcd。

**30 秒答**｜机制：informer 靠长连接 watch 增量同步，apiserver 重启连接全断，client-go reflector 的恢复路径是**先 LIST 全量、再 watch**。而 apiserver 刚起来时 watch cache 是空的，恢复期第一批 LIST（尤其不带 resourceVersion 的 quorum 读）直接压到 etcd，这就是重启后 1–2 分钟的"抖"。缓解三板斧：① 滚动重启逐台来、间隔拉够，别三副本同时换；② WatchBookmark（GA）让客户端的 resourceVersion 保持新鲜，断连后可以从不老的版本续 watch，减少必须全量 LIST 的客户端；③ APF 保证控制面组件（system 优先级）的 LIST 不被业务客户端挤掉。判断线：抖动时间窗 = watch cache 暖机时间，超过 5 分钟还在 429 就不是余波，是容量问题。

**弹药**
- client-go 默认限速 5 QPS / burst 10，自研客户端别裸调 REST API
- controller-manager/scheduler 有自己的 `--kube-api-qps/--kube-api-burst`，大规模集群重启后先看它们的退避
- `--watch-cache` 默认开；`--target-ram-mb` 控制 cache 占用，内存紧张的 apiserver 会加剧 LIST 回源 etcd
- 观察暖机：看 `etcd_request_duration_seconds{operation="list"}` 的尖峰窗口，窗口收窄 = watch cache 暖起来了

**红线**｜❌ 三副本 apiserver 一次全重启（换证书/升级时最常见）——自己制造 LIST 风暴；❌ 重启后 429 就盲目扩容——先等暖机，再看 APF 拒绝集中在谁。

**下一问**｜怎么让 500 个 controller 的重 LIST 错峰？（→ APF 优先级规划 + 客户端限速 + 滚动窗口）

---

> **Q13–Q20：集群组成组件专项**（2026-10-16 追加）。底料取自知识库 `01-集群基础/03-控制平面/` 深挖笔记（scheduler 20 / KCM 13 / kubelet 15 / kube-proxy 16 / runtime 21 / PKI 38 / HA 03）。Q1–Q12 已打透 etcd 与 apiserver，这批补齐剩余核心组件。
> 同主题白话讲解版（先看它建立理解，再回来背这边的「30 秒答」）：`36-报告/k8s-集群核心组件-快问快答-白话版.md`。

## Q13. 讲讲 Pod 从提交到落节点的完整调度过程（控制面/scheduler）

**一句话**｜过滤（Filter）→ 打分（Score）→ 绑定（Bind）三步；调度器只做"决策"，kubelet 负责"执行"——Pod 调度成功 ≠ 起得来。

**30 秒答**｜周期：informer 感知 `spec.nodeName` 为空的 Pod → 进入调度队列。**过滤**：资源不够（Insufficient cpu/memory）、端口冲突、taint 没容忍、nodeSelector/亲和不匹配的节点直接淘汰；**打分**：LeastRequested（剩余资源多）、BalancedAllocation（资源均衡）、ImageLocality（镜像已在）、PodTopologySpread 等插件打分，最高分胜出，平票随机。**绑定**：先 assume（写进调度器本地缓存占坑）再发 Bind 到 apiserver。大规模优化：`--percentage-of-nodes-to-score` 采样过滤打分（0=按规模自适应，最低 5%），牺牲"绝对最优"换调度吞吐。判断线：scheduler 只服务"未绑定的 Pod"——**scheduler 全挂，已运行 Pod 毫发无伤**，只是新 Pod 一直 Pending，这是声明式系统"决策与执行分离"的必考点。

**弹药**
- `kubectl describe pod` 末尾 Events 是调度器亲口说的拒绝原因（`0/6 nodes are available: Insufficient cpu`），排查调度失败第一入口
- `kubectl get pod <p> -o jsonpath='{.spec.nodeName}'` 为空 = 没调度上；非空但没起来 = kubelet 侧问题（跟 Q7 的分界呼应）
- 调度框架扩展点：queueSort / preFilter / filter / postFilter（抢占）/ preScore / score / reserve / bind，自定义插件走 SchedulerConfiguration，多 profile 可一进程多策略
- 直接 `spec.nodeName` 指定节点 = 绕过调度器，亲和/污点/资源校验全跳过

**红线**｜❌ "scheduler 挂了集群会崩"——只影响新 Pod 调度；❌ 把调度失败归因 kubelet——Pod 根本没到节点；❌ 分不清 Filter 失败（Events 里 `0/x nodes available`）和 kubelet 拉起失败。

**下一问**｜高优先级 Pod 调度不上，抢占（preemption）怎么选受害者？（→ PostFilter 阶段，按 PriorityClass 找"牺牲代价最小"的低优先级 victims）

---

## Q14. kube-controller-manager 都管什么？它和 scheduler 怎么分工？（控制面/KCM）

**一句话**｜KCM 是几十个"期望态→实际态"调和控制器的合集，scheduler 只回答"Pod 放哪"一个问题——一个管调和、一个管决策，都靠 list-watch，都不碰容器。

**30 秒答**｜大控制器盘点：①**Deployment/ReplicaSet**——三级结构：Deployment 只管 RS 版本和滚动节奏，**副本数是 RS 管的**；②**node lifecycle**——失联节点打 `unreachable:NoExecute` taint，taint manager 默认 300s 后驱逐（接 Q7 的 10s/40s/300s 数字链）；③**endpoints/endpointslice**——把 selector 匹配的 Pod IP 同步给 Service（Q3 里 Endpoints 的生产者）；④**garbage collector**——级联删除、ownerReference 清孤儿；⑤serviceaccount/token、csr 审批。分工判断线：任何"这个资源该变成什么样"→ 控制器；"这个 Pod 放哪个节点"→ scheduler。高可用：同种控制器全集群只允许一个 active，多副本靠 leader election（Lease）做热备。

**弹药**
- `kubectl get lease -n kube-system` — 看 KCM/SCM 的 holder，默认 leaseDuration **15s** / renewDeadline **10s** / retryPeriod **2s**
- `--controllers` 可启停单个控制器；`--concurrent-*-syncs` 控调和并发
- GC 三模式：foreground（前台级联，先清依赖）/ background / orphan（摘 ownerReference 留孤儿）

**红线**｜❌ "KCM 去节点上拉容器"——那是 kubelet 的活，控制面从不直接操作节点进程；❌ 分不清 Deployment 和 RS 谁管副本数（RS 管，直接 scale Deployment 也是落到 RS）；❌ 手删 RS 想减少 Pod——只会重建。

**下一问**｜节点失联后为什么打 taint 而不是直接删 Pod？（→ tolerationSeconds 让业务自己决定容忍多久，语义解耦）

---

## Q15. kubelet 的三大名场面：PLEG、静态 Pod、cgroup driver（节点/kubelet）

**一句话**｜kubelet 是节点上的最终执行者：对比 PodSpec 与容器实态、调 CRI 调和；面试只深挖三处——PLEG not healthy、静态 Pod 删不掉、cgroup driver 不一致。

**30 秒答**｜①**PLEG**（Pod Lifecycle Event Generator）：每 1s（`--pleg-relist-period`）relist 全部容器状态 diff 出事件；runtime 卡顿时 relist 超时 → `PLEG is not healthy` → 节点 NotReady（Q7 分叉点之一）。②**静态 Pod**：kubelet 直读 `/etc/kubernetes/manifests` 起 Pod，不经 apiserver 下发，mirror pod 只读回显——**kubectl delete 删不掉，必须改/删节点上的 manifest 文件**；kubeadm 控制面组件（etcd/apiserver/KCM/SCH）全是静态 Pod，就是为了 API 死了控制面还能自举。③**cgroup driver**：kubelet 与 containerd 必须同为 systemd（或同为 cgroupfs），不一致 → 资源统计错乱、驱逐行为异常、OOMKilled 假象。节点状态 10s 一报（Q7 已背）；eviction 默认阈值：memory.available<100Mi、nodefs.available<10%、imagefs.available<15%。

**弹药**
- `journalctl -u kubelet | grep -i pleg` — relist timeout 铁证
- `--pod-infra-container-image`：pause 镜像，每个 Pod 第一个起的容器，占住 netns/IPC；kubelet 与 runtime 配置要一致
- kubelet 证书自动轮换 `--rotate-certificates`（默认开）——它不参与 kubeadm 的 certs renew（见 Q20）

**红线**｜❌ 想删静态 Pod 用 `kubectl delete`——删 mirror pod 无效，改节点 manifest；❌ 改 cgroup driver 只改一边（kubelet 或 containerd 单边 systemd）；❌ PLEG not healthy 就无脑重启 kubelet——runtime 卡死的根因（IO 挂死、镜像拉取风暴）还在。

**下一问**｜pause 容器死了，业务容器会怎样？（→ sandbox 销毁，netns 没了，整个 Pod 里的容器全部退出重建）

---

## Q16. Service 上千条后为什么必须 IPVS？kube-proxy 两种模式差在哪（网络/kube-proxy）

**一句话**｜kube-proxy 是"Service 规则的编程器"不是代理：iptables 线性扫规则 O(n)，IPVS 内核哈希表 O(1) 还自带 LB 算法——Service 规模上千就是分界线。

**30 秒答**｜机制：Service/Endpoints 变化 → kube-proxy watch → 编程内核。**iptables 模式**：每个 Service、每个 endpoint 一条规则挂进 KUBE-SERVICES 链，每个包线性匹配，几千 Service 后命中延迟肉眼可见；**IPVS 模式**：内核哈希表查找，真正的四层 LB（rr/lc/sh 等算法），天然支持大规模和 sessionAffinity。1.29+ 新增 nftables 模式，逐步接替 iptables。上限之上是 **eBPF（Cilium）直接绕开 kube-proxy**：Service 转换在 socket 层完成，包根本不进 DNAT 规则链——"为什么 Cilium 能替代 kube-proxy"这题的标准答案。判断线：数据面的包流转在内核 netfilter/IPVS 里，kube-proxy 进程死了，已写入的规则照常转发（只是不再更新）。

**弹药**
- `kubectl -n kube-system get cm kube-proxy -o yaml | grep mode`（空 = 默认 iptables）
- `ipvsadm -Ln | grep <ClusterIP>`；iptables 模式 `iptables -t nat -L KUBE-SERVICES -n`（接 Q3）
- IPVS 靠内核模块 `ip_vs` `ip_vs_rr` 等，**没加载会静默降级 iptables**——上线前必查
- UDP 老 conntrack：无握手、表项过期慢，旧 endpoint 表项把流量继续发已死 Pod → 5s 超时经典来源（`conntrack -L | grep <svcIP>`）

**红线**｜❌ "kube-proxy 处理数据包"——它只写规则，包路径在内核；❌ 大集群 iptables 模式还怪网络慢；❌ IPVS 模式不验内核模块就上线。

**下一问**｜Endpoints 已经更新，流量还在打旧 Pod，为什么？（→ conntrack 既有表项 + 优雅终止：preStop / terminationGracePeriodSeconds 给 endpoint 摘除留窗口）

---

## Q17. 控制面高可用：谁能多副本、谁只能单活、靠什么选主？（控制面/HA）

**一句话**｜apiserver 无状态可横扩（状态全在 etcd），KCM/SCH 只能单 active 靠 Lease 抢主；HA = 3 apiserver + LB + 3(奇数) etcd，etcd 部署形态选 stacked 还是 external。

**30 秒答**｜三层拆开答：①**apiserver**：无状态，多副本挂 LB 后，任何一台都能服务，扩副本只分摊连接与缓存读（写还是全落 etcd——接 Q9 红线）；②**etcd**：Raft 多数派，3 或 5 奇数副本（Q1 已背）；③**KCM/SCM**：多副本是热备，同种控制器同一时刻只有一个 active——holder 持 Lease，默认 15s 续期、10s 续期超时、2s 重试，主挂了最多 leaseDuration 时间接管，期间控制器调和短暂暂停。部署形态：**stacked**（etcd 与控制面同节点：省机器，挂一台损两件，kubeadm 默认）vs **external**（etcd 独立：故障域隔离，双倍机器）。判断线：控制面 HA 解决的是"管理平面"可用性，**数据面从不过控制面**——控制面全挂，存量业务流量照常走。

**弹药**
- `kubectl -n kube-system get lease` — `kube-scheduler`、`kube-controller-manager` 两个 Lease 的 holderIdentity
- apiserver `/livez` 与 `/readyz` 分离：LB 只摘 readyz 失败的实例，livez 挂的留着重启自愈
- LB 健康检查周期 < kubelet 心跳感知时间，否则主观摘除放大抖动

**红线**｜❌ "控制面挂了业务流量断"——kube-proxy 规则还在节点上，断的只是调度/调和/新变更；❌ KCM 出现双 active——租约参数错配，会重复创建对象，比单点更糟；❌ etcd 用 2 副本做 HA——偶数没有意义，分区时双方都凑不齐 quorum。

**下一问**｜控制面全挂时，运行中的 Pod 容器重启了会怎样？（→ kubelet 有本地 PodSpec，照常拉起；冻结的是新调度和新配置下发）

---

## Q18. kubelet → CRI → containerd 这条链路上，pause 和 shim 各干什么？（容器运行时）

**一句话**｜K8s 不直接管容器：kubelet → CRI（gRPC）→ containerd → shim → runc；Pod ≠ 容器——Pod 是共享 netns 的一组容器 + pause 作锚，shim 让容器不随 containerd 重启而死。

**30 秒答**｜CRI 是两层抽象：**PodSandbox + Container**。起 Pod 先 `RunPodSandbox` 起 **pause 容器**——占住 netns/IPC（业务容器 join 它的网络栈），然后 sandbox 里再起业务容器。每个容器配一个 **containerd-shim** 做父进程：containerd 重启/升级时 shim 带着容器活下来，stdout 日志与退出状态由 shim 兜底——这就是"升级 containerd 不用驱逐 Pod"的原因。1.24 移除 dockershim：不是 Docker 镜像不能用（OCI 兼容照跑），是 kubelet 同时维护两套 CRI 成本太高；注意 containerd 拉的是 registry 镜像，与节点上 `docker images` 完全两个存储。排障统一用 **crictl**，不是 docker。

**弹药**
- `crictl pods`（sandbox 层）/ `crictl ps -a`（容器层）——两层分开看
- `crictl logs --previous` 看上一次崩溃日志；`ctr -n k8s.io containers list` 直查 containerd
- shim 残留：`ps -ef | grep containerd-shim`，sandbox 已删 shim 还在 = 清理泄漏
- pause 镜像三处一致：kubelet `--pod-infra-container-image`、containerd 配置、私有仓库 mirror

**红线**｜❌ 节点上 `docker ps` 找 K8s 容器（dockershim 已移除）；❌ 分不清 sandbox 和容器两层（crictl pods ≠ crictl ps）；❌ 手动 `docker rm`/"crictl rm" 帮 K8s 清理运行中容器——绕过调和循环制造漂移。

**下一问**｜镜像拉取慢或私有仓库认证失败，kubelet 和 containerd 各管哪一半？（→ kubelet 负责 imagePullPolicy 决策与 imagePullSecrets 下发，仓库认证/mirror 配置在 containerd 的 hosts.toml）

---

## Q19. 集群 DNS 解析链路，ndots:5 坑在哪？（网络/CoreDNS）

**一句话**｜解析三级跳：Pod resolv.conf → kube-dns ClusterIP（或 NodeLocal 缓存）→ CoreDNS → 上游；`ndots:5` 让外部域名解析 QPS 翻 6 倍，是 DNS 性能黑洞第一名。

**30 秒答**｜链路：Pod 的 `nameserver` 指向 kube-dns Service ClusterIP，search 域带 `ndots:5`——查 `api.example.com`（dots=2 < 5）会先拼满 5 个 search 域（`api.example.com.default.svc.cluster.local` …）逐个 NXDOMAIN 后才查真名，**外部域名解析放大 6 倍**。两个经典坑：①**UDP conntrack race** → 偶发整 5 秒超时（DNAT 后 SNAT 端口冲突丢应答，重试即成功）；②CoreDNS 副本/资源不足，高峰拒答。解法：**NodeLocal DNSCache**——DaemonSet 在节点上开缓存（169.254.20.10），命中不出节点，还用 TCP 兜底绕开 conntrack race。CoreDNS 配置就一个 Corefile：`kubernetes` 插件管 cluster.local，`forward . /etc/resolv.conf` 管外部。

**弹药**
- `kubectl -n kube-system edit cm coredns` — 改完 `kubectl rollout restart deploy/coredns`（Corefile 不热加载，严格说要重启才稳）
- 判据三连：`nslookup kubernetes.default` 短名/`kubernetes.default.svc`/FQDN 逐步收敛；resolv.conf 的 nameserver 变成 169.254.20.10 = NodeLocal 已接管
- `dnsPolicy`：ClusterFirst（默认）/ Default（继承节点，宿主机软件如 systemd-resolved 接管）/ None（自定义）
- 高 NXDOMAIN 洪泛看 CoreDNS 日志 + `apiserver_storage_objects` 里 endpoints 包不大但 QPS 高（接 Q11 的定位手法）

**红线**｜❌ Service 不通先重启 CoreDNS（Q3 红线复刻：Endpoints 为空与 DNS 无关）；❌ 出口流量大的应用不做 ndots 优化（full FQDN 或 NodeLocal）；❌ CoreDNS 单副本还说"DNS 高可用"。

**下一问**｜为什么 DNS 超时总是整 5 秒？（→ conntrack race 的重试节奏，TCP 兜底为何能根治）

---

## Q20. kubeadm 集群证书到期，节点集体失联，怎么救 + 怎么防？（控制面/PKI）

**一句话**｜一套 CA（10 年）+ 各组件证书（默认 1 年）——到期忘续 = 组件互信断裂、节点集体 NotReady，最经典的"周年爆炸"事故；救法 `kubeadm certs renew all` 后**必须重启静态 Pod**。

**30 秒答**｜两类证书分开答：①**控制面组件证书**（apiserver/etcd/KCM 之间互信 + 对外）：kubeadm 默认 1 年，`kubeadm certs check-expiration` 一眼看完；`kubeadm certs renew all` 续期后**组件不会热加载，必须让静态 Pod 重建**（移走 manifests 再放回，或删容器让 kubelet 重建）。②**kubelet 证书**：`--rotate-certificates` 默认自动轮换，不参与 certs renew。etcd 另有一套独立 CA。故障特征：组件日志 `x509: certificate has expired`，kubelet 全部连不上 apiserver → 集群整体 NotReady，而节点本身好好的。CA 本身 10 年，**renew all 不会续 CA**——CA 到期要全量重签，必须提前一两年规划。

**弹药**
- `kubeadm certs check-expiration` — 到期时间全列
- 触发重启：`mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp && sleep 20 && mv 回`（kubelet 会清掉旧容器再重建）
- 防线：对**剩余有效期**建监控告警（探测 /readyz 只能证明现在通，不能预警周年炸弹）+ 变更窗口提前续期（比如剩 30 天例行续）

**红线**｜❌ 只 renew 不重启组件——证书躺在磁盘上没生效，白高兴一场；❌ 手工 openssl 重签——SAN 与原证书不一致，apiserver 直接起不来；❌ 被问"怎么保证证书不炸"时答不出告警与提前期——这题考的是预防体系，不是抢救。

**下一问**｜续证书导致 apiserver 短暂重启，业务会感知到什么？（→ 数据面无感；watch 断开触发一轮小 LIST 风暴，接 Q12）

---

## Q21. Terway 是什么？为什么 ACK 生产不选 Flannel？（网络/ACK·Terway）

**一句话**｜Terway 是阿里云自研 CNI，把 Pod 直接接入 VPC 网络平面——Pod IP 就是 VPC 内网 IP，无 NAT、无 Overlay；ACK 生产默认 Terway ENIIP 模式。

**30 秒答**｜四个对比点：①**IP 来源**：Terway 从 VPC 子网（vSwitch）分配，Flannel 用独立 Pod CIDR；②**跨节点**：Terway 走 VPC 路由直达，Flannel VXLAN 封装（性能损耗 ~30% vs Terway ~5%）；③**安全组**：Terway 可到 Pod 级，Flannel 只有节点级；④**NetworkPolicy**：Terway 原生支持 L3/L4，Flannel 不支持。加分项：SLB 直通（ENI 模式 Pod 直接挂 SLB 后端，Flannel 必须 NodePort）。代价也要说：Terway 密度受 ENI/辅助 IP 配额限制、强绑定阿里云——Windows 节点和混合云非阿里节点不能用。

**弹药**
- 性能口径：ENI/ENIIP ~90-95% 线速，Flannel VXLAN ~70%，Calico ~90%（IPIP/VXLAN）
- 开源：github.com/AliyunContainerService/terway，Apache 2.0，Go，ACK 托管安装
- 多云/混合云统一网络 → Calico/Cilium；要 L7 策略 → Cilium

**红线**｜❌ 说 Terway "经过 SNAT 转发"——卖点恰是无 NAT 直通；❌ 只背优点说不出代价（密度受限 + 云厂商绑定），面试官会追问"那 Terway 有什么缺点"。

**下一问**｜Terway 五种模式怎么选？

---

## Q22. Terway 五种模式怎么选？（网络/ACK·Terway 选型）

**一句话**｜按"性能 vs 密度"取舍五档：VPC 路由（兼容过渡）、ENI 独占（性能王/密度最低）、**ENIIP（推荐默认）**、Trunking（密度王）、IPvlan（性能密度兼得但门槛最高）。

**30 秒答**｜①**VPC 路由**：类似 host-gw，每节点占一条 VPC 路由，默认 48 条 = 节点数上限，只给 <50 节点兼容场景；②**ENI 独占**：一 Pod 一 ENI，~95% 线速，Pod 数 = ENI 数 - 1，给 DB/网关；③**ENIIP**：ENI 辅助 IP + veth pair，~90%，通用大规模默认；④**ENIIP-Trunking**：Trunk ENI 挂 Member ENI，每 Pod 独立 MAC/安全组/带宽，单节点 200+ Pod，内核 4.19+，给 Serverless/高密度（ECI/ASK 底层就是它）；⑤**IPvlan**：子设备绕过 veth，~95% 且高密度，内核 4.19+（推荐 5.10+），**建集群时选定不能在线切换**。判断线：先问业务要密度还是性能，再核对内核/规格族支持（Trunk 只有部分规格族支持）。

**弹药**
- 决策树：通用大规模/微服务 → ENIIP；极致性能低密度 → ENI 独占；Serverless/高密度弹缩 → Trunking；HPC/低延迟 → IPvlan
- IPvlan 相比 veth：P99 延迟降 20-40%，吞吐升 15-30%
- 高性能网络场景 + 固定 IP 需求（有状态服务）→ ENI 独占或 ENIIP + ReservedIP

**红线**｜❌ IPvlan/Trunking 说成"可以在线切换"——建集群时选定；❌ 不知道 IPvlan 要内核 4.19+；❌ 说不出默认是 ENIIP 及为什么（性能 ~90% 与密度的最佳平衡点）。

**下一问**｜单节点最多能跑多少 Pod，怎么算？

---

## Q23. 单节点最多能跑多少 Pod？容量怎么规划？（网络/ACK·容量）

**一句话**｜ENIIP 模式：单节点最大 Pod =（最大 ENI 数 - 1）× 单 ENI 最大辅助 IP 数，再预留 10-20% 给 IPAM 缓冲。

**30 秒答**｜公式带两个实例数：ecs.g7.2xlarge = (6-1)×15 = **75**；ecs.g7.4xlarge = (8-1)×30 = **210**；g7.8xlarge = (16-1)×30 = 450。**减 1 是主 ENI（eth0）留给节点自身**。网段规划三条：①VPC 网段创建后**不可修改**，一次给足（建议 /16 级别）；②Pod 用独立 vSwitch，不与 ECS 混用，跨 3 AZ 各建子网；③业务预留 2-3 倍弹性；Service 网段不能与 VPC/Pod 网段冲突。IP 总量估算：节点数 × 每 ENI IP 数 × ENI 密度 + 节点自身。

**弹药**
- `kubectl get node -o custom-columns=NAME:.metadata.name,PODS:.status.allocatable.pods` — 看节点 Pod 容量
- 配额参考：单 VPC 路由表默认 48 条（VPC 模式限制）；单地域 ENI 默认 5000（可提额）；单 VPC vSwitch 150
- Pod vSwitch /16 = 65536 IP，/24 = 256——按集群终态规模倒推

**红线**｜❌ 算容量忘减 1（主 ENI）；❌ 拿理论值当可用值——IP 池预热/GC 在途都要吃余量；❌ "VPC 网段以后再改"——改不了，只能迁集群。

**下一问**｜Pod 卡 ContainerCreating 报 IP 分配失败，怎么查？

---

## Q24. Pod 卡 ContainerCreating，报 IP 分配失败，怎么查？（网络/ACK·Terway 排障）

**一句话**｜四步收敛：vSwitch 余量 → 实例 ENI/IP 配额 → 本地 IP 池（terway-cli）→ Terway 日志；本质都是 VPC 侧 IP 资源见底，不是网络不通。

**30 秒答**｜事件特征 `failed to allocate address` / `no available IP`。①**vSwitch 可用 IP 耗尽**（最常见）：扩 CIDR 或加多 AZ vSwitch；②**实例配额顶格**：ENI 数 × 辅助 IP 数见顶，升配或加节点；③**本地池异常**：进 Terway Pod `terway-cli show` 看 ENI/IP 映射与池状态；④**Terway 日志**找分配报错。另有隐形杀手：Terminating 卡住的 Pod 占 IP 不放——强制删除或调 GC。缓解：调 `max_pool_size`/`min_pool_size` 预热池、多 vSwitch 多 AZ 分摊。大规模弹缩的深层瓶颈是 OpenAPI 限流（ENI/IP 操作 ~100 QPS）→ Trunking 预分配 Member ENI 缓解。

**弹药**
- `kubectl -n kube-system logs ds/terway-eniip -c terway --tail=100 | grep -i "alloc\|eni"`
- `terway-cli show [--detail | --eni | --pod ns/name]`；泄漏 IP 手动 `terway-cli release --ip <ip>`
- CRD 侧：`kubectl get podeni -A` 看 Pod↔ENI 绑定
- KB 案例：`13-生产运维/05-工单案例/ticket-case-001-terway-eni-exhaustion`

**红线**｜❌ IP 不够就重启 Terway DaemonSet——治标，弹缩风暴时反而制造抖动；❌ 不看 vSwitch 余量直接升配节点——烧钱且可能没解决根因；❌ 答不出 terway-cli 这个工具名。

**下一问**｜Terway 的 NetworkPolicy 怎么实现？固定 IP 怎么做？

---

## Q25. Terway 的 NetworkPolicy 怎么实现？固定 IP 怎么做？（网络/ACK·Terway 架构）

**一句话**｜NetworkPolicy 按模式三选一落地：ENI 安全组（独占/Trunking）、iptables（ENIIP）、eBPF（IPvlan 可选）——只支持 L3/L4，L7 找 Cilium；固定 IP 用 ReservedIP CRD（v1.3+）。

**30 秒答**｜Terway Controller watch NetworkPolicy，按数据面翻译：ENI 独占/Trunking 转成**云端安全组规则**，ENIIP 在节点配 **iptables**，IPvlan+eBPF 走 **TC/eBPF 程序**（性能最优）。范围只有 L3/L4（IP/端口），HTTP/gRPC 的 L7 策略不支持。固定 IP：**ReservedIP CRD** 防止指定 IP 被 GC 回收，典型场景是数据库主从切换 IP 不变、三方 IP 白名单。配套 CRD 五件套要能点名：PodENI（Pod↔ENI 绑定）、NodeNetworking（节点网络容量）、PodNetworking（网络配置模板）、ReservedIP、IPInstance（IP 生命周期 Available→InUse→Deleting→Available）。IPAM 顺带一句：两级设计——本地预热池（max/min_pool_size）+ 云端 OpenAPI；ADD 先查本地池，DEL 归还池，GC 每 60s 清超池空闲 IP；Terway 组件是 hostNetwork DaemonSet（terway-eniip），CNI binary 经 /run/terway/terway.sock（gRPC）调 daemon。

**弹药**
- RAM 授权走 ECS 实例角色（Instance RAM Role），最小权限 = ENI/IP 的 CRUD + Describe，不硬编码 AK/SK
- `kubectl get crd | grep terway/network` 验证 CRD 安装
- 高频 API 限流口径：ENI/IP 写操作 ~100 QPS，Describe ~500 QPS

**红线**｜❌ 说 Terway 支持 L7 策略；❌ 答不出"按模式分流"的实现方式；❌ 固定 IP 说成"改 Pod IP 字段"——是 ReservedIP/CRD 机制；❌ IPAM 只会说"自动分配"，说不出本地池 + GC 两级结构。

---

## Q26. 单节点 NotReady：从心跳停止到 Pod 被驱逐的时间线？双通道心跳为什么这么设计？（节点）

**一句话**｜NodeLifecycleController 被动等心跳判定（不是 apiserver 主动探测）：40s 无 Lease 续约即 Unknown → 打 unreachable taint → 默认 300s 后驱逐；拆双通道是为了砍掉大集群的 NodeStatus 写放大。

**30 秒答**｜kubelet 双通道心跳：**NodeLease** 每 10s 更新 `kube-node-lease` 里 Lease 对象的 renewTime（极轻）；**NodeStatus** 全量上报 conditions/capacity（默认 5m 或关键状态变化时，重）。NodeLifecycleController 超过 `--node-monitor-grace-period`（默认 40s）没见到续约 → Ready=Unknown → 打 `node.kubernetes.io/unreachable` taint：先 NoSchedule 拒新 Pod，再 NoExecute 触发 TaintBasedEviction，Pod 默认 `tolerationSeconds=300`——**约 T+5min 开始驱逐**，DaemonSet Pod 不驱逐（接 Q7/Q14 的数字链）。恢复：心跳回来 40s 内 Ready=True、摘 taint。为什么拆两条：KEP-0009 之前每 10s 全量写 NodeStatus，几千节点集群把 etcd/apiserver 写爆；Lease 把高频心跳降为一次 renewTime 更新，重的 NodeStatus 降频到 5m。

**弹药**
- `kubectl get lease -n kube-node-lease` 看 renewTime 在不在动——第一刀，不用上节点
- `kubectl describe node` 看 Conditions 的 LastHeartbeatTime
- 时间线速记：**10s Lease 停 → 40s Unknown → taint(NoSchedule→NoExecute) → 5min 驱逐**
- 红线知识：K8s Lease 是普通 etcd 对象（`/registry/leases/kube-node-lease/*`），**永不过期、从不删除**——只是 renewTime 停更，别和 etcd 原生 TTL lease 混

**红线**｜❌ 说 Lease 到期会被 etcd 删掉；❌ 说 NotReady 是 apiserver 主动探测的——控制器是被动等心跳；❌ 心跳数字说不出口（10s/40s/300s，这题及格线就是数字链）。

**下一问**｜心跳正常但节点打了 disk-pressure，Pod 为什么照样被驱逐？（→ condition 翻译成 taint 的第二条路，下一题）

---

## Q27. Node Conditions 有哪几个？condition 和 taint 什么关系？（节点）

**一句话**｜五个灯：Ready / MemoryPressure / DiskPressure / PIDPressure / NetworkUnavailable；TaintNodesByCondition 把关键 condition 翻译成污点——**真正挡住调度器的是 taint**。

**30 秒答**｜Ready=True 才可调度（False/Unknown → 超时驱逐已有 Pod，上一题时间线）；MemoryPressure → 优先驱逐 BestEffort；DiskPressure → 禁调度 + 清日志/未用镜像（imagefs 和 nodefs 两块盘分开监控，Q5 已实战）；PIDPressure → 禁调度；NetworkUnavailable 由 CNI 上报。机制：kubelet/controller 把 condition 同步成 `node.kubernetes.io/*` 系列污点（memory-pressure、disk-pressure、unschedulable…），调度器按污点拒绝新 Pod——所以"为什么 Pod 不往这台调"先看 `spec.taints` 再看 conditions，拦截动作发生在污点层。

**弹药**
- `kubectl describe node <n>` 的 Taints 字段一行看全 + Conditions 段逐灯过
- cordon 的本质就是打 `node.kubernetes.io/unschedulable:NoSchedule` 污点
- `kubectl taint node <n> key=value:NoSchedule-`（末尾减号移除）

**红线**｜❌ 只答得出 Ready 一个 condition；❌ 把 condition 当拦截动作本身；❌ DiskPressure 只想到磁盘满——inode 耗尽也算。

**下一问**｜8Gi 的机器 allocatable 为什么只有 7Gi？（→ 预留扣减，下一题）

---

## Q28. capacity 和 allocatable 差在哪？不设预留会死得有多难看？（节点/资源）

**一句话**｜allocatable = capacity − kube/system 预留 − 驱逐阈值，**调度器只认 allocatable**；不设预留，节点紧张时系统进程和 Pod 抢食，kubelet 自己可能被 OOM——节点假死比 Pod 被杀严重得多。

**30 秒答**｜8Gi 机器 allocatable 可能只剩 7Gi：**kube-reserved**（kubelet/containerd 自用）+ **system-reserved**（sshd/journald 等系统进程）+ **eviction-hard**（默认 memory.available<100Mi，Q15 已背全阈值）全从可调度额度里扣；`enforceNodeAllocatable=pods` 让这笔账不只是展示而是 cgroup 硬约束。单节点 Pod 上限 `--max-pods` 默认 110——Terway 场景真实上限是 ENI 公式（Q23 已背），**取两者更小值**。

**弹药**
- `kubectl describe node` 看 capacity / allocatable 两栏对照
- kubelet config：`systemReserved` / `kubeReserved` / `enforceNodeAllocatable`
- 容量告警按 allocatable 利用率建，不是 capacity

**红线**｜❌ 生产节点零预留裸奔；❌ 容量规划按 capacity 算——调度器根本不看它；❌ 把 cgroup driver 混进这题（那是 kubelet 起不来，Q15 已答）。

**下一问**｜节点要下线维修，Pod 怎么优雅搬走？（→ drain，下一题）

---

## Q29. 节点维护流程？drain 卡住怎么办？drain 和 delete 区别？（节点/变更）

**一句话**｜drain = cordon（打 unschedulable 污点拒新）+ eviction API（逐个驱逐、受 PDB 约束）；卡住三类原因各对一个 flag；和 delete 的区别是"走不走 PDB 和优雅终止"。

**30 秒答**｜链路：drain → 维护 → uncordon。卡住三类：**DaemonSet Pod**（`--ignore-daemonsets`）、**emptyDir 数据**（`--delete-emptydir-data`）、**PDB 拦 eviction**（drain 挂起等待不是报错——先看 PDB 是否允许动一个，`maxUnavailable=0` 的服务 drain 不动才对）。drain vs delete：drain 走 **eviction API**，受 PDB 与优雅终止约束、逐个等完成；`kubectl delete pod` 是强删，PDB 拦不住。下线节点：drain → `kubectl delete node` → 云侧回收，emptyDir 数据随节点消失。范围别混：PDB 管的是**自愿驱逐**（drain/升级），资源压力驱逐（Q35）不看 PDB。

**弹药**
- `kubectl drain <n> --ignore-daemonsets --delete-emptydir-data`
- `kubectl get pdb -A` 维护前先扫有没有 0 可用的
- 排空验证：`kubectl get pods -A --field-selector spec.nodeName=<n>`

**红线**｜❌ 生产节点不 drain 直接 reboot；❌ drain 控制面节点前不确认 etcd quorum（stacked 部署一台既是节点又是 etcd 成员，Q17 已背）；❌ 没配 PDB 的核心服务就敢随便 drain。

**下一问**｜mass NotReady 会了（Q7），单节点 NotReady 上节点后固定看哪五样？（→ 下一题）

---

## Q30. 单节点 NotReady，ssh 上节点后的固定排查动作？（节点/排障）

**一句话**｜五步固定刀法：kubelet 服务状态 → 日志关键字 → 10248 healthz → 运行时 → 磁盘/inode；先二分"kubelet 进程层"和"kubelet ↔ apiserver 通道层"，别一上来就重启。

**30 秒答**｜①`systemctl status kubelet`——死了看 `journalctl -u kubelet` 尾部；②日志关键字分诊：`PLEG is not healthy`（Q15，容器爆炸/runtime 卡死）、`x509`（证书到期，Q20）、`failed to create kubelet`（cgroup driver 不一致，Q15）、`no space left on device`（磁盘）；③`curl -s localhost:10248/healthz`——通 = 进程活着，问题在到 apiserver 的网络/凭证；④`systemctl status containerd` + `crictl ps -a | wc -l`——runtime 层与容器数量（PLEG 嫌疑）；⑤`df -h` + `df -i`——磁盘与 inode（DiskPressure 前兆）。判断线：进程层挂 → 修 kubelet/runtime；进程活着但不通 → 查证书/网络/apiserver 侧。

**弹药**
- `journalctl -u kubelet --since "10 min ago" -p err`
- 端口分工：10248 本地 healthz（无鉴权）、10250 带鉴权 kubelet API、10255 只读已废弃
- `crictl pods` 看沙箱泄漏（PLEG 根因之一）

**红线**｜❌ 不看日志直接 `systemctl restart kubelet`——PLEG 的根因还在（Q15 红线复刻）；❌ 只看 `df -h` 不看 `df -i`；❌ 10248/10250 端口分工说不清。

**下一问**｜转到 Pod 侧：`kubectl get pod` 的 STATUS 一列，底下其实是几层状态？（→ 三层账本，下一题）

---

## Q31. Pod 的状态有几层？Pending 和 Waiting 谁是谁？（Pod）

**一句话**｜三层账本：Pod phase（五相粗粒度）→ conditions（四个里程碑）→ containerStatuses.state（容器级 waiting/running/terminated）——面试官在分辨你答的是哪层。

**30 秒答**｜**phase** 只有 Pending/Running/Succeeded/Failed/Unknown（Unknown 基本等于所在节点失联，接 Q26 时间线）；**conditions** 是 PodScheduled → Initialized → ContainersReady → Ready 四个里程碑，排障看哪个 False + reason；**容器状态**才是细账，`Waiting.reason` 才说真话（ContainerCreating / ImagePullBackOff / CrashLoopBackOff）。"Pod Pending"是 phase 层、"容器 Waiting"是容器层，别混着说。Succeeded/Failed 主要是 Job 关心：所有主容器 exit 0 且不再重启 = Succeeded。

**弹药**
- `kubectl get pod <p> -o jsonpath='{.status.containerStatuses[*].state}'` — 容器层真相
- `kubectl describe pod` 先看 Conditions 段、再看 Events
- STATUS 一列只是 phase 的糖，不等于容器健康

**红线**｜❌ 把 phase 当容器状态报；❌ Ready=True 但流量不进只会重启 Pod——先查 Endpoints（readiness 决定端点，Q3 已背）；❌ 三层答不全。

**下一问**｜容器死活谁在盯？三种探针怎么配才不踩坑？（→ 下一题）

---

## Q32. 三种探针的区别和典型误配？（Pod）

**一句话**｜liveness 失败重启容器，readiness 失败摘 Endpoints，startup 成功前屏蔽另两个——**只有 readiness 影响流量**，liveness 的动作是重启。

**30 秒答**｜**liveness** 治死锁卡死（进程活着但不干活）；**readiness** 治"暂时不能接客"（预热、依赖抖动）；**startup** 给慢启动独立窗口（期间不跑 liveness——"起来就被杀"的标准解法）。误配三连：①健康端点里查数据库——依赖抖动引发连环重启风暴，健康检查不依赖外部服务；②initialDelay 短于启动时间——用 startup 解决，不是无限调 delay；③探针 timeout 没给 CPU 毛刺留余量，限流瞬间被 liveness 杀。判据：restartCount 涨但 Endpoints 一直在 = liveness 在杀；Endpoints 反复进出 = readiness 抖。

**弹药**
- `kubectl describe pod` 看 probe 结果与 restartCount；`kubectl get events --field-selector reason=Unhealthy`
- 四件套：periodSeconds / timeoutSeconds / failureThreshold / successThreshold，**timeout < period 是底线**
- gRPC 探针 1.24+：`grpc: { port: 9090 }`

**红线**｜❌ readiness 配依赖检查——依赖挂 → 全集群摘流，故障放大；❌ timeout 比 period 还长；❌ "探针失败就是重启"——readiness 失败是摘流不是重启。

**下一问**｜restartCount 在涨但不是探针杀的，怎么分诊？（→ exit code，下一题）

---

## Q33. CrashLoopBackOff / ImagePullBackOff 怎么分诊？exit code 怎么读？（Pod/排障）

**一句话**｜ImagePull 先分 401/tag/限流三选一，CrashLoop 先 `logs --previous` 再读 exit code——**137 不一定是 OOM**。

**30 秒答**｜**ImagePullBackOff**：describe pod Events 看拉取报错——401/403 是凭证（imagePullSecrets），`manifest unknown` 是 tag 错，`toomanyrequests` 是镜像源限流；节点上 `crictl pull <img>` 复现最直接。**CrashLoopBackOff**：先 `kubectl logs --previous` 看死前现场，再读 exit code：1 应用错误、**137 = SIGKILL（OOMKilled 或被杀，Q2 已拆 cgroup/节点两层）**、126/127 启动命令问题；容器起不来用 `kubectl debug -it --target=<c>` 注入临时容器。节奏：BackOff 10s 起指数翻倍 5min 封顶；restartPolicy（Always/OnFailure/Never）决定退不退。调度失败的 Pending 归 Q13（FailedScheduling 四选一），这题管"到了节点之后"的失败。

**弹药**
- `kubectl logs <p> -c <c> --previous`；`kubectl get pod <p> -o jsonpath='{.status.containerStatuses[0].lastState}'`
- `kubectl debug -it <p> --image=busybox --target=<c>`（共享进程/网络 ns，不动原容器）
- `crictl pull` / `crictl images` 节点侧复现凭证与 mirror 问题（crictl 两层看 Q18）

**红线**｜❌ CrashLoop 只看当前日志不看 --previous；❌ 137 直接断言 OOM——要看 lastState.reason 和 dmesg；❌ 不分诊先 `kubectl delete pod` 重启大法。

**下一问**｜应用要下线，怎么退出才不掉请求？（→ 优雅终止，下一题）

---

## Q34. Pod 删除时发生了什么？优雅终止怎么配才不丢请求？（Pod/变更）

**一句话**｜删除 → 宽限期倒计时**立即开始** → PreStop ∥ SIGTERM → terminationGracePeriodSeconds（默认 30s）耗尽 → SIGKILL；Endpoints 摘除是异步的，和容器退出赛跑——preStop sleep 就是给这场比赛让时间。

**30 秒答**｜完整链：apiserver 删 Pod → kubelet 触发 PreStop hook + 发 SIGTERM（宽限期从删除那刻就计时）→ 同时 Endpoints 摘除经 watch 异步传播到 kube-proxy/ingress（Q16 的 conntrack 旧表项也在这条线上）→ 宽限期耗尽 SIGKILL。滚动更新丢请求的根因 = **摘流异步 + 容器退太快**的竞态：负载没摘干净 Pod 已没了。标准姿势：`preStop: sleep 5–10s` 留摘流窗口 + 应用处理 SIGTERM 排空在途请求 + grace period 按排空时长给。**宽限期计时包含 PreStop 耗时**——PreStop 睡 25s，应用只剩 5s 排空。

**弹药**
- `terminationGracePeriodSeconds`：日志/网关类可给 60–120s；preStop exec `["/bin/sh","-c","sleep 5"]`
- 摘流最慢一环决定 sleep 时长：Endpoints → kube-proxy → 客户端 conn 池（keep-alive 连着就还打旧 Pod）
- 验收看滚动更新期间的 502/conn refused，不是只看 Pod 状态

**红线**｜❌ 应用不处理 SIGTERM——30s 后被 -9，在途请求全断；❌ PreStop 做超过宽限期的事；❌ 把 grace period 当"多活 30s 缓冲"乱调大（占着端口不排空）。

**下一问**｜节点内存吃紧时，kubelet 按什么顺序挑 Pod 驱逐？（→ QoS，下一题）

---

## Q35. QoS 怎么分级？节点压力下先杀谁？（Pod/资源）

**一句话**｜Guaranteed（全容器 requests==limits）＞ Burstable ＞ BestEffort（裸奔），节点压力按倒序驱逐；**CPU 超限只限流（可压缩），memory 超限直接杀（不可压缩）**。

**30 秒答**｜evictionManager 在节点可用内存低于 hard 阈值时，按 **QoS 等级 + usage 超 requests 的比例**排序驱逐；内核 OOM killer 用 oom_score_adj 兜底：BestEffort=1000 先死，Guaranteed=−997 活到最后。requests 是调度与驱逐保护线，limits 是 cgroup 上限——"超 limit 被 cgroup OOM"和"节点压力被驱逐"是两层（Q2 已拆）。关键服务保命组合：**Guaranteed + 高 PriorityClass**。范围别混（Q29 已埋）：PDB 管自愿驱逐（drain），资源压力驱逐不看 PDB、priority 参与排序。

**弹药**
- `kubectl get pod <p> -o jsonpath='{.status.qosClass}'`
- 容器内 `cat /proc/self/oom_score_adj` 验证（Guaranteed ≈ −997，BestEffort=1000）
- `kubectl describe node` 的 Allocated resources 看 requests 打满度——驱逐风险预警

**红线**｜❌ 核心服务不设 requests/limits 裸奔；❌ "设了 limit 就不会被驱逐"——节点压力看 usage vs requests；❌ QoS 和 PriorityClass 混为一谈（一个是资源形态，一个是调度/驱逐优先级）。

**下一问**｜Pod 里"先干活的容器"和"常驻陪伴的容器"怎么表达？（→ init 与 sidecar，下一题）

---

## Q36. init 容器和原生 sidecar 什么区别？（Pod）

**一句话**｜init 容器串行跑完**退出**才轮到主容器；原生 sidecar（1.28+，`restartPolicy: Always` 的 initContainer）先起、常驻、Pod 结束时**先被停**。

**30 秒答**｜**init**：顺序执行、必须全部成功，失败按 restartPolicy 重试——适合前置检查/迁移/配置注入。**原生 sidecar**（initContainers 里写 `restartPolicy: Always`）解决传统 sidecar 三大痛点：①**启动顺序**——主容器不再抢在代理前面连不上网；②**生命周期**——Job 里 sidecar 不退出导致 Job 永不完成，现在 sidecar 会被先终止、Job 正常 Succeeded；③**独立探针**——sidecar 自己配 startup/readiness/liveness，挂了独立重启不拖累主容器。典型场景：Istio/envoy 代理、日志采集器。版本线：1.28 alpha → 1.29 beta 默认开启。

**弹药**
- 写法速记：`initContainers` + `restartPolicy: Always` = sidecar
- 老版本土法（主容器 sleep 等代理、postStart 等待脚本）在 1.29+ 该退役
- init 失败 ≠ sidecar 失败：init 挂住 Pod 卡 `Init:*`；sidecar 挂只影响自己重启

**红线**｜❌ sidecar 放主容器列表里——生命周期管不住（Job 永不完成就是它）；❌ sidecar 不配 readiness；❌ 版本答不出 1.28/1.29（这题一半考版本意识）。

**下一问**｜原生 sidecar 的 resources 参与调度算分吗？（→ 版本敏感点：restartable init 容器是否计入 Pod effective requests，答前先报版本）

---

## Q37. Pod 从提交到容器 Running，kubelet 这边发生了什么？（Pod/生命周期）

**一句话**｜调度（Q13）只是把名字写进 `spec.nodeName`；kubelet 的 SyncPod 才是施工队：起 sandbox（CNI 分配 IP）→ init 容器逐个跑 → 主容器并行起 → 探针接管。

**30 秒答**｜kubelet watch 到属于自己的 Pod 后，SyncPod 分步执行：①对比期望与实态，决定要不要重建 sandbox；②建 **PodSandbox**——pause 容器起来，CNI 插件（`/opt/cni/bin` 二进制 + `/etc/cni/net.d` 配置）给 netns 分配 podIP（Q18 的锚）；③init 容器逐个起、等退出；④主容器并行起（镜像按 imagePullPolicy 在这步拉）；⑤probeManager 从 startup 探针开始接管；⑥状态回写 apiserver。卡点对号入座：卡在 sandbox = CNI 或卷（Q24 的 IP 分配失败、PVC 没挂上）；卡在拉镜像 = Q33；卡在 `Init:*` = init 容器自己崩。判断线：describe pod 的 Events 就是施工日志，按阶段读。

**弹药**
- `crictl pods`（sandbox 在不在）→ `crictl ps`（业务容器在不在），两层分别看
- `kubectl get pod -o wide` 有 IP = sandbox 成功过
- `/etc/cni/net.d` 的文件列表顺序 = CNI 配置链

**红线**｜❌ 说 kubelet 直接调 docker（CRI 抽象，Q18）；❌ 没起 sandbox 就想给业务容器配网；❌ Pod 调度成功就当它起来了。

**下一问**｜新节点加入集群，从 kubelet 启动到 Ready 要过哪几关？（→ CSR，下一题）

---

## Q38. 新节点 join 集群的流程？CSR 卡住怎么查？（节点/注册）

**一句话**｜bootstrap token 认证 → kubelet 提交 CSR → KCM 的 csrapproving 自动审批 → 拿正式客户端证书 → 注册 Node 对象 → 心跳上线（Q26）。

**30 秒答**｜四关：①**认证关**——kubelet 带 bootstrap token 连 apiserver，此时只有"提交 CSR"一个权限；②**签发关**——kubelet 提交 CertificateSigningRequest，signerName 是 `kubernetes.io/kube-apiserver-client-kubelet`，KCM 里的 csrapproving 控制器按 RBAC 自动 approve、signer 出证书；③**注册关**——kubelet 用正式证书创建 Node 对象、打 label；④**心跳关**——进 Lease 上报循环（Q26 时间线）。CSR 卡 Pending 两个常见原因：自动审批的 RBAC 被删改、bootstrap token 过期。

**弹药**
- `kubectl get csr` 看 APPROVED/REQUESTED 与 signerName
- `kubectl certificate approve <csr>` 人工救急
- `journalctl -u kubelet | grep -iE "csr|certificate"`

**红线**｜❌ 说 CSR 由 apiserver 自己审批——是 KCM 的控制器；❌ 人工签超长有效期证书凑合；❌ join 失败就重建节点不看 CSR。

**下一问**｜节点磁盘快满了，kubelet 在驱逐前还做了什么"自救"？（→ 软/硬驱逐与镜像 GC）

---

## Q39. kubelet 驱逐深挖：软阈值、硬阈值、镜像 GC 各是什么？（节点/资源）

**一句话**｜软阈值 = 持续超限 + 宽限期才驱逐（给喘息）；硬阈值 = 立即驱逐（保命）；镜像 GC 是另一条独立回收线——驱逐删 Pod、GC 删镜像，别混。

**30 秒答**｜①**硬驱逐**：阈值一破立即执行——memory.available<100Mi、nodefs.available<10%、imagefs.available<15%、nodefs.inodesFree<5%（Q15 已背）；②**软驱逐**：如 memory.available<1.5Gi 配 soft-grace-period（如 90s），持续超限满宽限期才动手，期间节点可能已打 MemoryPressure 拒新；③**回收优先于驱逐**：kubelet 先 reclaim（镜像 GC、清 dead 容器），不够再按 Q35 顺序驱逐 Pod；`--eviction-minimum-reclaim` 保证每次至少回收多少，防抖动式小步驱逐；④**镜像 GC 独立运行**：磁盘用量超 `image-gc-high-threshold`（默认 85%）删镜像回到 low（80%）——与驱逐无关但同样救磁盘。判断线：驱逐的结果是"Pod 去新节点重建"，GC 的结果是"镜像下次重拉"，后果完全不同。

**弹药**
- `journalctl -u kubelet | grep -i eviction` 看驱逐记录与理由
- `kubectl describe node` 的 Conditions 看 MemoryPressure/DiskPressure 挂没挂
- 调参三件套：evictionHard / evictionSoft(+gracePeriod) / evictionMinimumReclaim

**红线**｜❌ 把镜像 GC 说成驱逐；❌ 软驱逐不配宽限期（一抖就驱逐，风暴放大）；❌ 以为被驱逐的 Pod 会"回原节点"——它在别处重建。

**下一问**｜Redis 这类延迟敏感服务怎么独占 CPU 核不被邻居吵？（→ CPU Manager）

---

## Q40. CPU 独占怎么实现？CPU Manager static policy 是什么？（节点/性能）

**一句话**｜kubelet 的 CPU Manager 设 static policy：Guaranteed Pod（requests==limits）且 CPU 申请是整数核 → 分到独占 cpuset，和邻居物理隔离——专治延迟敏感的毛刺。

**30 秒答**｜默认 none policy 下所有容器共享 CPU 池，邻座 Pod 一忙就把你的缓存和上下文切换打爆。static policy 规则：①只有 **Guaranteed 且整数核**的容器进独占池（0.5 核的不配）；②其余容器挤共享池；③系统组件靠 kube-reserved/systemReserved 先圈出保留核——不预留就开 static 没意义。效果：容器绑到专属核（cpuset），没有跨核迁移和邻居争抢，P99 稳定。适合 Redis、网关、NFV/DPDK。代价：独占核利用率偏低，容量规划要重算。

**弹药**
- kubelet config：`cpuManagerPolicy: static` + `reservedSystemCPUs`（或 kube-reserved CPU）
- 验证：容器 cgroup 下 `cat /sys/fs/cgroup/cpuset.cpus` 看绑核
- 内存侧对应物：Memory Manager static policy（NUMA 绑定，1.22+）

**红线**｜❌ Burstable 也想要独占（规则不允许）；❌ 独占核容量还按共享池口径估算；❌ 说"绑核=一定更快"——批处理任务独占是浪费。

**下一问**｜节点不够了，集群怎么自动加机器？CA 和 Karpenter 本质区别？

---

## Q41. 节点自动扩缩容：Cluster Autoscaler 和 Karpenter 的本质区别？（节点/弹性）

**一句话**｜CA 是"改节点组"——模拟调度 Pending Pod 算出要几台，让 ASG/节点池去加；Karpenter 是"直接买机器"——绕过节点组，JIT 起一台最合适的实例并直接把 Pod 绑上去。

**30 秒答**｜**CA**：watch 不可调度的 Pending Pod → 用调度器逻辑模拟"加一台能不能放下" → 改节点组（ASG/MIG/ACK 节点池）期望数 → 等实例启动+join（Q38）→ scheduler 正常调度。缩容：节点利用率低于阈值（默认 50% requests）且 Pod 能搬走（查 PDB）→ 排空删节点。链路长，分钟级。**Karpenter**：不依赖节点组，直接 watch Pending Pod → 自己挑实例类型 → 调云 API 起实例 → **直接 bind Pod**（不经 scheduler 重排队），秒级到几十秒；按需 bin-packing + consolidation 回收碎片节点。判断线：节点组刚性（固定机型/标签，好管理）vs Karpenter 灵活采购（省钱、快，但深绑云）。

**弹药**
- CA 排障：日志 `pod didn't trigger scale-up: 1 insufficient cpu`；节点组 maxSize 顶满是最常见哑火
- Karpenter：NodePool/NodeClaim CRD，`kubectl get nodeclaims` 看实时供给
- 验证扩容链路四步：Pending → 节点出现 → join Ready → Pod Running

**红线**｜❌ "CA 扩容 Pod 就上去了"——中间还有实例启动和 join；❌ 缩容不看 PDB 硬删；❌ 说 Karpenter 依赖节点组——它就是为消灭节点组而生。

**下一问**｜改了 ConfigMap，为什么容器里没生效？（→ 注入方式差异）

---

## Q42. 改了 ConfigMap/Secret，为什么容器里没生效？（Pod/配置）

**一句话**｜注入方式决定生效方式：env 是"出生快照"永不更新；volume 挂载热更（分钟级）但 **subPath 例外**；想立刻生效只能重建 Pod。

**30 秒答**｜三种注入三条命：①**env 环境变量**——容器创建时注入一次，之后改 CM 永不生效，必须重建 Pod（`rollout restart`）；②**volume 挂载**——kubelet 周期同步，改 CM 后通常 1 分钟内更新文件（程序还得自己支持配置重载，否则改了也白改）；③**subPath 挂载**——为单文件覆盖用了 subPath 就**失去热更能力**，"我明明改了为什么没生效"第一名。Secret 同规则；另注意 Secret 在 etcd 默认只是 base64 不是加密（要开 encryption at rest），不敏感就别叫 Secret。兜底：`immutable: true` 锁死配置，防误改还省 watch 负载——改配置走 GitOps+发布，不走在线改。

**弹药**
- `kubectl rollout restart deploy <d>` — env 场景的标准重建
- 验证热更：改完等一个 sync 周期，`kubectl exec <p> -- cat /etc/config/xx`
- `kubectl get cm <c> -o yaml | grep immutable`

**红线**｜❌ env 以为会热更；❌ subPath 以为会热更（两大翻盘点）；❌ 改完 CM 不进容器验证文件就说"已生效"。

**下一问**｜低优任务怎么保证被高优任务"插队"？（→ PriorityClass 与抢占）

---

## Q43. PriorityClass 与抢占：高优 Pod 调度不上时，谁被赶走？（Pod/调度）

**一句话**｜scheduler 的 PostFilter 阶段：给高优 Pod 挑一组"代价最小"的低优 victims，删掉（走优雅终止）腾位置——victims 的选择看优先级、尽量少打断、尊重 PDB；这就是 Q13 下一问的答案。

**30 秒答**｜①PriorityClass 是集群级资源，priority 值越大越优先；系统两个天王级：system-cluster-critical（2000000000）、system-node-critical（2000001000，kubelet/CNI 用）；②调度不上 → **抢占**：scheduler 只在更低优先级里挑 victims——尽量少赶人、少跨 namespace、查 PDB；③victims 被删除走**优雅终止**（Q34 的 30s 流程），高优 Pod 随后调度进去；④`preemptionPolicy: Never` = 只参与排序、不抢人。和节点压力驱逐（Q35）的区别：抢占是 scheduler 为了"放进来"，压力驱逐是 kubelet 为了"活下去"——两套排序，priority 都参与但触发方不同。

**弹药**
- `kubectl get priorityclass`；`kubectl get pod <p> -o jsonpath='{.spec.priority}'`
- 抢占证据：victims 的 Events 出现 `Preempted by pod ...`
- 批任务给低优 + `preemptionPolicy: Never`，避免反杀在线业务

**红线**｜❌ 全员高优先级（排序失效等于没有）；❌ 说抢占瞬移完成——victims 有优雅终止窗口；❌ 把抢占和节点压力驱逐混为一谈。

**下一问**｜有些 Pod 要直接用节点 IP，hostNetwork 的代价是什么？

---

## Q44. hostNetwork / hostPID / hostIPC 什么时候用？风险在哪？（Pod/网络·安全）

**一句话**｜三个"共享宿主机命名空间"开关：hostNetwork 共网络栈（podIP=节点IP）、hostPID 看全部进程、hostIPC 共共享内存——系统组件刚需，业务容器用就是埋雷。

**30 秒答**｜**hostNetwork**：Pod 直接用节点 netns，podIP 就是节点 IP，端口直接占宿主机——CNI 自身（Terway/Calico 组件）、NodeLocal DNSCache、节点级 agent 的刚需；代价：端口冲突、绕开 Service/NetworkPolicy 抽象、端口规划变节点级问题。**hostPID**：能看到并操作宿主机全部进程（kill 节点进程都行）——安全/eBPF 排障类 agent 用，业务禁。**hostIPC**：共享宿主 System V IPC/共享内存，极少用。PSS 里 hostPID/hostIPC 连 baseline 都禁；hostNetwork baseline 允许（系统组件要用）但业务别碰。hostPort 与 NodePort 区别：hostPort 绑死单个节点端口（CNI 层），NodePort 是 Service 层的全集群入口。

**弹药**
- 审计：列出 hostNetwork=true 的 Pod——应该只有系统组件
- PSS 检查：`kubectl get ns <ns> --show-labels` 看 enforce 档位
- 告警线：业务命名空间出现 hostNetwork/hostPID/privileged = 立即告警

**红线**｜❌ 业务容器开 hostNetwork 图省事（端口冲突+绕过策略）；❌ 说 hostPod（没这个词，是 hostPort）；❌ 给普通应用 hostPID（等于给容器宿主机进程视角）。

**下一问**｜这些开关之外，Pod 安全的"官方档位"是什么？（→ PSS）

---

## Q45. Pod 安全标准（PSS）三档是什么？怎么在命名空间上落地？（Pod/安全）

**一句话**｜privileged（不管）/ baseline（挡明显提权：privileged、hostPath、hostPID/IPC）/ restricted（强制硬化：非 root、seccomp、drop ALL）——用命名空间 label（PSA）按 enforce/audit/warn 三种力度落地。

**30 秒答**｜**三档**：①privileged：无限制，系统组件专用；②baseline：挡最危险的——禁 privileged 容器、禁 hostPath 卷、禁 hostPID/hostIPC、hostPort 默认禁；③restricted：在 baseline 上再强制 `runAsNonRoot`、`seccompProfile: RuntimeDefault`、`capabilities drop ALL`、`allowPrivilegeEscalation: false`——业务命名空间的终点。**落地**：Pod Security Admission（内建，接替 PSP）在命名空间打 label：`enforce=restricted`（违规直接拒）+ `audit/warn=restricted`（只记日志/只提示不拦）。**迁移姿势**：先 warn 跑两周收告警清单 → 修完 → 再 enforce，避免一刀切拦死存量。

**弹药**
- `kubectl label ns <ns> pod-security.kubernetes.io/enforce=restricted pod-security.kubernetes.io/warn=restricted`
- restricted 模板四件套：runAsNonRoot + seccomp RuntimeDefault + drop ALL + 禁 allowPrivilegeEscalation
- 违规报错会精确指出撞了哪条控制项，照着改就行

**红线**｜❌ 生产核心业务 privileged 裸奔；❌ 新集群直接 enforce=restricted 不给迁移期；❌ 把 PSS 当细粒度策略引擎——它只有内置三档，细粒度要 Kyverno/OPA。

**下一问**｜Pod 重启和 Pod 重建，数据还在吗？（→ 存活期语义）

---

## Q46. 容器重启 vs Pod 重建，emptyDir 数据还在吗？（Pod/存储）

**一句话**｜两层语义：容器重启 = kubelet 原地拉起（Pod 对象/IP/emptyDir 都在）；Pod 重建 = 控制器删旧建新（新名字新 IP，emptyDir 清零）——数据要不要活过 Pod，是 emptyDir 和 PVC 的分界线。

**30 秒答**｜**容器重启**（CrashLoop/OOM 后按 restartPolicy 重拉）：Pod UID、Pod IP、emptyDir、PVC 挂载全部保留——丢的只是进程内存。**Pod 重建**（滚动更新/drain/被驱逐/手动删）：新 Pod 新 UID 新 IP，emptyDir 随旧 Pod 消失，PVC 数据还在（RWO 卷解绑后可被新 Pod 接走）。emptyDir 生存期 = Pod 生存期，官方口径"Pod 从节点移除即永久删除"；`medium: Memory` 是 tmpfs 更别指望，而且占容器的 memory limit（Q2 的账本又多一口）。要数据活过 Pod → PVC；要活过节点 → 带副本的存储层。判断线：被问"Pod 重启日志还在吗"，先反问"容器重启还是 Pod 重建"。

**弹药**
- 看是不是同一次生命：`kubectl get pod -o jsonpath='{.metadata.uid}'` 对比
- RESTARTS 列看重启次数；上次死因 `--previous`（Q33）
- emptyDir 的 sizeLimit 超限表现为驱逐/清理，不是报错

**红线**｜❌ 用 emptyDir 存状态还敢扩容/升级（数据蒸发第一名）；❌ "restartPolicy: Always = 数据不丢"——它只管容器层；❌ `medium: Memory` 当免费内存盘（吃 limit 的）。

**下一问**｜CPU 用量没到 limit，为什么 P99 还有毛刺？（→ CFS 限流）

---

## Q47. CPU 没打满但 P99 毛刺，为什么？（Pod/性能）

**一句话**｜CFS 限流按 100ms 周期记账：limit 500m = 每 100ms 只许烧 50ms，超了就地集体暂停——平均用量不高但周期内被掐，P99 就毛了。

**30 秒答**｜机制：CPU limit 落成 CFS quota——quota = limit × 100ms 周期；周期内配额烧完，cgroup 里所有线程**集体暂停**到下个周期。多线程应用瞬时并发（GC、突发流量）一个周期把配额打光 → 被掐几十 ms → 尾延迟。所以"平均 CPU 50% 没到 limit"和"P99 毛刺"完全兼容。确认：看 throttled 比例（throttled_periods / periods_total）。解法按序：①调大 limit；②降瞬时并发/优化 GC；③延迟敏感的用 CPU Manager 独占核（Q40）绕开共享池排队；④HPA 别拿被限流后的 CPU 用量当指标（会互相欺骗）。

**弹药**
- PromQL：`rate(container_cpu_cfs_throttled_periods_total[5m]) / rate(container_cpu_periods_total[5m])`
- 容器内验证：`cat /sys/fs/cgroup/cpu.max`（v2，"quota period"两列）
- 经验阈值：throttled 比例 >10% 就该动 limit

**红线**｜❌ 只看平均 CPU 判"资源够"；❌ limit 给 0.5 核还跑 16 线程服务；❌ 尾延迟毛刺先怀疑网络——先看 throttling 计数器，最便宜。

**下一问**｜内存和 CPU 的 limit 行为差异已在 Q2/Q35/Q47 凑齐三层——下一专题：存储（PVC/PV/StorageClass）还是 HPA/VPA？

---

## Q48. PV 和 PVC 是什么关系？为什么这么设计？（存储/PV·PVC）

**一句话**｜PV 是集群里的实际存储资源（供给侧），PVC 是用户对存储的声明式申请（消费侧），控制面把两者 **1:1 绑定**——解耦"谁供存储"和"谁用存储"。

**30 秒答**｜PV 由管理员静态创建或 StorageClass 动态供给（见文末深挖），生命周期**独立于 Pod**（cluster 级资源，Q46 的对照：PVC 数据活过 Pod 重建）；PVC 由用户声明容量、accessModes、storageClassName，Pod 通过 `persistentVolumeClaim` 卷类型挂载。设计意图同 Pod/Node：开发者不碰底层盘，管理员不追着业务问规格。绑定关系是**双向指针**——PV 的 `spec.claimRef` 指向 PVC，PVC 的 `spec.volumeName` 指向 PV，由 `persistentvolume-controller` 异步执行。

**弹药**
- `kubectl get pv,pvc -n <ns>` 一眼看绑定（PV 看 CLAIM 列，PVC 看 VOLUME 列）
- 类比记忆：PV:PVC ≈ Node:Pod——供给与消费解耦
- Pod 删了 PV/PVC 都还在；删 PVC 才触发 reclaimPolicy

**红线**｜❌ 把 PV 当 Pod 附件——PV 生命周期是集群级，Pod 全删 PV 纹丝不动；❌ 说不出双向指针在哪（claimRef / volumeName）。

**下一问**｜PV 有哪些 phase？Released 之后数据还在吗？

---

## Q49. PV 的生命周期阶段（phase）有哪些？（存储/PV·PVC）

**一句话**｜Available → Bound → Released（→ Failed），由 PV Controller 维护的状态机。

**30 秒答**｜**Available** 未绑定任何 PVC；**Bound** 已绑定；**Released** 是 PVC 已删但 reclaimPolicy=Retain——PV 对象还在、**数据还在底层卷**，等管理员处置；**Failed** 自动回收失败。没有 "Terminating" 这个 phase，删除中的 PV 靠 deletionTimestamp + finalizer 表达。Recycle 策略（清空复用）已废弃，一句带过即可。

**弹药**
- `kubectl get pv -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,CLAIM:.spec.claimRef.name,POLICY:.spec.persistentVolumeReclaimPolicy`
- Released → Available 复用：`kubectl patch pv pvc-xxx --type=json -p='[{"op":"remove","path":"/spec/claimRef"}]'`（数据仍在，相当于重新上架）
- 误删恢复黄金窗口：PVC 刚删、PV 还是 Released → 新建同规格 PVC 用 `volumeName` 直绑接回去

**红线**｜❌ "删了 PVC 数据就没了"——Retain 下 Released 数据完好，这是数据恢复的第一现场；❌ 把 Released 说成"卷已销毁"。

**下一问**｜Retain 和 Delete 具体差在哪？动态供给默认哪个？

---

## Q50. reclaimPolicy 的 Retain / Delete / Recycle 区别？（存储/PV·PVC）

**一句话**｜Retain：删 PVC 后 PV 转 Released 留数据等人工处置；Delete：连 PV 带底层云盘一起删；Recycle：已废弃。

**30 秒答**｜**Retain** 是生产数据库的保险绳，删 PVC 后数据可恢复，代价是管理员手工清理；**Delete** 是动态供给的**默认值**——由 StorageClass 的 `reclaimPolicy` 决定，不写就是 Delete，测试环境方便，生产误删 PVC = 直接删云盘；**Recycle** 是 `rm -rf` 复用卷，安全风险大已废弃。存量 PV 可 patch 改策略，云上常见操作就是给动态 PV 补 Retain 防误删。

**弹药**
- `kubectl patch pv pvc-xxx -p '{"spec":{"persistentVolumeReclaimPolicy":"Retain"}}'`
- `kubectl get sc` 看 RECLAIMPOLICY 列——SC 定基调，动态 PV 继承
- 最佳组合：动态供给拿便利 + 关键 SC 手工写 `reclaimPolicy: Retain`

**红线**｜❌ 生产关键数据挂 Delete 策略的 SC（删 PVC = 删盘，无确认无回收站）；❌ 答不出"动态供给默认 Delete"——这题考的就是默认值意识。

**下一问**｜accessModes 有几种？RWO 的 "Once" 到底约束谁？

---

## Q51. 四种 accessModes？RWO 约束的是节点还是 Pod？（存储/PV·PVC）

**一句话**｜RWO 单节点读写、ROX 多节点只读、RWX 多节点读写、RWOP 单 Pod 读写（1.22+）——**约束的是节点级挂载，不是 Pod 级**。

**30 秒答**｜**RWO（ReadWriteOnce）**：同一节点上多个 Pod 可共享挂载，跨节点才 Multi-Attach 报错——云盘/本地盘只支持 RWO；**ROX**：多节点只读分发；**RWX**：必须共享文件系统（NFS/CephFS/NAS），块存储给不了；**RWOP（ReadWriteOncePod）**：收紧到单 Pod，1.22 alpha / 1.27 beta 默认开启 / 1.29 GA。accessModes 是集群层声明，**实际能力由底层存储决定**——给云盘写 RWX 连绑定都绑不上。ACK 口径：云盘 RWO、NAS RWX。

**弹药**
- 经典报错：`Multi-Attach error for volume "xxx" Volume is already used by pod(s) ...`
- 判断线：问"两个 Pod 能不能共用一个 RWO 卷"——同节点能，跨节点不能
- RWOP 的价值：防同节点第二个 Pod 也挂进来（独占语义/多租户）

**红线**｜❌ "RWO = 只能一个 Pod"——Once 指一次**节点挂载**；❌ 块存储硬要 RWX（要么换 NAS 要么改架构）。

**下一问**｜PV/PVC 绑定要满足什么条件？谁来执行？

---

## Q52. PV/PVC 绑定的匹配条件？谁来执行绑定？（存储/PV·PVC）

**一句话**｜容量 ≥、accessModes 包含、storageClassName 一致、selector 匹配，四条同时满足；由 `persistentvolume-controller` 异步 reconcile。

**30 秒答**｜PV Controller watch 到新 PVC，遍历 Available 的 PV 逐一匹配四条件（PV 容量 ≥ PVC 请求；PV 的 accessModes **包含** PVC 请求的模式；两侧 storageClassName 相等——PV 侧空串匹配 PVC 侧不写；PVC 的 selector 命中 PV label）。命中后写双向指针（Q48），双双转 Bound。绑定后 **PVC 的 spec 不可再改（immutable）**——换卷只能新建 PVC 迁数据。PVC 还可 `volumeName` 直绑指定 PV（绕过匹配，运维迁移常用）。

**弹药**
- `kubectl describe pvc` Events：`no persistent volumes available` = 静态无匹配；`waiting for first consumer` = WaitForFirstConsumer 正常等待
- 直绑迁移模板：旧 PV 改 Retain → 删 PVC → 新 PVC 写 `volumeName` 指回旧 PV

**红线**｜❌ 以为绑定是同步的——创建 PVC 到 Bound 有 controller reconcile 延迟；❌ 说 PVC 绑定后还能改 storageClassName（immutable）。

**下一问**｜没有现成 PV 时怎么办？动态供给的完整链路走一遍。

---

## Q53. StorageClass 动态供给的完整流程？（存储/StorageClass·CSI）

**一句话**｜PVC 指定 storageClassName → 无匹配 PV 时 external-provisioner 调 CSI `CreateVolume` → 生成 `pvc-<uid>` 命名的 PV → claimRef 直接绑定。

**30 秒答**｜链路五步：①PV Controller 发现新 PVC，解析出 SC 名；②SC 对应的 sidecar **external-provisioner**（watch PVC）接单；③调 CSI **CreateVolume** RPC，底层走云 API 建云盘/NAS 目录；④成功后创建 PV 对象——名字固定 `pvc-<pvc-uid>`，claimRef 直接指向 PVC（跳过匹配），capacity/accessModes/reclaimPolicy 全按 SC 定义；⑤PVC 转 Bound，等 Pod 调度后再走 Attach/Stage/Publish。失败 PVC 停 Pending，Event 记 `ProvisioningFailed`。in-tree provisioner（`kubernetes.io/aws-ebs` 等）已冻结，新集群一律 CSI。

**弹药**
- CSI 三件套：controller（Deployment，建删卷）+ node plugin（DaemonSet，节点挂载）+ sidecar 组（provisioner/attacher/resizer/snapshotter 各司其职）
- 默认 SC：annotation `storageclass.kubernetes.io/is-default-class: "true"`；PVC 不写 storageClassName → 匹配默认 SC，没有默认 SC → Pending
- ACK 的 provisioner：`diskplugin.csi.alibabacloud.com`（云盘）/ `nasplugin.csi.alibabacloud.com`（NAS）

**红线**｜❌ "PVC 不写 storageClassName 就没有动态供给"——错，是匹配默认 SC；❌ 把 CreateVolume 和 Attach 混为一谈（前者建卷，后者是调度后的挂载）。

**下一问**｜Immediate 和 WaitForFirstConsumer 差在哪？云上为什么必须后者？

---

## Q54. volumeBindingMode：Immediate vs WaitForFirstConsumer？（存储/StorageClass）

**一句话**｜Immediate：PVC 一创建立即供卷；WaitForFirstConsumer：等第一个 Pod 调度定案才供卷——保证卷和 Pod 同拓扑（同 AZ）。

**30 秒答**｜云盘绑定 AZ：Immediate 模式下 PVC 一创建就按 SC 默认拓扑建卷，Pod 若被调到别的 AZ，挂载直接失败（卷跨 AZ attach 不过去）。**WaitForFirstConsumer** 让 scheduler 先算出 Pod 落点（PVC 的拓扑需求喂进调度），provisioner 再在目标 AZ 建卷，从根上消灭跨 AZ。代价：Pod 没调度前 PVC 一直 Pending——**这是正常状态不是故障**。local PV 的节点亲和同样靠它。

**弹药**
- `kubectl get sc` 看 VOLUMEBINDINGMODE 列；ACK 默认 SC（alicloud-disk-essd 等）已是 WaitForFirstConsumer
- 判断线：PVC Pending **先看绑定模式再谈故障**——`waiting for first consumer` 去Pod 侧查调度
- StatefulSet 扩副本同样生效：每个新 PVC 都等对应 Pod 的调度结果

**红线**｜❌ 云上自建 SC 不配 WaitForFirstConsumer（默认 Immediate）——跨 AZ 挂载失败是必踩坑；❌ 把"等 Pod 调度"的 Pending 当 provisioner 故障去重启 CSI。

**下一问**｜PVC 真的异常 Pending，排查顺序是什么？

---

## Q55. PVC 一直 Pending，排查顺序？（存储/排障）

**一句话**｜`kubectl describe pvc` 看 Events，按"SC 存在 → 绑定模式 → provisioner → 匹配条件 → 配额"五层切。

**30 秒答**｜①**describe pvc** 看 Events 拿第一线索；②storageClassName 拼写 / SC 存在 / 默认 SC 是否符合预期；③WaitForFirstConsumer 下先查 **Pod 为什么没调度**（资源不够、节点亲和、卷拓扑冲突——此时 Pending 是果不是因）；④动态供给失败看 **csi-provisioner 日志**：云盘配额打满、AZ 无货、RAM 权限、盘型不支持；⑤静态绑定复查四条件（Q52）；⑥ResourceQuota 限制 PVC 数量或存储总量。配套看 PV 侧有没有 Available 存货、Released 可复用。

**弹药**
- `kubectl get pvc -A | grep -v Bound` — 全局找异常
- `kubectl logs -n kube-system csi-provisioner-xxx --tail=100` — 供给失败的真话在这
- `kubectl get resourcequota -n <ns>`；`kubectl get pv | grep -E "Available|Released"`
- 事件关键词对号：`ProvisioningFailed`（供给层）/ `no matching`（匹配层）/ `waiting for first consumer`（调度层）

**红线**｜❌ 上来就删 PVC 重建——Event 和 provisioner 日志就是现场，删了证据就没了；❌ 不分层乱猜，开口就"重启 CSI 试试"。

**下一问**｜PVC 能扩容吗？能缩容吗？

---

## Q56. PVC 扩容和缩容？（存储/PV·PVC）

**一句话**｜在线扩容可以——前提 SC 开 `allowVolumeExpansion: true`；缩容**一律不支持**。

**30 秒答**｜`kubectl patch pvc` 调大 `requests.storage` → CSI `ControllerExpandVolume` 扩云盘 → `NodeExpandVolume` 扩文件系统；多数驱动在线完成，部分要求 Pod 重启（文件系统扩展需卸载后做，Event 显示 `FileSystemResizePending`）。PVC 的 storage 在 immutable 约束里是**只能加大**的方向，扩容不可逆——容量规划初始给足 + 预留 ~30% 增长。`allowVolumeExpansion` 后开对存量 PVC 同样生效（看的是 SC 当前状态）。

**弹药**
- `kubectl patch pvc data -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'`
- 扩容三事件：`Resizing` → `VolumeResizeSuccessful` → `FileSystemResizeSuccessful`
- 想缩容 = 新建小卷 + 数据迁移（dump/同步）+ 切流量，当变更项目做

**红线**｜❌ 答"能缩容"——这题就是钓鱼；❌ SC 没开 expansion 就 patch，PVC 卡 `Resizing` 反复报错。

**下一问**｜StatefulSet 怎么用 PV/PVC？缩容后数据呢？

---

## Q57. StatefulSet 怎么用 PV/PVC？缩容后数据呢？（存储/StatefulSet）

**一句话**｜`volumeClaimTemplates` 为每个副本自动创建独立 PVC（`data-web-0/1/2...`），缩容只删 Pod 不删 PVC，数据保留。

**30 秒答**｜每副本独立盘：Pod 重建后**按序号绑回同名 PVC**（web-0 永远拿 data-web-0），数据不漂移；扩容副本数自动新建 PVC；**缩容只删 Pod，PVC 原样保留**——重新扩回来数据还在，要清理需手动删 PVC（此时 reclaimPolicy 才生效）。配合 headless service 实现稳定网络标识。数据库标准形态 = StatefulSet + 每副本独立 RWO PVC + Retain。

**弹药**
- `kubectl get pvc -l app=web` 看 `data-web-<ordinal>` 命名规律
- volumeClaimTemplates 改模板**不回改存量 PVC**（只影响新副本）——存量要扩容得逐个 patch（Q56）
- 滚动更新默认倒序（N-1→0），扩缩容按序号增删

**红线**｜❌ 多副本共享同一个 RWO PVC"省盘钱"——第二个副本调度到别的节点 Multi-Attach 直接挂（Q51）；❌ 缩容后顺手删 PVC 当"清理"——reclaimPolicy=Delete 时等于删云盘（Q50）。

**下一问**｜PV 供给的两条路——静态和动态到底怎么选？（→ 文末深挖）

---

## 深挖：PV 供给两条路——静态供给 vs 动态供给

**定义一句话**｜静态 = 管理员把**已存在的真实存储**"登记"成 PV 对象，集群只认领不创建；动态 = PVC 指向 StorageClass，由 CSI provisioner **现场建卷**、自动生成 PV。

**两条链路对照**

```
静态：真实存储已存在（NFS export / 云盘 / local 盘）
      管理员手写 PV（写死 capacity / accessModes / storageClassName / volume source）
      PVC 创建 → PV Controller 匹配四条件（Q52）→ 绑定
      集群全程不创建任何底层卷

动态：PVC（带 storageClassName）→ external-provisioner watch 到
      → CSI CreateVolume（云 API 建卷）→ 自动生成 PV（pvc-<uid>）
      → claimRef 直绑 → Bound（完整链路见 Q53）
```

**对比表**

| 维度 | 静态供给 | 动态供给 |
|------|---------|---------|
| 谁建底层卷 | 没人建——存储必须**已存在** | CSI provisioner 现场建 |
| 谁建 PV 对象 | 管理员手写 YAML | provisioner 自动生成（`pvc-<uid>`） |
| 容量规划 | 靠人，容易闲置/不足 | 按需供给 |
| 响应速度 | 用户等管理员 | 秒级自动化 |
| 适用场景 | 已有存储资产、**local PV**、特殊参数卷 | 云环境一切标准卷 |
| 规模化 | 不可维护（几十个 PV 起步就乱） | 唯一正解 |

**静态的三块自留地**（说出这些是加分项）
1. **local PV**：本地盘必须静态——`spec.local.path` + **nodeAffinity 锁节点**，拓扑只有人知道；`local-volume-provisioner`（DaemonSet）能半自动发现挂载点生成 PV，但底层盘仍是人给的。代价：节点挂 = PV 永久不可用，Pod 调度被卷拓扑焊死（配 WaitForFirstConsumer 缓解调度失败）。
2. **已有存储资产**：公司 NAS/NFS export、旧 SAN LUN——存储生命周期在集群外管理，K8s 只做"登记 + 挂载"。
3. **特殊参数卷**：云 API 覆盖不到的预配置卷（预格式化、专属实例上的盘）。

**动态的三要素与两道保险**
- 三要素：**StorageClass**（类目：provisioner + parameters + reclaimPolicy + volumeBindingMode）、**CSI 驱动**、**PVC 指向 SC**。SC 本质是"卷的模板/类目"，parameters 直接透传云 API（如 ACK 的 `type: cloud_essd`、`performanceLevel: PL1`）。
- 两道保险：**默认 SC**（is-default-class annotation，PVC 不写类名时兜底）；**WaitForFirstConsumer**（先调度后供卷，拓扑对齐，Q54）。
- 失败面：provisioner 挂 / 云配额满 / AZ 无货 / RAM 权限——全部落 PVC Events + csi-provisioner 日志（Q55）。

**面试答法（45 秒版）**｜"PV 供给两条路：静态是管理员把已存在的存储登记成 PV，适合 NFS 资产和 local PV；动态是 PVC 指向 StorageClass，external-provisioner 调 CSI CreateVolume 现场建卷、自动生成 pvc-<uid> 的 PV 并绑定。选型上云环境一律动态 + WaitForFirstConsumer 保证同 AZ，关键数据 reclaimPolicy 改 Retain；本地盘、NFS 这类生命周期在集群外的存储走静态。混合集群两条路并存很常见——数据库云盘动态供给，日志/共享目录 NAS 静态。"

**追问预判**
- 静态 PV 的 storageClassName 有什么用？——纯匹配凭据；写空串 `""` 表示只匹配"不指定 SC"的 PVC，别和"PVC 不写 = 默认 SC"混淆。
- 动态 PV 为什么叫 `pvc-<uid>`？——provisioner 按绑定目标命名，天然防重名、可追溯归属。
- in-tree 迁 CSI 后老 PV 怎么办？——CSIMigration 特性门翻译旧记录；盘存量看 `kubectl get pv` 的 PROVISIONER 列，新 PV 一律 CSI。
- 静态 PV 还能再"动态"一点吗？——local-volume-provisioner 按 discovery 目录自动生成 PV（半自动），但盘的分区挂载仍需人工/ansible 铺。

**下一专题**｜HPA/VPA（Q47 的另一半钩子）。

---

> **Q58–Q67：Deployment 专项**（2026-10-16 追加）。Q14 已点过三层结构与"副本数归 RS"、Q34 已打透优雅终止，本批把 Deployment 本体一次补齐：结构（Q58/Q63/Q64）、滚动与防护（Q59/Q60/Q62）、回滚（Q61）、发布手法（Q65/Q66）、选型收口（Q67）。

## Q58. Deployment、ReplicaSet、Pod 三层结构，谁管副本数？为什么要三层？（工作负载/Deployment）

**一句话**｜Deployment 只管"版本和滚动节奏"，**副本数是 RS 管的**，Pod 是 RS 建的——三层是为了"每个版本留一个存档"，回滚才有得回。

**30 秒答**｜归属链：Deployment → RS（ownerReference）→ Pod。滚动更新 = Deployment 按新模板**新建一个 RS**（pod-template-hash 区分）→ 新 RS 扩、旧 RS 缩（Q59 节奏）→ 回滚就是反过来把旧 RS 的模板抄回去。副本数真正挂在 RS 上：`kubectl scale deployment` 是改 Deployment spec，由 Deployment controller 换算后落到 RS——所以直接改 RS 副本数，下一次滚动就被改回去。判断线：Pod 是耗材、RS 是分身术、Deployment 是版本管理器；任何"Pod 层面的修补"都是错的作业面，必须回到 Deployment template。

**弹药**
- `kubectl get rs -l app=x` 看 DESIRED/CURRENT/READY——滚动中间态一眼见底
- RS 的 Pod 都带 `pod-template-hash` label，这就是"分版本"的锚
- `kubectl get deploy x -o jsonpath='{.spec.replicas} {.status.updatedReplicas} {.status.availableReplicas}'`

**红线**｜❌ 手改 RS 的 replicas 想"微调"——下次滚动直接覆盖；❌ 分不清谁管副本数（Q14 已背：RS 管）；❌ 对 Pod 做修补而不是改 template。

**下一问**｜新旧 RS 一扩一缩的节奏由谁控制？（→ maxSurge / maxUnavailable，下一题）

---

## Q59. 滚动更新的完整节奏：maxSurge / maxUnavailable 怎么算？（工作负载/Deployment）

**一句话**｜滚动 = 新 RS 一边扩旧 RS 一边缩，两个旋钮：maxSurge 管"最多比期望多几个"（向上取整），maxUnavailable 管"可用数最多比期望少几个"（向下取整），默认各 25%。

**30 秒答**｜replicas=10、maxSurge=25% → ceil(2.5)=**3**，最多同时 13 个 Pod；maxUnavailable=25% → floor(2.5)=**2**，任意时刻可用最少 8 个。循环：新 RS 扩 surge 份额 → 新 Pod 过 readiness 且稳定满 minReadySeconds（Q62）计入 available → 旧 RS 缩 unavailable 份额 → 直到新 RS 到位、旧 RS 归零。**生产推荐 maxUnavailable=0 + maxSurge 25%**——容量永不缩水，代价是发布期临时多占资源，容量规划要留 surge 余量。两者可写数字或百分比；一个为 0 时另一个不能为 0。Recreate 策略是先全杀再建——有停机窗口，只给"不允许新旧共存"的场景（如 schema 不兼容）。

**弹药**
- `kubectl get deploy x -w` 配合 `kubectl get rs -w`——新旧 RS 的 DESIRED 此消彼长
- 策略块：`strategy: { type: RollingUpdate, rollingUpdate: { maxSurge: 25%, maxUnavailable: 0 } }`
- 发布期间 502/连接拒绝高发 → 先看 maxUnavailable 是不是 >0（叠加 Q34 的摘流竞态）

**红线**｜❌ 取整方向说反（surge 向上、unavailable 向下）；❌ maxUnavailable=0 还说"发布不占额外资源"；❌ 说默认策略是 Recreate——默认是 RollingUpdate。

**下一问**｜新版本有 bug，滚动一半 Pod 起不来，谁来报"发布失败"？（→ progressDeadlineSeconds，下一题）

---

## Q60. 发布卡住/失败怎么发现？progressDeadlineSeconds 是什么？（工作负载/Deployment）

**一句话**｜Deployment 自带进度闹钟 progressDeadlineSeconds（默认 600s）：窗口内没有任何进展就置 Progressing=False / ProgressDeadlineExceeded——**它只报警，不回滚、不清理**。

**30 秒答**｜判定：describe deploy 的 conditions 里 Progressing=False, reason=ProgressDeadlineExceeded；`kubectl rollout status` 会一直挂着直到此刻才报错退出——这就是 CI 流水线天然的发布卡点。卡住典型原因：新版本 ImagePullBackOff / CrashLoopBackOff（Q33）、readiness 永远不过（探针/依赖，Q32）。三个关键认知：①超时后 K8s **不会自动回滚**，卡住的新 Pod 原地烂着，旧版本还没杀完的继续服务——自动回滚要自己搭（Argo Rollouts + Analysis / CI 脚本）；②progressDeadlineSeconds 必须**大于** minReadySeconds（API 校验，写反直接拒）；③发布失败期间服务没挂——旧 RS 还在扛流量，这是滚动更新对坏版本的天然隔离。

**弹药**
- `kubectl rollout status deploy/x --timeout=120s` —— CI 里显式给超时
- `kubectl get deploy x -o jsonpath='{range .status.conditions[*]}{.type}={.status}:{.reason}{"\n"}{end}'`
- Progressing=False 不止超时一种：ReplicaSetCreateError 等调谐错误也置 False，看 message 区分

**红线**｜❌ "K8s 会自动回滚失败发布"——原生只报状态，回滚永远是人/工具的决策；❌ 超时后不查根因直接重置 Deployment——先 describe + logs --previous 看新 Pod 为什么起不来；❌ 答不出默认 600s。

**下一问**｜确认是坏版本，怎么回滚？（→ rollout undo，下一题）

---

## Q61. 回滚怎么做？rollout undo / revisionHistoryLimit 有什么坑？（工作负载/Deployment·回滚）

**一句话**｜每个 RS 就是一个版本存档，回滚 = undo 把旧 RS 的模板抄回 Deployment → **触发一次全新滚动**（产生新 revision）——不是把流量秒切回旧 RS。

**30 秒答**｜操作链：`rollout history deploy/x` 列 revision（对应 RS）→ `rollout undo [--to-revision=N]`。三个必考坑：①**undo 也是一次滚动**，按 maxSurge/maxUnavailable 节奏走，不是瞬时切流；②undo 会产生**新 revision**（rev 1→2 后 undo，得到的是 rev 3，内容等于 rev 1）——"undo 两次回到最初"就是这么来的；③**revisionHistoryLimit 默认 10**，只留最近 N 个版本（当前版本永不清理），被清掉的版本 undo 不了——回滚能力是拿 RS 存档的内存/etcd 空间换的。GitOps 集群（Q4 已背）：`kubectl rollout undo` 会和仓库漂移，下次 sync 被覆盖——终态回滚 = revert 提交再同步，undo 只做救急。

**弹药**
- `kubectl rollout history deploy/x --revision=3` 看某版完整模板
- `kubectl get rs -l app=x` 配 REVISION 注解（`deployment.kubernetes.io/revision`）看存档全景
- revisionHistoryLimit 设 0：历史全不存（回滚能力归零）

**红线**｜❌ 说"回滚是秒切"；❌ GitOps 里把 undo 当终态；❌ 生产 revisionHistoryLimit 给 0 还敢说"我们有回滚方案"。

**下一问**｜滚动更新怎么判定"这个新 Pod 真的好了"？minReadySeconds 干嘛的？（→ 下一题）

---

## Q62. minReadySeconds 是什么？为什么没有它发布会"假成功"？（工作负载/Deployment）

**一句话**｜新 Pod 过 readiness 后还要再稳 minReadySeconds 秒才计入 availableReplicas——没有它，探针一绿就算数，"起来十秒必炸"型坏版本会带着整条滚动线推进到全量。

**30 秒答**｜口径：**availableReplicas = ready 且已持续满 minReadySeconds** 的副本数，滚动推进（旧 RS 能不能继续缩）看的正是它。默认 0：坏版本只要探针过得去，滚动一路绿灯把旧版本杀光，全量事故。配上 readiness 探针（Q32）+ minReadySeconds=30~60，"慢炸"型坏版本会在滚到一小半时卡住 availableReplicas 不再增长 → progressDeadlineSeconds（Q60）报警 → 人或工具回滚——发布防护闭环就此完整：**探针挡"起不来"，minReadySeconds 挡"站不稳"，deadline 负责呼救**。别和 startupProbe 混（那是给慢启动留窗口，不参与可用性判定）。

**弹药**
- `kubectl get deploy` 三列对照：READY=readyReplicas/replicas，UP-TO-DATE=updatedReplicas，AVAILABLE=availableReplicas
- 经验值：Web 类 30s，需预热的（缓存/JIT）60–120s
- 滚动中间态读法：UP-TO-DATE 涨、AVAILABLE 不涨 = 新版本站不稳，别干等 deadline，主动查

**红线**｜❌ 把 minReadySeconds 当"延迟启动探针"（那是 startupProbe）；❌ 没有 readiness 探针单配 minReadySeconds——ready 默认即容器 Running，稳不稳全看进程死没死；❌ 设得比 progressDeadlineSeconds 大（API 直接拒）。

**下一问**｜新旧 RS 靠什么共存不互抢 Pod？selector 为什么定死不能改？（→ pod-template-hash，下一题）

---

## Q63. pod-template-hash 是什么？selector 为什么不可变？（工作负载/Deployment）

**一句话**｜hash 是 Deployment controller 给每个 RS 打的"模板指纹"，塞进 RS 的 selector 和 Pod label，让新旧版本在同一 Deployment 下精确分家；selector 定死不可改——改它等于动"谁归我管"的产权边界。

**30 秒答**｜机制：每次模板变化 → 算出新 hash → 新 RS 的 selector = 你写的 selector + `pod-template-hash=<新值>`，Pod 也带这个 label——新旧 RS 的 Pod 永远不会被对方认领，而 Service selector（不带 hash）始终罩住新旧两版（发布不掉流量的前提）。selector immutable：apps/v1 起 `spec.selector` 创建后不可改（报 `field is immutable`）——selector 决定 RS 认领哪些 Pod，改了就是产权重划：旧 Pod 变孤儿、别人的 Pod 可能被抢。硬规矩：**同 namespace 内两个 Deployment 的 selector 不得重叠**——新版 apiserver 直接拒（selector conflict），老版本放行就是两套 RS 抢同一批 Pod 打架。独占 selector 是规范不是建议：只写 `app=x` 这种大路货 label 迟早撞车。

**弹药**
- `kubectl get rs --show-labels` 看每代 RS 的 hash
- 报错对照：`field is immutable`（改 selector）/ `selector does not match template labels`（selector 与模板 label 不自洽）
- 团队规范：selector 用专属 label（app.kubernetes.io/name + instance），Service selector 再叠一层

**红线**｜❌ selector 写宽泛的 `app=web` 还想开第二个 Deployment（直接建不出来）；❌ 想靠改 selector 把存量 Pod"划给"新 Deployment——路就走错了，重建才对；❌ 分不清 Deployment selector（产权边界）和 Service selector（流量入口）的作用域。

**下一问**｜Pod 被人手动删/改，RS 什么反应？缩容时挑谁下手？（→ 下一题）

---

## Q64. 手删/手改 Pod，RS 什么反应？缩容时删谁？（工作负载/Deployment·RS）

**一句话**｜删了会补、改不回来（RS 只数数量不纠内容）；缩容删谁有一套"择劣"排序——未调度的、没 Ready 的、最新的先走。

**30 秒答**｜①手删 Pod → RS 按 template 补一个（新名新 IP）——但这不是优雅重启（Q66）；②手改 Pod：spec 大多数字段 immutable 直接报错；能改的 label/annotation RS **不会纠偏**——RS 的调和目标是"数量 = 期望"，不是"内容 = 模板"（高级考点：改个 label 让 Pod 脱离 selector，RS 数量少一立刻补新的，原 Pod 成孤儿没人管）。③缩容择劣顺序：**没调度上的先删 → Pending/Unknown 先于 Running → 未 Ready 先于 Ready → PodDeletionCost 注解高的留下 → 同条件删最新的**（`controller.kubernetes.io/pod-deletion-cost` 1.22+ 稳定，让业务自己指定谁先走）。判断线：Pod 层面的一切手工操作，RS 都用"补数量"回应，永远治不了内容的病。

**弹药**
- `kubectl annotate pod <p> controller.kubernetes.io/pod-deletion-cost="999"` 保护关键 Pod 不被缩容选中
- 孤儿验证：`kubectl get pod <p> -o jsonpath='{.metadata.ownerReferences}'` 为空即脱离
- 缩容保护的正解是 deletion cost，不是掐时机删别的

**红线**｜❌ "RS 会把改过的 Pod 恢复原样"——它不比对内容；❌ edit Pod spec 硬改（immutable 报错）；❌ 把 delete pod 当发布的"分批手段"（无滚动保护、无摘流窗口，Q34/Q66）。

**下一问**｜想发一版只放 5% 流量试试，原生 Deployment 怎么做灰度？（→ 下一题）

---

## Q65. 用 Deployment 怎么做灰度/金丝雀？（工作负载/Deployment·发布）

**一句话**｜原生三板斧：**pause 半路停车**、**双 Deployment 按副本比分流**、**精细按权重上 Argo Rollouts**——精度递增，侵入也递增。

**30 秒答**｜①**pause 金丝雀**：官方配方 `kubectl set image ... && kubectl rollout pause deploy/x` 连招——只滚出一小批新版本就停在半路，验证指标后 `rollout resume` 全量或 undo 弃（副本少滚得快，连招要快；注意暂停期间**不能 undo**，要先 resume）。②**双 Deployment**：stable + canary 两个 Deployment（label version 区分，遵守 Q63 selector 独占规矩），Service selector 同时罩住两者 → 流量比 ≈ 副本比，5% = 1:19 副本——精度差，但零依赖。③**Argo Rollouts**：Rollout CRD 替代 Deployment，canary 按 weight 精确切流（接 Ingress/服务网格）、AnalysisTemplate 挂 Prometheus 指标自动判生死、不过线自动回滚——"原生 Deployment 做不了精细灰度"的行业标准补位；蓝绿它也有。滚动更新丢请求的底子（Q34 摘流竞态）在三种手法里都存在，preStop 不配哪种都白搭。

**弹药**
- pause 配方验证：另一终端 `kubectl get rs -w` 看新旧并存、`rollout status` 停滞
- 双 Deployment 时 Service 的 selector 只写公共 label（app），version 留给副本比控
- Argo Rollouts 核心字段：`strategy.canary.steps`（setWeight / pause / analysis）

**红线**｜❌ 两个 Deployment 写相同 selector（apiserver 拒，Q63）；❌ 金丝雀只看"Pod 起没起"不看业务指标（延迟/错误率）——金丝雀的全部意义在指标；❌ 说"Deployment 原生支持按权重切流"——它没有这个能力。

**下一问**｜只改了 ConfigMap，想优雅地让全部 Pod 生效，总不能 delete pods 吧？（→ rollout restart，下一题）

---

## Q66. kubectl rollout restart 的原理？（工作负载/Deployment·变更）

**一句话**｜只给 pod template 打一个注解 `kubectl.kubernetes.io/restartedAt=<时间戳>` → 模板"变了" → hash 变 → 新建 RS 走一遍完整滚动——所谓重启，就是一次什么都没改的发布。

**30 秒答**｜价值：全程受 maxSurge/maxUnavailable 保护、保留回滚链、Pod 走优雅终止（Q34）——比 delete pods 高一个量级。三大场景：①env 注入的 ConfigMap/Secret 改了要生效（Q42：env 是出生快照）；②证书/凭据轮换；③定期换血防内存泄漏。等价手工姿势：`kubectl patch deploy x -p '{"spec":{"template":{"metadata":{"annotations":{"kubectl.kubernetes.io/restartedAt":"..."}}}}}'`。GitOps 集群要处理漂移：restartedAt 是集群内变更、Git 没有记录——Argo CD 用 ignoreDifferences 忽略该注解，或让流水线把注解写回仓库。反面教材：`kubectl delete pod -l app=x` 一把梭——并发重建风暴、绕过滚动保护、无回滚点，生产禁。

**弹药**
- `kubectl rollout restart deploy/x`（kubectl 1.15+）
- 验证：`kubectl get deploy x -o jsonpath='{.spec.template.metadata.annotations}'`
- 大规模定时换血：CronJob 调 restart + rollout status 确认完成

**红线**｜❌ 生产 `delete pods` 当重启手段；❌ GitOps 里随手 restart 不管漂移——要么 ignore 要么写回仓库；❌ 以为 restart 会"原地重启容器"——它重建 Pod，新 UID 新 IP（Q46）。

**下一问**｜收口：什么时候不该用 Deployment？（→ StatefulSet 分界，下一题）

---

## Q67. Deployment 和 StatefulSet 怎么选？（工作负载/选型）

**一句话**｜判断线就一条：**副本之间能不能互换**——能随便死一个补一个的用 Deployment，有编号、有身份、有专属盘的用 StatefulSet。

**30 秒答**｜Deployment：Pod 名/IP 随机、模板完全一致、任意缩扩、滚动乱序——Web/API/worker 的答案。StatefulSet：**稳定网络标识**（web-0 永远叫 web-0，配 headless Service + Pod 级 DNS）+ **有序部署终止**（部署 0→N-1，终止反序，滚动默认倒序 N-1→0）+ **独立 PVC**（Q57 volumeClaimTemplates）——DB/MQ/ZK 的答案。灰色地带：有本地缓存/会话粘滞的"半状态"服务——能外置状态（Redis/对象存储）换 Deployment 就别上 StatefulSet，有序性是拿速度换的（逐个滚动慢）。四兄弟收口：DaemonSet（每节点一份）、Job/CronJob（跑完即退）、StatefulSet（有身份）、Deployment（可互换）——按"跑多久、几份、有没有身份"三维区分。

**弹药**
- StatefulSet 滚动慢的解药：`podManagementPolicy: Parallel`（放弃有序性换并发，慎用于真有依赖的集群）
- StatefulSet 的原生金丝雀：`updateStrategy.rollingUpdate.partition`——序号 ≥ partition 的先更新，拿最大号当试验品
- 选型口诀：**状态外置 → Deployment；身份即状态 → StatefulSet**

**红线**｜❌ MySQL/Kafka 用 Deployment（IP 漂移 + 无序终止能把集群搞脑裂，Q57 红线呼应）；❌ 反向矫枉过正——无状态 Web 也上 StatefulSet（白丢滚动灵活性）；❌ 只会说"有状态用 StatefulSet"，说不出三个具体机制（稳定标识/有序性/独立盘）。

**下一专题**｜HPA/VPA（Q47 的另一半钩子：CFS 限流之后，副本层的弹性怎么谈）。

---

## Q68. 专有云 ACK 节点池扩容的完整链路？（专有云/ACK·节点池）

**一句话**｜每个 ACK 节点池背后是一个专有云 ESS 伸缩组：Pending Pod → Cluster Autoscaler 算数量 → 调专有云 POP（内网 VIP）改 ESS 期望实例数 → ESS 经伏羲资源池交付 ECS → 节点 join 注册 → Pod 调度上去。

**30 秒答**｜七步链路：①业务 Pod Pending（资源不够）；②Cluster Autoscaler watch 到、模拟调度算出规格与数量（Q41 CA 逻辑同款）；③CA 调**专有云 POP 网关**（内网 VIP，不是公网 OpenAPI）→ ESS 修改伸缩组期望实例数；④ESS 交付 ECS——底层**伏羲**调度物理机资源池、**天基**编排（专有云交付慢就卡这，受物理库存约束）；⑤ECS 用与节点池**严格一致的镜像**（kubelet/CSI/Terway 都打在里面）+ UserData 启动；⑥kubelet 走注册链路 join（Q38 的 CSR 四关）；⑦scheduler 把 Pending Pod 放上去。前置三条：**ASOP 里租户 ECS 配额 ≥ 伸缩组 max**、CA 的 ServiceAccount/RAM 授权通了、POP 网络连通。

**弹药**
- `kubectl get pods -n kube-system | grep cluster-autoscaler` + CA 日志看有没有调 ESS/POP
- `aliyun ess DescribeScalingActivities --ScalingGroupId asg-xxx --RegionId cn-apsara-local` 看伸缩活动
- 节点/集群卡住先查 **ASO 控制台「变更中心」**——ECS 创建、网络打通发生在 K8s 之前，kubectl 看不到

**红线**｜❌ 把专有云扩容当公有云"秒级弹"——物理库存约束下是分钟级；❌ 只用 kubectl 排查——交付链路在 K8s 之前，先看 ASO/天基；❌ 说不出组件名（POP/ESS/伏羲/天基/ASOP/天目，专有云面试的行话门槛）。

**下一问**｜扩容失败第一原因是什么？专有云和公有云差在哪？（→ 配额与差异）

---

## Q69. 专有云扩容和公有云差在哪？为什么 InsufficientCapacity 是常态？（专有云）

**一句话**｜专有云是私有物理机资源池：POP 内网接入、受租户配额和物理库存硬约束、交付分钟级、`InsufficientCapacity` 高发——瓶颈往往不在 ESS，而在资源池配额。

**30 秒答**｜五维差异：①**资源池**——私有物理机集群 vs 公有云共享池，扩容上限就是租户配额和库存；②**API 接入**——POP 网关内网 VIP vs 公网 OpenAPI，专有云无公网出口（镜像仓库、aliyun-service 全走内网）；③**交付延迟**——受物理库存约束分钟级 vs 秒级，CA 超时/冷却参数要放宽；④**失败特征**——`InsufficientCapacity` 常见 vs 罕见；⑤**监控链路**——指标由**天目**采集推送（公有云是 CloudMonitor），天目 Agent 挂了报警触发整条失效。第一排查动作：**ASOP 查租户 ECS 配额**（计算配额：CPU/内存/实例数），再查伏羲资源池水位——配额不够就是扩不出，找平台管理员扩配额或加物理节点。

**弹药**
- ASOP 路径：租户管理 → 资源配额（计算/存储/网络/日志四类，计算配额是扩容瓶颈）
- 报警触发链：天目采集 → 推送 ESS → 伸缩活动；Agent 不在 = 报警静默
- 镜像一致性：伸缩配置 ImageId 必须与节点池一致，否则节点起得来但注册失败

**红线**｜❌ 公有云经验原样照搬——配额模型和交付速度完全不同；❌ 扩容失败先怀疑 CA/ESS——先查配额；❌ 忘了专有云无公网，镜像/依赖要全内部化。

**下一问**｜弹性链路四个经典故障怎么切？（→ 排障）

---

## Q70. 专有云节点池扩容失败，四类经典故障怎么切？（专有云/排障）

**一句话**｜按链路切四段：①`InsufficientCapacity` = 配额/库存；②实例起了不入组 = 心跳/镜像（aliyun-service）；③ESS 弹了 ACK 没节点（或反向）= ProviderID 映射/授权；④指标超了不动 = 天目采集/规则关联。

**30 秒答**｜①**资源不足**：伸缩活动报 `InsufficientCapacity`、Pod 长期 Pending——ASOP 查租户配额、查伏羲水位，解法是扩配额或加物理节点，不是重试；②**心跳超时**：ECS 创建成功但加不进伸缩组/集群——查镜像里 `aliyun-service` 守护进程起没起、POP 网络连通性，修 UserData/镜像；③**伸缩组与节点池不一致**：ESS 弹了实例 ACK 没节点，或 ACK 删节点 ESS 不缩——查 CCM/CA 日志、节点 **ProviderID** 映射、CA 的 RAM 权限；④**报警不触发**：CPU 超阈值没动静——天目 Agent 活没活、报警规则关联伸缩组没、指标上报没。判断线：先看伸缩活动列表"发起过没"——没发起是触发层（③④），发起失败是资源层（①②）。

**弹药**
- `aliyun ess DescribeScalingActivities` — 伸缩活动是总入口
- `kubectl describe pod <pending>` 区分"无节点可调度"（触发层）与其他
- ASO 变更中心 + 天基部署日志看 ECS 交付卡在哪步

**红线**｜❌ 配额不够还盲重试扩容；❌ 只查 K8s 层不查 ESS 层；❌ 四类故障混着说、切不开层。

**下一问**｜日常运维这套路要守哪些纪律？（→ 最佳实践）

---

## Q71. 专有云节点池扩缩容的运维纪律？（专有云/运维）

**一句话**｜六条：配额前置、镜像标准化、PDB 护体、缩容宜慢、天目大盘预警、定期演练弹性链路。

**30 秒答**｜①**配额前置**：上线前 ASOP 预分配够 ECS 配额（≥ 伸缩组 max），别等峰时失败；②**镜像标准化**：伸缩配置镜像与节点池严格一致（kubelet/CSI/Terway 版本都打在里面），改镜像走统一发布；③**PDB**：关键业务必配，缩容驱逐受它约束（Q29：PDB 只管自愿驱逐，缩容/drain 正是）；④**缩容宜慢**：冷却时间放长（默认 300s 起调）、移除策略 OldestInstance（回收最老实例降滚动影响）、核心实例开保护；⑤**监控大盘**：天目建"伸缩活动 / 节点数 / Pending Pod 数"三联大盘——Pending 涨就是弹性要动的前兆；⑥**定期演练**：周期验证 Pending → 弹节点 → 调度全链路，别让弹性能力在生产事故里第一次实战。

**弹药**
- 伸缩组参数速记：min/max 留 20% 冗余、期望实例数对齐业务基准、冷却 300s、健康检查开
- 大盘三指标：ScalingActivityCount / 节点总数 / Pending Pod 数
- 演练验收：造 Pending Pod 到 Pod Running 计时长——专有云口径分钟级正常

**红线**｜❌ 峰值当天才想起查配额；❌ 缩容用扩容的激进度（缩容引发抖动比扩容失败更伤）；❌ 弹性链路从不上演练（真要用时链路早锈了）。

**下一问**｜节点池版本升级和飞天底座补丁怎么排期？（→ 006 升级与补丁管理，可做下一批）

---

## Q72. SOFA 侧 CCM 可以叠加在 ACK CCM 之上吗？两个 CCM 怎么配合干活？（蚂蚁/专有云/架构）

**一句话**｜可以叠加，但前提是"分域共治"而不是装第二个全功能 CCM：K8s 官方语义一个集群只有一个 cloud provider 通道，两个控制器必须把**对象所有权切开**——ACK CCM 管阿里云云资源（SLB/节点/路由），SOFA 侧控制器管自有设施（VIP/机房流量接入/裸金属节点接入），靠注解分域 + 云资源物理隔离做到互不踩脚。

**30 秒答**｜先立规矩：kubelet `--cloud-provider=external` 只对应一个 CCM，"叠加"≠再装一个完整 CCM——否则两边抢 Node 元数据（ProviderID、zone 标签）和 LB 后端列表，互踢、flapping。正确形态是**所有权分域**，四条机制：①**Service 分域**——type=LoadBalancer 的 SLB/NLB/ALB 生命周期归 ACK CCM；SOFA 侧只认自己注解/标签标记的 Service，或干脆只 watch Endpoints/EndpointSlice 做自有 LB 的后端同步；②**云资源物理隔离**——蚂蚁侧写自己的 VIP/服务器组，与 SLB 的监听/vGroup 不重叠，各管各的实例；③**云身份初始化只有一个权威**——ProviderID、`topology.kubernetes.io/zone`、路由表归 ACK CCM 独占，SOFA 侧只做附加标签和自有系统接入，不改不删；④**写路径隔离**——独立注解 key、独立 finalizer、独立 RBAC 和 leader 选举，升级回滚互不影响。本质一句话：**CCM 之争是对象所有权之争，配合 = 划分所有权 + 各写各的资源**。

**弹药**
- ACK CCM 职责清单（分域基准）：LB Service 全生命周期（创建/监听/后端组，ECS 与 ENI 型）、节点删除自动摘 SLB 后端、ProviderID/zone 标签、Flannel 路由表；复用存量 SLB 走注解（如 `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id`）——这就是"存量资源不归 CCM 重建"的所有权声明样板
- SOFA 侧控制器的实质形态：不是重写 cloud provider，而是 **watch Service/Endpoints → 同步蚂蚁侧 VIP/LB 真实服务器组** 的 controller + IDC/裸金属节点接入器（BMCP 场景天然需要），与 ACK CCM 操作对象不重叠，所以能叠
- 真实先例：ACK One 注册集群里 IDC 自建集群的云资源各归各的 CCM、跨集群不抢所有权；公网 SLB 与机房内 VIP/单元化流量是两条物理通道，天然免冲突
- 冲突自检信号：LB 后端列表反复抖、node 标签被删、finalizer 拉锯、CCM 日志报资源不属于本控制器——先查是不是第二个控制器在抢写

**红线**｜❌ 说"直接部署第二个 CCM 就行"——完整身份双 CCM 必然抢 Node/LB；❌ 两个控制器管同一个 SLB 的同一批监听/后端组（后端列表互相覆盖）；❌ 说 K8s 官方支持多 CCM。

**下一问**｜注册集群（ACK One）把 IDC 集群接进来后，云资源所有权怎么切？（→ ACK One 注册集群）

---

## Q73. Calico 和 Terway 什么区别？什么时候选谁？（网络/CNI 选型）

**一句话**｜Terway = 阿里云自研，Pod IP 直通 VPC/ENI、云绑定换不走；Calico = 开源通用，BGP 纯路由或 IPIP/VXLAN Overlay、云厂商无关——**纯 ACK 选 Terway，多云/自建选 Calico**。

**30 秒答**｜五维对比：①**出身与绑定**：Terway 深度集成 VPC/ENI/OpenAPI，只在阿里云 ECS 上能跑；Calico 云厂商无关，自建/IDC/多云标配。②**数据面**：Terway ENIIP 无封装 ~90-95% 线速；Calico 三种模式——BGP 纯路由性能最优但**云上 VPC 不透传 BGP**（节点公告的 Pod CIDR 进不了 VPC 路由表，跨节点黑洞），云上实际默认 IPIP/VXLAN Overlay（封装损耗 ~10-30%）。③**IPAM/网络平面**（本质区别）：Terway 的 IP 从 vSwitch 分配、**活在 VPC 平面**（RDS/ECS 可直访 Pod IP）；Calico IP 自管在节点 CIDR、**活在 Overlay 平面**，与 VPC 无关。④**策略与加密**：都支持 L3/L4 NetworkPolicy；Calico 有 GlobalNetworkPolicy/DNS 策略扩展 + 开源版自带 WireGuard 加密；Terway 无 L7、无加密。⑤**云集成**：Terway 联动 SLB/ALB、Pod 级安全组、Trunking/固定 IP；Calico 全要自己搭（BGP RR、calicoctl）。判断线：先问环境再谈性能——多云统一网络的诉求直接否掉 Terway。

**弹药**
- ACK 网络插件选项只有 Terway（推荐）/ Flannel 两个；Calico 场景 = 自建 K8s、IDC、ACK One 注册集群接管的边缘/IDC 节点
- 性能排序：Terway ENI ≈ Calico-BGP(能打通时) > Terway ENIIP > Calico-IPIP/VXLAN > Flannel-VXLAN
- Calico 云上务实姿势：VXLAN + Calico IPAM；BGP 纯路由留给 IDC/自建
- 合规加密需求（Pod 流量 WireGuard）→ Calico，Terway 无对应能力
- L7 策略两家都缺 → Cilium（接 Q21 红线）

**红线**｜❌ "云上跑 Calico 用 BGP 模式"——VPC 不收动态路由，跨节点不通，本题最大的坑；❌ 说两者可共存或在线切换 CNI——建集群时选定；❌ 只背性能数字，说不出本质区别（IP 在 VPC 平面 vs Overlay 平面）；❌ 云上拿 Overlay 模式 Calico 跟 Terway 比性能还得出"差不多"。

**下一问**｜为什么 VPC 打不通 BGP？那跨集群 Pod 网络互通怎么办？（→ Pod 网段不进 VPC 路由表；跨集群靠 CEN 打通节点网段或 SLB 跨地域）

---

## Q74. Pod 没设 memory limit，为什么还会被 OOM 杀？——kubepods 总闸（节点/OOM·cgroup）

**一句话**｜kubelet 默认 `enforceNodeAllocatable=pods`，会在所有 pod 的共同祖先 cgroup（kubepods）上设 memory 总闸 = 节点 allocatable——单个 pod 不设 limit 只是没有"小笼子"，全体 pod 合用一个"大笼子"。

**30 秒答**｜四层天花板从内到外：①**容器 limit**——设了才有；②**pod 级 cgroup**——全部容器都有 limit 时才设（= 求和），有裸奔容器就不设；③**kubepods 聚合 cgroup——默认必设，= allocatable**（capacity − kube-reserved − system-reserved − eviction 预留），这就是总闸；④节点物理内存。所以"没 limit ≠ 没天花板"。总闸撞满的流程：内核**先 reclaim**（踢 page cache），anon 回收不动 → 在 kubepods 子树内触发 **cgroup OOM**（不是全局 node OOM），按 oom_score 挑人杀。v1 查 `/sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes`，v2 查 `/sys/fs/cgroup/kubepods.slice/memory.max`——值 = allocatable 即总闸存在。

**弹药**
- `cat /sys/fs/cgroup/kubepods.slice/memory.max`（v2）对照 `kubectl get node -o jsonpath='{.status.allocatable.memory}'`
- kubelet 配置：`enforceNodeAllocatable: ["pods"]`（默认值）
- 进阶：`--qos-reserved=memory`（QOSReserved feature gate，alpha）——把 QoS 等级落成 QoS 级 cgroup 硬限制，默认关

**红线**｜❌ "不设 limit 就不会被杀"——总闸在头顶；❌ 把 pod 级 cgroup 和 kubepods 聚合层混为一谈；❌ 忘了系统预留不在总闸内（这正是 kubelet 没驱逐、内核先 OOM 的原因）。

**下一问**｜总闸 OOM 挑谁杀？（→ oom_score_adj）

---

## Q75. 总闸 OOM 挑谁杀？oom_score_adj 与 memory.oom.group（节点/OOM·内核）

**一句话**｜cgroup OOM 在 kubepods 子树内挑 oom_score 最高的进程：BestEffort=1000 最先死，Burstable 按 `1000 − 1000×request/节点内存` 给分——**request 越小越先死**，Guaranteed=−997 活到最后；cgroup v2 下 kubelet 给 pod cgroup 开了 memory.oom.group，选中即整 pod 团灭。

**30 秒答**｜先与 Q35 的节点压力驱逐区分：kubelet 驱逐看节点 memory.available 低于阈值，动作是 **Evicted**（优雅驱逐、Pod 重建）；总闸 OOM 是**内核**行为，动作是 **OOMKilled**（SIGKILL，exit 137）。本案关键：pod 把 allocatable 吃满时，系统预留的内存还空着——节点 MemAvailable 仍高于 eviction-hard（100Mi），所以 kubelet 不动、内核先动手，这就是"为什么是 OOMKilled 而不是 Evicted"。Burstable 分数公式背下来：request 越小 → 分数越高 → 越先被挑。系统组件（CSI/CNI）没 limit 但 request 高 = 分数低 = 相对安全——**request 是保命符，limit 是误杀器**（Q73→Q74 闭环）。

**弹药**
- 容器内验证：`cat /proc/self/oom_score_adj`（Guaranteed ≈ −997，BestEffort = 1000）
- v2 查 pod 是否开团灭：`cat /sys/fs/cgroup/kubepods.slice/kubepods-pod*.slice/memory.oom.group`
- 区分证据：`kubectl get pod` 的 REASON——OOMKilled（内核）vs Evicted（kubelet）

**红线**｜❌ 把 cgroup OOM 和 kubelet 驱逐混为一谈（两层两套机制）；❌ 系统组件 BestEffort 裸奔还想活到最后；❌ 忘了 v2 的 memory.oom.group——一次 OOM 事件可能整 pod 全灭，kill 计数虚高。

**下一问**｜一夜 60 次 OOM 怎么查？（→ 案例复盘）

---

## Q76. 案例复盘：单节点一夜 60+ 次 OOM，排查五步（节点/排障·工单）

**一句话**｜五步：dmesg 看 constraint → 确认 kubepods 总闸存在 → memory.stat 拆 anon/file → 对时间轴找定时任务 → 看被杀名单验证 oom_score 规律；60 次的形态 = 杀→释放→重启→重建缓存→再杀的循环。

**30 秒答**｜现象：一个节点十几个 pod（全 Burstable：有 request 无 limit），凌晨 3 点起 OOM 60+ 次。①`dmesg -T | grep oom`——出现 `Memory cgroup out of memory` = cgroup 层、锁定总闸；全局 OOM 会打 Nodes information；②确认 `kubepods memory.max` = allocatable，排除有人手设了怪值；③`memory.stat` 拆账：**anon 大 = 真涨**（泄漏/批任务），**file 大 = page cache 挤闸**（写日志读大文件 charge 给发起容器，接 Q2 的账本）；④对时间轴：3 点整开始 = 定时任务（备份/日志压缩/报表批算）是诱因不是受害者；⑤被杀名单按 oom_score 排——死的多是 request 最小的 pod，验证 Q75 公式。60 次的解释：单次 OOM 只释放一点 → 容器重启缓存重建又涨回 → 循环；v2 memory.oom.group 让一次选中算多条 kill，计数虚高。

**弹药**
- `dmesg -T | grep -iE "oom|killed process"` — 第一现场
- `cat /sys/fs/cgroup/kubepods.slice/memory.current` vs memory.max 看水位比
- `kubectl get events --field-selector reason=OOMKilling` + 容器 restartCount 对照时间分布

**红线**｜❌ 看到 OOM 就把 limit 全调大——先分清 cgroup 层还是节点层（Q2 红线复刻）；❌ 只治受害者不查凌晨 3 点的诱因；❌ 把 60 次当 60 个问题。

**下一问**｜治完之后怎么防复发？（→ 治理三板斧）

---

## Q77. 治理三板斧：业务关笼子、系统穿盔甲、诱因拆弹（节点/OOM·治理）

**一句话**｜业务 pod 设 limit（关笼子）+ request 校准；系统组件（CSI/CNI）limit 保持空、request 调高（穿盔甲）；再拆掉凌晨 3 点的定时任务诱因——三件缺一不可。

**30 秒答**｜①**业务 pod 关笼子**：limit = 3~4×request 留 headroom，request 按真实水位校准——每个 pod 有自己的小天花板，公共总闸就摸不到；②**系统组件穿盔甲**：CSI plugin 这类"挂了全场遭殃"的组件，**limit 别加**（格式化/卷扩容尖峰会被误杀 → 整节点卷操作停摆），但 **request 必须调大**（拉低 oom_score_adj），再叠 PriorityClass 双保险；③**诱因拆弹**：凌晨 3 点的备份/日志压缩/批任务——错峰、限流（worker 数×内存上限 < 节点余量）、或污点隔离到独立节点池。防复发监控：kubepods `memory.current/memory.max > 85%` 告警（比 dmesg 早半天）+ OOMKilled 事件告警。判断线：治理目标不是"永不出 OOM"，是"总闸永远不该成为最先撞上的那面墙"。

**弹药**
- 告警两件套：kubepods 水位比 + OOMKilling 事件流
- 批任务约束：requests/limits 写死 + 独立节点池（污点隔离）或低峰窗口
- 演练：节点内存压测复现总闸 OOM，验证被杀顺序符合 oom_score 预期

**红线**｜❌ 全员 BestEffort 裸奔；❌ 给 CSI/CNI 加了 limit 还纳闷卷操作为什么偶发失败；❌ 只调参不查定时任务——下个月凌晨 3 点再来一轮。

**下一问**｜节点内存水位监控之外，kubepods 的 CPU 侧总闸（cpu.max）对延迟敏感服务意味着什么？（→ 呼应 Q40 CPU Manager / Q47 CFS 限流）

---

## Q78. ACK One 为何选型 OCM？有没有别的选法？（ACK One/多集群·选型）

**一句话**｜ACK One 只在**纳管层**选了 OCM（集群注册+下发通道），调度层是 Karmada 内核——根本原因：OCM 是阿里云+蚂蚁+Red Hat 2021-11 联合发起开源的项目（阿里是发起方不是采购方），纳管层用自己参与定义的开放标准做"薄底座"，换来分发引擎可整体替换（KubeVela→Karmada 切换而纳管层纹丝不动）。

**30 秒答**｜先分层再谈选型，Fleet 四层各选其长：**纳管 OCM**（ManagedCluster 注册表 / klusterlet agent 出站反连 / ManifestWork 下发 / add-on 托管扩展组件）+ **调度 Karmada 托管变体**（PropagationPolicy 切副本/动态权重/重调度，API 组 `policy.one.alibabacloud.com`）+ **交付托管 ArgoCD** + **数据面 MSE/ALB 网关与多集群 Service**。五条选 OCM 的逻辑：①**同源发起**——2021-11 Red Hat+蚂蚁+阿里云共同开源，OCM 是 Red Hat ACM 的上游，技术同源深度协同；②**职责匹配**——OCM 恰好只做注册/证书轮换/下发通道/add-on 这些无差异化价值的"薄底座"，不做副本调度；③**解耦实证**——Fleet 分发形态 2023 年是 KubeVela Application、后来整体切到 Karmada 语义，纳管层零改动；④**生产背书**——ACM 上游经 Red Hat 企业客户打磨；⑤**开放性**——`kubectl get managedclusters` 是标准 OCM 语义，客户接入层不被私有协议锁死。备选为何出局：Karmada 全栈（定位独立多集群调度控制面，CNCF 官方口径与 OCM 互补而非替代）；自研通道（Anthos GKE Hub/Azure Arc 路线，成立但丢开放性叙事）；Clusternet（腾讯主导）；KubeFed（社区停滞）；Rancher/ACM（竞对商业产品+无副本级调度）。

**弹药**
- CNCF 成熟度反差题："OCM 还是 Sandbox，为何敢选？"——OCM 2021-11-09 进 CNCF Sandbox 至今；Karmada 2021-09 Sandbox → 2023-12 Incubating → 现已 Graduated。答法：纳管层看生产背书（ACM 上游）和发起方身份，不看徽章；调度层要社区活力，选了毕业项目——每层按最重的权重选
- 现象级证据：Fleet 上 `kubectl get managedclusters`（HUB ACCEPTED/JOINED/AVAILABLE 字段就是 OCM clusteradm 语义）、AMC CLI、`work.one.alibabacloud.com` API 组
- 网络安全点：klusterlet 只**出站**反连 Hub（mTLS+证书轮换），IDC 防火墙不用开任何入站端口；注册集群另有受限模式（agent 不读取集群内其他信息）
- 边界金句："Placement 只决定选哪些集群，不决定每家分几个副本"——副本切分是上层 Karmada 的事，这也是为什么 OCM 上必须叠调度引擎
- 云厂商对照：Anthos=GKE Hub 自研、Azure Arc=自研 agent+开源 KubeFleet 编排、AWS=EKS Connector——自研纳管是主流，阿里云开源同源才是差异点；云厂商真正的差异化在调度内核与云产品数据面，底座开放反而扩大地盘

**红线**｜❌ 说"ACK One 是基于 Karmada 的产品"——全称命题错：纳管 OCM、调度 Karmada、交付 ArgoCD；❌ OCM=Red Hat ACM——OCM 是上游开源，ACM 是商业发行版；❌ 把 OCM 说成多集群调度器——Placement 只选集群不切副本；❌ 说开源 Karmada YAML 可原样搬进 Fleet——apiVersion 是 `policy.one.alibabacloud.com`/`work.one.alibabacloud.com`。

**下一问**｜注册集群（ACK One）把 IDC 集群接进来后，云资源所有权怎么切？（→ 兑现 Q72 的钩子，呼应 Q72 分域共治）

---

## Q79. CSI 挂载链路：Attach → Stage → Publish 各是谁在做？卡住怎么区分？（存储/CSI·挂载链路）

**一句话**｜Pod 调度后卷过三道门——**Attach**（AD Controller 建 VolumeAttachment → external-attacher 调 ControllerPublishVolume 把卷挂到节点）、**Stage**（kubelet 调 NodeStageVolume 格式化+挂节点全局目录）、**Publish**（kubelet 调 NodePublishVolume bind mount 进 Pod 目录）。

**30 秒答**｜三层粒度不同：Attach 是**集群级**（每卷每节点一次，由 KCM 里的 AD Controller 建 VolumeAttachment 对象、CSI external-attacher sidecar 执行云 API attach）；Stage 是**节点级**（每卷每节点一次，格式化 + 挂到 `/var/lib/kubelet/plugins/kubernetes.io/csi/pv/<pv>/globalmount`）；Publish 是 **Pod 级**（每 Pod 一次，bind mount 到 `/var/lib/kubelet/pods/<uid>/volumes/...`）。报错对号入座：`FailedAttachVolume` = Attach 层卡（Multi-Attach、VolumeAttachment 卡住、云 API 失败）；`FailedMount` = Stage/Publish 层卡（驱动节点侧问题、文件系统损坏）。attach/detach 的 owner 是控制面，stage/publish 的 owner 是 kubelet——这条分界先说清。

**弹药**
- `kubectl get volumeattachment` — Attach 层总账；卡住看 finalizer `attacher-kubernetes.io/csi-*`
- describe pod Events 顺序：`SuccessfulAttachVolume` → `SuccessfulMountVolume`，卡在哪步一目了然
- Multi-Attach 处理路径：先确认旧节点是否假死未 detach → force detach（删 VolumeAttachment）→ 云控制台确认无残留挂载再重试

**红线**｜❌ 说 Attach 是 kubelet 干的——Attach 在控制面（AD Controller），kubelet 只管 Stage/Publish；❌ Multi-Attach 去重启 Pod——先查 VolumeAttachment 和旧节点状态；❌ Stage 和 Publish 混为一谈（一个每节点一次，一个每 Pod 一次）。

**下一问**｜删除 PVC 时，这条链路怎么倒着放完？

---

## Q80. PVC 删除的完整链路？pvc-protection finalizer 干什么的？（存储/删除链路·finalizer）

**一句话**｜删 PVC → `kubernetes.io/pvc-protection` finalizer 卡住等所有 Pod 停用 → PVC 消失 → PV 按 reclaimPolicy 分叉：Retain 转 Released 留数据；Delete 走 CSI DeleteVolume 删底层卷再删 PV 对象。

**30 秒答**｜**pvc-protection** 是 PVC 被任何 Pod 使用时自动加的 finalizer，删除时 PVC 卡 Terminating 直到没有 Pod 引用它——防止"PVC 删了、卷还挂在活 Pod 上"。PV 侧有对称的 pv-protection（绑定期间防删）。Delete 分支：PV 加 deletionTimestamp → external-provisioner 发现 claimRef 指向已消失的 PVC → 调 **CSI DeleteVolume**（真删云盘）→ 删 PV 对象。卡 Terminating 三层：①Pod 还在用（正常等待，先删 Pod）；②finalizer 残留（确认无使用后 patch 清空——🔴 不可逆）；③底层卷删除失败（云侧资源锁/依赖，看 provisioner 日志）。

**弹药**
- `kubectl get pvc <p> -o jsonpath='{.metadata.finalizers}'` — 看是谁在挡
- 🔴 `kubectl patch pvc <p> -n <ns> -p '{"metadata":{"finalizers":null}}'` — Delete 策略下执行 = 手动触发删云盘
- `kubectl get pv | grep Released` — 删除后的数据黄金窗口（Q49）

**红线**｜❌ 见 Terminating 就清 finalizer——先确认没有 Pod 在用、数据真的不要了；❌ Delete 策略下清 finalizer 当"解卡技巧"——那是按下删除云盘的按钮。

**下一问**｜底层卷真删了还能救吗？——救不了，所以要有快照（下一题）。

---

## Q81. VolumeSnapshot 快照与恢复？快照能当备份吗？（存储/数据保护）

**一句话**｜CRD 三件套 VolumeSnapshot（用户声明）/ VolumeSnapshotContent（底层实体）/ VolumeSnapshotClass（驱动+deletionPolicy），CSI CreateSnapshot 实现；恢复 = 新 PVC 写 `dataSource`；**快照 ≠ 备份**。

**30 秒答**｜动态快照走 CSI **CreateSnapshot**（存储层写时复制，秒级、不拷全量数据）；恢复时新建 PVC 的 `dataSource` 指向 snapshot，CreateVolume 从快照长出新卷。两个必答口径：①**一致性**——快照是崩溃一致性（crash-consistent），数据库要应用一致需先暂停写入/fsfreeze 或靠 WAL 重放；②**备份关系**——快照与原卷同集群同存储域，集群/区域级故障一起没，文件级误删也不归它管（它还原的是时间点卷状态）；生产姿势 = CSI 快照 + Velero 编排 + 关键数据跨区域副本。deletionPolicy 用 Retain，防止删 snapshot 连 content 带底层快照一起没。

**弹药**
- `kubectl get volumesnapshot,volumesnapshotcontent,volumesnapshotclass`
- 恢复模板：PVC 加 `dataSource: {name: <snap>, kind: VolumeSnapshot, apiGroup: snapshot.storage.k8s.io}`
- Velero 关系：Velero 调 VolumeSnapshot 做编排 + 备份元数据；裸 CSI 快照只管卷不管对象

**红线**｜❌ "有快照就不用备份"；❌ 数据库快照前不 quiesce（恢复出来的是崩溃现场，InnoDB 能救但别赌）；❌ deletionPolicy: Delete 还敢拿快照当唯一副本。

**下一问**｜卷建好了，调度器怎么知道该把 Pod 放到哪个 AZ？→ 拓扑。

---

## Q82. 卷拓扑与 CSIStorageCapacity：调度器怎么知道卷能不能建在哪？（存储/拓扑·调度）

**一句话**｜CSI 驱动经 NodeGetInfo 在每个节点的 CSINode 上注册拓扑键（如 zone），CreateVolume 带 accessible_topology；WaitForFirstConsumer 让调度器先定 Pod 落点再供卷，CSIStorageCapacity 让调度器还能感知各拓扑域剩余容量。

**30 秒答**｜链路四层：①CSI 驱动 DaemonSet 在节点上向 CSINode 注册 `topologyKeys`——**拓扑是驱动在节点上声明的，不是 SC 的属性**；②SC 可写 `allowedTopologies` 收窄建卷范围；③**WaitForFirstConsumer**（Q54）下调度器先算出 Pod 落点，把拓扑需求传给 provisioner，卷建在同 AZ——Immediate 模式没有这一步，建错 AZ 挂载必失败；④**CSIStorageCapacity**（1.24 GA）：驱动上报各拓扑域剩余容量，调度器提前否掉"这个 AZ 建不出 1Ti 盘"的调度，避免 Pod 卡过去再失败。静态 local PV 不走这套——nodeAffinity 直接参与调度过滤。

**弹药**
- `kubectl get csinode <node> -o yaml | grep -A5 topology` — 看节点上驱动注册了什么键
- `kubectl get csistoragecapacities` — 各拓扑域容量账本
- 判断线：Pod 卡调度且 Events 提到 volume topology/node affinity → 拓扑层，别去查 CPU 内存

**红线**｜❌ 说拓扑信息存在 SC 里——SC 只消费，键是驱动在 CSINode 上注册的；❌ 节点上 CSI 驱动没起来还怪调度——查 CSINode 有没有注册。

**下一问**｜accessModes 里那个"一字之差"的 RWOP，实现上差在哪？

---

## Q83. RWOP 怎么实现的？和 RWO 一字之差差在哪？（存储/accessModes 进阶）

**一句话**｜RWO 约束**节点**（同节点多 Pod 共享合法），RWOP 收紧到**全集群单 Pod**——第二个挂载同一 PVC 的 Pod 直接被拒；版本线 1.22 alpha / 1.27 beta 默认开启 / **1.29 GA**。

**30 秒答**｜实现两层拦截：①调度器把"该 RWOP PVC 已被 Pod A 使用"算进调度 predicate，Pod B 无论落到哪个节点都不可调度（跨节点撞卷约束、同节点撞 kubelet 的 VolumeRestrictions 检查）；②kubelet 兜底拒绝同节点二次挂载。**版本敏感点**：GA 语义要求全节点 kubelet ≥1.29——滚动升级到一半时语义不完整，答这题先报版本（Q36 同款考法）。适用：单写者数据库、独占加密卷；多副本共享存储还是要 RWX（NAS/CephFS）。

**弹药**
- `accessModes: ["ReadWriteOncePod"]` — 写法与 RWO 一字之差
- 被拒样式：调度 Events 提 node conflict / volume node conflict
- 面试反问位："同节点两个 Pod 共享 RWO 合法吗？"——合法，这正是 RWOP 存在的理由（Q51）

**红线**｜❌ 版本报不出 1.29 GA；❌ 认为 RWOP 能让多节点同时读写（那是 RWX 的活）；❌ 混合版本集群上 RWOP 当 GA 用。

**下一问**｜临时任务要真盘、跑完不留遗产，用什么？→ Generic Ephemeral Volume。

---

## Q84. Generic Ephemeral Volume 和 emptyDir、普通 PVC 差在哪？（存储/卷类型）

**一句话**｜Generic Ephemeral = Pod spec 内嵌 `volumeClaimTemplate` 的"随 Pod 生死"PVC——Pod 起时动态建 PVC，Pod 删时 PVC 级联删；**有 PVC 全部能力，生命周期随 Pod**。

**30 秒答**｜三者对照：**emptyDir** = 节点本地临时目录，无供给能力（没有 SC/配额/快照），Pod 挪节点即消失；**Generic Ephemeral** = 真 PVC——任意 SC 动态供给、可扩容、可快照，Pod 删除时由控制器级联删 PVC（reclaimPolicy 随之生效，Delete 策略下底层卷真删）；**普通 PVC** = 生命周期完全独立，手工管理。典型场景：Job/CI 要高性能真卷（ESSD）但跑完不留遗产。辨析：别和 **CSI ephemeral inline 卷**混淆——inline 是驱动直接给的临时卷，没有 PV/PVC 对象、不能扩容快照。

**弹药**
- 写法：`volumes: [{name: scratch, ephemeral: {volumeClaimTemplate: {spec: {...}}}}]`
- PVC 自动命名 `<pod-name>-<volume-name>`，kubectl get pvc 直接能看到
- 判断线："要不要真持久卷能力 + 要不要随 Pod 清理"——两要 = Generic Ephemeral

**红线**｜❌ 和 CSI inline ephemeral 混为一谈（一个有 PVC 对象一个没有）；❌ ephemeral + reclaimPolicy: Delete 还指望数据活过 Pod——设计上就是随删随走。

**下一问**｜卷挂上了，容器里却 Permission denied——为什么？

---

## Q85. 容器里写卷 Permission denied：fsGroup 与 root squash？（存储/排障·权限）

**一句话**｜两类根因机理完全不同——**Pod UID 与卷上文件属主不匹配**（fsGroup 解决），或**存储服务端身份映射**（NFS root squash），先分清再动手。

**30 秒答**｜**fsGroup**：`spec.securityContext.fsGroup` 让 kubelet 挂载时对卷目录递归 chgrp + setgid，容器进程以 supplementary group 获得写权限；大存量目录首次挂载会因递归改属卡慢（FailedMount 超时的冷门成因），1.22+ 用 `fsGroupChangePolicy: OnRootMismatch` 只检查根目录提速。**NFS root squash**：服务端把客户端 root(uid 0) 映射成 nobody——容器里是 root 也写不进 export，解法在 NFS 服务端（对齐 UID/GID 或评估后 no_root_squash），客户端改什么都没用。**云盘没这类问题**——块设备由应用自己格式化，文件系统权限随首格式化生成。判断线：先 `id` 看进程身份、`ls -ln` 看卷上数字 UID，对不上号再选药方。

**弹药**
- `kubectl exec <p> -- id` + `kubectl exec <p> -- ls -ln /data` — 两头对数字
- 挂载慢的 Event：`FailedMount` + 耗时异常 + fsGroup 字样 → OnRootMismatch
- runAsNonRoot 应用配 fsGroup 是标准组合（restricted 档位不冲突）

**红线**｜❌ 图省事容器跑 root + NFS no_root_squash 全开（双重安全红线）；❌ 大目录 fsGroup 当挂载卡死，去重启 kubelet。

**下一问**｜这么多环，平时监控盯哪几个数？

---

## Q86. PV/PVC 该监控什么指标？（存储/可观测）

**一句话**｜四类——容量（`kubelet_volume_stats_*`，bytes + **inodes 双口径**）、对象状态（PVC 非 Bound / PV Released·Failed）、供给链路（provisioner 错误率 + VolumeAttachment 卡住时长）、性能（云盘 IOPS/吞吐限速）。

**30 秒答**｜**容量**：`available_bytes/capacity_bytes` < 15% 告警（磁盘满是第一故障源）；`inodes_free` 单独告警——小文件场景（日志碎片、session、临时文件）容量没满 inode 先爆，和内存 page cache 一样是"隐性账本"（Q2）。**状态**：PVC phase != Bound 常驻即告警；PV Released 计数——那是被遗忘的数据；VolumeSnapshot 失败率——备份断了要早知道。**链路**：csi-provisioner 日志错误率、VolumeAttachment 存在超时（Attach 层卡住的量化）。**性能**：云盘是硬限速（ESSD 按 PL 级别给基线），IOPS 打满的表现是"应用慢但所有状态正常"——限速水位纳入告警，别等业务报慢。

**弹药**
- PromQL：`kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.15`
- `kubelet_volume_stats_inodes_free` — inode 水位
- 巡检一行：`kubectl get pvc -A --field-selector=status.phase=Pending` + `kubectl get pv | grep -c Released`

**红线**｜❌ 只看 bytes 不看 inodes（爆得最早的反而不在监控里）；❌ Released PV 没告警——那是删除链路断掉的信号（Q80）；❌ 云盘按"本地盘无限速"的假设做容量规划。

**下一问**｜本专题（PV/PVC 基础 Q48–Q57 + 进阶 Q79–Q86）已收束成 KB 深挖笔记 `22-概念/04-存储/03-pv-pvc-lifecycle-deep-dive.md`——下一专题：HPA/VPA（Q47 的另一半钩子）。


## Q87. "外部 controller 用 patch 更新 Service，会冲掉别的 controller 加的 finalizer"——这话对吗？（API 机制/finalizer·并发写）

**一句话**｜方向反了——patch 的基准是**服务端最新对象**，patch 体不碰 `finalizers` 字段就动不了别人的锁；会覆盖的是**不带 resourceVersion 的整体 update（PUT）**，基准是你手里可能过期的副本；再精确一层：默认 strategic merge patch 对 finalizers 是**并集语义**（加得进、删不掉），真正能误伤的只有 `--type=merge` 携带旧列表、SSA 认领后漏带、和无 RV PUT 三种。

**30 秒答**｜先补背景：finalizer 是"删除锁"，`metadata.finalizers` 非空时 DELETE 只写 deletionTimestamp，对象卡 Terminating。LB Service 链路：CCM 打 `service.kubernetes.io/load-balancer-cleanup` → 用户 delete → CCM watch 到 deletionTimestamp、清云 SLB/EIP → 摘锁 → 真删。关键机关在**存储层**：finalizer 判定不在任何 controller，任何一次让 finalizers 变空的写操作都顺带触发真删。所以无 RV 整体 PUT 冲掉 finalizer 的瞬间，存储层发现"deletionTimestamp 已设 + finalizers 空"→ 立即硬删 → CCM 清理逻辑永远不执行 → 云资源孤儿、持续计费。四种 patch 对 finalizers 行为完全不同：strategic=并集（误伤风险最低）；merge patch（RFC 7386）=整体替换、null 清空；json patch=精确 op 但盲删索引有竞态；SSA=所有权语义、认领后漏带别人条目会静默移除。正确姿势：finalizer 增删用 `controllerutil.Add/RemoveFinalizer` + `Update()` + 冲突重试，改其他字段用最小 patch，SSA 不认领 finalizers。

**弹药**
- 嫌疑人排序（谁在做无 RV PUT）：自研 operator 两种反面教材（凭空构造对象只填几个字段就 `Update()`；对账逻辑把陌生 finalizer 当漂移清掉）＞ CI/CD 的 `kubectl replace -f`/`apply --force` ＞ 多集群搬运工具（副本是另一个集群的版本，RV 天然对不上）＞ 平台层 label/annotation 例行回写。官方 KCM/CCM 全走 informer + RV + `RetryOnConflict`，是 finalizer 的守护者不是破坏者——真实踩坑组合是"CCM 守规矩 + 自研 operator 不守规矩"
- 解卡命令即语义证据：`kubectl patch svc x -p '{"metadata":{"finalizers":null}}' --type=merge` 能生效，恰恰证明 merge patch 对 finalizers 是替换语义；摘锁前必须逐项确认外部资源已清理（云控制台核对 SLB/EIP/监听器），否则亲手制造孤儿
- 姊妹卡：Q80 的 `kubernetes.io/pvc-protection` 是同一条删除锁的存储侧实例（PVC 卡 Terminating 就是它），本卡讲的是这条锁**被并发写破坏**时的 API 机制——两卡合起来是"finalizer 正向链路 + 反向事故"全图
- 事后排查：`managedFields` 看哪个 fieldManager 拥有 `f:finalizers`；audit log 找 requestObject 无 resourceVersion 的 PUT（audit 默认不开，需提前覆盖写请求）
- 与 GC 分界：ownerReference 级联管"集群内从属对象跟着删"，finalizer 管"删除前清集群外欠账"，两者可共存
- 陷阱价值：题目本身是个方向性陷阱，先纠偏再分层作答，比背"patch 更安全"高一个段位

**红线**｜❌ 说"默认 strategic patch 会覆盖别人的 finalizer"——并集语义只加不删；❌ 说"patch 天然安全可以随便写 finalizers 字段"——`--type=merge` 携带旧列表同样整体替换；❌ 对共享对象做无 RV 整体 PUT；❌ 不确认外部资源清理就手工摘 finalizer；❌ controller 用 SSA 认领 `metadata.finalizers`。

**下一问**｜对象删除时，ownerReference 级联（GC）和 finalizer 谁先动？有没有两者都管不到的删除死角？（→ 呼应 [[22-概念/01-核心架构/02-garbage-collection-owner-reference.md|GC 级联删除]]）

---

---

> **Q88–Q93：集群组件·扩展与治理层专项**（2026-10-20 追加）。Q13–Q20 覆盖核心组件本身，这批补齐围绕 apiserver 的扩展与治理层：请求链路总纲 / CRD vs 聚合 apiserver / 准入 webhook / CCM / 指标链路 / TLS Bootstrap。底料与展开版（含全景串联图）：`01-集群基础/03-控制平面/42-cluster-extension-and-governance-deep-dive.md`。与 Q87（finalizer 并发写）互补：那卡讲单点 API 机制，这批讲整条治理链。

## Q88. 一个写请求进 apiserver，要过几道门？（控制面/链路总纲）

**一句话**｜六道门按序背：认证（你是谁）→ 授权（你能干什么）→ Mutating 准入（改写请求）→ Schema 校验（结构对不对）→ Validating 准入（语义行不行）→ etcd 落库——所有扩展机制（webhook、CRD、聚合层）全挂在这条链上。

**30 秒答**｜①**认证**：x509 / SA token / OIDC，认不出 → **401**；②**授权**：RBAC（默认）/ Node（kubelet 专用）/ Webhook，拒了 → **403**；③**Mutating admission**：可改对象（sidecar 注入、默认值），并发跑；④**Schema 校验**：OpenAPI 字段级，错 → **422**；⑤**Validating admission**：只答 yes/no（ResourceQuota、EventRateLimit、OPA/Kyverno）；⑥**etcd**：落库即广播 watch，全集群控制器从这一刻开始干活（Q87 的 finalizer 判定也发生在存储层这一步）。判断线：**401 查身份、403 查权限（`kubectl auth can-i --list` 以对方身份看）、422 查对象**——三者排查方向完全不同。这条链也是性能链：慢的 authz webhook、准入 webhook 是 Q9 里 etcd 之外的两大嫌疑人。

**弹药**
- 顺序铁律：**Mutating 先于 Validating**——validation 看到的是改写后的最终对象；Schema 校验夹在两者之间
- 授权是**加法没有 deny**：RBAC 只能"少给"不能"给全再扣"；`system:` 前缀的 view/edit/admin/cluster-admin 是内置角色
- Node 授权专门给 kubelet（只读写本节点相关对象），和 Q18 的 CRI 链呼应：kubelet 不是普通 RBAC 用户
- 排障：`kubectl get apiservice` 看聚合层、audit log 看被拒的 verb/resource、`apiserver_admission_webhook_admission_duration_seconds` 抓慢 webhook

**红线**｜❌ 把 403 当 401 处理（重启 kubelet 换证书——其实是 Binding 没给）；❌ 准入 webhook 的 namespaceSelector 不排 `kube-system`——一个拦截控制面的 webhook 能打死全集群；❌ 背成"先校验后准入"——顺序错说明没真懂 Mutating 为什么要先跑。

**下一问**｜这六道门里，哪几步是在 etcd 之前就可能"写坏"数据的？（→ Mutating 改写后的对象若 Validating 没兜住，落库的就是"你没提交过的东西"；并发写竞态见 Q87）

---

## Q89. CRD 和聚合 apiserver，扩展 API 走哪条路？（API 扩展）

**一句话**｜数据模型是"存对象"走 CRD（零代码、apiserver 替你存 etcd 出 REST）；是"实时计算/外置存储"走聚合 apiserver（挂一个完整 apiserver 在主 apiserver 后面，独立扩缩、与主 etcd 故障隔离）。

**30 秒答**｜CRD：写个 YAML 声明资源类型，apiserver 替你做存储（主 etcd）+ REST + watch，业务逻辑放 operator 的调和循环——几乎所有 operator 这么干。聚合 apiserver：通过 APIService 把 `metrics.k8s.io` 这类 API 组**路由**到你自己实现的完整 apiserver，典型就是 **metrics-server**——指标是实时算的、只留内存态，绝不能压进主 etcd。选型判断线：问自己"这个数据放主 etcd 会不会伤它"——会，就聚合层。CRD 的两个隐藏成本必背：**conversion webhook**（多版本 API 转换）和 **structural schema** 约束；CRD 高频写大对象 = 自己种下 Q11 的大 LIST 问题。

**弹药**
- `kubectl get apiservice` — 看集群挂了哪些聚合 API、Ready 与否；`kubectl top` 没数据第一刀就是查 `v1beta1.metrics.k8s.io` 的 APIService
- CRD 版本策略：每版本一份 schema + conversion webhook 在版本间转；storage version 只能有一个，降级前先确认
- 聚合层断连表现：APIService NotReady 后该 API 组整体 503/404，主 API 不受影响——故障隔离正是卖点
- etcd 侧约束同 Q10/Q11：CRD 对象也占 db size、也进 `apiserver_storage_objects` 排行

**红线**｜❌ "CRD 比聚合层低级"——metrics-server、Istio 生态反而是聚合层用户，路线由数据模型决定；❌ CRD 塞 MB 级大对象高频更新——主 etcd 的 fsync 和 db size 替你买单；❌ 忘了 CRD 删除的级联（CRD 一删，**所有实例对象全没**）。

**下一问**｜CRD 的 storage version 升级后，旧版本对象还在 etcd 里，怎么无损迁移？（→ storage migration：逐对象读-写一遍触发转换，或等 K8s 自动迁移特性）

---

## Q90. 准入 webhook 是怎么把集群搞挂的？（控制面/准入）

**一句话**｜准入 webhook 是治理的最后一道闸，也是 Q9"慢 webhook"的真身：failurePolicy 选 Fail，webhook 一挂相关资源全部阻塞；timeout 默认 10s，webhook 卡 10 秒 = 每个写请求卡 10 秒。

**30 秒答**｜两类：**Mutating**（先跑，可改对象，典型 sidecar 注入）→ **Validating**（后跑，只答 yes/no，典型 OPA/Gatekeeper、Kyverno）。四种搞挂姿势：①**failurePolicy=Fail** + webhook 挂 → 新 Pod 全部起不来（安全策略该 Fail，可用性敏感的注入类才考虑 Ignore——取舍本身就是面试题）；②**timeout 卡顿**：webhook 依赖的数据库抖了，全集群写请求跟着抖；③**范围失控**：namespaceSelector/objectSelector 没排除 kube-system，拦截到控制面组件自己的请求，鸡生蛋死锁；④**sideEffects 没声明 None**：server dry-run 不敢调你，CI 直接报错。治法四件套：范围必收敛、超时收紧（3–5s）、副本 ≥2 且与主集群故障域分离、给 webhook 自己的 namespace 打豁免标签。

**弹药**
- `kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations -o wide` — 审计范围和 failurePolicy
- apiserver 日志 `failed calling webhook "<name>"` — 定位是哪只 webhook 在挡
- `apiserver_admission_webhook_admission_duration_seconds` 按 name 拆 P99 — Q9 排查清单第 5 步的落点
- 默认不开的重要插件：EventRateLimit、PodNodeSelector 要显式 `--enable-admission-plugins`——"哪些默认开"是高频追问
- 自救最后一刀：`kubectl delete validatingwebhookconfiguration <坏的>`（先用 namespaceSelector 豁免能救命，别到这步）

**红线**｜❌ 策略引擎（Kyverno/OPA）装完不配 namespaceSelector 就全量启用——等于给 apiserver 植入单点；❌ webhook 服务和它要保护的集群同故障域——脑裂时策略和业务一起死；❌ Mutating webhook 里调外部 API 做重活——所有写请求串行等你。

**下一问**｜Mutating 改过的对象，Schema 校验和 Validating 各看到哪一版？（→ Schema 校验 Mutating 后的版本，Validating 看最终版；顺序背错这题就翻车）

---

## Q91. cloud-controller-manager 是干嘛的？没有它会怎样？（云控制器）

**一句话**｜CCM 是"跟云 OpenAPI 说话"的独立二进制：四类控制器（cloud-node / cloud-node-lifecycle / route / service）负责 Kubernetes 对象与云资源之间的调和——没有它，数据面照跑，但 LB 分不出 IP、VPC 路由不下发、节点缺 providerID 云盘挂不上。

**30 秒答**｜背景：云相关代码原来内嵌在 KCM/kubelet，换云要重编控制面、云 API 故障拖死控制器——所以拆出独立 CCM，与 KCM 同构（list-watch 调和）不同责（调和"K8s ↔ 云资源"）。四控制器四种症状：①**cloud-node**：节点初始化填 `spec.providerID`、打 region/zone 标签——缺 providerID，之后所有云盘 attach 失败；②**cloud-node-lifecycle**：云侧已释放的机器 → 删集群 Node——失效则退了机器 Node 还挂，调度往死节点派 Pod；③**route**：Pod CIDR→节点路由下发给 VPC——失效则跨节点 Pod 不通（VPC 路由模式）；④**service**：LoadBalancer Service → 创建/绑定 SLB——失效则 LB 一直 pending。专有云/混合云可能出现**双 CCM 分域共治**，边界看"谁拥有那份云资源"（呼应 Q72）。

**弹药**
- `kubectl get nodes -o custom-columns=NAME:.metadata.name,PROVIDER:.spec.providerID` — providerID 是 CCM 干活的证据
- 判断"要不要 CCM"：自建 IDC 无云 API → 不需要；托管云 → 控制面自带；专有云 → 看厂商方案
- CCM 挂了 kubelet 不挂——区分"云集成瘫"和"节点瘫"：Node Ready 但 LB/路由/providerID 异常 = 只查 CCM
- 云 API 限流（OpenAPI QPS）会反过来卡 CCM 调和——批量建 LB Service 时限流放大
- CCM 摘 finalizer 是 LB Service 删除链路的关键一步——摘锁前的清理职责见 Q87 的反例

**红线**｜❌ "CCM 挂了集群 NotReady"——恰好相反，它管云集成不管数据面；❌ 把 in-tree 云提供商代码和 CCM 混为一谈——新版本集群一律 external CCM；❌ VPC 路由不下发归因 CNI——云厂商实现里 route 下发是 CCM 的活，先分清再查。

**下一问**｜双 CCM（公有云 + 专有云）同时跑，节点被两个 CCM 同时调和会怎样？（→ 分域：按 providerID/标签分片各管各的云资源——Q72 分域共治的组件落地）

---

## Q92. kubectl top 的数字从哪来？metrics-server 和 Prometheus 什么关系？（可观测/指标链路）

**一句话**｜链路固定四跳：容器 cgroup → kubelet 内嵌 cAdvisor 汇总成 Summary API → metrics-server 每 60s 拉取、内存态聚合 → 以聚合 apiserver 形式暴露 metrics.k8s.io 给 kubectl top 和 HPA——它不是 Prometheus 的对手，是"喂 HPA"的专职短记忆。

**30 秒答**｜分工一句话：**Prometheus 管看清历史，metrics-server 管喂饱当下**。metrics-server 只存最近聚合值（内存态、重启即丢），专职给 HPA 资源指标和 `kubectl top` 供数；Prometheus 拉式采集全量、存时序、做告警。HPA 要按业务指标（QPS/连接数）扩缩时 metrics-server 不够——走 custom.metrics.k8s.io（prometheus-adapter）或 external.metrics。排障第一刀：`kubectl get apiservice | grep metrics` 看 `v1beta1.metrics.k8s.io` 是否 Available——metrics-server 本身就是聚合 apiserver（接 Q89），APIService NotReady 链路就断在聚合层。采样口径：`--metric-resolution` 默认 60s，top 的数字天生滞后一分钟，别拿它做秒级定位。

**弹药**
- `kubectl top` 出错先看 `kubectl get apiservice`，再看 metrics-server 日志（常见：自签证书不被 kubelet 信 → `--kubelet-insecure-tls`，仅测试环境）
- `kubectl describe hpa` 里 `unable to get metrics for resource cpu` = 这条链断了，与扩缩容不动互为因果（HPA 全景见 Q88 弹性金字塔）
- 与 Q6 呼应：Prometheus 双副本管"看"；metrics-server 单副本也无所谓——挂了只影响扩缩决策，不丢历史
- cAdvisor 指标口径与 Q2 三层账本对齐：容器用量读的是 cgroup，不是进程 RSS

**红线**｜❌ 把 metrics-server 当监控系统（没历史、没存储、重启即丢）；❌ `kubectl top` 显示 0 就说 Pod 没跑——先查链路再下结论；❌ HPA 抖动只调目标值不查采样间隔——60s 采样 + 快速扩缩策略会来回横跳。

**下一问**｜HPA 扩容后新 Pod 要 2 分钟才接流量，怎么避免"扩了但没接住"的假性不足？（→ minReadySeconds + stabilizationWindow + behavior 缩容策略，Q88 金字塔的细节层）

---

## Q93. 新节点加入时，怎么拿到第一张证书？（控制面/TLS Bootstrap）

**一句话**｜bootstrap token（默认 24h）只给一件事的权限——"申请成为节点"：kubelet 用它提交 CSR → 自动批准（csr-approver）→ 集群 CA 自动签发（csr-signer）→ 拿到正式身份再申请 server 证书 → 此后 `--rotate-certificates` 周期自轮换，bootstrap 使命完成。

**30 秒答**｜五步：①运维在控制面 `kubeadm token create`（默认 24h）；②新节点 kubelet 带 `--bootstrap-kubeconfig` 启动，凭 token 以 system:bootstrappers 组身份提交 **CertificateSigningRequest**；③**csr-approver**（RBAC 决定谁有资格批）自动批准；④**csr-signer** 用集群 CA 自动签发——节点证书由**集群 CA** 签，不是 bootstrap token；⑤kubelet 用正式身份申请 serving 证书，之后进 Q15/Q20 的自动轮换轨道。设计动机：千节点没法人工发证书，token 最小权限自举、用后即废（`kubeadm token delete`）。与 `certs renew` 完全两套：那套管控制面组件证书（人工/半自动、要重启静态 Pod），这套管 kubelet 自己的（全自动）。

**弹药**
- `kubectl get csr` — Pending 的 CSR 先分清卡在批准（approver 没跑/RBAC 缺）还是签发（signer 缺）
- `kubeadm token create --print-join-command` — 标准入场姿势，join 命令里就是 bootstrap token
- Token 泄漏应急：`kubeadm token delete` + 审计 `certificatesigningrequests` 创建记录；TTL 到期自动失效
- 常见卡点：节点时钟漂移 → 证书 `not yet valid`；CSR 批了不签 → 查 KCM 的 `--cluster-signing-cert-file` 配置

**红线**｜❌ join 命令和 token 进 Git/聊天群长期存留——等于留一个 24h 重放窗口；❌ "签发失败先重启 kubelet"——先分清卡在批准还是签发，两步是两个组件；❌ 把 bootstrap 轮换和 Q20 控制面 `certs renew` 混为一谈——kubelet 这套全自动，控制面那套人工触发还要重启。

**下一问**｜节点磁盘满了连 kubelet 证书轮换都写不进去，会先炸什么？（→ 轮换失败 + Q5 DiskPressure，kubelet 续不上心跳被拉黑，连环故障）

---

> **Q94–Q103：HPA/VPA 弹性伸缩专项**（2026-10-20 追加）。Q68–Q87 已被并行会话占用（专有云节点池 / SOFA CCM / Calico / OOM 系列 / ACK One OCM / 存储 CSI·快照·finalizer / 控制面链路·API 扩展·TLS Bootstrap 系列），本批从 Q94 续编。Q47 已打透 CFS 限流、Q41 已背 CA/Karpenter，本批补齐副本层与规格层弹性。深度版（四层金字塔/决策树/共存机理图）：`22-概念/07-调度与资源/hpa-vpa-autoscaling-deep-dive.md`。

## Q94. HPA 的扩缩公式？Utilization 的分母是谁？（弹性/HPA）

**一句话**｜`desiredReplicas = ceil[当前副本 × (当前值/目标值)]`，多指标取最大；**Utilization 的分母是 requests 不是 limit**——没设 requests 的 Pod 不参与计算，全员没设则指标算不出、HPA 不动手。

**30 秒答**｜4 副本、CPU 80%、目标 50% → `ceil(4×1.6)=7`。分母细节是本题的分数线：`实际用量/requests`，requests 设 4 核跑 1 核 = 利用率 25%，目标 50% 永远不扩——"HPA 不扩容"第一查 requests 虚不虚。不想被分母绑架用 `AverageValue`（绝对值平均）。指标拉不到时跳过该指标不动手，**绝不在没指标时缩容**——宁可多花钱不丢容量的兜底。

**弹药**
- `kubectl describe hpa` Conditions：`ScalingActive=False` + reason 直指指标链问题
- `kubectl top pods` 通不通 = metrics-server 活没活（HPA 的 Resource 指标唯一来源）
- v2 API `autoscaling/v2`；`metrics` 数组多条并存，各算各的 desired 取 max

**红线**｜❌ 分母说成 limit；❌ "没设 requests 也能用 Utilization"；❌ 说指标缺失时 HPA 会缩容保成本。

**下一问**｜HPA 每轮都动手吗？怎么防它抽风？（→ 防抖三参数，下一题）

---

## Q95. HPA 依赖链是什么？"不扩容"一分钟排查？（弹性/HPA·排障）

**一句话**｜链路 `Pod→kubelet cAdvisor→metrics-server→HPA controller`；排查从 `describe hpa` 的 Conditions 切入，按"指标源→分母→上限→行为参数"四层收敛。

**30 秒答**｜①`kubectl describe hpa` 看 Conditions：`ScalingActive=False/FailedGetResourceMetric` = metrics-server 挂了（`kubectl top` 验证）；②利用率始终不达标 = requests 设太大（Q94 分母戏法）；③`maxReplicas` 已到顶 / ResourceQuota 卡 Pod 数 / 节点资源不足——扩出的 Pod Pending 会传导给 CA/Karpenter（Q41）；④behavior 限速或正处缩容稳定窗口内，看 Events 时间线。调和节奏默认 **15s** 一轮（`--horizontal-pod-autoscaler-sync-period`），端到端瓶颈通常在 Pod 启动/预热，不在 HPA 本身。

**弹药**
- HPA 经 **scale subresource** 生效：Deployment/RS/StatefulSet 都能管，但它只改 `spec.replicas`（Q58：生杀归 RS controller）
- 自定义指标链路：Prometheus Adapter / custom-metrics API，指标名拼写与 API 版本是常见翻车点
- 预热慢的服务用 startupProbe（Q32）兜底，别靠调 HPA 频率

**红线**｜❌ 不看 Conditions 先重启 metrics-server；❌ 说 HPA 直接创建/删除 Pod——它只改 replicas 字段；❌ 排查不看 ResourceQuota/节点资源（HPA 管不了这两层）。

**下一问**｜指标毛刺会不会让副本数坐过山车？（→ tolerance/稳定窗口，下一题）

---

## Q96. HPA 防抖三参数：tolerance、稳定窗口、behavior？（弹性/HPA）

**一句话**｜三道闸：tolerance 默认 **10%**（偏离不够不动手）、缩容稳定窗口默认 **300s**（取窗口内最大期望，缩要稳）、扩容窗口默认 **0s**（扩要快）——不对称是刻意设计。

**30 秒答**｜`--horizontal-pod-autoscaler-tolerance` 默认 0.1：利用率偏离目标不足 10% 直接跳过本轮，防毛刺第一闸。缩容 `stabilizationWindowSeconds` 300s：每轮看的是窗口内历史期望值的**最大值**，流量抖动打不出缩容冲动。autoscaling/v2 的 `behavior` 字段逐向精调：scaleUp/scaleDown 各配 stabilizationWindowSeconds + policies（type: Pods/Percent 限每 periodSeconds 步长，如"每分钟最多缩 10%"）。设计意图一句话：扩容晚一步是体验问题，缩容早一步是容量事故。

**弹药**
- behavior 模板：scaleUp `{Percent 100 / 30s}`、scaleDown `{Percent 10 / 60s}` + 300s 窗口
- 仍震荡 → 给 scaleUp 也加稳定窗口，或指标源侧平滑（rate 窗口拉长）
- 面试加分点：说出"两窗口不对称的原因"比背数值值钱

**红线**｜❌ tolerance 说成 5%/20%；❌ 以为扩容也有 300s 冷却；❌ 把 behavior 当 v1 API 字段（autoscaling/v2 才有）。

**下一问**｜手动改 replicas 和 HPA 同时在，听谁的？GitOps 呢？（→ 下一题）

---

## Q97. 手动 scale 和 HPA 冲突听谁的？GitOps 集群怎么处理？（弹性/HPA·GitOps）

**一句话**｜听 **HPA** 的——每个调和周期都会把 replicas 掰回自己算的值；GitOps 集群必须对 `spec.replicas` 做 ignoreDifferences，否则 Argo/Flux 和 HPA 天天打架（Q66 同款漂移）。

**30 秒答**｜HPA 的生效方式就是改目标对象的 `.spec.replicas`，每周期覆盖一次——`kubectl scale` 手改的值活不过 15s。GitOps 后果：仓库写的 replicas 和集群实际永远对不上，每次 sync 抹掉 HPA 成果、下个周期又被 HPA 掰回，来回震荡。正解：**Argo CD 对 `spec.replicas` ignoreDifferences / Flux 标记跳过**，副本数真相源从 Git 移交给 HPA，Git 里只保留 min/max（HPA 对象本身入库管理）。想临时接管副本数 → 改 HPA 的 min/max，别和 HPA 抢 replicas。`rollout restart` 的 restartedAt 注解是同类漂移点（Q66），一起 ignore。

**弹药**
- Argo CD：`ignoreDifferences: [{group: apps, kind: Deployment, jsonPointers: ["/spec/replicas"]}]`
- KEDA/Argo Rollouts 等所有"会改 replicas 的控制器"都进 ignore 清单
- 判断线：**谁持续调和谁拥有该字段**——声明式系统里的字段所有权（SSA 同款思想）

**红线**｜❌ GitOps 里 replicas 和 HPA 同时管（谁 sync 谁打脸）；❌ "关掉 HPA 再手动 scale"当日常方案——等于没有弹性；❌ 忘了 HPA 对象本身也要进 Git。

**下一问**｜CPU 利用率不高但 P99 毛刺，HPA 为什么见死不救？（→ Q47 钩子回收，下一题）

---

## Q98. CPU 做 HPA 指标最大的坑：与 CFS 限流互骗？（弹性/HPA×性能）

**一句话**｜CPU limit 触发 CFS 限流 → usage 被压平 → HPA 看到的利用率"健康"→ 不扩容 → 继续限流——**限流和 HPA 互相欺骗**，平均用量 50% 和 P99 毛刺完全兼容（Q47 闭环）。

**30 秒答**｜机理：limit 落成 CFS quota，超了就地暂停线程，被掐掉的用量**根本记不进 HPA 读到的平均值**；多线程突发服务最典型——瞬时需求 2 倍 limit，HPA 看到的可能只有 55%。三个规避：①延迟敏感/毛刺型服务改**自定义指标**（QPS、P99、连接数，`type: Pods` + Prometheus Adapter）——业务指标才和用户体验同频；②`AverageValue` 按绝对值配，绕开 utilization 分母；③监控 `container_cpu_cfs_throttled_periods_total` 比例（>10% 必须动 limit，Q47），**HPA 的指标健康 ≠ 服务健康**。同款陷阱：requests 设虚高（Q94）让 HPA 钝感——一个压低分子、一个抬高分母，都是"看不准"。

**弹药**
- PromQL：`rate(container_cpu_cfs_throttled_periods_total[5m]) / rate(container_cpu_periods_total[5m])`
- 自定义指标选型：对齐"用户能感知的量"（QPS/延迟/队列深度），别拿 GC 时间这种间接量
- `behavior` 治抖动，治不了"指标说谎"——说谎要从指标选型治

**红线**｜❌ "CPU 指标够用了"——限流互骗讲不出来等于没真调过 HPA；❌ 一毛刺就无条件调大 limit——掩盖问题且成本失控；❌ HPA 目标值贴着实际负载设（没有余量，天天扩缩）。

**下一问**｜VPA 什么架构？为什么它改存量 Pod 必须驱逐？（→ 三件套，下一题）

---

## Q99. VPA 三件套架构？updateMode 全谱？（弹性/VPA）

**一句话**｜Recommender 算账、Updater 动手、Admission Controller 塞值（mutating webhook **只拦 Pod 创建**）——所以存量 Pod 想吃新推荐只有驱逐重建一条路；updateMode 五档：Off / Initial / Recreate / InPlaceOrRecreate / Auto（=Recreate 别名）。

**30 秒答**｜链路：Recommender 吃 metrics-server + 历史 checkpoint，产出四值推荐（target / lowerBound / upperBound / uncappedTarget）写进 VPA status；**webhook 只拦 CREATE**——这是理解 Recreate 的钥匙：Pod 跑起来就改不了（Q63/Q64 immutable 语义），Updater 按模式驱逐，重建时 webhook 注入新 requests。五档：**Off** 只出推荐（起步姿势）/ **Initial** 只在创建时注入、存量不管 / **Recreate** 偏离超阈值就驱逐重建 / **InPlaceOrRecreate** 优先原地 resize（依赖 K8s 1.33+ InPlacePodVerticalScaling，特性门默认关，答前先报版本）/ **Auto** 新版弃用别名等同 Recreate。**OOM 加成**：Recommender 把 OOMKilled 事件计入历史自动上调内存推荐——吃过亏的容量顾问。

**弹药**
- 看推荐：`kubectl describe vpa` 四值；uncappedTarget 被 minAllowed/maxAllowed 夹住时对比着看"夹疼没有"
- `resourcePolicy`：minAllowed/maxAllowed 夹范围、controlledValues `RequestsOnly` 只动 requests、`mode: Off` 排除 sidecar
- 推荐"不出现"三查：metrics-server 活着吗 / recommender 活着吗 / Pod 历史够不够（几分钟起步）

**红线**｜❌ 说 VPA webhook 能改运行中 Pod；❌ Recreate 不配 PDB 就上核心服务（Updater 驱逐尊重 PDB，没配等于裸奔）；❌ 版本敏感点（InPlaceOrRecreate/Auto 语义）答得斩钉截铁不报版本。

**下一问**｜VPA 有什么边界？哪些对象根本不能用？（→ 下一题）

---

## Q100. VPA 的边界与安全阀：DaemonSet 例外、PDB、夹紧范围？（弹性/VPA）

**一句话**｜三不与三保险：**不用于 DaemonSet**（addon-resizer 替代）、不碰没 PDB 的核心服务、不给中断敏感服务开 Recreate；保险靠 minAllowed/maxAllowed 夹范围、RequestsOnly 限动作、mode Off 排除 sidecar。

**30 秒答**｜**DaemonSet 为什么不行**：每节点一份、副本拓扑焊死，靠驱逐重建做不了均衡——官方替代 addon-resizer / Cluster Proportional Vertical Autoscaler（按节点数线性缩资源）。**PDB 是硬前置**：Updater 走 eviction API、尊重 PDB（Q29 的 voluntary eviction 线），没配 PDB 的服务开 Recreate = 被自己的弹性工具打挂。**推荐值会学坏**：recommender 看到什么学什么，异常流量教出来的推荐也歪——minAllowed/maxAllowed 必须夹，对比 uncappedTarget 看夹疼没有。落地节奏：`Off` 跑两周看推荐 → 合理后 `Initial`（只管新 Pod）→ 真需要才 `Recreate`。定位一句话：**VPA 是垂直精调不是弹性主力**——单机规格有天花板，流量翻十倍还得靠水平。

**弹药**
- StatefulSet 用 VPA 慎之又慎：驱逐重建有有序性开销（Q67），建议 Off + 手工 rightsizing
- recommender checkpoint 保存历史，重启不丢推荐
- 清单口诀：**夹范围、限动作、排 sidecar、配 PDB、Off 起步**

**红线**｜❌ DaemonSet 上 VPA（不支持）；❌ minAllowed/maxAllowed 不设任由推荐放飞；❌ 把 VPA 宣传成"自动扩容"——它调规格不调副本数。

**下一问**｜HPA 和 VPA 能同时上吗？冲突在哪？（→ 共存方案，下一题）

---

## Q101. HPA×VPA 冲突机理与两个共存方案？（弹性/共存）

**一句话**｜冲突根源：HPA 的 Utilization 分母是 requests，而 VPA 恰恰在改 requests——VPA 一上调，HPA 眼里利用率瞬间"跌"→ 缩容 → 剩余 Pod 更忙 → 利用率又超 → 扩容……两个控制器互相甩锅。

**30 秒答**｜能画出这条反馈回路，比背方案值钱。两个正交化方案：①**分维度（推荐）**：HPA 管自定义指标（QPS/P99，`type: Pods`），VPA `RequestsOnly`/`Initial` 管 requests——一个看业务量、一个看资源规格，互不踩脚；②**分资源**：HPA 只配 CPU 指标，VPA `controlledResources: [memory]` 只管内存——CPU 随流量弹、内存随泄漏/缓存调，维度天然正交。官方的 HPA×VPA 多维联合决策还在 alpha，生产主流就是分维度共存。答题结构：先讲冲突机理（分母联动）→ 再给方案 → 收一句"本质都是把两个控制器的输入变量解耦"。

**弹药**
- 冲突白板演示：VPA 上调 requests 50% → 同负载下 utilization 掉 33% → HPA 趋向缩容
- 方案一模板：VPA `controlledValues: RequestsOnly` + HPA `type: Pods` 自定义指标（深度笔记有完整 YAML）
- 连带效应：VPA 把 requests 越调越大 → 调度占用越高 → 间接触发 CA 扩节点（Q41 成本联动）

**红线**｜❌ "HPA+VPA 同指标同时用没事"——必震荡；❌ 只背"不能一起用"给不出共存方案（面试官等的是正交化思路）；❌ 忘了 VPA 改 requests 对调度和节点层的传导。

**下一问**｜夜间没人流量想把服务缩到 0，HPA 做得到吗？（→ KEDA，下一题）

---

## Q102. 缩到 0 与事件驱动：KEDA 是什么？和 HPA 什么关系？（弹性/KEDA）

**一句话**｜HPA 的 minReplicas ≥ 1 缩不到 0；KEDA 用 ScaledObject CRD + 60+ 触发器补上 0↔1 这一段，1↔N 仍交给它自己生成的原生 HPA——**KEDA 不替代 HPA，是在它前面补零**。

**30 秒答**｜架构：KEDA operator watch ScaledObject（触发器：Kafka lag、SQS 队列深度、Cron、云事件……）→ 内部生成并托管一个 HPA；队列空了缩到 0，消息来了 activation 阈值先拉回 1，之后就是原生 HPA 的地盘。适用：事件驱动消费者、夜间无流量服务、成本敏感批处理。一句话对比 Knative 的 KPA：那是按并发/请求数直接驱动的另一路（自带流量门面 + 缩零），不依赖指标适配器，但整平台更重。加分句："缩零的真正成本在冷启动——镜像拉取 + 预热，缩零方案都要配 latency 预期管理"。

**弹药**
- `kubectl get scaledobject` 看 KEDA 对象；`kubectl get hpa` 能看到它生成的同名 HPA
- 触发器选型：有队列 lag 就用 lag（背压天然合理）；HTTP 服务别硬上 KEDA（冷启动伤体验）——留 1 副本或上 Knative
- Cron 触发器适合"上班前预热"，把冷启动成本挪到流量来之前

**红线**｜❌ 说 KEDA 是另一种 HPA 实现（它是 HPA 的上层封装）；❌ 给原生 HPA 写 minReplicas=0 想缩零（不支持）；❌ 缩零方案不谈冷启动延迟。

**下一问**｜收口：四层弹性各归谁管？一张图说全。（→ 下一题）

---

## Q103. 弹性四层金字塔：HPA/VPA/CA/KEDA 各管哪一层？（弹性/总纲）

**一句话**｜**HPA 管"几个"、VPA 管"多大"、CA/Karpenter 管"多少台"、KEDA/Knative 管"要不要（0 还是 1）"**——被问"你们怎么做弹性"，先分层再谈参数。

**30 秒答**｜自下而上：**副本层** HPA/KEDA（Q94–Q98）；**规格层** VPA（Q99–Q101）；**节点层** CA 改节点组 vs Karpenter JIT 直接 bind（Q41），触发信号是 HPA 扩容后的 Pending——两层在"Pending"上咬合；**请求驱动层** Knative/KPA（Q102 一句带过）。联动链背一遍：流量涨 → HPA 扩副本 → 节点不够 Pod Pending → CA/Karpenter 补节点 → Pod 落地。选型判断线：无状态可水平扩 → HPA 主力；单实例大应用 → VPA 精调（Off 起步）；事件驱动/缩零 → KEDA；**副本数总在 min/max 边缘 → 回头调 HPA 目标或做 VPA rightsizing，别硬堆 maxReplicas**。配套深度笔记：`22-概念/07-调度与资源/hpa-vpa-autoscaling-deep-dive.md`（金字塔图 + 决策树 + 共存机理）。

**弹药**
- 联动故障面：HPA 扩容打爆镜像仓库（Q7 尾钩）；VPA 调大 requests 间接触发 CA 扩节点（成本联动，Q101）
- 一句话收尾模板："分层是 HPA 管量、VPA 管规格、Karpenter 管节点、KEDA 管事件缩零；指标优先业务指标，避开 CFS 限流互骗"
- 监控对齐：副本层看 HPA conditions、规格层看 VPA 推荐 vs 实际、节点层看 Pending 与节点池水位

**红线**｜❌ 只会答"我们有 HPA"——四个动词（几个/多大/多少台/要不要）一个都说不全；❌ 弹性只谈扩不谈缩（缩容策略才是生产事故高发段）；❌ 分不清"HPA 扩容失败"（指标/参数层）和"节点不够"（Pending → CA 层）。

**下一专题**｜渐进式交付（Argo Rollouts/Flagger 金丝雀分析——Q65 埋的钩子；底料在 `11-发布变更/03-Progressive-Delivery/`）。

---

## Q104. K8s 运维 Agent 的"判断层"怎么设计？——判断模型（Jev/System One）与置信门控（AIOps/Agent·架构）

**一句话**｜把运维 Agent 循环里"要判断但不用写字"的时刻（事件分诊/告警分级/危险命令放行/变更验收/工具路由/值班打断）从 LLM 剥出来，交给只输出"标签+分数+概率"的判断模型（TypeSafe 2026-09 发布的 Jev 是这一类——System One Model——的首个产品），阈值和副作用留在代码里、低置信升级给人；四层分工一句话：强模型探索、快模型判断、代码执行、人兜底。

**30 秒答**｜①**为什么剥离**：Agent 每轮都有小判断（继续/重试/问人、这条 `kubectl delete` 危不危险、rollout 完没完），今天都是唤醒 LLM 生成一段话再解析——贵、慢、没有可编程的概率。②**判断模型是什么**：输入 `state`（精简事件/告警/命令+最近变更）+ 一组原子问题，输出三种原语：**Choice**（选标签+confidence+各选项概率）、**Score**（有序量尺期望分，可落档间如 2.3）、**Noul**（0–1 是非概率）；一次调用多问题并行隔离评估、几乎不加时、免 context rot；毫秒级，拆解文口径 $0.042/百万输入 token、输出免费。③**两种挂法**：主模型当工具调（要花 token 教它何时问）vs **Harness 在工具执行前后自动问、主模型无感**（更像系统）。④**K8s 六个判断点**：事件分诊 Choice{act_now/watch/noise}、告警分级 Score[P3/P2/P1/P0]、危险命令门控 Noul"破坏性且不可逆"+Choice 影响范围、变更验收 Noul 完成度+Score 回滚必要性、工具路由 Choice、值班打断 Noul。⑤**置信门控三段**：高置信自动执行白名单动作 / 中置信降级（`--dry-run=server`、只告警）/ 低置信升级给人并附概率分布。⑥**能不能自己做+多少钱**：Jev 本体闭箱——不私有化、不微调、全客户共享权重，定制只靠 state/questions，所以"K8s 版 Jev"=写模板不训练，¥0 训练、每天 1 万次判断约 ¥3–6；要自训只能训"同范式替代"——difflogic 可微分逻辑门网络（坊间"与非门模型"）：特征向量输入非文本、类别训练时钉死、CPU 微秒推理、Mac 即可训、算力 ¥0–20、真实成本是几千条标注，**专有云/离线/数据不能出域时是唯一选项**。判断线：判断点同时满足"文本/结构化输入·动作可枚举·秒级·高频·允许置信分流"才上判断模型，否则 LLM+RAG 或人工。

**弹药**
- 原子问题原则：别问"节点健康吗"，拆成 mem_pressure/disk_pressure/kubelet_stale/pleg_unhealthy 四个 Noul 同次并行问，权重是代码里的系数——改优先级改一行不用重写 prompt
- 铁律金句："Jev 回答问题，不执行决定"——`noul < 0.3 放行` 写在 harness，别让 LLM 再去"理解"Jev 的输出（钱白省）
- 门控是叠在 RBAC/准入之上的一层：RBAC 管"能不能"，门控管"该不该现在这么做"；需拦形态 `delete ns`/`delete pod --all`/`drain --force`/`helm uninstall`/`etcdctl del`/改 finalizers
- 校准自验方法：历史告警+真实处置结果画可靠性曲线（预测概率 vs 实际命中率）再定阈值；验证层持续回流就是未来自训的标注集
- state 裁剪：精简字段不给整份 YAML、永远附最近变更、Secret/Token/证书绝不进 state
- 成本算式：$0.042/M × 1.5K token ≈ $0.000063/次 ≈ ¥0.00044，×1 万次 ≈ ¥4.4/天；flash 级 LLM 钱同量级——差别在延迟、校准概率、输出可靠、多问隔离
- 与 LLM function calling 方向相反：LLM 生成 JSON 让你调函数，Jev 本身就是函数（state+questions 进、类型化判断出）
- 深度文档：15-AI基础设施/02-AI-Agents/53-jev-judgment-model-system-one-k8s-ops（API 契约/挂法时序图/六模板/三路线成本/决策树）

**红线**｜❌ 把判断模型当小号聊天模型让它解释根因/生成修复 YAML——输出里根本没有文本位；❌ 说 Jev 可以私有化部署或拿集群数据微调——官方明确不做，所有账户共享权重；❌ 阈值或副作用放在模型/prompt 里——必须在代码里；❌ 直接信 confidence 0.9 不做校准验证——"校准"是训练目标不是保证；❌ 集群原始 Secret/日志不脱敏发外部 API；❌ 高错误成本且无兜底流程的动作（删 ns、drain 生产节点、改 etcd）交给概率接口自动执行；❌ 把官方"快 193 倍省 444 倍"当承诺——理想条件数字且定价无公开页。

**下一问**｜判断层把低置信升级给人之后，"该不该叫醒值班人"本身能不能量化成告警降噪的一环？（→ 可观测性/告警治理专题；呼应 Q77 kubepods 水位告警"比 dmesg 早半天"的前置思路）

## Q105. 企业级 TAM 负责头部银行大型项目：核心原则、方法论、哲学分别是什么？（TAM/服务方法论·职业软技能）

**一句话**｜TAM 的真正交付物是**信任**、技术只是介质——原则是五条铁律（稳定压倒一切 / No Surprise / 承诺即负债 / 合规红线高于商务 / 长期主义），方法论是六模块动作系统（干系人地图、全生命周期与割接三件套、故障黄金 24 小时、例行运营、指挥家式资源编排、价值翻译），哲学归结为一句话：**一致性信任 + 长期主义，客户成功才是你的成功**。

**30 秒答**｜先说三层框架再逐层给货。**原则（底线，违反一次就可能清零）**：①稳定压倒一切——变更三问"可灰度？可回退？有预案？"答不上就退回方案阶段；②No Surprise——坏消息第一时间带选项主动报，客户最不能接受的是"从别处先知道"；③承诺即负债——只承诺能交付的，口头承诺当天纪要留痕；④合规红线高于商务——等保/密评/审计/采购合规不为成单让路；⑤长期主义——卖的是十年关系不是单个项目。**方法论（动作）**：干系人地图分层沟通（向上讲价值与风险、向下讲细节、对同级厂商讲责任界面；关键人轮岗靠机制传承信任而非私人关系）；全生命周期管理（售前交接→POC→架构评审→实施→割接→护航→复盘，割接三件套＝演练≥2 次·回退条件前置写死·战时值守表）；故障黄金 24 小时（止血→定性→RCA→对上沟通模板六要素→改进闭环，故障后两周是信任修复窗口）；例行运营（周报/月报/QBR/高管拜访——TAM 的敌人不是故障，是存在感归零）；TAM 是指挥家不是独奏家（拉通产品/研发/SLA，自己只做判断、协调、兜底）；价值翻译（99.99%＝全年不可用<53 分钟，给对接人"向他老板汇报的素材"）。**哲学（信念）**：敬畏生产与监管、一致性信任（大事靠谱＝无数小事言出必行）、敢于说不（拒绝+理由+替代方案）、翻译官兼传教士、赋能而非替代。

**弹药**
- 银行场景四特殊性：监管刚性（等保三级/密评/重大故障监管报送/数据不出域）、稳定 KPI 一票否决（客户保守是理性不是落后）、组织复杂决策链长+轮岗常态、多厂商责任界面（故障时第一战场）
- 权力来源分析：TAM 无客户编制、无公司资源线，只有"信任+信息"两种货币——判断任何动作就问"让这两样变多还是变少"
- 对上沟通六要素：现象（客户视角）/影响面（业务·客户数·资损）/根因（标注"初步"）/已采取措施/恢复预期（区间+下次同步时点）/后续动作
- 接手新客户 30/60/90：30 天摸底不表态 → 60 天建机制赢一场小胜 → 90 天出年度服务规划对齐高层
- QBR 三段式：回顾（数据）→对齐（下季度重点）→升级（高层现场拍板）；会前逐一对齐，QBR 上不允许出现 surprise
- 续约提前 6 个月布局：技术信任是续约的必要条件不是充分条件
- 技术侧对照：故障处理的技术动作见 Q74–Q77 OOM 工单复盘（五步刀法是技术侧范例）；KB 深挖 `13-生产运维/09-TAM服务方法论/01-tam-banking-large-project-methodology.md`

**红线**｜❌ 故障时消失或先隐瞒再"确认一下"（No Surprise 是铁律，一次清零）；❌ 会上随口"没问题"、纪要不写分歧只写共识；❌ "顺手帮忙"不登记，滚成免费劳动力还价值不可见；❌ 只懂技术不懂监管与客户 KPI，永远进不了决策桌；❌ 为成单/续约越合规红线；❌ 只做传声筒（只转发不复核）或只做保姆（事事下场），前者丢信任后者丢杠杆。

**下一问**｜重大故障的黄金 24 小时，TAM 在战时指挥部里具体做什么、不做什么？（→ 呼应 [[13-生产运维/03-事件响应/02-war-room-coordination-procedures.md|War-Room 协同]]；技术侧呼应 Q76）

---

## Q106. ACK 集群新建的流程？哪些东西建完就改不了？（云厂商/ACK·集群生命周期）

**一句话**｜授权 → 选形态（托管 Pro 为主）→ 选版本 → **网络规划（CNI + 三张网段，选错不可改）** → 组件 → 节点池 → 管控面拉起 + 节点注册 + addons 就绪拿 kubeconfig；面试真正考的是"哪些建完就改不了"。

**30 秒答**｜七步：①首次开通要 RAM 服务关联角色（AliyunCSManagedDefaultRole 等）；②形态：**托管版 Pro**（Master 阿里云托管，默认答案）/ 专有版（自管 Master，已停止演进，存量金融才碰）/ ASK Serverless（无节点）/ ACK Edge / 注册集群（纳管外部集群）；③版本选当前默认的较新 minor——老版本维护窗口短，建完就得升；④网络：VPC + 多 AZ vSwitch，CNI 选 Terway（推荐）或 Flannel，Terway ENIIP 要独立 Pod vSwitch（Pod 直接占 VPC IP，呼应 Q23 容量公式），Service CIDR 不与 VPC/Pod 重叠；⑤组件：Ingress（Nginx/ALB）、CSI、SLS、可观测、安全；⑥节点池：规格/系统盘/登录方式/数量，也可**仅建管控面**事后挂节点池；⑦就绪链路：管控面分钟级拉起 → 节点经 ESS 扩出、cloud-init 注入标识、kubelet 注册（Q38）→ CoreDNS/terway/csi/metrics-server 就绪。**四个不可改**：CNI（Terway↔Flannel）、所属 VPC、Service CIDR、Terway IPvlan 模式（Q22）。可改的：Pod vSwitch 可追加、节点池随时增删、组件可单独升。

**弹药**
- CLI/IaC：`aliyun cs POST /clusters`；Terraform `alicloud_cs_managed_kubernetes` / ROS——生产集群一律 IaC 建，版本号进代码
- 建完立刻做的基线：RBAC + RRSA、节点池拆分（系统/业务/GPU）与标签 taint、基线加固扫描、SLS 告警、备份中心开启、ENI/ECS 配额复核
- 网段一次给足：VPC /16 级别、Pod vSwitch 按终态规模倒推、Service CIDR 留 /20 以上
- 专有版 vs 托管版一句话：Master 谁养——养 etcd 的活归谁
- KB 深挖：`18-云厂商/01-阿里云/公有云-ACK/08-ack-cluster-lifecycle-create-upgrade.md`（不可改清单 / 七阶段 / precheck 表 / 排障表）

**红线**｜❌ "以后再把 Flannel 换成 Terway / 改 Service 网段"——都改不了，只能迁集群；❌ 网段不做终态规划，Pod IP 用光才想起扩（只能加 vSwitch，VPC 网段不能改）；❌ 只会点控制台，说不出授权角色、节点注册链路这些背后动作；❌ 新建还选专有版（除非客户合规硬要求）。

**下一问**｜集群升级怎么走？为什么说升级是单向门？

---

## Q107. ACK 集群升级怎么走？为什么说升级是单向门？（云厂商/ACK·集群生命周期）

**一句话**｜precheck + 备份 + 容量缓冲 → **控制面先升、节点池后升**、节点分批 cordon→drain→换镜像→重入 → 验证并回写 IaC；"单向门"是因为 etcd 里的对象已按新版本 schema 落盘，控制面**没有降级路径，只能向前修复**。

**30 秒答**｜六步：①定目标——控制台只给允许的目标版本，不能任选中间版本，老集群逐级多轮升到位；②precheck（ACK 内建 + 人工）——**废弃 API 排查**（1.25 移除 PSP 是经典事故源，审计日志里查谁还在调）、组件兼容（Terway/CSI/Ingress 与集群版本**解耦**，可在组件管理先单独升）、无 NotReady 节点、`failurePolicy: Fail` 的 webhook、PDB 够不够排水；③备份——etcd/备份中心，清单在 Git；④**容量缓冲**——按一批节点的 Pod 量提前扩临时节点，排水才有地方落；⑤执行——低峰窗口，**控制面先升**（托管版由阿里云滚动升级 APIServer/CM/Scheduler，存量 Pod 无感，API 短暂抖动，窗口内不发版），再**节点池按批升**：系统节点池 → 业务节点池，每台 cordon → drain → **替换系统盘换新镜像**（默认）或**原地升级**只换 kubelet/runtime 不换盘 → 重入；kubelet 允许短暂落后控制面；⑥验证——节点版本一致、组件 Running、业务拨测，再把 Terraform/ROS 里的 version 回写消漂移。失败可暂停/重试，**不可回滚**。

**弹药**
- 为什么"先控制面后节点"：控制面升级不动节点、零排水；排水只在节点池阶段——两阶段分开讲，面试官就知道你真升过
- 升级铁三角：**备份 + 低峰 + precheck**，因为回滚这条路不存在
- 原地升级 vs 替换系统盘：原地快、保留节点本地数据、但不刷 OS；替换干净、顺带换 OS 镜像、但节点本地盘数据归零
- 常年不升的代价：出维护窗口 → 不再有补丁 → 最后被迫大跨度升级，风险反而更高
- 专有版额外盯 etcd 备份与管控面证书有效期；托管版这些阿里云兜底

**红线**｜❌ "升级出问题就回滚版本"——控制面不可降级；❌ 跳过 precheck、无视废弃 API 警告强升；❌ 不留容量缓冲直接升——drain 把节点排空后副本 Pending，业务雪崩；❌ 把"控制面升级"说成会动业务 Pod；❌ 升完不回写 IaC，下次 apply 冒出非预期 diff。

**下一问**｜升级卡住了——节点排不空、Pod Pending、任务失败，分别怎么处理？

---

## Q108. 集群升级卡住了——节点排不空、Pod Pending、任务失败，怎么处理？（云厂商/ACK·升级排障）

**一句话**｜升级卡住九成卡在**节点池阶段的 drain**：先暂停任务止损，再分清是"**排不出去**"（PDB/webhook/本地数据/裸 Pod 挡住驱逐）还是"**落不下来**"（容量/IP 不够导致 Pending）；控制面阶段失败只有重试，没有回退。

**30 秒答**｜三类：①**排不出去**——drain 走 Eviction API：被 PDB 挡（`kubectl get pdb -A` 看 ALLOWED DISRUPTIONS=0：单副本却写了 minAvailable=1，或 Pod 本身不 Ready 导致可用数不够）、被匹配 `pods/eviction` 的 Fail-closed webhook 拦住、带 emptyDir 需 `--delete-emptydir-data`、裸 Pod（无控制器）默认拒绝驱逐——处置是修 PDB / 临时放宽 webhook / 确认数据可丢再加参数，不是上 `--force`；②**落不下来**——驱逐出来的 Pod Pending（Q41 信号）：容量不够（升级前没留缓冲）、Terway IP 余量不足（Q23）、亲和/污点把可用节点排除——先扩临时节点或加 Pod vSwitch，再继续；③**任务失败/节点 NotReady**——新镜像节点注册不上（Q38 链路：ESS 起实例 → cloud-init → kubelet 注册，查 ECS 系统日志 + `kubectl describe node` events）、单节点异常可暂停任务，处理后重试或把该节点移出节点池；控制面阶段失败走重试/工单。节点池升级任务原生支持**暂停/继续/重试**，排障第一步永远是暂停。

**弹药**
- 演练：`kubectl drain <node> --dry-run=server --ignore-daemonsets --delete-emptydir-data` 先看会卡在哪
- 找拦路者：`kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations -o yaml | grep -B5 -A5 'failurePolicy: Fail'`，看 rules 是否覆盖 pods / pods/eviction
- 驱逐事件：`kubectl get events -A --sort-by=.lastTimestamp | grep -iE "evict|disruption|drain"`
- 最常见的"自己锁死自己"：StatefulSet 单副本 + PDB minAvailable=1，永远排不空；解法是先扩副本或临时调 PDB
- 升级前缓冲估算：最大并行节点数 × 单节点 Pod 数 ≈ 需要预留的调度容量

**红线**｜❌ `drain --force --grace-period=0` 一把梭——绕过 PDB 与优雅终止（Q34），有状态服务直接数据不一致；❌ 看到 Pending 就删 Pod——没容量删了还是 Pending，先扩容；❌ 控制面升级失败幻想"回退旧版本"——单向门；❌ 不暂停任务就排障——一边排一边继续滚，雪球越滚越大。

**下一问**｜收口对比：节点池滚动升级 vs Deployment 滚动更新——三点像（分批 / 排水即优雅终止 / 可暂停），一点本质不同（不可回滚）；深挖见 KB 笔记 `18-云厂商/01-阿里云/公有云-ACK/08-ack-cluster-lifecycle-create-upgrade.md` §6。

---

## Q109. 多集群三种架构模式（Hub-Spoke / Federation / Service Mesh）本质差在哪？生产为什么常三者叠加？（多集群/架构·总纲）

**一句话**｜三者不是三选一，是**三个不同抽象层**——Hub-Spoke 管"配置治理"（应用部署层）、Federation 管"工作负载分布"（调度层）、Mesh 管"服务通信"（数据面）；各解各的问题，所以企业方案常分层叠加。

**30 秒答**｜判别法 = 看"**同步的是什么、方向如何、业务流量过不过**"。Hub-Spoke：Hub 单向下发 manifest/策略/RBAC/证书，自身不碰业务流量（ArgoCD ApplicationSet / Rancher / Red Hat ACM / OCM），强中心、治理合规友好；Federation：在一个 API 处声明工作负载，联邦控制面决定副本切分、差异化配置与故障迁移（Karmada `PropagationPolicy`/`OverridePolicy`/`ResourceBinding`），但**不解决跨集群服务通信**；Mesh：共享服务注册 + 跨集群 mTLS，Pod 真正跨集群互调（Istio multi-primary / Cilium ClusterMesh / Linkerd multicluster），要么平等要么主从。叠加范例：ACK One = OCM 纳管（Hub-Spoke）+ Karmada 调度（Federation）+ MSE/ALB 多集群网关与多集群 Service 补数据面 + 托管 ArgoCD 交付——四块拼图各占一层。

**弹药**
- 对比表五维度：抽象层（部署/调度/通信）、同步方向（单向下发 / 下发+回报 / 双向发现）、跨集群流量（❌/❌/✅）、全局调度（❌/✅副本权重/⚠️仅 Locality）、中心强弱（强/弱/平等或主从）
- 第四块拼图是 **N/S**：多集群 Ingress / Gateway API 多集群网关按权重把入口流量分到不同集群，三模式都不管这段
- Karmada 官方口径是"多集群调度与编排控制面"，服务网格要靠伴生组件；OCM 的 Placement 只选集群不切副本（呼应 Q78）——两者都证明"调度 ≠ 通信、纳管 ≠ 调度"
- 决策树三句：只要统一治理与配置下发 → Hub-Spoke；要按策略分布副本、多地容灾 → Federation；微服务要跨集群互调、统一身份 → Mesh；全要 → 叠加
- KB 深挖：`22-概念/01-核心架构/05-multi-cluster-architecture-patterns-deep-dive.md`；三模式架构图见 `28-资产/architecture-diagrams/architecture-multi-cluster.md`

**红线**｜❌ 把 Karmada / KubeFed 说成能让 Pod 跨集群通信；❌ 把 ArgoCD ApplicationSet 多集群部署说成联邦调度（它是模板展开，没有副本切分与故障迁移）；❌ 把三模式当互斥选项二选一；❌ 说 KubeFed 还是主流（v2 社区已归档，Karmada 已 CNCF Graduated）。

**下一问**｜Hub 挂了 Spoke 会怎样？Karmada 控制面挂了、istiod 挂了业务分别什么表现？（→ Q110）

---

## Q110. Hub 挂了、Karmada 控制面挂了、istiod 挂了——三种模式下业务分别会怎样？（多集群/故障域·判断）

**一句话**｜三种模式都遵守"**控制面挂 ≠ 数据面停**"：存量业务继续跑，丢的是"变更能力"——Hub 挂丢下发与漂移纠偏、Karmada 挂丢重调度与故障迁移、istiod 挂丢新配置与证书续签（工作负载证书默认 24h，到期才真要命）。

**30 秒答**｜分三类说。**Hub-Spoke（OCM / ArgoCD）**：klusterlet / agent 是 pull 模式出站反连，Hub 不可用时 Spoke 上已下发的资源原样运行，只是收不到新配置、self-heal 不再纠偏（期间手改的漂移检测不到）；恢复后 agent 重连全量对账。**Federation（Karmada）**：成员集群上的 `Work` 早已物化成普通 Deployment，由**本地 kube-controller-manager** 维持副本数，Karmada 挂只是不能重调度、不能 failover、不能改副本分布，集群失联判定也同时停止；push 模式 Karmada 主动连成员 apiserver，pull 模式 `karmada-agent` 出站反连——方向影响防火墙与故障传播。**Mesh（Istio）**：sidecar 用缓存的 xDS 配置继续转发，既有 mTLS 连接不受影响；但新 Pod 注入不了 / 拿不到证书，路由变更不生效，工作负载证书默认 24h，istiod 长时间不可用会在到期后出现大面积握手失败——这是 Mesh 控制面的"定时炸弹"。**Cilium ClusterMesh**：clustermesh-apiserver 挂 → 远端 endpoints 视图冻结（stale），本地流量不受影响，远端变化不再同步。

**弹药**
- 故障分析三问金句："**谁在物化资源**（本地 kcm）、**谁在推配置**（控制面）、**谁有到期物**（证书 / lease / token）"——答对哪一层停、哪一层不停
- Hub 恢复后的真风险：全量对账把应急期间在 Spoke 上的手工改动**冲回去**——应急改动必须同步到 Hub（或先暂停该应用 sync），否则"修好了又坏"
- ACK One 的 Fleet 主控是托管 Hub，SLA 由云厂商兜底；自建 Hub 要把 Hub 自身当生产集群做 HA（etcd 备份、多副本控制器），但业务 SLO 不应依赖 Hub 可用性
- Istio 应急动作：延长证书有效期（`workloadCertTtl`）不是常规手段；正解是 istiod 多副本 + 跨可用区 + 监控 `pilot_xds_pushes`/`citadel_server_csr_count` 等指标
- Karmada 侧对照：成员集群 NotReady 到触发迁移要过 `cluster-failure-threshold` / `failover-eviction-timeout`（默认分钟级），Karmada 自己挂时这些定时器全停——不是"自动容灾"而是"容灾能力离线"

**红线**｜❌ 说"Hub 挂了业务就不可用"（存量不受影响）；❌ 说"istiod 挂了流量马上断"（短期不断，证书到期才断）；❌ 应急时只在 Spoke 手改不处理 Hub（恢复即被冲回）；❌ 把"Karmada 控制面在就等于业务有容灾"（它还要有流量切换配合，见 Q114）。

**下一问**｜跨集群服务发现有没有标准？ServiceExport / ServiceImport 是什么？（→ Q111）

---

## Q111. 跨集群服务发现有标准吗？MCS API（ServiceExport / ServiceImport）是什么，落地现状如何？（多集群/服务发现·标准）

**一句话**｜有——sig-multicluster 的 **Multi-Cluster Services API（KEP-1645）**：源集群给 Service 打一个 `ServiceExport`，消费集群自动派生出 `ServiceImport` + 跨集群 EndpointSlice，DNS 用 `<svc>.<ns>.svc.clusterset.local`；但 API **至今仍是 v1alpha1**（`sigs.k8s.io/mcs-api`，2026 年没有 beta），落地靠实现方：Submariner Lighthouse、Cilium ≥1.17（Beta）、GKE MCS、ACK One 多集群 Service 等。

**30 秒答**｜三个核心概念要先摆出来：**ClusterSet**（一组互信集群）、**namespace sameness**（同名 namespace 在 ClusterSet 内被视为同一个——这是跨集群合并的前提，也是安全边界的隐含假设）、**ServiceExport / ServiceImport**（导出是显式声明、导入由控制器自动派生，用户不手写）。机制：控制器把远端 endpoints 同步成本地 EndpointSlice（带 `multicluster.kubernetes.io/service-name` 标签），DNS 解析 `clusterset.local` 返回 ClusterSetIP（ClusterSetIP 型）或各 Pod 记录（Headless 型）。它**只定义"发现"的 API，不规定底层网络怎么通**——Pod 可达性仍要靠 ClusterMesh / Submariner 隧道 / 云网络打通（见 Q112）。

**弹药**
- 版本敏感事实，答前先报："MCS API 2026 年仍 v1alpha1、未进 kube-apiserver 内置，是 CRD + 外部控制器"
- 实现矩阵：Submariner Lighthouse（最早的 MCS 实现，配 Globalnet 解决 CIDR 重叠）；Cilium 1.17 引入 MCS-API 支持为 **Beta**，与旧版 Global Services 注解（`service.cilium.io/global: "true"`）并存；Istio **不走 MCS**，用自己的 registry 合并（namespace sameness 思想同源）；GKE MCS 实现开放 MCS API；ACK One 多集群 Service 是云产品化变体
- 为什么要显式 Export：默认不导出是安全默认值（不然所有 Service 自动跨集群暴露）；Export 之后权限/NetworkPolicy 是否跨集群生效是追问点（MCS 不管策略，CNI 管）
- 最小 YAML：`apiVersion: multicluster.x-k8s.io/v1alpha1, kind: ServiceExport, metadata: {name: <与 Service 同名>, namespace: <同 ns>}`——名字和 namespace 必须与 Service 一致
- 对照 Istio 老模型：1.8 之前用 `.global` 后缀 + CoreDNS 插桩；1.8 起废弃，改为 sidecar DNS proxying + 同名合并

**红线**｜❌ 说 ServiceImport 要手写（控制器派生）；❌ 说 MCS 解决网络打通（只管发现不管可达）；❌ 说 MCS 已 GA / 内置（v1alpha1 CRD）；❌ 忽略 namespace sameness 的安全含义（跨集群同名 ns 被视为同一租户边界）。

**下一问**｜两个集群 Pod CIDR 撞了，还能互联吗？东西向流量有哪几条路？（→ Q112）

---

## Q112. 跨集群东西向（E/W）流量打通有哪几条路？Pod CIDR 重叠了怎么办？（多集群/网络·选型）

**一句话**｜四条路——①**扁平网络直连**（Cilium ClusterMesh：要求 Pod CIDR 全局不重叠，eBPF 直通性能最好）；②**隧道 + 网关**（Submariner：IPsec / WireGuard / VXLAN，Globalnet 用 NAT 兜底 CIDR 重叠）；③**Mesh 东西向网关**（Istio east-west gateway：跨集群走网关 mTLS，不要求 Pod 互通）；④**云产品网关**（ACK One 多集群 Service / MSE、GKE MCS）。CIDR 重叠时排除①，选②（Globalnet）或③/④（网关做代理，天然不依赖 Pod IP 可达）。

**30 秒答**｜判断顺序两问。先问"**Pod 网络能否打平**"（专线 / VPN + CIDR 规划登记）——能则 ClusterMesh 直连：延迟最低、Hubble 跨集群身份可观测；不能则问"**要不要 Pod IP 可达**"——Istio 东西向网关 / 云网关只需网关 IP 互通，付一跳代理与证书管理的代价；Submariner 介于两者之间：隧道打平 + Globalnet 给每个集群分配 global CIDR 做 SNAT/DNAT。两个常被漏掉的硬约束：**跨集群身份**（SPIFFE trust domain，共享 root CA 或各自 intermediate CA 签自同一 root）；**集群标识进 identity**（Cilium `cluster.name` 唯一 + `cluster.id` 1–255 且不重复，否则安全身份冲突；Istio 用 `meshID` / `clusterID` / `network` 三元组）。

**弹药**
- Cilium ClusterMesh 四硬要求：唯一 cluster.name/id、Pod CIDR 不重叠、节点间直接可达（或隧道）、共享 CA；clustermesh-apiserver 暴露给对端是攻击面，要锁 LB 来源
- Istio 两拓扑：**multi-primary**（每集群一个 istiod，各自 watch 对方 apiserver，需要 remote secret）vs **primary-remote**（单 istiod 托管远端，远端是纯数据面）；再乘"同网络 / 跨网络"——跨网络才需要 east-west gateway
- Istio 1.8 起废弃 `.global` 后缀模型，改为 DNS proxying + namespace sameness 同名合并——老资料里的 `.global` 别当现状答
- 最佳实践篇反模式："网段不登记就建集群 → IP 重叠后跨集群互联永久无解"（Globalnet 是补救不是设计）
- ACK One 不做 Pod 直连而以网关承接——不要求全局 CIDR 规划，代价是多一跳

**红线**｜❌ 说"用 VPN 把网打通就是多集群了"（缺服务发现与身份）；❌ 给重叠 CIDR 推 ClusterMesh；❌ 说 Istio 多集群要求 Pod 网络互通（east-west gateway 模式不要求）；❌ 漏掉 cluster.id 唯一性（Cilium identity 冲突很隐蔽）。

**下一问**｜到底该一个大集群还是多个小集群？什么情况下不该上多集群？（→ Q113）

---

## Q113. 一个大集群还是多个小集群？什么情况下不该上多集群？（多集群/取舍·反模式）

**一句话**｜多集群买的是"**故障域隔离 + 合规边界 + 升级独立 + 信任边界**"，付的是"治理复杂度 + 跨集群通信 + 资源碎片"；上游测试上限 5000 节点 / 15 万 Pod / 30 万容器只是硬顶，真正逼你拆集群的通常是爆炸半径、合规分区、升级窗口和多租户信任边界，**不是规模**。

**30 秒答**｜拆的四个合理触发：①**爆炸半径**（一次 apiserver / etcd / CNI 升级事故不能带走全部业务）；②**合规 / 数据域**（监管要求数据不出域、等保分区、专有云 / 公共云分离）；③**升级独立**（业务线接受不了同一维护窗口，蓝绿换集群比原地升级稳）；④**信任边界**（namespace 多租隔离不够时，集群是最硬的隔离）。不该拆的信号：只是为了"部门各自开"（治理成本指数上升）、规模远未到瓶颈却靠多集群掩盖调度 / 配额治理问题、没有网段登记与统一身份就铺集群。规模底线数字：节点 5000、Pod 15 万、单节点 110 Pod、etcd 建议 ≤8GB——触顶前先看的是 apiserver list/watch 压力与 etcd 延迟，不是节点数。

**弹药**
- 多集群成熟度阶梯：单集群 → 多环境 Hub-Spoke（dev/staging/prod）→ 多地 Federation（容灾 / 就近）→ 跨集群微服务 Mesh——每上一级多一层控制面，**别跳级**
- 三件反模式一起答：集群无模板手工建（配置漂移、无法重建）、按部门随意开（数量失控）、网段不登记（互联永久无解）；补一件：各集群独立账号体系（审计漏成筛子）
- 多活不谈数据层就是灾难放大：流量切过去数据不一致（见 Q114）
- 治理前提：Cluster API / Terraform 模板化建集群 + 统一身份（SSO / RAM）+ 统一可观测（跨集群指标联邦）——这三样没有，多集群只是多了几个孤岛
- 一句反问练判断："你拆集群是为了解决什么故障？"——答不上来就不该拆

**红线**｜❌ 说"K8s 单集群上限 5000 节点所以必须多集群"（多数场景远未触顶，拆分理由是爆炸半径与合规）；❌ 多活只切流量不管数据层；❌ 把多集群当多租户的唯一手段（先 namespace + NetworkPolicy + ResourceQuota，撑不住再拆）；❌ 拆完不做统一身份与网段登记。

**下一问**｜多集群容灾切流具体怎么做？RTO 由什么决定？（→ Q114）

---

## Q114. 多集群容灾：集群级故障的切流由谁触发、RTO 由什么决定？Karmada 故障迁移与网关/DNS 切流各管哪段？（多集群/容灾·链路）

**一句话**｜切流分三段——**探测**（谁判定集群挂：Karmada 成员集群 Ready 超时 / 网关健康检查 / 外部探针）、**重调度**（Karmada 把副本迁到健康集群，受镜像拉取与冷启动约束）、**流量切换**（N/S 走多集群网关 / DNS 权重，E/W 走 Mesh locality failover）；**RTO = 探测超时 + 决策 + 冷启动 + 流量收敛**（DNS TTL / 连接 draining），而 **RPO 取决于数据层，与 K8s 无关**。

**30 秒答**｜Karmada 两级 failover：**集群级**——成员集群 NotReady / Unknown 超过阈值后打 taint（`cluster.karmada.io/not-ready` 等），受 `Failover` 特性门控与 `failover-eviction-timeout` 控制，副本在健康集群补齐；**应用级**——`PropagationPolicy.spec.failover.application` 按资源健康解释器判定（`decisionConditions.tolerationSeconds`），`purgeMode: Directly | Graciously | Never`（v1.17 API 口径，旧版叫 Immediately）决定旧副本怎么清（Graciously 等新副本 Ready 再删，`gracePeriodSeconds` 兜底；注意**失联集群上的 Directly 清理不保证执行**——控制面连不上它，故障迁移天然有一段"新旧双跑"窗口，有状态应用要靠 fencing / 租约兜底）。但 Karmada 只是"**把 Pod 拉起来**"，流量要网关 / DNS 配合：多集群网关（ACK One MSE/ALB、Gateway API 多集群）按权重分流 + 健康探测自动摘除；Mesh 侧 Istio locality load balancing 在 outlierDetection 触发后跨集群兜底。真正的 RTO 杀手：DNS TTL（客户端缓存）、跨地域镜像拉取、有状态服务的主从切换。**同城双活 vs 两地三中心**：前者数据层同步复制 RPO≈0 可真双活；后者异地异步复制 RPO>0，异地通常冷备 / 只读，不要当双活吹。

**弹药**
- `karmada-scheduler-estimator` 先估目标集群容量再迁，避免迁过去 Pending；`karmada-descheduler` 处理长期 Pending 的副本回收重分配
- 迁回策略要显式：故障集群恢复后是否自动迁回（避免乒乓），与 `purgeMode` 搭配想清楚
- 探测段 RTO 下限 = 网关健康检查间隔 × 失败阈值；Karmada 的集群失联阈值默认分钟级——两者谁先触发决定"先切流还是先迁副本"
- 演练铁律：切流演练 ≥2 次、回退条件前置写死、战时值守表——技术侧对照 TAM 卡 Q105 的割接三件套
- 多活数据层三选项：同步双写（同城）、单写多读 + 主从切换（最常见）、单元化分片（按用户路由，异地也能双活但工程量最大）

**红线**｜❌ 把"Karmada 做了故障迁移"等同"业务已恢复"（流量没切就是空转）；❌ 不谈数据层就承诺多活 RPO=0；❌ 异地机房当同城双活用（延迟决定复制模式）；❌ 演练过一次就当容灾达标。

**下一问**｜多集群 GitOps 怎么防漂移？ArgoCD ApplicationSet 多集群生成器与 Hub 单点怎么解？（→ 底料 `11-发布变更/01-GitOps/14-fleet-gitops-operations-guide.md`，呼应 Q110 的 Hub 对账风险）

---

## Q115. 线上怀疑两个 CCM/控制器在抢写 LB 或 Node，怎么定位与收敛？（蚂蚁/排障·双CCM）

**一句话**｜先认信号（LB 后端/ingress IP 抖、node 标签 taint 反复、finalizer 拉锯，最危险是节点莫名被删），再按"谁有写权"定位（Service 的 loadBalancerClass/注解归属 → 云侧 ActionTrail 看哪个 RAM 身份在改资源 → 双方日志时间线对齐 → RBAC 盘点），收敛三步：定唯一写者 → 用 `spec.loadBalancerClass`/专属注解把 Service 划给一方 → 收走非主方的 `nodes delete` 与 `services patch`。

**30 秒答**｜①**信号**：`kubectl get svc -w` 看 `status.loadBalancer.ingress` 来回换；SLB 控制台后端服务器变更记录高频增删；`kubectl get node -o yaml` 里 zone 标签消失又出现、uninitialized taint 回来；Service 删除卡 Terminating、finalizer 一加一摘；**最高优先级信号是节点被"莫名"删除**——第二个 CCM 的 cloud-node-lifecycle 拿 ECS 节点查自己的云查不到，判"实例不存在"直接删 Node（正当逻辑变成误删机关）。②**定位**：看 Service 的 `spec.loadBalancerClass`（设了 class 云厂商 CCM 就该跳过）和注解归属 → ActionTrail 查时间窗内哪些 RAM 身份在改监听/后端 → 两控制器日志按时间线对齐找交叉写 → RBAC 盘点谁有 `nodes delete`/`services patch`。③**收敛**：先定唯一写者（云资源默认归 ACK CCM、自有设施归 SOFA 侧）；要划给 SOFA 侧的 Service 设 loadBalancerClass（**不可变，只能重建**，切 VIP 前双跑）；非主方 RBAC 收到"只读 + 自有前缀注解写"；观察一个发布周期无抖动后用 admission 规则固化"无 class 的 LB Service 一律归 ACK CCM"。

**弹药**
- 根因机关：cloud-node-lifecycle"云 API 查不到实例 → 删 Node"是正当逻辑，叠加第二个 CCM 时它就是误删机关——原则：**查不到 ≠ 不存在，只有持有该节点云身份的一方才有删除判定权**
- `spec.loadBalancerClass`（KEP-1959，1.24 GA）是官方给"多 LB 实现共存"的分域开关：设了 class 的 Service 云厂商 service 控制器直接忽略；MetalLB 与云 CCM 共存走的就是它
- ACK 存量所有权样板：复用已有 SLB 走 `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id`，默认不覆盖已有监听（force-override-listeners 默认 false）
- 深挖笔记：[[01-集群基础/03-控制平面/43-dual-cloud-controller-coexistence-deep-dive.md|双 CCM 分域共治深度剖析]]；架构分工见 Q72

**红线**｜❌ 看到后端抖就重启 CCM——抢写不解决，重启后照旧；❌ 拿删 finalizer 当常规解卡手段（Q87 的并集语义坑）；❌ 给存量 Service 现场改 loadBalancerClass——字段不可变，只能重建滚动切。

**下一问**｜存量 SLB 想从 CCM 托管切成自管（或反向），怎么安全迁移不掉流量？（→ LB 所有权迁移）

---

## Q116. quorum 到底是什么？为什么 4 节点不比 3 节点强？（控制面/etcd·共识）

**一句话**｜quorum=⌊N/2⌋+1 的法定多数派，威力来自**交集定理**——任意两个多数派必有公共节点，所以一个 term 只能有一个 Leader、已提交数据不会丢；2f+1 容忍 f，4 节点 quorum=3 只容忍 1，和 3 节点一样还多一台复制开销。

**30 秒答**｜先定义后数学：写要落到多数派才 commit、选主要拿多数派选票；因任意两个多数派集合必相交，可推出 ①选举安全性（一个 term 至多一个 Leader）②Leader 完备性（新 Leader 必含全部已提交条目）——这就是"CP、失 quorum 拒写"的数学根基。容错表：3 容 1、5 容 2、7 容 3。判断线：**日常 3 节点够；要"一台维护+一台故障"并行才上 5**（代价：每次写等 3 台落盘、peer 流量涨）；偶数无容错增益还易选举平票。拓扑判断线：**etcd 单 Region 跨 AZ 展开**（3→1-1-1，5→2-2-1），跨 Region RTT 30–100ms 会打爆默认心跳（100ms）/选举超时（1000ms），先调拓扑再调参（election ≥10× heartbeat）。加分细节：etcd 3.5 起 `--pre-vote` 默认开（分区节点回来先预选确认能拿多数派才 bump term，不凭高 term 顶掉健康 Leader）；Learner 不计票、不计入 quorum；`etcdctl move-leader` 计划内转移。深度笔记：`22-概念/01-核心架构/etcd-quorum-lease-restore-deep-dive.md` §1。

**弹药**
- 追问"脑裂怎么办"：少数派分区选不出 Leader、无法 commit，恢复后按更高 term 收敛日志——Raft 的脑裂防护不是额外机制，就是交集定理本身
- 3 节点挂 2 的处置与 `--force-new-cluster` 红线是 Q1，本题答"数学"，那题答"操作"，别串
- `etcd_server_has_leader==0`（critical）与 `leader_changes_seen_total>3/h` 是 quorum 健康的两只表

**红线**｜❌ 只背"过半"说不出交集定理（面试官下一句必然是"为什么过半就行"）；❌ 说偶数"只是浪费"——平票拖长选举才是偶数独有坏处；❌ 把 Learner 算进多数派。

**下一问**｜一次写请求从进来到应答经过哪几步、卡在哪？（→ Q117）

---

## Q117. 一次 etcd 写请求的完整路径？瓶颈在哪？（控制面/etcd·写路径）

**一句话**｜Follower 收写转发 Leader → Leader 写 WAL 并 fdatasync → 并行复制 → **多数派落盘即 commit** → apply 协程异步写入 MVCC（内存 treeIndex + bbolt 批量提交）→ 应答；瓶颈是磁盘 fsync，且分两段：WAL 顺序写与 bbolt 落盘。

**30 秒答**｜五步背全：①Leader 追加 WAL 并 fdatasync（`etcd_disk_wal_fsync_duration_seconds`，p99>10ms 告警）；②AppendEntries 并行发 Follower（`peer_round_trip_time`，p99>50ms）；③多数派确认、commit index 前进；④apply 异步：更新 treeIndex + bbolt 事务**批量提交**（默认 100ms/1 万条一批，`etcd_disk_backend_commit_duration_seconds`，p99>25ms）；⑤应答客户端。两个高频坑：**commit≠apply**——`proposals_pending` 高、日志 `apply entries took too long` 是 apply 跟不上（读延迟/Watch 滞后），不是磁盘问题；**两段延迟对应两种病**——wal_fsync 高=顺序写慢（NVMe、`--wal-dir` 分盘），backend_commit 高=bbolt 落盘慢（碎片/数据量）。判断线：磁盘红线是本地 NVMe、绝不上网络盘/共享盘，上线前 fio 实测——`fio --rw=write --ioengine=sync --fdatasync=1 --size=22m --bs=2300 --name=test` 99 分位 <10ms（etcd 官方口径）。

**弹药**
- "Leader 单点写"不是缺陷：Raft 靠唯一 Leader 化解并发写冲突，Leader 挂则多数派重选（秒级不可用换一致性）
- 读不需要写盘：线性读只走 ReadIndex 确认（Q122），fsync 只发生在写路径
- 因果链串场：磁盘慢 → WAL fsync 堆积 → 心跳/提交超时 → Leader 切换（Q116 指标）→ 集群抖动

**红线**｜❌ 说"Follower 也能写"（必须转发 Leader）；❌ 把 commit 当 apply（有异步滞后窗口）；❌ etcd 放 NFS/网络盘还答"做了备份"。

**下一问**｜db 涨满谁的锅、compact 到底谁在做？（→ Q118；NOSPACE 处置步骤见 Q10）

---

## Q118. etcd 的 compact 是谁在做？compact、defrag、quota 满是什么关系？（控制面/etcd·MVCC 维护）

**一句话**｜K8s 集群里默认**是 kube-apiserver 每 5 分钟 compact**（`--etcd-compaction-interval=5m`，多副本靠 etcd 里的协调 key 轮流执行），etcd 自身 `--auto-compaction-retention` 是独立保险；compact 只清历史版本、文件不缩小，**defrag 才把空间还给磁盘**。

**30 秒答**｜先纠正最大误区"etcd 自己会压缩"：K8s 默认链路是 apiserver 内置 compactor 5 分钟一次；kubeadm 默认不配 etcd 自身自动压缩，生产建议两条都留（先到先压不冲突）。原理链：每次写全局 revision+1、bbolt 保留全部历史版本 → compact(rev) 删 rev 之前的旧版本（每个 key 保留快照点位最新值），清出的页进 freelist **标记可复用但文件不缩** → defrag 重写 db 文件才真还空间，期间该节点不可服务、必须逐台串行。诊断判断线：`etcd_debugging_mvcc_db_total_size_in_bytes`（文件大小）vs `_in_use`（真实使用）——差距大=碎片，compact+defrag 能救；都贴 quota=数据真大，治理大对象（Events/超大 ConfigMap/CR）或调 quota（默认 2GB→生产 8GB）。版本细节：多 apiserver 副本经 etcd 的 `compact_rev_key` 协调、同一窗口只有一个副本真压——被问"3 副本 apiserver 会不会压 3 次"就这么答。

**弹药**
- compact 的副作用打在 Watch 上：客户端 RV 被 compact 掉 → ErrCompacted → 410（Q119 展开）
- 命令链：`rev=$(etcdctl endpoint status -w json | jq -r '.[0].Status.header.revision')` → `etcdctl compact $rev` → 逐台 `etcdctl defrag` → `alarm disarm`（Q10 全步骤）
- 顺序不能反：不 compact 就 defrag，拼回去的文件里历史版本还在

**红线**｜❌ 答"etcd 自动压缩"不知道 apiserver 在压（高频栽点）；❌ compact 后以为空间立刻回来；❌ `--cluster` 一把梭 defrag（同时打掉多数派）。

**下一问**｜Watch 客户端怎么感知"版本被压掉了"？（→ Q119）

---

## Q119. Watch 报 "too old resource version" 是什么原理？kubectl get 读的是 etcd 还是缓存？（控制面/etcd·Watch/读路径）

**一句话**｜续订的 revision 已被 compact → etcd 返 ErrCompacted → apiserver 转成 **410 Gone** → 客户端只能重新 List；`kubectl get` 读哪取决于 RV 语义——默认 `""` 是线性一致读（1.31 起默认从 watch cache 服务但语义等价），`RV=0` 走 cache 可能略旧。

**30 秒答**｜链路五步：①客户端带 `resourceVersion=N` 续订 Watch；②apiserver 的 **watch cache（Cacher）** 每资源一份内存事件窗口，所有 client watch 挂 cache 上，**etcd 只看到每资源每 apiserver 一条 watch**——K8s 把 watch 压力从 etcd 挪走的第一层；③窗口接不上且 N 已 compact → etcd ErrCompacted；④apiserver 转 410 "too old resource version"；⑤Reflector 自动 relist 全量再续订——所以 compact 过激/事件风暴后常见 list 风暴，大集群靠 APF 挡。RV 三语义背表：`""`=最新（quorum read）、`"0"`=任意旧值（cache）、`"N"`=不旧于 N；Informer 初始 list 走 RV=0。进阶分：**ConsistentListFromCache**（KEP-2340，1.30 Alpha、**1.31 起 Beta 默认开启**）让 RV="" 的 list 也从 cache 服务——先借 etcd **progress notify** 确认 cache 追上当前 revision 再返回，保住线性一致语义同时给 etcd 大幅减负（GA 在 1.34/1.35 推进中）。缓解 410：Watch bookmark 让客户端及时前移 RV；1.27+ WatchList 流式 list 减全量压力。

**弹药**
- resync（本地周期重放 cache 内容）≠ relist（打 etcd 全量）——"Resync 会不会压垮 etcd"考的就是这个
- 一致性主线一句话："读从 etcd 挪到 apiserver cache，但不牺牲语义（progress notify 对水位）"
- 排查 410 风暴：先看 etcd compact 策略与 Events 量（事件是 key 大户，Q118）

**红线**｜❌ 认为 kubectl get 永远直打 etcd（cache 在中间，RV 语义决定）；❌ 把 resync 当 relist；❌ 说不清 410 后客户端行为（必须 relist，不是原样重试 watch）。

**下一问**｜Lease 存哪、kubelet 停心跳后 Lease 会被删吗？（→ Q120）

---

## Q120. Node 心跳的 Lease 存在哪？kubelet 停心跳后 Lease 会被删吗？（控制面/etcd·Lease）

**一句话**｜Lease 是普通 coordination.k8s.io 对象、经 apiserver 存 etcd `/registry/leases/kube-node-lease/<node>`（一节点一个、名字同节点名）；停心跳**不删对象**，只是 renewTime 变旧——NodeLifecycleController 比对宽限期（默认 40s，**K8s 1.32 起改 50s**）判 NotReady → 打 taint → 默认 300s 后驱逐 Pod。

**30 秒答**｜节奏背全：kubelet 每 10s（leaseDuration 40s 的 1/4）更新 renewTime；Node status 降为 5 分钟一次（KEP-0009/NodeLease 1.14 GA，动机是写入放大——Node status 几十 KB×5000 节点×每 10s 会写穿 etcd，Lease 只几十字节）；租约或 status 任一更新都算心跳。超时动作链：Ready=Unknown → `node.kubernetes.io/unreachable`（NoExecute+NoSchedule）→ 默认 tolerationSeconds=300 → Taint 管理器驱逐 → 控制器他节点重建。恢复：kubelet 续**同一个** Lease（对象从没被删），taint 移除回 Ready。现场核验：`kubectl -n kube-node-lease get lease <node> -o yaml` 看 renewTime。故障因果链（高频案例题）：**etcd 磁盘慢 → renewTime 写不进去 → 被误判 NotReady**——心跳故障第一嫌疑人不是网络是 etcd（案例见 KB etcd-×-节点诊断）。

**弹药**
- 版本细节是加分项：`--node-monitor-grace-period` 默认 40s（1.32 前文档口径）→ **1.32 起 50s**（k8s.io KCM 参考现行值）；说 40s 要带"1.32 前"
- 追问"为什么不用 etcd TTL 自动过期"：控制器模式（打 taint/分级驱逐是业务语义，TTL 只会删 key）+ 声明式不搞隐式删除（节点恢复要续同一对象）+ 判定与观测同层好排障
- 对照选主租约：同机制另一用法，`kube-system` 下 15s/10s/2s（Q121）

**红线**｜❌ "没心跳就删 Lease/删 Node"（对象永存，只是 renewTime 停更）；❌ 把 K8s Lease 当 etcd 原生 TTL（Q121 的坑）；❌ 宽限期只说 40s 不标版本。

**下一问**｜两种 Lease 什么关系？K8s 哪里真用了 etcd 原生 Lease？（→ Q121）

---

## Q121. K8s 的 Lease 对象和 etcd 原生 Lease 什么关系？K8s 哪里真用了 etcd 原生 Lease？（控制面/etcd·Lease 两层）

**一句话**｜没关系的两层：K8s Lease 是普通对象、过期由**控制器算时间**（节点心跳/组件选主）；etcd 原生 Lease 是**服务端 TTL 到期自动删 key**——K8s 真用它的是 **Events 的 1 小时 TTL**（`--event-ttl=1h`），事件消失不是 controller 删的。

**30 秒答**｜etcd 原生 Lease 三板斧：`LeaseGrant(TTL)` → `Put` 带 `WithLease` → KeepAlive 流续约，到期 etcd 服务端自动删除绑定的 key。apiserver 的 etcd3 存储层 leaseManager 把带 TTL 的对象（核心对象里就是 Events）挂到原生 Lease 上，且 **TTL 相近的对象复用同一个 Lease**（默认 60s 复用窗口）控制 Lease 数量——`etcdctl lease list` 可见。对照表背四行：谁判定过期（控制器 vs etcd 服务端）/过期动作（置 NotReady+驱逐、选主转移 vs 删 key）/对象会不会没（永不 vs 会）/K8s 用途（心跳+选主 vs Events TTL）。选主补齐：kube-scheduler/KCM/controller-runtime operator 在 `kube-system` 持 Lease，默认 **leaseDuration 15s / renewDeadline 10s / retryPeriod 2s**，`holderIdentity` 写自己，抢占靠 Lease 对象的 resourceVersion 乐观锁（Update CAS）；1.14 前选主锁放在 Endpoints/ConfigMap 注解里（历史考点）。

**弹药**
- 追问"Events 为什么不用 controller 删"：存储层 TTL 是 etcd 原生能力，写路径顺带解决，省一轮控制器
- 追问"选主为什么不用 etcd 原生分布式锁"：同 Q120 四条——声明式对象可观测（`kubectl get lease` 直接看 holder）、参数在 K8s 层、不依赖 etcd 会话、对象留存可审计
- 现场核验：`kubectl -n kube-system get lease kube-scheduler -o yaml`（holderIdentity/renewTime）；`etcdctl lease timetolive <id>`

**红线**｜❌ 说"K8s 心跳过期靠 etcd TTL 删"（两层混一层，暴露没写过 controller）；❌ 说 Events 靠 compact 清（TTL 删 key 与 compact 清历史版本是两回事）；❌ 不知道选主抢占靠 resourceVersion CAS。

**下一问**｜etcd 读请求为什么不需要写盘？（→ Q122）

---

## Q122. etcd 线性一致读和串行读差在哪？（控制面/etcd·读一致性）

**一句话**｜默认线性一致读走 **ReadIndex**：Leader 记 commitIndex → 一轮心跳向多数派确认自己仍是 Leader → 等 appliedIndex 追上 → 读 bbolt，**全程不写盘**只花一次 RTT；串行读（`--consistency=s`）直接读本地可能旧值，零网络开销。

**30 秒答**｜ReadIndex 四步：①读请求到 Leader（Follower 收到会转发）记当前 commitIndex 为 readIndex；②带 readIndex 走一轮心跳，多数派确认"你还是本 term 的 Leader"（防分区旧 Leader 读旧数据）；③等本地 appliedIndex ≥ readIndex（commit≠apply，Q117）；④读 bbolt 返回。Follower 上的线性读要先**向 Leader 要 readIndex** 再本地等水位。串行读：`etcdctl get --consistency=s` / 客户端 `WithSerializable`，只读本地状态机、不保证最新——只用于监控轮询等不敏感场景。指标：`etcd_server_slow_read_indexes_total`、`read_indexes_failed_total` 应≈0，持续涨=网络或 apply 有病。与 Q119 合璧的完整口径：**K8s 读一致性 = apiserver watch cache + etcd progress notify（ConsistentListFromCache）+ etcd ReadIndex 三层拼图**——把读从 etcd 挪走但不牺牲语义，这是近年大集群可扩展性的主线。

**弹药**
- 高频陷阱"线性读要 fsync 吗"：不要，ReadIndex 只花 RTT——写才落盘（Q117）
- "读旧 Leader"攻防：只凭"我是 Leader"读会被分区旧 Leader 骗，ReadIndex 的心跳确认轮就是防它
- 判断线：读多写少可加 Follower 分摊线性读（每个 Follower 可独立服务读），写不能（都要过 Leader）

**红线**｜❌ 说"读也要多数派落盘"（那是写）；❌ 把串行读当默认（默认线性一致）；❌ 不知道 Follower 线性读要向 Leader 要 readIndex。

**下一问**｜全量恢复是脚本还是逐台？恢复后 revision 倒退怎么办？（→ Q123）

---

## Q123. 生产 etcd 全量恢复是脚本自动还是逐台手动？恢复后 revision 倒退怎么办？（控制面/etcd·恢复实务）

**一句话**｜**备份全自动（定时 snapshot+校验+异地），恢复 runbook 驱动半手动**——脚本只生成每台命令、人逐台执行每步验收，可靠性靠演练不靠脚本；恢复会把 revision 拨回快照点（比故障前小），必须防 watch cache/Informer 的 RV 倒退。

**30 秒答**｜先答"为什么不对称"：备份每小时甚至每 5–15 分钟一次、生命周期成千上万次，必须自动化+监控备份任务本身；全量恢复一两次、不可逆、每台参数不同、中间有门禁——自动化 ROI 为负，没演练过的恢复脚本比没有更危险。runbook 六步（kubeadm 静态 Pod 版）：①冻结写入（挪走 kube-apiserver/KCM/scheduler 的 manifest；apiserver 活着先补最新快照）；②留退路：每台 `mv /var/lib/etcd{,.bak-<ts>}` 绝不原地删；③逐台 `etcdutl snapshot restore`（三台同一快照+同一新 cluster token，`--name`/`--initial-advertise-peer-urls` 各自不同）；④门禁：逐台启动，`endpoint health --cluster` 确认 quorum、revision 对齐快照；⑤恢复 apiserver，`kubectl get all -A` 抽查；⑥观察 informer relist 与 410 风暴。**进阶分**：恢复后 revision 比故障前小——informer 手里 RV 更高，续订轻则 410 风暴重则读到不一致视图；防护两招：重启全部 apiserver+依赖 informer 的控制器（强制 relist）；或 etcd 3.5.x 后期起 `snapshot restore --bump-revision 1000000000 --mark-compacted`（3.6 移入 etcdutl）——把 revision 抬到故障前之上、旧区间标 compacted 让旧 RV 客户端干净地收 410。

**弹药**
- kubeadm 特有：etcd 是静态 Pod，"停 etcd"=挪 `/etc/kubernetes/manifests/etcd.yaml`
- 备份口径张口就来：关键集群每 5–15 分钟+变更前手动一次（Q1 的及格前提）
- 季度演练=非生产克隆环境真跑 restore 验 RTO/RPO；"没恢复过的备份=没有备份"

**红线**｜❌ 原地 restore 覆盖旧目录不留退路；❌ 三台快照或 token 不一致（新集群起不来）；❌ 恢复后不重启 apiserver/控制器就放客户端续订（RV 倒退错乱）；❌ 答"我们有全自动恢复脚本"（暴露没经历过真实 DR）。

**下一问**｜只坏一台呢？也 restore 吗？（→ Q124）

---

## Q124. etcd 只坏一台怎么修？加节点为什么要先 Learner？（控制面/etcd·成员管理）

**一句话**｜单节点故障**不走 restore**——quorum 还在、服务不受影响：`member remove` 旧 ID → 新节点空 data-dir `member add --learner` → 追平日志 `member promote`；Learner（etcd 3.4+）不参与投票，加入期间不降低 quorum 有效性，是安全阀。

**30 秒答**｜先定性：3 节点坏 1，quorum=2 成立、读写正常，但**再坏一台就失 quorum——时间窗口要快**；修复是"换成员"不是"恢复数据"（新成员从活集群同步，不引入 RPO 损失，对比 Q1/Q123 的全量路径）。四步：①`member list -w table` 找坏成员 ID → `member remove`（先摘，避免同名/同 peer-urls 冲突）；②`member add etcd-new --learner --peer-urls=...`；③新节点**空 data-dir** + `--initial-cluster-state=existing` 启动追日志；④`endpoint status` 看 RAFT INDEX 追平后 `member promote`。**Learner 存在的理由**：新成员追大日志期间若立即计票，quorum 实质变"2 存活+1 追日志中"，期间再挂一台直接瘫；大快照传输还可能拖垮 Leader——非投票成员把两个风险都摘掉。kubeadm 场景 `kubeadm join --control-plane` 自动处理成员添加；升级铁律"先 Follower 后 Leader、必要时 `move-leader` 先转移"也在这里收口。

**弹药**
- 现场判断追平没：`member list` 的 IS LEARNER 列 + `endpoint status` 的 RAFT INDEX 差值
- 追问"坏的那台修好能直接 add 回来吗"：不能复用旧 data-dir——member ID/数据分叉，必须清空或换机
- 5 节点的维护优势落地：容 2=一台维护+一台故障并行（Q116 的 3v5 判断线）

**红线**｜❌ 单台坏就全量 restore（自己制造 RPO 损失）；❌ add 时不清空新节点旧 data-dir（member ID 冲突/数据分叉）；❌ 忘了 promote，集群长期"表面 N 节点、实际投票少一员"。

**下一问**｜部署拓扑：etcd 和控制面放一起还是分开？（→ Q125）

---

## Q125. stacked etcd 和 external etcd 怎么选？跨 AZ 怎么放？（控制面/etcd·拓扑）

**一句话**｜stacked（kubeadm 默认，etcd 与控制面同机）省机器但故障域耦合；external（独立 etcd 集群）隔离好但多一套主机/TLS 成本；跨 AZ 展开（3→1-1-1、5→2-2-1）且**留在单 Region 内**是铁律，跨 Region 先调拓扑再谈参数。

**30 秒答**｜对比三行：①故障域——stacked 上控制面重负载（大 list 风暴、事件洪峰）直接波及 etcd 磁盘，一机损=同时少一个控制面成员和一个 etcd 成员；external 隔离但要独立维护证书/升级。②成本复杂度——stacked 3 台搞定；external 至少 3+3 台、两套 TLS。③运维面——kubeadm 对 stacked 全自动（静态 Pod/证书/备份），external 全自己管。判断线：中小集群/默认 kubeadm 用 stacked+独占 NVMe 即可；大集群/严格 SLA/想独立扩缩 etcd → external。云上托管版（ACK 托管集群）etcd 由云侧运维，客户侧考点转为备份策略、quota 观测与控制面 SLA（专有云才自管）。拓扑铁律复述：奇数、单 Region 跨 AZ（1-1-1/2-2-1）、不与其他重 IO 负载同盘同机；RTT 高才动 `--heartbeat-interval`/`--election-timeout`（默认 100ms/1000ms；>10ms RTT 建议 500/5000，跨 Region 1000/10000——但优先别跨 Region）。

**弹药**
- stacked 的 etcd 参数在 `/etc/kubernetes/manifests/etcd.yaml`（静态 Pod）里改；external 走 systemd/自家部署系统
- "控制面重负载为什么会打到 etcd 磁盘"：大 list 直读 bbolt+事件写入——Q119 的 cache 层就是为这个存在的
- 上线检查清单口径：NVMe 独占盘、fio fsync p99<10ms、跨 AZ、quota 8GB、压缩双保险（apiserver 5m+etcd auto）、每小时备份

**红线**｜❌ 跨 Region 拉 3 节点凑"异地高可用"（RTT 先打爆心跳再谈容错）；❌ stacked 还与重 IO 业务混部署；❌ 托管版集群还按自管口径背备份命令（云侧管 etcd，你管策略与观测）。

**下一专题**｜前面埋的钩子二选一收口：渐进式交付（Q103/Q65 的 Argo Rollouts/Flagger）或告警降噪量化（Q104 的值班打断）。

---

## Q126. 一个写请求 P99 飙到 800ms，怎么定界慢在 apiserver 哪一段？（控制面/apiserver·定界）

**一句话**｜写请求在 apiserver 里按固定顺序过 authn → APF 排队 → authz → mutating 准入 → 校验 → validating 准入 → etcd 落盘，每段一把独立尺子——先用 etcd / webhook / flowcontrol 三把尺子把 800ms 切成"落盘、准入、排队"三块，三把都不背锅才是 apiserver 自身。

**30 秒答**｜三把尺子对着总延迟量：①`etcd_request_duration_seconds{operation=~"create|update"}` ≈ 总延迟 → 回 etcd 三件证据（fsync/db size/Leader，Q9；etcd 内部两段延迟拆解见 Q117）；②`apiserver_admission_webhook_admission_duration_seconds` 某个 name 高 → 慢 webhook（`timeoutSeconds` 默认 10s、`failurePolicy` 默认 Fail——超时即写失败）；③`apiserver_flowcontrol_request_wait_duration_seconds` 高 → APF 排队，座位被大户占了（Q127）。三把尺子都低但总延迟高 → apiserver 自身 CPU/GC/加密开销，抓 pprof。要把"慢"归因到具体客户端，用 audit 日志：每条 event 带 `requestReceivedTimestamp` 与各 stage 的 `stageTimestamp`，差值就是该请求全程耗时，还带 user/userAgent/objectRef——按 UA 一排就锁定凶手。判断线：先拆 verb 再谈 P99，写慢和读慢的根因在两个组件。

**弹药**
- audit 定界：`jq -r 'select(.stage=="ResponseComplete") | [((.stageTimestamp|fromdateiso8601)-(.requestReceivedTimestamp|fromdateiso8601))*1000|floor, .verb, .user.username, .userAgent] | @tsv' audit.log | sort -rn | head`
- pprof：`kubectl get --raw '/debug/pprof/profile?seconds=30'`（加固集群常关 `--profiling`）
- webhook 缩拦截面：objectSelector/namespaceSelector + `timeoutSeconds` 调 1–2s；webhook 别拦自己所在 ns（自锁死循环）
- 定界决策树与指标对照表：[[22-概念/01-核心架构/05-apiserver-latency-triage-deep-dive.md|apiserver 延迟定界深挖]]

**红线**｜❌ 不拆 verb 看总 P99 就下结论；❌ webhook failurePolicy=Fail 又不给短超时——webhook 一抖全集群写瘫；❌ 拿 kubectl 耗时当 apiserver 延迟（里面含客户端 TLS/LB/网络）。

**下一问**｜etcd 和盘都洗清嫌疑、也没有慢 webhook，429 还在刷——429 到底是故障还是设计？→ Q127

---

## Q127. APF 最小必要深度：座位模型、两张表、429 的语义（控制面/apiserver·限流）

**一句话**｜APF 把 apiserver 总并发（`--max-requests-inflight` 400 + `--max-mutating-requests-inflight` 200 = 600 个"座位"，LIST 按预估对象数折算占多座）按优先级分配，FlowSchema 决定请求归哪级、按 user/ns 公平排队——队列满才 429；**429 是保护控制面的设计，先看谁被拒，而不是加副本**。

**30 秒答**｜两个对象：**PriorityLevelConfiguration** 管座位怎么分（`nominalConcurrencyShares`，`lendablePercent/borrowingLimitPercent` 允许级别间借座）；**FlowSchema** 管请求归级（匹配规则 + flow distinguisher 按 user 或 namespace 分队，默认 queues=64 / handSize=8 / queueLengthLimit=50）。默认级别记五个：exempt（system:masters 等，不排队不限流）、system（kubelet）、leader-election（控制器选主续约）、workload-high（控制器日常调和）、global-default/catch-all（没匹配上的野客户端）。事故时两个动作：`kubectl get flowschemas,prioritylevelconfigurations` 看现状；给关键用户临时建高优先级 FlowSchema 或调借座比例——不改全局参数。请求归了哪级看响应头 `X-Kubernetes-PF-FlowSchema-UID`/`X-Kubernetes-PF-PriorityLevel-UID`，调试端点 `/debug/api_priority_and_fairness/dump_priority_levels`。1.29 GA，是唯一限流机制——老 `--max-requests-inflight` 只是座位的计算基数了。判断线：429 集中在 global-default = 有野客户端；集中在 workload-low = 一批 CI 在跑，正常。

**弹药**
- 指标四件套：`apiserver_flowcontrol_rejected_requests_total` / `request_wait_duration_seconds` / `current_inqueue_requests` / `dispatched_requests_total`
- LIST 座位折算：宽度估算器按预估返回对象数占座——大 LIST 天然多吃座位，所以最容易触发 429（呼应 Q11 大 LIST 定位）
- 深挖：[[01-集群基础/03-控制平面/18-api-priority-fairness.md|APF 机制详解]] 与 [[22-概念/01-核心架构/05-apiserver-latency-triage-deep-dive.md|延迟定界深挖]] 3.2 节

**红线**｜❌ 关 APF 退回 max-inflight（丢公平性，kubelet 会被 CI 挤死）；❌ 把所有人都塞进 exempt（不排队等于无限流）；❌ 429 就扩 apiserver 副本——座位随副本翻倍但 etcd 不翻，只是把打爆点往后挪。

**下一问**｜etcd 的"背景写"预算怎么算：5000 节点的 Lease 心跳 + Events 每秒吃掉多少 revision？（→ 呼应 Q23 容量规划、Q120/Q121 Lease 两层；Lease 按 N/10s 起步，Events 洪峰另计——分库见深挖笔记 2.1）
