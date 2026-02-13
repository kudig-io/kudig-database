# Controller Manager 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

Controller Manager 是集群的“执行官”，负责确保集群的“实际状态”始终趋向于“期望状态”。如果它停摆，集群将失去所有的自愈和自动化能力。

### 🎓 初学者视角
- **核心概念**：Controller Manager 其实是一个“控制器”的集合包。每个控制器（如 Deployment 控制器、Node 控制器）都运行在一个死循环里：查看当前情况 -> 发现不对劲 -> 动手修复。
- **简单类比**：它就像一个恒温器的控制器。你设定了 26 度（期望状态），如果感应到是 28 度（实际状态），它就启动空调降温。

### 👨‍💻 资深专家视角
- **工作队列（Workqueue）**：深度理解限速队列（Rate Limiting Queue）如何防止因为某个故障资源的反复同步而拖垮整个控制器。
- **Informer 机制**：分析控制器如何通过本地缓存减少对 API Server 的请求压力，以及 `resyncPeriod` 对资源最终一致性的保障。
- **并发同步**：掌握如何通过 `--concurrent-*-syncs` 参数调优高压力集群下的资源同步吞吐量。

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 10 分钟快速诊断

1. **健康与选举**：`curl -k https://127.0.0.1:10257/healthz?verbose`、`kubectl get lease -n kube-system kube-controller-manager -o wide`，若无 Leader 或频繁切换先查证书/网络/LB。
2. **核心控制器快照**：快速查看 `kubectl get deploy,rs,ds,statefulset,job,cronjob -A | head`、`kubectl get endpoints -A | head`、`kubectl get nodes`，锁定异常资源。
3. **事件与队列深度**：`kubectl describe <resource>` 关注控制器事件；`kubectl get --raw "/metrics" | grep -E "workqueue_(depth|retries|adds)_total" | head` 识别堆积。
4. **API/限流**：日志 grep `throttling` 或 `rate limiter`；检查 `--kube-api-qps/--kube-api-burst`、`--concurrent-*-syncs` 是否过低。
5. **Token/证书/SA**：若 Pod 无法创建 SA Token，检查 `kube-controller-manager` 是否有 `--use-service-account-credentials`，并确认签发证书未过期。
6. **快速缓解**：
   - 功能缺失：临时调高受影响控制器的 `--concurrent-*-syncs`（如 endpoints、replicaset、deployment）。
   - 压力过载：降低外部大规模变更，或临时提升 CM 资源 request/limit。
   - 选举异常：确保只有一个活跃 CM，排查 LB/iptables 导致的租约漂移。
7. **证据留存**：保留 healthz 输出、Leader 租约 YAML、关键控制器日志、workqueue 指标快照用于复盘。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Controller Manager 服务不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 进程未运行 | `kube-controller-manager not running` | systemd/容器 | `systemctl status kube-controller-manager` |
| 连接 API Server 失败 | `error retrieving resource lock` | CM 日志 | CM 日志 |
| 证书错误 | `x509: certificate has expired` | CM 日志 | CM 日志 |
| Leader 选举失败 | `failed to acquire lease` | CM 日志 | CM 日志 |
| 配置错误 | `unable to start controller` | CM 日志 | CM 启动日志 |

#### 1.1.2 控制器功能异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Deployment 不更新 | RS 副本数不变 | kubectl | `kubectl get rs` |
| ReplicaSet 不扩缩容 | Pod 数量不变 | kubectl | `kubectl get pods` |
| Service Endpoints 不更新 | Endpoints 为空 | kubectl | `kubectl get endpoints` |
| Node 状态不更新 | Node 长期 NotReady | kubectl | `kubectl get nodes` |
| Job 不完成 | Job 状态不变 | kubectl | `kubectl get jobs` |
| PV 不绑定 | PVC 长期 Pending | kubectl | `kubectl get pvc` |
| SA Token 不创建 | Pod 启动失败 | Pod Events | `kubectl describe pod` |

#### 1.1.3 特定控制器故障

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Node Controller 异常 | `nodes are not ready` | CM 日志 | CM 日志 |
| Endpoint Controller 异常 | `unable to sync endpoints` | CM 日志 | CM 日志 |
| ReplicaSet Controller 异常 | `unable to manage pods` | CM 日志 | CM 日志 |
| Namespace Controller 异常 | namespace 无法删除 | kubectl | `kubectl get ns` |
| GC Controller 异常 | 孤儿资源累积 | kubectl | `kubectl get all` |

#### 1.1.4 性能问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 控制循环延迟 | `controller sync took too long` | CM 日志 | CM 日志 |
| 工作队列堆积 | 大量待处理事件 | Prometheus | 监控系统 |
| API 限流 | `rate limiter Wait returned an error` | CM 日志 | CM 日志 |
| 内存使用高 | OOM Kill | 系统日志 | `dmesg` |

### 1.2 报错查看方式汇总

```bash
# 查看 Controller Manager 进程状态（systemd 管理）
systemctl status kube-controller-manager

# 查看 Controller Manager 日志（systemd 管理）
journalctl -u kube-controller-manager -f --no-pager -l

# 查看 Controller Manager 日志（静态 Pod 方式）
kubectl logs -n kube-system kube-controller-manager-<node-name> --tail=500

# 查看 Controller Manager 容器日志
crictl logs $(crictl ps -q --name kube-controller-manager)

# 检查健康状态
curl -k https://127.0.0.1:10257/healthz

# 查看详细健康状态
curl -k 'https://127.0.0.1:10257/healthz?verbose'

# 查看 Leader 信息
kubectl get leases -n kube-system kube-controller-manager -o yaml

# 查看控制器启用状态
curl -k https://127.0.0.1:10257/metrics | grep controller_manager

# 查看各控制器工作队列
curl -k https://127.0.0.1:10257/metrics | grep workqueue
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **Deployment 管理** | 失效 | Deployment 无法创建/更新 ReplicaSet |
| **ReplicaSet 管理** | 失效 | ReplicaSet 无法维护 Pod 副本数 |
| **Service Endpoints** | 失效 | Endpoints 无法自动更新 |
| **Node 管理** | 失效 | Node 状态无法检测和更新 |
| **命名空间清理** | 失效 | 删除的命名空间无法清理资源 |
| **垃圾回收** | 失效 | 孤儿资源无法自动清理 |
| **ServiceAccount** | 失效 | Token 无法自动创建 |
| **PV/PVC 绑定** | 失效 | PVC 无法自动绑定 PV |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **现有工作负载** | 短期无影响 | 已运行的 Pod 继续运行 |
| **自愈能力** | 丧失 | Pod 崩溃后无法自动重建 |
| **滚动更新** | 阻塞 | 无法完成 Deployment 更新 |
| **扩缩容** | 失效 | 手动和自动扩缩容都无法执行 |
| **服务发现** | 部分影响 | 新 Pod 无法加入 Endpoints |
| **故障转移** | 失效 | 节点故障后 Pod 无法迁移 |
| **资源清理** | 累积 | 删除的资源无法清理 |

#### 1.3.3 控制器影响矩阵

| 控制器 | 管理资源 | 故障影响 |
|--------|----------|----------|
| **Deployment Controller** | Deployment → ReplicaSet | 无法滚动更新 |
| **ReplicaSet Controller** | ReplicaSet → Pod | 无法维护副本数 |
| **DaemonSet Controller** | DaemonSet → Pod | 新节点无 DaemonSet Pod |
| **StatefulSet Controller** | StatefulSet → Pod | 有状态应用无法管理 |
| **Job Controller** | Job → Pod | Job 无法执行 |
| **CronJob Controller** | CronJob → Job | 定时任务不执行 |
| **Endpoint Controller** | Service → Endpoints | 服务发现异常 |
| **Node Controller** | Node 状态 | 节点状态不更新 |
| **ServiceAccount Controller** | SA → Token | Pod 无法获取 Token |
| **PV/PVC Controller** | PVC → PV 绑定 | 存储无法挂载 |
| **Namespace Controller** | Namespace 清理 | NS 无法删除 |
| **GC Controller** | 孤儿资源 | 资源泄露 |

---

## 2. 排查方法与步骤

### 2.1 排查原理

Controller Manager 运行多个控制器，负责维护集群期望状态。排查需要从以下层面：

#### 2.1.1 服务层面
- **多控制器架构**：Controller Manager 实际是多个控制器的集合体，包括 Deployment、ReplicaSet、Node、Endpoint、ServiceAccount 等 20+ 个控制器
- **独立协程运行**：每个控制器在独立 goroutine 中运行，互不阻塞（但共享 API 客户端和 Informer）
- **控制循环模型**：每个控制器持续运行 `watch → compare → reconcile` 循环
  1. **Watch**：通过 Informer 监听资源变化
  2. **Compare**：比较实际状态与期望状态
  3. **Reconcile**：执行调和操作（创建/更新/删除子资源）
- **启动依赖**：API Server 可达、证书有效、Leader 选举成功、Informer 缓存同步完成

#### 2.1.2 连接层面
- **Shared Informer 机制**：所有控制器共享 Informer 工厂，避免重复 watch 相同资源
- **Informer 缓存**：本地内存缓存所有监听资源的全量数据，减少 API 调用
- **List-Watch 协议**：
  1. 启动时 LIST 全量加载
  2. 运行时 WATCH 增量更新
  3. 定期 Resync（默认 30s）触发全量对账，保证最终一致性
- **客户端限流**：
  - `--kube-api-qps`（默认 20）：每秒最大请求数
  - `--kube-api-burst`（默认 30）：突发请求数
  - 超出时排队等待，避免过载 API Server

#### 2.1.3 选举层面
- **Lease 租约机制**：多个 Controller Manager 实例通过 Lease 资源竞争 Leader
- **租约参数**：
  - `--leader-elect-lease-duration`（默认 15s）：租约有效期
  - `--leader-elect-renew-deadline`（默认 10s）：续期 deadline
  - `--leader-elect-retry-period`（默认 2s）：重试间隔
- **单 Leader 保证**：只有 Leader 执行控制逻辑，非 Leader 待命（避免并发冲突）
- **自动故障转移**：Leader 失联后，其他实例自动接管（通常 < 30s）

#### 2.1.4 控制器层面 - 核心控制器详解

##### 1. Deployment Controller
- **职责**：管理 Deployment → ReplicaSet 生命周期，实现滚动更新
- **工作流程**：
  1. 监听 Deployment 变化
  2. 创建新 ReplicaSet（或更新现有）
  3. 按 `maxSurge`/`maxUnavailable` 策略逐步缩放新旧 RS
  4. 更新 Deployment 状态（replicas/updatedReplicas/availableReplicas）
- **并发参数**：`--concurrent-deployment-syncs`（默认 5）

##### 2. ReplicaSet Controller
- **职责**：维护 ReplicaSet 的 Pod 副本数（期望数 vs 实际数）
- **工作流程**：
  1. 监听 ReplicaSet 和 Pod 变化
  2. 计算需要创建/删除的 Pod 数
  3. 批量创建/删除 Pod（每轮最多 500 个）
  4. 更新 ReplicaSet 状态
- **并发参数**：`--concurrent-replicaset-syncs`（默认 5）
- **关键点**：Pod 的 `ownerReferences` 指向 ReplicaSet，保证 GC 回收

##### 3. Endpoint Controller
- **职责**：根据 Service 选择器自动生成 Endpoints（Pod IP:Port 列表）
- **工作流程**：
  1. 监听 Service 和 Pod 变化
  2. 过滤符合选择器且 Ready 的 Pod
  3. 生成 Endpoints 资源（每个 Service 一个）
  4. 更新 Endpoints（增量更新，避免冲突）
- **并发参数**：`--concurrent-endpoint-syncs`（默认 5）
- **性能优化**：大规模集群（> 5000 Pod）建议提高并发数至 20-50

##### 4. Node Controller
- **职责**：监控节点健康状态，处理节点故障（驱逐 Pod）
- **关键参数**：
  - `--node-monitor-period`（默认 5s）：节点状态检查间隔
  - `--node-monitor-grace-period`（默认 40s）：节点无响应宽限期
  - `--pod-eviction-timeout`（默认 5m）：节点 NotReady 后开始驱逐 Pod 的等待时间
- **工作流程**：
  1. 定期检查 kubelet 上报的 NodeStatus
  2. 超过宽限期无响应 → 标记 NotReady
  3. NotReady 超过驱逐超时 → 删除节点上所有 Pod
- **Taint 管理**：自动为 NotReady 节点添加 `node.kubernetes.io/not-ready:NoExecute` 污点

##### 5. ServiceAccount Controller
- **职责**：为每个 ServiceAccount 自动创建 Secret（存储 Token）
- **工作流程**：
  1. 监听 ServiceAccount 创建事件
  2. 创建对应的 Secret（type: `kubernetes.io/service-account-token`）
  3. Token 签发（使用 `--service-account-private-key-file` 配置的私钥）
  4. 更新 ServiceAccount 的 `secrets` 字段
- **关键配置**：
  - `--use-service-account-credentials`（推荐启用）：每个控制器使用独立 SA
  - `--root-ca-file`：CA 证书路径（注入到 Pod Token Secret）

##### 6. PersistentVolume Controller
- **职责**：管理 PV/PVC 绑定、回收、扩容
- **绑定流程**：
  1. PVC 创建 → 查找匹配的 PV（容量/StorageClass/AccessMode）
  2. 绑定 PV 与 PVC（双向引用）
  3. 更新状态为 Bound
- **回收策略**：
  - `Retain`：手动回收
  - `Delete`：自动删除（动态 PV）
  - `Recycle`（已废弃）：清空数据后重新可用

##### 7. Namespace Controller
- **职责**：处理 Namespace 删除（级联删除所有子资源）
- **删除流程**：
  1. Namespace 标记为 Terminating
  2. 遍历所有资源类型（Pod/Service/ConfigMap...）
  3. 删除 Namespace 下所有资源
  4. 删除 Namespace 本身
- **卡死原因**：子资源删除失败（Finalizer 阻塞）或 API 资源未正确注册

##### 8. GarbageCollector Controller
- **职责**：清理孤儿资源（ownerReferences 指向的资源已删除）
- **删除策略**：
  - `Foreground`：先删子资源，再删父资源
  - `Background`：立即删父资源，后台异步删子资源
  - `Orphan`：删除时断开 ownerReferences，保留子资源
- **工作原理**：维护资源依赖图，检测孤儿对象并删除

#### 2.1.5 性能层面
- **工作队列（Workqueue）机制**：
  - **限速队列**：防止热点资源频繁入队（指数退避重试）
  - **去重**：同一资源在队列中只保留一份
  - **延迟**：失败后延迟重试（避免 API 过载）
- **并发同步数**：`--concurrent-*-syncs` 控制每个控制器的并发 goroutine 数
- **批量操作**：ReplicaSet Controller 创建 Pod 时批量操作（每轮最多 500 个）
- **Informer Resync**：定期全量对账，补偿 watch 丢失的事件（默认 30s）
- **内存消耗**：Informer 缓存所有资源，大集群可达数 GB

### 2.2 排查逻辑决策树

```
开始排查
    │
    ├─► 检查 CM 进程状态
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
    ├─► 检查 Leader 选举
    │       │
    │       ├─► 非 Leader ──► 检查是否有其他 Leader
    │       │
    │       └─► 是 Leader ──► 继续下一步
    │
    ├─► 检查控制器状态
    │       │
    │       ├─► 控制器异常 ──► 分析具体控制器问题
    │       │
    │       └─► 控制器正常 ──► 继续下一步
    │
    └─► 检查性能
            │
            ├─► 延迟高 ──► 分析资源使用和 API 限流
            │
            └─► 性能正常 ──► 完成排查
```

### 2.3 排查步骤和具体命令

#### 2.3.1 第一步：检查进程状态

```bash
# 检查进程是否存在
ps aux | grep kube-controller-manager | grep -v grep

# systemd 管理的服务状态
systemctl status kube-controller-manager

# 静态 Pod 方式检查
crictl ps -a | grep kube-controller-manager

# 查看进程启动参数
cat /proc/$(pgrep kube-controller-manager)/cmdline | tr '\0' '\n'

# 检查健康端点
curl -k https://127.0.0.1:10257/healthz

# 查看详细健康状态
curl -k 'https://127.0.0.1:10257/healthz?verbose'
```

#### 2.3.2 第二步：检查 API Server 连接

```bash
# 查看 CM 日志中的连接错误
journalctl -u kube-controller-manager | grep -iE "(unable to connect|connection refused|error)" | tail -20

# 测试 kubeconfig 是否有效
kubectl --kubeconfig=/etc/kubernetes/controller-manager.conf get nodes

# 检查证书有效期
openssl x509 -in /etc/kubernetes/pki/controller-manager.crt -noout -dates 2>/dev/null

# 检查 API Server 可达性
curl -k https://<api-server-ip>:6443/healthz
```

#### 2.3.3 第三步：检查 Leader 选举

```bash
# 查看 Controller Manager Lease
kubectl get leases -n kube-system kube-controller-manager -o yaml

# 检查当前哪个 CM 是 Leader
kubectl get leases -n kube-system kube-controller-manager -o jsonpath='{.spec.holderIdentity}'

# 查看 CM 日志中的选举信息
journalctl -u kube-controller-manager | grep -iE "(became leader|acquired lease|lost lease)"

# 高可用场景：检查所有 CM 实例
for node in master-1 master-2 master-3; do
  echo "=== $node ==="
  ssh $node "crictl ps | grep kube-controller-manager"
done
```

#### 2.3.4 第四步：检查控制器状态

```bash
# 查看所有启用的控制器
curl -k https://127.0.0.1:10257/metrics | grep controller_manager_controller_started

# 检查各控制器工作队列深度
curl -k https://127.0.0.1:10257/metrics | grep workqueue_depth

# 检查控制器同步延迟
curl -k https://127.0.0.1:10257/metrics | grep workqueue_work_duration_seconds

# 检查控制器错误率
curl -k https://127.0.0.1:10257/metrics | grep workqueue_retries_total

# 查看 CM 日志中的控制器错误
journalctl -u kube-controller-manager | grep -iE "controller.*error" | tail -30

# 检查特定控制器
# Deployment Controller
kubectl get deployments -A -o wide
kubectl describe deployment <name> | grep -A20 Events

# ReplicaSet Controller
kubectl get rs -A
kubectl describe rs <name> | grep -A20 Events

# Endpoint Controller
kubectl get endpoints -A
kubectl describe endpoints <name>

# Node Controller
kubectl get nodes
kubectl describe node <name> | grep -A20 Conditions
```

#### 2.3.5 第五步：检查资源同步状态

```bash
# 检查 Deployment 是否正常同步
kubectl get deployments -A -o wide
# 检查 READY 列是否与 DESIRED 一致

# 检查 ReplicaSet 状态
kubectl get rs -A -o wide
# 检查 READY 是否与 DESIRED 一致

# 检查 Service Endpoints 是否更新
kubectl get endpoints -A
# 检查 ENDPOINTS 列是否有 IP

# 检查 Node Controller 是否正常
kubectl get nodes
# 检查 STATUS 列

# 检查 PVC 绑定状态
kubectl get pvc -A
# 检查 STATUS 是否为 Bound

# 检查命名空间删除状态
kubectl get ns
# 检查是否有长期 Terminating 的 NS
```

#### 2.3.6 第六步：检查性能和资源

```bash
# 检查 CM 资源使用
top -p $(pgrep kube-controller-manager) -b -n 1

# 检查内存使用
cat /proc/$(pgrep kube-controller-manager)/status | grep -E "(VmRSS|VmSize)"

# 检查文件描述符
ls /proc/$(pgrep kube-controller-manager)/fd | wc -l

# 检查 CM metrics 中的资源指标
curl -k https://127.0.0.1:10257/metrics | grep -E "process_resident_memory|process_cpu"

# 检查工作队列堆积
curl -k https://127.0.0.1:10257/metrics | grep workqueue_depth

# 检查 API 请求延迟
curl -k https://127.0.0.1:10257/metrics | grep rest_client_request_duration_seconds

# 检查 API 请求错误
curl -k https://127.0.0.1:10257/metrics | grep rest_client_requests_total | grep -v '="200"'
```

#### 2.3.7 第七步：检查日志

```bash
# 实时查看日志
journalctl -u kube-controller-manager -f --no-pager

# 查看最近的错误日志
journalctl -u kube-controller-manager -p err --since "1 hour ago"

# 静态 Pod 方式查看日志
crictl logs $(crictl ps -q --name kube-controller-manager) 2>&1 | tail -500

# 查找特定控制器错误
journalctl -u kube-controller-manager | grep -i "deployment" | tail -50
journalctl -u kube-controller-manager | grep -i "replicaset" | tail -50
journalctl -u kube-controller-manager | grep -i "endpoint" | tail -50
journalctl -u kube-controller-manager | grep -i "node" | tail -50

# 查找同步错误
journalctl -u kube-controller-manager | grep -iE "(sync.*error|failed to sync)" | tail -50
```

### 2.4 排查注意事项

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **kubeconfig 安全** | CM 的 kubeconfig 有高权限 | 不要泄露 |
| **证书安全** | 证书用于 API Server 认证 | 妥善保管 |
| **云凭证** | CM 可能有云平台凭证 | 注意保密 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **高可用场景** | 多 CM 实例需要 Leader 选举 | 确保只有一个 Leader |
| **控制器耦合** | 某些控制器相互依赖 | 全面检查 |
| **资源累积** | CM 故障可能导致资源累积 | 恢复后检查 |
| **日志级别** | 高日志级别会影响性能 | 调试完成后恢复 |

### 🚀 2.5 深度解析（专家专区）

#### 2.5.1 理解 Informer 与缓存一致性
Controller Manager 并不直接查询 etcd，而是通过 Informer 机制在本地维护一份资源缓存。
- **专家提示**：如果发现 `kubectl get` 显示 Pod 已删除，但控制器仍然认为它存在（例如 Deployment 没创建新 Pod），通常是 Informer 的缓存同步出现了延迟或丢失了事件。此时重启 Controller Manager 是最快的强制刷新手段。

#### 2.5.2 工作队列的退避（Backoff）机制
当控制器处理某个资源失败时，该资源会被重新放入队列，但会等待一段时间（Backoff）。
- **现象**：日志中出现大量的 `retrying` 信息。
- **专家提示**：通过监控 `workqueue_retries_total` 指标可以发现哪些资源处于“死循环”重试中。常见的重试原因包括权限不足（RBAC）或 API Server 响应超时。

#### 2.5.3 节点驱逐（Eviction）的保护逻辑
Node Controller 负责在节点 NotReady 时驱逐 Pod。
- **核心参数**：`--node-eviction-rate` (默认 0.1/s)。
- **专家提示**：在大型集群中，如果网络出现大面积抖动，Node Controller 会进入“二级限制”状态（Secondary Health State），自动降低驱逐速率以防止大规模业务震荡。这是 Kubernetes 自身的熔断机制。

---

## 3. 解决方案与风险控制

### 3.1 Controller Manager 进程未运行

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查启动失败原因
journalctl -u kube-controller-manager -b --no-pager | tail -100

# 步骤 2：检查配置文件语法
python3 -c "import yaml; yaml.safe_load(open('/etc/kubernetes/manifests/kube-controller-manager.yaml'))"

# 步骤 3：检查证书文件
ls -la /etc/kubernetes/pki/
ls -la /etc/kubernetes/controller-manager.conf

# 步骤 4：验证 kubeconfig
kubectl --kubeconfig=/etc/kubernetes/controller-manager.conf cluster-info

# 步骤 5：修复问题后重启
# systemd 方式
systemctl restart kube-controller-manager

# 静态 Pod 方式
mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/
sleep 5
mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/

# 步骤 6：验证恢复
kubectl get pods -n kube-system | grep controller-manager
curl -k https://127.0.0.1:10257/healthz
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启期间控制循环中断 | 在维护窗口操作 |
| **低** | 配置检查一般无风险 | - |
| **中** | 配置修改可能引入新问题 | 修改前备份 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. CM 不可用期间集群自愈能力丧失
2. 已运行的 Pod 不受直接影响
3. 高可用集群确保其他 CM 实例正常
4. 修改配置前备份原始文件
5. 恢复后检查各控制器是否正常工作
```

### 3.2 Deployment/ReplicaSet 控制器异常

#### 3.2.1 解决步骤

```bash
# 步骤 1：确认问题
kubectl get deployments -A -o wide
kubectl get rs -A -o wide

# 步骤 2：检查具体 Deployment 状态
kubectl describe deployment <name> -n <namespace>

# 步骤 3：查看 CM 日志中的相关错误
journalctl -u kube-controller-manager | grep -i "deployment\|replicaset" | tail -50

# 步骤 4：检查 API 请求是否被限流
curl -k https://127.0.0.1:10257/metrics | grep rest_client_requests_total

# 步骤 5：如果是限流问题，调整 CM 参数
# 修改 CM 启动参数：
# --kube-api-qps=50          # 默认 20
# --kube-api-burst=100       # 默认 30

# 步骤 6：手动触发同步（通过添加标签强制更新）
kubectl annotate deployment <name> -n <namespace> force-sync=$(date +%s)

# 步骤 7：验证恢复
kubectl rollout status deployment <name> -n <namespace>
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | 增加 API QPS 可能增加 API Server 负载 | 监控 API Server |
| **低** | 手动触发同步一般无风险 | 仅用于诊断 |
| **中** | 参数修改需要重启 | 在维护窗口操作 |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. Deployment 控制器异常会影响应用滚动更新
2. 增加 API QPS 需要评估 API Server 承载能力
3. 手动 annotate 不会影响实际应用
4. 检查是否有大量 Deployment 同时更新导致负载过高
5. 考虑分批滚动更新减少峰值负载
```

### 3.3 Endpoints 控制器异常

#### 3.3.1 解决步骤

```bash
# 步骤 1：确认问题
kubectl get endpoints -A
# 检查是否有 Service 的 Endpoints 为空

# 步骤 2：检查 Service 和 Pod 标签匹配
kubectl get svc <name> -o yaml | grep -A5 selector
kubectl get pods -l <selector-key>=<selector-value>

# 步骤 3：查看 CM 日志中的 Endpoints 错误
journalctl -u kube-controller-manager | grep -i "endpoint" | tail -50

# 步骤 4：检查 Pod 是否 Ready
kubectl get pods -o wide
kubectl describe pod <name> | grep -A5 Conditions

# 步骤 5：手动检查 Endpoints 对象
kubectl get endpoints <service-name> -o yaml

# 步骤 6：强制重建 Endpoints
# 方法 1：重启关联的 Pod
kubectl rollout restart deployment <name>

# 方法 2：删除并重建 Service
kubectl delete svc <name>
kubectl apply -f <service-yaml>

# 步骤 7：验证恢复
kubectl get endpoints <service-name>
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启 Pod 会导致短暂服务中断 | 在维护窗口操作 |
| **高** | 删除 Service 会导致服务不可用 | 确保有 YAML 可恢复 |
| **低** | 查看日志和状态无风险 | - |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. Endpoints 为空会导致服务无法访问
2. 删除 Service 前确保有配置备份
3. 检查 Service selector 是否正确匹配 Pod
4. 使用 EndpointSlice（v1.21+）可能有不同表现
5. 考虑使用 headless Service 排除 Endpoints Controller 问题
```

### 3.4 Node Controller 异常

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认问题
kubectl get nodes
# 检查是否有节点长期处于 NotReady 状态

# 步骤 2：检查 CM 日志中的 Node Controller 错误
journalctl -u kube-controller-manager | grep -i "node" | tail -50

# 步骤 3：检查 Node Controller 参数
# 查看当前配置
cat /etc/kubernetes/manifests/kube-controller-manager.yaml | grep -E "node-monitor|pod-eviction"

# 步骤 4：检查节点上的 kubelet 状态
ssh <node-ip> "systemctl status kubelet"
ssh <node-ip> "journalctl -u kubelet --since '10 minutes ago' | tail -50"

# 步骤 5：调整 Node Controller 参数（如果容忍度过低）
# 修改 CM 启动参数：
# --node-monitor-period=5s           # 默认 5s
# --node-monitor-grace-period=40s    # 默认 40s
# --pod-eviction-timeout=5m0s        # 默认 5m0s

# 步骤 6：手动更新节点状态（测试用）
kubectl cordon <node-name>
kubectl uncordon <node-name>

# 步骤 7：验证恢复
kubectl get nodes
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 调整监控周期可能延迟故障检测 | 根据网络质量调整 |
| **中** | cordon/uncordon 会影响调度 | 仅用于诊断 |
| **低** | 查看日志无风险 | - |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. Node Controller 异常会延迟节点故障检测
2. Pod 驱逐超时过短可能导致不必要的驱逐
3. 网络不稳定时考虑增加 grace-period
4. 检查节点 kubelet 是否正常是首要步骤
5. 大规模节点 NotReady 可能是网络问题而非 CM 问题
```

### 3.5 Namespace 无法删除

#### 3.5.1 解决步骤

```bash
# 步骤 1：确认问题
kubectl get ns
# 检查是否有 Terminating 状态的 namespace

# 步骤 2：检查 namespace 中的资源
kubectl get all -n <namespace>
kubectl api-resources --verbs=list --namespaced -o name | xargs -n 1 kubectl get -n <namespace>

# 步骤 3：检查 finalizers
kubectl get ns <namespace> -o yaml | grep -A5 finalizers

# 步骤 4：查看 CM 日志中的 Namespace Controller 错误
journalctl -u kube-controller-manager | grep -i "namespace" | tail -50

# 步骤 5：强制删除（移除 finalizers）
# ⚠️ 警告：这可能导致资源泄露
kubectl get ns <namespace> -o json | jq '.spec.finalizers = []' | kubectl replace --raw "/api/v1/namespaces/<namespace>/finalize" -f -

# 步骤 6：验证删除
kubectl get ns <namespace>

# 步骤 7：清理可能遗留的资源
kubectl get all -A | grep <namespace>
```

#### 3.5.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 强制删除可能导致资源泄露 | 先尝试正常删除资源 |
| **中** | 遗留的 CRD 资源可能影响后续使用 | 检查并清理 CRD 资源 |
| **低** | 查看 finalizers 无风险 | - |

#### 3.5.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 移除 finalizers 是最后手段，可能导致资源泄露
2. 先检查是否有 webhook 阻止删除
3. 检查是否有 CRD 资源未被删除
4. 云资源（如 LoadBalancer）可能需要手动清理
5. 记录强制删除的 namespace 用于后续检查
```

### 3.6 PersistentVolume Controller 异常

#### 3.6.1 解决步骤

```bash
# 步骤 1：确认问题
kubectl get pv
kubectl get pvc -A
# 检查是否有 PVC 长期处于 Pending 状态

# 步骤 2：检查 PVC 详情
kubectl describe pvc <name> -n <namespace>
# 查看 Events 中的错误信息

# 步骤 3：检查 StorageClass
kubectl get sc
kubectl describe sc <name>

# 步骤 4：查看 CM 日志中的 PV Controller 错误
journalctl -u kube-controller-manager | grep -i "persistentvolume" | tail -50

# 步骤 5：检查 CSI 驱动状态（如使用 CSI）
kubectl get pods -n kube-system | grep csi

# 步骤 6：手动创建 PV（如果自动配置失败）
cat << EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: <storage-class>
  # ... 具体存储配置
EOF

# 步骤 7：验证绑定
kubectl get pvc <name> -n <namespace>
```

#### 3.6.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 手动创建 PV 可能与自动配置冲突 | 确认 StorageClass 配置 |
| **低** | 查看状态和日志无风险 | - |
| **中** | CSI 驱动问题可能需要深入排查 | 查看 CSI 驱动文档 |

#### 3.6.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. PV/PVC 绑定失败会导致 Pod 无法启动
2. 检查云厂商存储配额和权限
3. StorageClass 配置错误是常见原因
4. CSI 驱动需要正确的 RBAC 权限
5. 生产环境建议使用自动存储配置
```

### 3.7 Controller Manager 性能优化

#### 3.7.1 解决步骤

```bash
# 步骤 1：确认性能问题
curl -k https://127.0.0.1:10257/metrics | grep workqueue_depth
curl -k https://127.0.0.1:10257/metrics | grep workqueue_work_duration_seconds

# 步骤 2：检查资源使用
top -p $(pgrep kube-controller-manager) -b -n 1

# 步骤 3：优化 CM 参数
# 修改启动参数：
# --kube-api-qps=50                 # 增加 API 请求速率
# --kube-api-burst=100              # 增加 burst 限制
# --concurrent-deployment-syncs=10  # 增加并发同步数
# --concurrent-replicaset-syncs=10
# --concurrent-endpoint-syncs=10
# --concurrent-service-syncs=5
# --concurrent-gc-syncs=30

# 步骤 4：调整资源限制（静态 Pod 方式）
# 在 manifest 中增加 resources 配置
# resources:
#   requests:
#     cpu: "200m"
#     memory: "512Mi"
#   limits:
#     cpu: "2000m"
#     memory: "2Gi"

# 步骤 5：重启 CM 应用配置
mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/
sleep 5
mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/

# 步骤 6：验证性能改善
curl -k https://127.0.0.1:10257/metrics | grep workqueue_depth
```

#### 3.7.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 增加并发可能增加 API Server 负载 | 监控 API Server |
| **中** | 资源限制变更需要重启 | 在维护窗口操作 |
| **低** | 查看指标无风险 | - |

#### 3.7.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 增加并发数需要评估 API Server 承载能力
2. 大规模集群（1000+ 节点）需要仔细调优
3. 监控 CM 内存使用，避免 OOM
4. 调整参数后观察至少 1 小时
5. 保留原始配置用于回滚
```

---

## 附录

### A. Controller Manager 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `workqueue_depth` | 工作队列深度 | > 100 |
| `workqueue_work_duration_seconds` | 处理时长 | P99 > 1s |
| `workqueue_retries_total` | 重试次数 | 异常增长 |
| `rest_client_requests_total` | API 请求数 | 错误率 > 1% |
| `process_resident_memory_bytes` | 内存使用 | > 2GB |

### B. 常见启动参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--controllers` | * | 启用的控制器列表 |
| `--kube-api-qps` | 20 | API 请求速率限制 |
| `--kube-api-burst` | 30 | API 请求 burst 限制 |
| `--concurrent-deployment-syncs` | 5 | Deployment 并发同步数 |
| `--node-monitor-period` | 5s | 节点监控周期 |
| `--node-monitor-grace-period` | 40s | 节点不健康容忍时间 |
| `--pod-eviction-timeout` | 5m | Pod 驱逐超时时间 |

### C. 控制器列表参考

```bash
# 查看所有可用控制器
kube-controller-manager --controllers=* --help 2>&1 | grep -A100 "controllers"

# 常见控制器
# - deployment
# - replicaset
# - daemonset
# - statefulset
# - job
# - cronjob
# - endpoint
# - endpointslice
# - namespace
# - node
# - persistentvolume-binder
# - persistentvolume-expander
# - serviceaccount
# - serviceaccount-token
# - garbagecollector
# - resourcequota
```

---

## 📚 D. 生产环境实战案例精选

### 案例 1：Endpoint Controller 并发数过低导致服务发现延迟

#### 🎯 故障场景
某大型互联网公司，集群规模 500 节点、5000 Service、50000 Pod，在业务高峰期进行大规模发布，导致新 Pod 长时间无法加入 Endpoints，流量无法到达，持续 10 分钟影响用户访问。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   # 发现新创建的 Pod Running 但未加入 Endpoints
   kubectl get pods -n production -l app=myapp --field-selector=status.phase=Running | wc -l
   # 200  # 新 Pod 已 Running
   
   kubectl get endpoints myapp -n production -o json | jq '.subsets[].addresses | length'
   # 50  # 但 Endpoints 只有 50 个旧 Pod ❌
   ```

2. **延迟分析**：
   ```bash
   # 查看 Pod 创建到加入 Endpoints 的时间差
   kubectl get pods -n production -l app=myapp -o json | jq -r '.items[] | 
     "\(.metadata.creationTimestamp) \(.status.podIP)"'
   # 2026-01-10T08:30:00Z 10.244.1.100  # Pod 创建时间
   
   kubectl get endpoints myapp -n production -o json | jq -r '.metadata.managedFields[] | 
     select(.manager=="kube-controller-manager") | .time'
   # 2026-01-10T08:40:00Z  # Endpoints 更新时间，延迟 10 分钟！❌
   ```

3. **Controller Manager 指标**：
   ```bash
   # 查看 Endpoint Controller 工作队列深度
   curl -k https://127.0.0.1:10257/metrics | grep 'workqueue_depth.*endpoint'
   # workqueue_depth{name="endpoint"} 3500  # ❌ 队列严重堆积！
   
   # 查看处理速率
   curl -k https://127.0.0.1:10257/metrics | grep 'workqueue_adds_total.*endpoint'
   # workqueue_adds_total{name="endpoint"} 125000  # 大量事件入队
   
   # 查看同步延迟
   curl -k https://127.0.0.1:10257/metrics | grep 'controller_sync_duration.*endpoint'
   # controller_sync_duration_seconds{controller="endpoint",...} 25.5  # P99 > 25s ❌
   ```

4. **配置检查**：
   ```bash
   # 查看 Controller Manager 启动参数
   kubectl get pod -n kube-system kube-controller-manager-master1 -o yaml | grep concurrent
   # --concurrent-endpoint-syncs=5  # ❌ 默认值，并发数过低！
   ```

5. **根因分析**：
   - 大规模发布导致大量 Pod 创建/销毁事件
   - Endpoint Controller 并发同步数仅 5，处理速度 < 事件产生速度
   - 工作队列堆积 3500+ 事件，每个 Service 的 Endpoints 更新延迟 10+ 分钟
   - 影响：新 Pod 无法接收流量，用户访问 5xx 错误

#### ⚡ 应急措施
1. **立即提高并发数**：
   ```bash
   # 修改 Controller Manager 静态 Pod 配置
   ssh master1 "vim /etc/kubernetes/manifests/kube-controller-manager.yaml"
   
   # 添加/修改参数
   spec:
     containers:
     - command:
       - kube-controller-manager
       - --concurrent-endpoint-syncs=50  # ✅ 提高至 50（10 倍）
       - --concurrent-service-syncs=10   # ✅ 同时提高 Service Controller
       - --concurrent-replicaset-syncs=20  # ✅ 提高 RS Controller
   
   # Controller Manager 会自动重启（静态 Pod）
   # 等待重启完成
   kubectl wait --for=condition=Ready pod -n kube-system -l component=kube-controller-manager --timeout=60s
   ```

2. **验证队列消化**：
   ```bash
   # 持续监控队列深度
   watch -n 5 'curl -sk https://127.0.0.1:10257/metrics | grep "workqueue_depth.*endpoint"'
   # workqueue_depth{name="endpoint"} 3500  # 初始
   # workqueue_depth{name="endpoint"} 2100  # 1 分钟后
   # workqueue_depth{name="endpoint"} 850   # 2 分钟后
   # workqueue_depth{name="endpoint"} 50    # 5 分钟后 ✅ 基本清空
   ```

3. **验证 Endpoints 恢复**：
   ```bash
   kubectl get endpoints myapp -n production -o json | jq '.subsets[].addresses | length'
   # 200  ✅ 全部 200 个 Pod 已加入 Endpoints
   
   # 验证流量恢复
   curl -s http://myapp.production.svc.cluster.local | grep "200 OK"
   # ✅ 服务正常
   ```

#### 🛡️ 长期优化
1. **动态调整并发数（根据集群规模）**：
   ```yaml
   # 推荐配置（500 节点集群）
   spec:
     containers:
     - command:
       - kube-controller-manager
       - --concurrent-endpoint-syncs=50        # 5000 Service，每个控制器处理 100 个
       - --concurrent-service-syncs=20
       - --concurrent-deployment-syncs=20
       - --concurrent-replicaset-syncs=20
       - --concurrent-statefulset-syncs=10
       - --concurrent-daemonset-syncs=10
       - --concurrent-job-syncs=10
       - --concurrent-namespace-syncs=10
       - --concurrent-gc-syncs=20
   
   # 并发数设置原则：
   # 小集群（< 100 节点）：使用默认值（5）
   # 中型集群（100-500 节点）：20-50
   # 大型集群（> 500 节点）：50-100
   ```

2. **提高 API 客户端限流**：
   ```yaml
   - --kube-api-qps=100   # 从默认 20 提高至 100
   - --kube-api-burst=150  # 从默认 30 提高至 150
   ```

3. **监控工作队列健康**：
   ```yaml
   # Prometheus 告警规则
   groups:
   - name: controller-manager-workqueue
     rules:
     - alert: ControllerWorkqueueDepthHigh
       expr: workqueue_depth > 1000
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "控制器工作队列堆积"
         description: "控制器 {{ $labels.name }} 工作队列深度 {{ $value }}，超过 1000，可能处理不及时"
     
     - alert: ControllerSyncSlow
       expr: histogram_quantile(0.99, rate(workqueue_queue_duration_seconds_bucket[5m])) > 60
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "控制器同步延迟高"
         description: "控制器 {{ $labels.name }} P99 同步延迟 {{ $value }}s，超过 60s"
     
     - alert: ControllerHighRetries
       expr: rate(workqueue_retries_total[5m]) > 10
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "控制器重试率高"
         description: "控制器 {{ $labels.name }} 重试率 {{ $value }}/s，可能存在问题"
   ```

4. **容量规划与压测**：
   ```bash
   # 使用 Kubernetes Bench 测试控制器性能
   git clone https://github.com/kubernetes/perf-tests
   cd perf-tests/clusterloader2
   
   # 模拟大规模 Pod 创建
   go run cmd/clusterloader.go \
     --testconfig=testing/load/config.yaml \
     --nodes=500 \
     --pods-per-node=100 \
     --enable-prometheus-server
   
   # 观察 Controller Manager 指标
   ```

5. **EndpointSlice 迁移（推荐）**：
   ```yaml
   # Kubernetes v1.21+ 推荐使用 EndpointSlice 替代 Endpoints
   # EndpointSlice 将大 Endpoints 拆分为多个小对象，提高扩展性
   
   apiVersion: v1
   kind: Service
   metadata:
     name: myapp
     annotations:
       endpointslice.kubernetes.io/enabled: "true"  # 启用 EndpointSlice
   spec:
     selector:
       app: myapp
     ports:
     - port: 80
   
   # 查看 EndpointSlice
   kubectl get endpointslices -n production
   # myapp-abc123   IPv4   10.244.1.100,10.244.1.101,...   3m
   # myapp-def456   IPv4   10.244.2.100,10.244.2.101,...   3m
   # ✅ 大 Service 自动拆分为多个 EndpointSlice
   ```

#### 💡 经验总结
- **默认配置不适用大集群**：并发数需根据集群规模调整
- **监控缺失**：未监控工作队列深度和同步延迟
- **容量规划不足**：未进行大规模发布压测
- **改进方向**：动态调参、监控告警、容量规划、EndpointSlice 迁移

---

### 案例 2：Node Controller 驱逐超时配置不当导致故障恢复慢

#### 🎯 故障场景
某金融公司生产集群，某物理机突然掉电，节点 NotReady，但节点上的 Pod 在 5 分钟后才开始迁移，导致业务中断 5+ 分钟，超出 SLA 要求（2 分钟）。

#### 🔍 排查过程
1. **时间线回溯**：
   ```bash
   # 节点掉电时间
   kubectl get events --sort-by='.lastTimestamp' | grep node-worker-05
   # 08:00:00  NodeNotReady  node-worker-05  Node node-worker-05 status is now: NotReady
   
   # Pod 开始驱逐时间
   kubectl get events --sort-by='.lastTimestamp' | grep "Evicting pod"
   # 08:05:30  EvictingPod  node-worker-05  Evicting pod production/myapp-abc123  # ❌ 5 分 30 秒后！
   
   # Pod 在新节点启动时间
   kubectl get events | grep myapp-abc123 | grep Scheduled
   # 08:06:00  Scheduled  myapp-abc123  Successfully assigned to node-worker-10  # 6 分钟后
   ```

2. **配置检查**：
   ```bash
   # 查看 Node Controller 参数
   kubectl get pod -n kube-system kube-controller-manager-master1 -o yaml | \
     grep -E "(node-monitor|pod-eviction)"
   # --node-monitor-period=5s              # ✅ 检查间隔 5 秒
   # --node-monitor-grace-period=40s       # ✅ 宽限期 40 秒
   # --pod-eviction-timeout=5m0s           # ❌ 驱逐超时 5 分钟！
   ```

3. **根因分析**：
   - `--pod-eviction-timeout=5m`：节点 NotReady 后 5 分钟才开始驱逐 Pod
   - **设计初衷**：避免网络短暂抖动导致误驱逐
   - **实际问题**：物理机掉电等永久性故障也要等待 5 分钟，恢复太慢
   - **时间轴**：
     ```
     08:00:00 节点掉电
     08:00:40 Node Controller 检测到 NotReady（40s 宽限期）
     08:05:40 开始驱逐 Pod（5m 驱逐超时）
     08:06:00 Pod 在新节点启动
     总计：6 分钟业务中断 ❌
     ```

#### ⚡ 应急措施
1. **手动触发 Pod 重建**：
   ```bash
   # 立即删除故障节点上的 Pod（不等待自动驱逐）
   kubectl get pods -A --field-selector spec.nodeName=node-worker-05 -o json | \
     jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"' | \
     xargs -I {} kubectl delete pod {} --grace-period=0 --force
   
   # 验证 Pod 在新节点启动
   kubectl get pods -n production -l app=myapp -o wide
   # myapp-abc123   1/1   Running   0   30s   10.244.10.50   node-worker-10  ✅
   ```

2. **优化驱逐超时配置**：
   ```bash
   # 修改 Controller Manager 配置
   vim /etc/kubernetes/manifests/kube-controller-manager.yaml
   
   spec:
     containers:
     - command:
       - kube-controller-manager
       - --node-monitor-period=5s            # 保持 5 秒
       - --node-monitor-grace-period=40s     # 保持 40 秒
       - --pod-eviction-timeout=1m0s         # ✅ 降低至 1 分钟
   
   # 等待重启
   kubectl wait --for=condition=Ready pod -n kube-system -l component=kube-controller-manager --timeout=60s
   ```

3. **验证新配置**：
   ```bash
   # 模拟节点故障（在测试环境）
   kubectl drain test-node --ignore-daemonsets --delete-emptydir-data
   
   # 观察驱逐时间
   kubectl get events --watch | grep Evicting
   # ✅ 约 1 分 40 秒后开始驱逐（40s 宽限 + 1m 驱逐超时）
   ```

#### 🛡️ 长期优化
1. **针对不同场景的差异化策略**：
   ```yaml
   # 方案 1：使用 PodDisruptionBudget 保证高可用
   apiVersion: policy/v1
   kind: PodDisruptionBudget
   metadata:
     name: myapp-pdb
     namespace: production
   spec:
     minAvailable: 80%  # 至少 80% 副本可用
     selector:
       matchLabels:
         app: myapp
   
   # 即使驱逐慢，也能保证多数副本在其他节点正常服务
   ```

2. **使用 Taints 和 Tolerations 加速驱逐**：
   ```yaml
   # Pod 配置容忍度，自定义驱逐时间
   apiVersion: v1
   kind: Pod
   metadata:
     name: myapp
   spec:
     tolerations:
     - key: node.kubernetes.io/not-ready
       operator: Exists
       effect: NoExecute
       tolerationSeconds: 30  # ✅ 30 秒后自动驱逐（覆盖全局配置）
     - key: node.kubernetes.io/unreachable
       operator: Exists
       effect: NoExecute
       tolerationSeconds: 30
   ```

3. **配置推荐（按业务类型）**：
   ```yaml
   # 关键业务（低容忍）：
   tolerationSeconds: 10-30  # 10-30 秒快速故障转移
   
   # 普通业务（平衡）：
   tolerationSeconds: 60-120  # 1-2 分钟，平衡误驱逐和恢复速度
   
   # 批处理任务（高容忍）：
   tolerationSeconds: 300-600  # 5-10 分钟，避免频繁迁移
   ```

4. **节点健康检查增强**：
   ```yaml
   # 使用 Node Problem Detector 主动上报节点问题
   apiVersion: apps/v1
   kind: DaemonSet
   metadata:
     name: node-problem-detector
     namespace: kube-system
   spec:
     template:
       spec:
         containers:
         - name: node-problem-detector
           image: k8s.gcr.io/node-problem-detector:v0.8.10
           args:
           - --logtostderr
           - --system-log-monitors=/config/kernel-monitor.json  # 监控内核日志
           - --custom-plugin-monitors=/config/custom-plugin.json  # 自定义检查
           volumeMounts:
           - name: log
             mountPath: /var/log
   
   # NPD 检测到硬件故障时立即给节点打 Taint，加速驱逐
   ```

5. **监控与告警**：
   ```yaml
   # 监控节点 NotReady 时长
   - alert: NodeNotReadyTooLong
     expr: kube_node_status_condition{condition="Ready",status="false"} == 1
     for: 2m
     labels:
       severity: critical
     annotations:
       summary: "节点长时间 NotReady"
       description: "节点 {{ $labels.node }} NotReady 超过 2 分钟，可能需要手动介入"
   
   # 监控 Pod 驱逐延迟
   - alert: PodEvictionSlow
     expr: (time() - kube_node_status_condition_last_transition_time{condition="Ready",status="false"}) > 180
       and
       sum by(node) (kube_pod_info{node=~".*"}) > 0
     labels:
       severity: warning
     annotations:
       summary: "Pod 驱逐延迟"
       description: "节点 {{ $labels.node }} NotReady 超过 3 分钟，但仍有 Pod 未驱逐"
   ```

#### 💡 经验总结
- **默认配置过于保守**：5 分钟驱逐超时不适合对 RTO 要求高的业务
- **一刀切策略**：未区分永久性故障（掉电）和临时性故障（网络抖动）
- **缺乏主动检测**：依赖被动心跳检测，无法快速识别硬件故障
- **改进方向**：差异化配置、Pod 级别容忍度、NPD 主动检测、PDB 保障高可用
```
