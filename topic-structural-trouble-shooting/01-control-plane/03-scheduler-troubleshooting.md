# Scheduler 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

Scheduler 是集群的“大脑”，决定了资源的使用效率和应用的稳定性。本文档不仅关注“为什么调度不了”，更关注“如何调度得更好”。

### 🎓 初学者视角
- **核心概念**：Scheduler 监听 API Server 中新创建且未分配节点的 Pod，根据一套算法为它选出一个“最合适”的家（Node）。
- **简单类比**：Scheduler 就像一个房产中介，手里有一堆客户（Pod）和一堆房源（Node）。它会根据客户的要求（资源请求、亲和性）和房源的情况（剩余 CPU、内存）来撮合交易。

### 👨‍💻 资深专家视角
- **调度框架（Framework）**：理解插件化的调度流程（Filter, Score, Bind 等扩展点）如何协同工作。
- **并发与性能**：分析 `parallelism` 参数对大规模集群调度的影响，以及 `Score` 插件的计算权重如何微调。
- **高级调度特性**：深入排查 Pod Topology Spread Constraints (拓扑分布约束) 与 Pod Disruption Budgets (PDB) 的冲突场景。

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 10 分钟快速诊断

1. **确认 Scheduler 存活与选举**：`curl -k https://127.0.0.1:10259/healthz`，`kubectl get lease -n kube-system kube-scheduler -o wide`，若无 Leader/频繁切换先查证书/网络。
2. **观察 Pending 原因**：`kubectl get pods -A --field-selector=status.phase=Pending -o wide | head` + `kubectl describe pod <name> | grep -A30 Events`，锁定主因（资源不足/污点/亲和/拓扑约束/PVC）。
3. **看调度性能**：监控指标 `scheduler_scheduling_attempts_total`、`scheduler_pod_scheduling_duration_seconds`、`workqueue_depth`，或 `kubectl get --raw "/metrics" | grep scheduler_scheduling_duration_seconds_bucket | head`。
4. **热点插件/扩展点**：`kubectl logs -n kube-system kube-scheduler-<node> | grep "took too long" | head`，定位 Filter/Score/Bind 插件耗时；检查 `--parallelism`、`--bind-timeout-seconds` 配置。
5. **拓扑/分布/PDB 冲突**：对 Pending Pod 查看 `topologySpreadConstraints`、`pdb`，必要时用 `kubectl describe pdb` 与节点标签交叉验证。
6. **快速缓解**：
   - 资源侧：临时给节点加资源或移除影响调度的污点/亲和约束。
   - 流量侧：暂缓大规模创建/批量 Job；对控制面低优先级 Flow 使用 APF 调整。
   - 配置侧：适度提高 `--parallelism`，对插件耗时高的场景关闭不必要扩展或调整权重。
7. **证据留存**：保存 Pending Pod 事件、Scheduler 日志、关键指标 (调度时延直方图、抢占次数) 以便复盘。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Scheduler 服务不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 进程未运行 | `kube-scheduler not running` | systemd/容器 | `systemctl status kube-scheduler` |
| 连接 API Server 失败 | `error retrieving resource lock` | Scheduler 日志 | Scheduler 日志 |
| 证书错误 | `x509: certificate signed by unknown authority` | Scheduler 日志 | Scheduler 日志 |
| Leader 选举失败 | `failed to acquire lease` | Scheduler 日志 | Scheduler 日志 |
| 配置错误 | `unable to load scheduler config` | Scheduler 日志 | Scheduler 启动日志 |

#### 1.1.2 Pod 调度失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 资源不足 | `Insufficient cpu/memory` | Pod Events | `kubectl describe pod` |
| 节点不满足条件 | `0/N nodes are available` | Pod Events | `kubectl describe pod` |
| 亲和性不满足 | `node(s) didn't match pod affinity/anti-affinity rules` | Pod Events | `kubectl describe pod` |
| 污点不容忍 | `node(s) had taints that the pod didn't tolerate` | Pod Events | `kubectl describe pod` |
| PVC 未绑定 | `persistentvolumeclaim not found` | Pod Events | `kubectl describe pod` |
| 端口冲突 | `node(s) didn't have free ports for the requested pod ports` | Pod Events | `kubectl describe pod` |
| 拓扑约束不满足 | `node(s) didn't match pod topology spread constraints` | Pod Events | `kubectl describe pod` |

#### 1.1.3 调度性能问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 调度延迟高 | `scheduling_duration_seconds increased` | Prometheus | 监控系统 |
| 调度队列堆积 | 大量 Pod 处于 Pending | kubectl | `kubectl get pods --field-selector=status.phase=Pending` |
| 插件执行慢 | `plugin <name> took too long` | Scheduler 日志 | Scheduler 日志 |
| 抢占频繁 | `preemption attempts increased` | Scheduler 日志 | Scheduler 日志 |

#### 1.1.4 调度策略问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 自定义调度器未生效 | Pod 未被预期调度器调度 | Pod Spec | `kubectl get pod -o yaml` |
| 优先级调度异常 | 高优先级 Pod 未抢占 | Pod Events | `kubectl describe pod` |
| 调度门控阻塞 | `schedulingGates not cleared` | Pod Events | `kubectl describe pod` (v1.28+) |
| 扩展点错误 | `extension point <name> failed` | Scheduler 日志 | Scheduler 日志 |

### 1.2 报错查看方式汇总

```bash
# 查看 Scheduler 进程状态（systemd 管理）
systemctl status kube-scheduler

# 查看 Scheduler 日志（systemd 管理）
journalctl -u kube-scheduler -f --no-pager -l

# 查看 Scheduler 日志（静态 Pod 方式）
kubectl logs -n kube-system kube-scheduler-<node-name> --tail=500

# 查看 Scheduler 容器日志
crictl logs $(crictl ps -q --name kube-scheduler)

# 检查 Scheduler 健康状态
curl -k https://127.0.0.1:10259/healthz

# 查看 Scheduler Leader 信息
kubectl get leases -n kube-system kube-scheduler -o yaml

# 查看调度失败的 Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending

# 查看 Pod 调度事件
kubectl describe pod <pod-name> | grep -A20 Events

# 查看 Scheduler 指标
curl -k https://127.0.0.1:10259/metrics | grep scheduler
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **新 Pod 调度** | 完全不可用 | 新创建的 Pod 无法被调度到节点 |
| **Pod 重调度** | 不可用 | 需要重调度的 Pod（如节点驱逐）无法调度 |
| **抢占机制** | 失效 | 高优先级 Pod 无法抢占低优先级 Pod |
| **资源分配** | 停滞 | 集群资源无法被合理分配 |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **现有工作负载** | 无直接影响 | 已运行的 Pod 继续运行 |
| **Deployment 扩容** | 失败 | 新副本无法调度 |
| **DaemonSet 部署** | 部分影响 | 新节点上的 DaemonSet Pod 无法调度 |
| **Job/CronJob** | 失败 | 新的 Job Pod 无法调度 |
| **故障恢复** | 延迟 | 节点故障后 Pod 无法重新调度 |
| **自动扩缩容** | 失效 | HPA 扩容的 Pod 无法调度 |
| **滚动更新** | 阻塞 | 新版本 Pod 无法调度，更新无法完成 |

#### 1.3.3 影响评估矩阵

| 故障持续时间 | 影响程度 | 业务影响 | 响应优先级 |
|--------------|----------|----------|------------|
| < 5 分钟 | 低 | 少量 Pod 调度延迟 | P2 |
| 5-30 分钟 | 中 | 新部署和扩容受阻 | P1 |
| 30-60 分钟 | 高 | 故障恢复受影响 | P0 |
| > 60 分钟 | 严重 | 业务连续性风险 | P0 紧急 |

---

## 2. 排查方法与步骤

### 2.1 排查原理

Scheduler 负责将 Pod 分配到合适的节点。排查需要从以下层面：

#### 2.1.1 服务层面
- **调度循环(Scheduling Cycle)**：Scheduler 持续监听 API Server 中 `spec.nodeName` 为空的 Pod，将其加入调度队列
- **绑定循环(Binding Cycle)**：异步执行最终的 Pod 与 Node 绑定操作，避免阻塞调度决策
- **健康检查**：`/healthz` 端点检查 Leader 状态、Informer 缓存同步状态
- **优先级队列**：Pod 按 `priorityClass` 排序，高优先级 Pod 优先调度，同优先级按创建时间 FIFO

#### 2.1.2 连接层面
- **Informer 机制**：Scheduler 通过 Informer 缓存节点/Pod/PV/PVC 等资源，减少 API 调用
- **List-Watch**：初始 LIST 全量加载，后续 WATCH 增量更新，网络中断会导致缓存失效重建
- **客户端限流**：Scheduler 对 API Server 的 QPS/Burst 限制，避免过载
- **证书认证**：`--kubeconfig` 或 `--authentication-kubeconfig` 配置客户端证书

#### 2.1.3 选举层面
- **Lease 机制**：多个 Scheduler 实例通过 Lease 资源竞争成为 Leader，非 Leader 待命
- **Leader 标识**：`kube-system/kube-scheduler` Lease 的 `spec.holderIdentity` 标识当前 Leader
- **租约续期**：Leader 每隔 `--leader-elect-renew-deadline`(默认 10s) 续期，失败则触发重新选举
- **脑裂保护**：Lease 有全局唯一性，保证只有一个 Leader 执行调度

#### 2.1.4 算法层面 - 调度框架(Scheduling Framework)
Scheduler v1.19+ 采用插件化调度框架，核心扩展点：

1. **队列排序(QueueSort)**：决定 Pod 从队列中被取出的顺序（默认按优先级）
2. **PreFilter**：预处理，如检查 PVC 是否存在，失败则跳过本轮调度
3. **Filter(Predicate)**：**过滤阶段**，并行检查每个节点是否满足 Pod 要求
   - `NodeResourcesFit`：检查 CPU/内存/临时存储是否充足
   - `NodeName`：检查 `spec.nodeName` 是否匹配
   - `PodToleratesNodeTaints`：检查 Pod 是否容忍节点污点
   - `NodeAffinity`：检查节点是否满足亲和性
   - `PodTopologySpread`：检查拓扑分布约束
   - `VolumeBinding`：检查 PVC 绑定与节点存储能力
4. **PostFilter**：所有节点都被过滤后触发，执行**抢占(Preemption)**尝试驱逐低优先级 Pod
5. **PreScore**：评分前的预处理，如计算节点间平衡度
6. **Score(Priority)**：**打分阶段**，为通过过滤的节点打分（0-100）
   - `NodeResourcesBalancedAllocation`：CPU/内存使用均衡度
   - `ImageLocality`：镜像是否已在节点上（减少拉取时间）
   - `InterPodAffinity`：Pod 间亲和性/反亲和性
   - `NodeAffinity`：节点亲和性权重
   - `TaintToleration`：容忍度匹配度
7. **NormalizeScore**：归一化分数到 0-100
8. **Reserve**：为 Pod 预留节点资源（内存中标记，未实际绑定）
9. **Permit**：准入检查，可暂停绑定等待外部条件（如批量调度）
10. **PreBind**：绑定前操作，如 Volume Attach
11. **Bind**：执行实际绑定（更新 Pod 的 `spec.nodeName`）
12. **PostBind**：绑定后操作（通常为空）

**并行化**：
- **Filter 并行**：`--parallelism`（默认 16）控制并发检查的节点数
- **Score 串行**：各插件顺序执行，权重累加

#### 2.1.5 配置层面
- **调度策略(Policy)**：v1.23 前通过 `--policy-config-file` 配置，已弃用
- **调度配置(KubeSchedulerConfiguration)**：v1.23+ 推荐，通过 `--config` 指定 YAML 配置文件
- **插件启用/禁用**：可选择性启用/禁用内置插件或注册自定义插件
- **插件权重调整**：Score 插件权重默认 1，可调整影响最终分数
- **调度门控(SchedulingGates)**：v1.27+ 特性，可暂停调度直到外部条件满足（如配额批准）
- **多调度器**：同一集群可运行多个调度器，Pod 通过 `spec.schedulerName` 指定

### 2.2 排查逻辑决策树

```
开始排查
    │
    ├─► 检查 Scheduler 状态
    │       │
    │       ├─► 进程不存在 ──► 检查启动失败原因
    │       │
    │       └─► 进程存在 ──► 继续下一步
    │
    ├─► 检查 API Server 连接
    │       │
    │       ├─► 连接失败 ──► 检查网络和证书
    │       │
    │       └─► 连接正常 ──► 继续下一步
    │
    ├─► 检查 Leader 选举（HA 场景）
    │       │
    │       ├─► 非 Leader ──► 检查是否有其他 Leader
    │       │
    │       └─► 是 Leader ──► 继续下一步
    │
    ├─► 检查调度失败原因
    │       │
    │       ├─► 资源不足 ──► 检查节点资源
    │       │
    │       ├─► 约束不满足 ──► 检查亲和性/污点配置
    │       │
    │       └─► 其他原因 ──► 根据事件分析
    │
    └─► 检查调度性能
            │
            ├─► 延迟高 ──► 分析插件执行时间
            │
            └─► 性能正常 ──► 完成排查
```

### 2.3 排查步骤和具体命令

#### 2.3.1 第一步：检查 Scheduler 进程状态

```bash
# 检查进程是否存在
ps aux | grep kube-scheduler | grep -v grep

# systemd 管理的服务状态
systemctl status kube-scheduler

# 静态 Pod 方式检查
crictl ps -a | grep kube-scheduler

# 查看进程启动参数
cat /proc/$(pgrep kube-scheduler)/cmdline | tr '\0' '\n'

# 检查健康端点
curl -k https://127.0.0.1:10259/healthz

# 查看详细健康状态
curl -k 'https://127.0.0.1:10259/healthz?verbose'
```

#### 2.3.2 第二步：检查 API Server 连接

```bash
# 查看 Scheduler 日志中的连接错误
journalctl -u kube-scheduler | grep -iE "(unable to connect|connection refused|error)"

# 测试 kubeconfig 是否有效
kubectl --kubeconfig=/etc/kubernetes/scheduler.conf get nodes

# 检查证书有效期
openssl x509 -in /etc/kubernetes/pki/scheduler.crt -noout -dates 2>/dev/null || \
openssl x509 -in /etc/kubernetes/scheduler.conf -noout -dates 2>/dev/null

# 检查 API Server 可达性
curl -k https://<api-server-ip>:6443/healthz
```

#### 2.3.3 第三步：检查 Leader 选举

```bash
# 查看 Scheduler Lease
kubectl get leases -n kube-system kube-scheduler -o yaml

# 输出示例：
# spec:
#   holderIdentity: master-1_<uuid>
#   leaseDurationSeconds: 15
#   renewTime: "2024-01-15T10:30:00Z"

# 检查当前哪个 Scheduler 是 Leader
kubectl get leases -n kube-system kube-scheduler -o jsonpath='{.spec.holderIdentity}'

# 查看 Scheduler 日志中的选举信息
journalctl -u kube-scheduler | grep -iE "(became leader|acquired lease|lost lease)"

# 高可用场景：检查所有 Scheduler 实例
for node in master-1 master-2 master-3; do
  echo "=== $node ==="
  ssh $node "crictl ps | grep kube-scheduler"
done
```

#### 2.3.4 第四步：检查调度失败原因

```bash
# 查看所有 Pending Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending

# 查看 Pod 调度事件
kubectl describe pod <pod-name> -n <namespace> | grep -A30 Events

# 查看 Pod 的调度条件
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A20 conditions

# 检查节点资源
kubectl describe nodes | grep -A10 "Allocated resources"

# 查看节点可用资源
kubectl top nodes

# 检查节点污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints'

# 检查特定 Pod 的亲和性配置
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A50 affinity

# 检查 PVC 状态
kubectl get pvc -n <namespace>

# 查看调度器记录的失败原因
kubectl get events --field-selector=reason=FailedScheduling --sort-by='.metadata.creationTimestamp'
```

#### 2.3.5 第五步：检查调度配置

```bash
# 查看 Scheduler 配置文件
cat /etc/kubernetes/scheduler-config.yaml

# 检查 Scheduler 启动参数
crictl inspect $(crictl ps -q --name kube-scheduler) | jq '.info.config.process.args'

# 查看默认调度器配置（v1.25+）
kubectl get configmap -n kube-system kube-scheduler -o yaml

# 检查调度器 Profile
cat /etc/kubernetes/scheduler-config.yaml | grep -A50 profiles

# 验证配置语法
kube-scheduler --config=/etc/kubernetes/scheduler-config.yaml --dry-run
```

#### 2.3.6 第六步：检查调度性能

```bash
# 获取调度器指标
curl -k https://127.0.0.1:10259/metrics | grep -E "scheduler_"

# 关键指标说明：
# scheduler_scheduling_duration_seconds - 调度延迟
# scheduler_pending_pods - 等待调度的 Pod 数
# scheduler_preemption_attempts_total - 抢占尝试次数
# scheduler_pod_scheduling_attempts - Pod 调度尝试次数

# 检查调度延迟分布
curl -k https://127.0.0.1:10259/metrics | grep scheduler_scheduling_duration_seconds

# 检查 Pending Pod 数量
curl -k https://127.0.0.1:10259/metrics | grep scheduler_pending_pods

# 检查调度队列状态
curl -k https://127.0.0.1:10259/metrics | grep scheduler_queue_incoming_pods_total

# 检查插件执行时间
curl -k https://127.0.0.1:10259/metrics | grep scheduler_plugin_execution_duration_seconds
```

#### 2.3.7 第七步：检查日志

```bash
# 实时查看日志
journalctl -u kube-scheduler -f --no-pager

# 查看最近的错误日志
journalctl -u kube-scheduler -p err --since "1 hour ago"

# 静态 Pod 方式查看日志
crictl logs $(crictl ps -q --name kube-scheduler) 2>&1 | tail -500

# 查找调度失败相关日志
journalctl -u kube-scheduler | grep -iE "(failed|unable|error|cannot)" | tail -50

# 提高日志级别进行调试（临时）
# 修改启动参数添加 --v=4 或更高

# 查看特定 Pod 的调度日志
journalctl -u kube-scheduler | grep "<pod-name>" | tail -20
```

### 2.4 排查注意事项

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **kubeconfig 安全** | Scheduler 的 kubeconfig 有集群权限 | 不要泄露 |
| **证书安全** | 证书用于 API Server 认证 | 妥善保管 |
| **配置敏感性** | 调度配置影响资源分配 | 变更需审批 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **高可用场景** | 多 Scheduler 实例需要 Leader 选举 | 确保只有一个 Leader |
| **配置变更** | 配置变更需要重启 Scheduler | 在维护窗口操作 |
| **日志级别** | 高日志级别会影响性能 | 调试完成后恢复 |
| **自定义调度器** | 检查是否使用了自定义调度器 | 确认 schedulerName |

### 🚀 2.5 深度解析（专家专区）

#### 2.5.1 调度器的乐观并发与重试机制
Scheduler 在做决定时并不会锁定节点，而是采用乐观锁（Optimistic Concurrency）。
- **专家提示**：如果在 `Bind` 阶段失败（通常是 etcd 响应慢或资源已被抢先占用），Pod 会重新进入调度队列。通过观察 `scheduler_pod_scheduling_attempts` 指标可以判断集群是否存在激烈的调度竞争。

#### 2.5.2 资源预留（Requests）与超售（Overcommit）
- **核心逻辑**：Scheduler 只看 `requests` 而非 `limits` 或节点实际 CPU/内存负载。
- **专家提示**：如果节点已经很卡但 Scheduler 还在往上面调度 Pod，说明 `requests` 设置得太小。建议使用 `VerticalPodAutoscaler` (VPA) 或手动调整 `requests` 以逼近真实消耗。

#### 2.5.3 亲和性冲突的“死结”
现象：多个 Pod 设置了强亲和性（Required），但集群中没有足够的节点能同时满足所有 Pod 的互斥条件。
- **排查思路**：检查 `podAntiAffinity` 的 `topologyKey`。如果所有 Pod 都要求在不同节点且 `topologyKey: kubernetes.io/hostname`，而节点数少于副本数，则必定有 Pod Pending。
- **解决方案**：评估是否可以使用 `preferredDuringSchedulingIgnoredDuringExecution` (软亲和性) 来增加灵活性。

---

## 3. 解决方案与风险控制

### 3.1 Scheduler 进程未运行

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查启动失败原因
journalctl -u kube-scheduler -b --no-pager | tail -100

# 步骤 2：检查配置文件语法
# 验证 YAML 语法
python3 -c "import yaml; yaml.safe_load(open('/etc/kubernetes/manifests/kube-scheduler.yaml'))"

# 步骤 3：检查证书文件
ls -la /etc/kubernetes/pki/
ls -la /etc/kubernetes/scheduler.conf

# 步骤 4：验证 kubeconfig
kubectl --kubeconfig=/etc/kubernetes/scheduler.conf cluster-info

# 步骤 5：修复问题后重启
# systemd 方式
systemctl restart kube-scheduler

# 静态 Pod 方式
mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/
sleep 5
mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

# 步骤 6：验证恢复
kubectl get pods -n kube-system | grep scheduler
curl -k https://127.0.0.1:10259/healthz
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启期间新 Pod 无法调度 | 在维护窗口操作 |
| **低** | 配置检查一般无风险 | - |
| **中** | 配置修改可能引入新问题 | 修改前备份 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. Scheduler 不可用期间新 Pod 将处于 Pending 状态
2. 已运行的 Pod 不受影响
3. 高可用集群确保其他 Scheduler 实例正常
4. 修改配置前备份原始文件
5. 验证恢复后检查 Pending Pod 是否被调度
```

### 3.2 Pod 因资源不足无法调度

#### 3.2.1 解决步骤

```bash
# 步骤 1：确认资源不足情况
kubectl describe pod <pod-name> | grep -A10 Events

# 步骤 2：检查节点资源使用
kubectl describe nodes | grep -A15 "Allocated resources"
kubectl top nodes

# 步骤 3：检查 Pod 资源请求
kubectl get pod <pod-name> -o yaml | grep -A10 resources

# 步骤 4：解决方案选择
# 方案 A：减少 Pod 资源请求（如果请求过大）
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"requests":{"cpu":"100m","memory":"128Mi"}}}]}}}}'

# 方案 B：扩容节点资源（添加新节点）
# 联系运维或云平台添加节点

# 方案 C：清理无用资源
kubectl get pods --all-namespaces | grep -E "(Evicted|Error|Completed)" | awk '{print $1,$2}' | xargs -L1 kubectl delete pod -n

# 方案 D：使用集群自动扩缩容（CA）
# 确保 Cluster Autoscaler 已配置并正常工作
kubectl get pods -n kube-system | grep cluster-autoscaler

# 步骤 5：验证调度成功
kubectl get pod <pod-name> -w
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | 减少资源请求可能影响性能 | 根据实际需求调整 |
| **中** | 添加节点需要时间 | 评估业务紧急程度 |
| **低** | 清理资源一般无风险 | 确认是无用资源 |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 减少资源请求前确认应用实际需求
2. 不要过度减少 request 导致资源争抢
3. 清理资源前确认不会影响业务
4. 添加节点后验证节点状态正常
5. 考虑设置 ResourceQuota 防止资源过度使用
```

### 3.3 Pod 因亲和性/污点无法调度

#### 3.3.1 解决步骤

```bash
# 步骤 1：检查 Pod 亲和性配置
kubectl get pod <pod-name> -o yaml | grep -A30 affinity

# 步骤 2：检查节点标签
kubectl get nodes --show-labels

# 步骤 3：检查节点污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints'

# 步骤 4：检查 Pod 容忍度
kubectl get pod <pod-name> -o yaml | grep -A10 tolerations

# 步骤 5：解决方案选择
# 方案 A：修改 Pod 亲和性配置
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"affinity":null}}}}'

# 方案 B：添加节点标签
kubectl label nodes <node-name> <key>=<value>

# 方案 C：移除节点污点
kubectl taint nodes <node-name> <key>-

# 方案 D：添加 Pod 容忍度
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"tolerations":[{"key":"<key>","operator":"Exists","effect":"NoSchedule"}]}}}}'

# 步骤 6：验证调度
kubectl get pod <pod-name> -w
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 修改亲和性可能影响高可用 | 评估调度策略变更影响 |
| **低** | 添加标签一般无风险 | 确认标签用途 |
| **中** | 移除污点可能导致不合适的调度 | 评估污点的作用 |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 修改亲和性前理解原有配置的目的
2. 节点污点通常有特定用途，移除前需评估
3. 批量修改亲和性可能导致大量 Pod 重调度
4. 建议使用软亲和性（preferred）而非硬亲和性（required）
5. 变更后监控 Pod 分布情况
```

### 3.4 Scheduler 性能问题

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认性能瓶颈
curl -k https://127.0.0.1:10259/metrics | grep scheduler_scheduling_duration_seconds

# 步骤 2：检查调度队列
curl -k https://127.0.0.1:10259/metrics | grep scheduler_pending_pods

# 步骤 3：分析插件执行时间
curl -k https://127.0.0.1:10259/metrics | grep scheduler_plugin_execution_duration_seconds

# 步骤 4：优化调度器配置
# 调整并行度（v1.25+）
cat > /etc/kubernetes/scheduler-config.yaml << EOF
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
parallelism: 32  # 默认 16
profiles:
  - schedulerName: default-scheduler
    plugins:
      preScore:
        disabled:
          - name: InterPodAffinity  # 禁用高开销插件（如不需要）
EOF

# 步骤 5：重启 Scheduler 应用配置
mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/
sleep 5
mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

# 步骤 6：验证性能改善
curl -k https://127.0.0.1:10259/metrics | grep scheduler_scheduling_duration_seconds
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 禁用插件可能影响调度策略 | 确认插件作用后再禁用 |
| **中** | 配置变更需要重启 | 在维护窗口操作 |
| **低** | 调整并行度一般无风险 | 根据 CPU 资源调整 |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 禁用插件前理解其功能
2. InterPodAffinity 插件对性能影响大，但某些场景必需
3. 增加并行度会增加 CPU 使用
4. 配置变更后监控调度延迟
5. 大规模集群建议使用调度框架扩展
```

### 3.5 自定义调度器问题

#### 3.5.1 解决步骤

```bash
# 步骤 1：确认 Pod 使用的调度器
kubectl get pod <pod-name> -o yaml | grep schedulerName

# 步骤 2：检查自定义调度器状态
kubectl get pods -n kube-system | grep <scheduler-name>

# 步骤 3：查看自定义调度器日志
kubectl logs -n kube-system <custom-scheduler-pod>

# 步骤 4：如果自定义调度器故障，临时使用默认调度器
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"schedulerName":"default-scheduler"}}}}'

# 步骤 5：修复自定义调度器
# 检查调度器 Deployment
kubectl describe deployment -n kube-system <custom-scheduler>

# 检查调度器 RBAC
kubectl get clusterrolebinding | grep <custom-scheduler>

# 步骤 6：恢复使用自定义调度器
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"schedulerName":"<custom-scheduler>"}}}}'
```

#### 3.5.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 切换调度器可能影响调度策略 | 临时措施，尽快修复原调度器 |
| **低** | 日志查看无风险 | - |
| **中** | RBAC 变更可能影响权限 | 谨慎修改 |

#### 3.5.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 自定义调度器可能有特定的调度策略
2. 切换到默认调度器是临时解决方案
3. 确保自定义调度器有正确的 RBAC 权限
4. 自定义调度器需要正确处理 Leader 选举
5. 监控自定义调度器的健康状态
```

---

## 附录

### A. Scheduler 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `scheduler_scheduling_duration_seconds` | 调度延迟 | P99 > 1s |
| `scheduler_pending_pods` | Pending Pod 数 | > 100 |
| `scheduler_preemption_attempts_total` | 抢占尝试数 | 异常增长 |
| `scheduler_pod_scheduling_attempts` | 调度尝试次数 | 每 Pod > 10 |
| `scheduler_queue_incoming_pods_total` | 入队 Pod 数 | 监控趋势 |

### B. 常见启动参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--config` | - | 调度器配置文件路径 |
| `--leader-elect` | true | 是否启用 Leader 选举 |
| `--bind-address` | 0.0.0.0 | 监听地址 |
| `--secure-port` | 10259 | HTTPS 端口 |
| `--v` | 0 | 日志级别 |

### C. 调度失败常见原因速查

| 错误信息 | 原因 | 解决方向 |
|----------|------|----------|
| `Insufficient cpu` | CPU 资源不足 | 添加节点或减少请求 |
| `Insufficient memory` | 内存资源不足 | 添加节点或减少请求 |
| `node(s) had taints` | 节点有污点 | 添加容忍度或移除污点 |
| `didn't match node selector` | 节点选择器不匹配 | 修改选择器或添加标签 |
| `didn't match pod affinity` | 亲和性不满足 | 修改亲和性配置 |
| `PersistentVolumeClaim not found` | PVC 不存在 | 创建 PVC |

---

## 📚 D. 生产环境实战案例精选

### 案例 1：拓扑分布约束配置错误导致大规模 Pod Pending

#### 🎯 故障场景
某电商公司在黑五促销前对核心服务进行扩容，将副本数从 50 提升至 200，结果 150 个新 Pod 全部 Pending，扩容失败，差点影响大促。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   kubectl get pods -n production | grep Pending | wc -l
   # 150
   
   # 查看调度失败原因
   kubectl describe pod my-service-7d8f9c-xxxxx -n production
   # Events:
   # Warning  FailedScheduling  0/100 nodes are available: 150 pod didn't match pod topology spread constraints.
   ```

2. **拓扑约束检查**：
   ```bash
   kubectl get pod my-service-7d8f9c-xxxxx -n production -o yaml | grep -A20 topologySpreadConstraints
   # topologySpreadConstraints:
   # - maxSkew: 1
   #   topologyKey: kubernetes.io/hostname
   #   whenUnsatisfiable: DoNotSchedule  # ❌ 硬约束
   #   labelSelector:
   #     matchLabels:
   #       app: my-service
   ```

3. **节点分布分析**：
   ```bash
   # 统计各节点现有 Pod 数
   kubectl get pods -n production -l app=my-service -o json | \
     jq -r '.items[] | select(.spec.nodeName!=null) | .spec.nodeName' | \
     sort | uniq -c | sort -rn
   # 50 node-01  # ❌ 第一批 50 个 Pod 全在同一节点
   ```

4. **根因分析**：
   - 配置了 `maxSkew: 1` + `whenUnsatisfiable: DoNotSchedule`（硬约束）
   - 意图：每个节点最多比其他节点多 1 个 Pod
   - 实际：第一批 50 个 Pod 因调度顺序都落在 node-01（该节点资源充足）
   - 扩容时：新 Pod 无法调度到 node-01（已有 50 个），其他节点最多只能放 51 个，无法满足 `maxSkew: 1` 约束
   - **死锁**：150 个新 Pod 永远无法调度！

#### ⚡ 应急措施
1. **临时放宽约束**（修改为软约束）：
   ```bash
   # 修改 Deployment
   kubectl edit deployment my-service -n production
   
   # 修改约束
   topologySpreadConstraints:
   - maxSkew: 1
     topologyKey: kubernetes.io/hostname
     whenUnsatisfiable: ScheduleAnyway  # ✅ 改为软约束
     labelSelector:
       matchLabels:
         app: my-service
   ```

2. **触发重新调度**：
   ```bash
   # Deployment 会自动触发滚动更新
   kubectl rollout status deployment my-service -n production
   
   # 验证 Pod 调度成功
   kubectl get pods -n production -l app=my-service --field-selector=status.phase=Running | wc -l
   # 200  ✅ 全部调度成功
   ```

3. **验证分布情况**：
   ```bash
   kubectl get pods -n production -l app=my-service -o json | \
     jq -r '.items[] | select(.spec.nodeName!=null) | .spec.nodeName' | \
     sort | uniq -c | sort -rn | head -10
   # 52 node-01  # 略有不均衡，但可接受
   # 51 node-02
   # 50 node-03
   # ...
   ```

#### 🛡️ 长期优化
1. **优化拓扑约束策略**：
   ```yaml
   # 推荐配置
   topologySpreadConstraints:
   - maxSkew: 2  # ✅ 放宽至 2，增加调度灵活性
     topologyKey: topology.kubernetes.io/zone  # ✅ 先保证跨可用区均衡
     whenUnsatisfiable: DoNotSchedule  # 跨 AZ 硬约束
     labelSelector:
       matchLabels:
         app: my-service
   - maxSkew: 5  # ✅ 节点级容忍更大偏差
     topologyKey: kubernetes.io/hostname
     whenUnsatisfiable: ScheduleAnyway  # 节点级软约束
     labelSelector:
       matchLabels:
         app: my-service
   ```

2. **使用 PodDisruptionBudget 保证可用性**：
   ```yaml
   apiVersion: policy/v1
   kind: PodDisruptionBudget
   metadata:
     name: my-service-pdb
     namespace: production
   spec:
     minAvailable: 80%  # 保证至少 80% 副本可用
     selector:
       matchLabels:
         app: my-service
   ```

3. **预扩容演练**：
   ```bash
   # 大促前模拟扩容
   kubectl scale deployment my-service --replicas=200 -n production
   
   # 观察 5 分钟内是否全部调度成功
   watch -n 5 'kubectl get pods -n production -l app=my-service | grep -E "(Pending|ContainerCreating)" | wc -l'
   ```

4. **监控告警**：
   ```yaml
   # Prometheus 告警规则
   - alert: PodSchedulingFailed
     expr: kube_pod_status_phase{phase="Pending"} > 10
     for: 5m
     labels:
       severity: warning
     annotations:
       summary: "大量 Pod 调度失败"
       description: "命名空间 {{ $labels.namespace }} 有 {{ $value }} 个 Pod Pending 超过 5 分钟"
   ```

#### 💡 经验总结
- **硬约束风险**：`DoNotSchedule` 可能导致调度死锁，生产环境慎用
- **测试不足**：未在预生产环境模拟大规模扩容
- **配置复杂性**：拓扑约束语义不直观，需深入理解
- **改进方向**：优先使用软约束、多层次拓扑策略、充分测试、监控告警

---

### 案例 2：Inter-Pod Affinity 导致调度性能暴跌

#### 🎯 故障场景
某 SaaS 公司集群规模 500 节点、5000 Pod，部署了一个新服务配置了 Pod 反亲和性，结果 Scheduler 调度延迟从 100ms 暴涨至 30s，导致所有新 Pod 调度缓慢，影响全局。

#### 🔍 排查过程
1. **性能指标异常**：
   ```bash
   # Scheduler 指标
   curl -k https://127.0.0.1:10259/metrics | grep scheduler_scheduling_duration_seconds_bucket
   # scheduler_scheduling_duration_seconds_bucket{le="30"} 1523  # P99 > 30s ❌
   
   # 调度队列堆积
   curl -k https://127.0.0.1:10259/metrics | grep scheduler_pending_pods
   # scheduler_pending_pods 350  # 大量 Pod 等待调度
   ```

2. **插件性能分析**：
   ```bash
   # 查看插件执行耗时
   curl -k https://127.0.0.1:10259/metrics | grep scheduler_plugin_execution_duration_seconds | grep InterPodAffinity
   # scheduler_plugin_execution_duration_seconds{plugin="InterPodAffinity",extension_point="Filter",...} 28.5  # ❌ 单次 28.5s！
   ```

3. **日志分析**：
   ```bash
   kubectl logs -n kube-system kube-scheduler-master1 | grep "took too long"
   # I1210 08:23:15.123456 1 scheduler.go:123] Plugin InterPodAffinity.Filter took too long to execute: 28.5s
   ```

4. **问题配置定位**：
   ```bash
   # 查找配置了复杂亲和性的 Pod
   kubectl get pods -A -o json | jq -r '.items[] | select(.spec.affinity.podAntiAffinity!=null) | "\(.metadata.namespace)/\(.metadata.name)"' | head -10
   # production/new-service-abc123  # 新部署的服务
   
   kubectl get pod new-service-abc123 -n production -o yaml | grep -A50 podAntiAffinity
   # podAntiAffinity:
   #   requiredDuringSchedulingIgnoredDuringExecution:  # ❌ 硬反亲和
   #   - labelSelector:
   #       matchExpressions:
   #       - key: app
   #         operator: Exists  # ❌ 匹配所有 Pod！
   #     topologyKey: kubernetes.io/hostname
   ```

5. **根因分析**：
   - 配置了 `operator: Exists`，匹配集群内所有 5000 个 Pod
   - Scheduler 需要遍历所有节点，检查每个节点上的所有 Pod 是否匹配亲和性规则
   - **时间复杂度**：O(节点数 × 每节点 Pod 数 × 规则复杂度) = O(500 × 10 × N) = **数万次匹配运算**
   - 影响全局：Scheduler 单线程 Score 阶段被阻塞，所有 Pod 调度变慢

#### ⚡ 应急措施
1. **立即删除问题服务**：
   ```bash
   # 删除新部署的服务（临时止血）
   kubectl delete deployment new-service -n production
   
   # 验证调度性能恢复
   curl -k https://127.0.0.1:10259/metrics | grep scheduler_scheduling_duration_seconds_bucket
   # P99 < 1s  ✅ 恢复正常
   ```

2. **修复配置后重新部署**：
   ```yaml
   apiVersion: apps/v1
   kind: Deployment
   metadata:
     name: new-service
     namespace: production
   spec:
     template:
       spec:
         affinity:
           podAntiAffinity:
             preferredDuringSchedulingIgnoredDuringExecution:  # ✅ 改为软反亲和
             - weight: 100
               podAffinityTerm:
                 labelSelector:
                   matchLabels:  # ✅ 精确匹配自身
                     app: new-service
                 topologyKey: kubernetes.io/hostname
   ```

3. **部署并验证**：
   ```bash
   kubectl apply -f new-service.yaml
   
   # 监控调度性能
   kubectl get pods -n production -l app=new-service --watch
   # 新 Pod 在 5 秒内调度成功 ✅
   ```

#### 🛡️ 长期优化
1. **准入控制验证亲和性配置**：
   ```yaml
   # 使用 OPA Gatekeeper 策略
   apiVersion: templates.gatekeeper.sh/v1
   kind: ConstraintTemplate
   metadata:
     name: podaffinityrestriction
   spec:
     crd:
       spec:
         names:
           kind: PodAffinityRestriction
     targets:
     - target: admission.k8s.gatekeeper.sh
       rego: |
         package podaffinity
         
         violation[{"msg": msg}] {
           affinity := input.review.object.spec.affinity.podAntiAffinity
           # 检查是否使用 Exists 操作符匹配所有 Pod
           affinity.requiredDuringSchedulingIgnoredDuringExecution[_].labelSelector.matchExpressions[_].operator == "Exists"
           not affinity.requiredDuringSchedulingIgnoredDuringExecution[_].labelSelector.matchExpressions[_].key == input.review.object.metadata.labels.app
           msg := "禁止使用 Exists 操作符匹配所有 Pod 的硬反亲和性"
         }
   ```

2. **Scheduler 配置优化**：
   ```yaml
   apiVersion: kubescheduler.config.k8s.io/v1
   kind: KubeSchedulerConfiguration
   parallelism: 32  # ✅ 提高并行度（默认 16）
   profiles:
   - schedulerName: default-scheduler
     plugins:
       score:
         disabled:
         - name: InterPodAffinity  # ✅ 如不需要，可禁用评分阶段亲和性插件
         enabled:
         - name: InterPodAffinity
           weight: 1  # ✅ 降低权重（默认 1，可调至更低）
     pluginConfig:
     - name: InterPodAffinity
       args:
         hardPodAffinityWeight: 1  # ✅ 降低硬亲和性权重
   ```

3. **性能基准测试**：
   ```bash
   # 使用 kube-scheduler-simulator 模拟调度
   git clone https://github.com/kubernetes-sigs/kube-scheduler-simulator
   cd kube-scheduler-simulator
   
   # 导入当前集群状态
   kubectl get nodes -o yaml > nodes.yaml
   kubectl get pods -A -o yaml > pods.yaml
   
   # 模拟新 Pod 调度
   # 测试不同亲和性配置的调度耗时
   ```

4. **监控告警**：
   ```yaml
   # 调度延迟告警
   - alert: SchedulerHighLatency
     expr: histogram_quantile(0.99, rate(scheduler_scheduling_duration_seconds_bucket[5m])) > 1
     for: 5m
     labels:
       severity: warning
     annotations:
       summary: "调度延迟过高"
       description: "Scheduler P99 调度延迟 {{ $value }}s，超过 1s 阈值"
   
   # 插件执行耗时告警
   - alert: SchedulerPluginSlow
     expr: histogram_quantile(0.99, rate(scheduler_plugin_execution_duration_seconds_bucket[5m])) > 5
     for: 5m
     labels:
       severity: warning
     annotations:
       summary: "调度插件执行缓慢"
       description: "插件 {{ $labels.plugin }} 在扩展点 {{ $labels.extension_point }} 执行耗时 {{ $value }}s"
   ```

#### 💡 经验总结
- **配置错误**：`operator: Exists` 匹配范围过大，导致性能灾难
- **测试不足**：未在预生产环境测试大规模亲和性配置
- **监控盲区**：未监控 Scheduler 插件级性能指标
- **改进方向**：准入控制验证、精确匹配、软亲和性优先、性能监控、定期基准测试

---

### 案例 3：节点资源碎片化导致大 Pod 无法调度

#### 🎯 故障场景
某 AI 公司需要部署 GPU 训练任务，Pod 请求 8 核 32GB 内存 + 1 GPU，但集群有 100 个节点，总资源充足，Pod 却一直 Pending。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   kubectl describe pod gpu-training-job-xxxxx
   # Events:
   # Warning  FailedScheduling  0/100 nodes are available: 
   #   50 Insufficient cpu, 
   #   30 Insufficient memory, 
   #   20 Insufficient nvidia.com/gpu.
   ```

2. **集群资源总览**：
   ```bash
   # 总资源统计
   kubectl describe nodes | grep -A5 "Allocated resources" | grep -E "(cpu|memory)" | \
     awk '{sum+=$2} END {print sum}'
   # 总 CPU: 800 核（100 节点 × 8 核）
   # 总内存: 3200 GB（100 节点 × 32GB）
   # 已分配: CPU 400 核, 内存 1600 GB  # ✅ 仅 50% 利用率
   ```

3. **单节点资源检查**：
   ```bash
   # 查看每个节点剩余资源
   kubectl get nodes -o json | jq -r '.items[] | 
     "\(.metadata.name) CPU:\(.status.allocatable.cpu) Mem:\(.status.allocatable.memory)"' | \
     head -10
   # node-01 CPU:8000m Mem:32Gi
   # node-02 CPU:8000m Mem:32Gi
   # ...
   
   # 检查实际可用资源（减去已分配）
   kubectl describe nodes | grep -A10 "Allocated resources:" | grep -E "cpu|memory" | head -20
   # node-01:
   #   cpu: 7500m (93%)  # ❌ 剩余仅 500m
   #   memory: 28Gi (87%)
   # node-02:
   #   cpu: 7200m (90%)  # ❌ 剩余仅 800m
   #   memory: 30Gi (93%)
   ```

4. **根因分析**：
   - **资源碎片化**：每个节点部署了大量小 Pod（每个 100m CPU、256Mi 内存）
   - 单节点剩余资源均不足 8 核 32GB
   - **类比**：停车场总车位充足，但都是小型车位，无法停下大型卡车

#### ⚡ 应急措施
1. **驱逐低优先级 Pod 释放资源**：
   ```bash
   # 查找占用资源多的节点
   kubectl top nodes --sort-by=cpu | head -5
   
   # 选择一个节点，驱逐低优先级 Pod
   kubectl get pods -A --field-selector spec.nodeName=node-01 -o json | \
     jq -r '.items[] | select(.spec.priorityClassName!="high-priority") | "\(.metadata.namespace)/\(.metadata.name)"' | \
     head -10 | xargs -n1 kubectl delete pod
   
   # 等待 Pod 被驱逐和重新调度到其他节点
   sleep 60
   
   # 验证节点资源释放
   kubectl describe node node-01 | grep -A5 "Allocated resources"
   # cpu: 2000m (25%)  # ✅ 大幅释放
   # memory: 8Gi (25%)
   ```

2. **调度 GPU 任务**：
   ```bash
   # 为 GPU Pod 指定节点（临时）
   kubectl patch pod gpu-training-job-xxxxx -p '{"spec":{"nodeName":"node-01"}}'
   
   # 或重新创建 Pod（调度器会自动选择）
   kubectl delete pod gpu-training-job-xxxxx
   kubectl apply -f gpu-job.yaml
   
   # 验证调度成功
   kubectl get pod gpu-training-job-xxxxx -o wide
   # NAME                      READY   STATUS    RESTARTS   AGE   NODE
   # gpu-training-job-xxxxx    1/1     Running   0          30s   node-01  ✅
   ```

#### 🛡️ 长期优化
1. **节点池分层策略**：
   ```yaml
   # 创建专用节点池
   # 节点池 1: 通用小 Pod（100 节点）
   kubectl label nodes node-{01..100} workload-type=general
   kubectl taint nodes node-{01..100} workload-type=general:NoSchedule
   
   # 节点池 2: 大内存/GPU 任务（20 节点，16 核 64GB + GPU）
   kubectl label nodes gpu-node-{01..20} workload-type=gpu
   kubectl taint nodes gpu-node-{01..20} workload-type=gpu:NoSchedule
   ```

2. **使用 NodeSelector 和 Tolerations**：
   ```yaml
   # GPU 任务配置
   apiVersion: v1
   kind: Pod
   metadata:
     name: gpu-training-job
   spec:
     nodeSelector:
       workload-type: gpu  # ✅ 仅调度到 GPU 节点池
     tolerations:
     - key: workload-type
       operator: Equal
       value: gpu
       effect: NoSchedule
     containers:
     - name: training
       image: pytorch/pytorch:latest
       resources:
         requests:
           cpu: "8"
           memory: 32Gi
           nvidia.com/gpu: "1"
   ```

3. **启用 Cluster Autoscaler**：
   ```yaml
   # 自动扩展节点池
   apiVersion: v1
   kind: ConfigMap
   metadata:
     name: cluster-autoscaler-config
     namespace: kube-system
   data:
     config.yaml: |
       nodePools:
       - name: gpu-pool
         minSize: 5
         maxSize: 50
         machineType: n1-standard-16-gpu
         autoscaling:
           scaleDownUtilizationThreshold: 0.5
           scaleDownUnneededTime: 10m
   ```

4. **资源预留策略**：
   ```yaml
   # 为系统组件预留资源
   apiVersion: kubelet.config.k8s.io/v1beta1
   kind: KubeletConfiguration
   kubeReserved:
     cpu: "1000m"  # 为 kubelet/系统预留 1 核
     memory: "4Gi"  # 预留 4GB
   systemReserved:
     cpu: "500m"
     memory: "2Gi"
   evictionHard:
     memory.available: "2Gi"  # 触发驱逐的阈值
   ```

5. **监控资源碎片化**：
   ```promql
   # 计算节点资源碎片率
   # (已分配 Pod 数 × 平均 Pod 大小) / 节点总资源
   (count(kube_pod_info) by (node) * 0.1) / 
   (kube_node_status_allocatable{resource="cpu"}) * 100
   
   # 告警：碎片率 > 80%
   - alert: NodeResourceFragmentation
     expr: (sum by(node) (kube_pod_container_resource_requests{resource="cpu"}) / kube_node_status_allocatable{resource="cpu"}) > 0.8 and
           (kube_node_status_allocatable{resource="cpu"} - sum by(node) (kube_pod_container_resource_requests{resource="cpu"})) < 2
     labels:
       severity: warning
     annotations:
       summary: "节点资源碎片化严重"
       description: "节点 {{ $labels.node }} 已用 80%+ 资源但剩余不足 2 核，无法调度大 Pod"
   ```

#### 💡 经验总结
- **资源规划不当**：未区分大小 Pod 的调度需求
- **节点同质化**：所有节点规格相同，缺乏灵活性
- **缺乏预留**：未为大 Pod 预留专用节点池
- **改进方向**：节点池分层、自动扩缩容、资源预留、碎片化监控
| `didn't have free ports` | 端口冲突 | 修改 hostPort |
