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

