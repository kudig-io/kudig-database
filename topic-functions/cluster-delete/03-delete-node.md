# 节点删除流程 — kubectl delete node 源码分析

## 概述

Kubernetes 节点删除分为两个层面：**API 层删除**（`kubectl delete node`，从 etcd 移除 Node 对象）和**节点级重置**（`kubeadm reset`，清理本地数据）。两者通常配合使用。本文档分析完整的节点删除流程。

---

## 删除流程全景

```
┌──────────────────────────────────────────────────────────────────┐
│                    节点删除完整流程                                 │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  Step 1: kubectl drain <node>                                      │
│    ├─ 驱逐所有非 DaemonSet Pod                                     │
│    ├─ 等待 Pod 优雅终止（尊重 terminationGracePeriodSeconds）      │
│    └─ 标记节点为 SchedulingDisabled                                │
│                                                                    │
│  Step 2: kubectl delete node <node>                                │
│    ├─ 从 etcd 删除 Node 对象                                       │
│    ├─ 触发 Node 相关 Controller 的清理逻辑                          │
│    └─ 关联的 Pod 被标记为 NodeLost                                  │
│                                                                    │
│  Step 3: kubeadm reset (在目标节点上执行)                           │
│    ├─ 从 etcd 集群移除本节点（如果是控制面）                        │
│    ├─ 停止 kubelet                                                 │
│    ├─ 删除容器、配置、证书                                          │
│    └─ 清理数据目录                                                  │
│                                                                    │
│  Step 4: 手动清理                                                   │
│    ├─ iptables / ipvs 规则                                         │
│    ├─ CNI 配置                                                     │
│    └─ 残留数据目录                                                  │
│                                                                    │
└──────────────────────────────────────────────────────────────────┘
```

---

## 1. kubectl drain — 节点驱逐

### 1.1 驱逐逻辑

```bash
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
```

**驱逐行为**:

| 资源类型 | 行为 |
|----------|------|
| ReplicaSet/Deployment Pod | 驱逐后在其他节点重建 |
| StatefulSet Pod | 驱逐后按序号重建（需要 PV 支持） |
| DaemonSet Pod | 默认阻止驱逐，需 `--ignore-daemonsets` |
| 使用 emptyDir 的 Pod | 默认阻止驱逐，需 `--delete-emptydir-data` |
| 使用 local PV 的 Pod | 无法驱逐（数据绑定节点） |

### 1.2 优雅终止

```
┌───────────────────────────────────────────────────┐
│  Pod 优雅终止流程                                   │
├───────────────────────────────────────────────────┤
│  1. kubelet 发送 SIGTERM 到 Pod 中 PID 1           │
│  2. 等待 terminationGracePeriodSeconds（默认 30s）  │
│  3. 超时后发送 SIGKILL 强制终止                      │
│  4. Pod 被从 etcd 中删除                            │
│  5. Controller 在其他节点重建 Pod                    │
└───────────────────────────────────────────────────┘
```

### 1.3 节点标记

drain 命令首先将节点标记为 `SchedulingDisabled`:

```go
patch := fmt.Sprintf(`{"spec":{"unschedulable":true}}`)
node.Spec.Unschedulable = true
```

等价于:
```bash
kubectl cordon <node>
```

---

## 2. kubectl delete node — Node 对象删除

### 2.1 API 层面

```bash
kubectl delete node <node-name>
```

发送 `DELETE /api/v1/nodes/<node-name>` 请求到 API Server。

### 2.2 删除触发的 Controller 行为

Node 对象删除后，以下 Controller 会响应：

| Controller | 行为 |
|-----------|------|
| Node Lifecycle Controller | 清理该 Node 上的所有 Pod（设置 `pod.DeletionTimestamp`） |
| ReplicaSet Controller | 在其他节点创建新 Pod |
| DaemonSet Controller | 不做额外操作（Node 已不存在） |
| GC Controller | 清理孤儿 Pod |
| PV Controller | 释放绑定到该节点的 local PV |

### 2.3 Node 上的 Pod 处理

```
┌────────────────────────────────────────────────────────┐
│  Node 删除后 Pod 的状态变化                               │
├────────────────────────────────────────────────────────┤
│                                                          │
│  Node 存在时:                                             │
│    Pod.status.phase = Running                            │
│    Pod.status.conditions[type=Ready] = Unknown           │
│                                                          │
│  Node 删除后:                                             │
│    Pod.status.reason = NodeLost                          │
│    Pod.status.message = "Node <name> which was running   │
│                          the pod is unresponsive"        │
│    Pod 被 Node Lifecycle Controller 强制删除              │
│                                                          │
│  --force 删除:                                            │
│    立即从 etcd 删除，不等待优雅终止                        │
│                                                          │
└────────────────────────────────────────────────────────┘
```

---

## 3. Node Lifecycle Controller — 节点生命周期管理

**源码路径**: `pkg/controller/nodelifecycle/`

### 3.1 节点心跳监测

```
┌──────────────────────────────────────────────────────────┐
│  Node Status 监测                                         │
├──────────────────────────────────────────────────────────┤
│  kubelet 每 10s 上报 NodeStatus 到 API Server             │
│  (node-status-update-frequency)                           │
│                                                            │
│  Node Lifecycle Controller 监控:                           │
│  - node-monitor-grace-period (默认 40s)                    │
│    → 超时后标记 NodeReady=Unknown                          │
│  - pod-eviction-timeout (默认 5m)                          │
│    → 超时后开始驱逐 Pod                                    │
│  - node-monitor-period (默认 5s)                           │
│    → Controller 检查间隔                                   │
└──────────────────────────────────────────────────────────┘
```

### 3.2 节点条件与 Pod 驱逐策略

```go
// 源码: pkg/controller/nodelifecycle/node_lifecycle_controller.go
const (
    NodeReady           = "Ready"
    NodeMemoryPressure  = "MemoryPressure"
    NodeDiskPressure    = "DiskPressure"
    NodePIDPressure     = "PIDPressure"
    NodeNetworkUnavailable = "NetworkUnavailable"
)
```

**Pod 驱逐策略** (`TaintBasedEviction`):

| Node Condition | Taint | Pod Toleration | 驱逐行为 |
|---------------|-------|---------------|---------|
| Ready=Unknown | `node.kubernetes.io/unreachable` | `tolerationSeconds` 后驱逐 | 默认 300s |
| Ready=False | `node.kubernetes.io/not-ready` | `tolerationSeconds` 后驱逐 | 默认 300s |
| MemoryPressure | `node.kubernetes.io/memory-pressure` | 无 toleration 的 Pod 被驱逐 | — |
| DiskPressure | `node.kubernetes.io/disk-pressure` | 无 toleration 的 Pod 被驱逐 | — |

---

## 4. kubeadm reset — 节点重置

### 4.1 控制面节点 vs 工作节点

```
┌──────────────────────────────────────────────────────┐
│  控制面节点 reset:                                      │
│  ├─ preflight (root 权限确认)                          │
│  ├─ remove-etcd-member (从 etcd 集群移除)              │
│  │   ├─ etcdctl member remove                         │
│  │   └─ 清理 /var/lib/etcd 数据目录                    │
│  └─ cleanup-node                                      │
│      ├─ 停止 kubelet                                  │
│      ├─ 删除所有容器                                   │
│      ├─ 清理 /etc/kubernetes/ (manifests, pki, conf)  │
│      └─ 清理 /var/lib/kubelet                          │
│                                                        │
│  工作节点 reset:                                        │
│  ├─ preflight (root 权限确认)                          │
│  ├─ remove-etcd-member (跳过，无 etcd)                 │
│  └─ cleanup-node                                      │
│      └─ (同上)                                         │
└──────────────────────────────────────────────────────┘
```

### 4.2 reset 后的 Node 对象状态

`kubeadm reset` **不会** 从 API Server 删除 Node 对象。Node 对象需要通过 `kubectl delete node` 或 Node Lifecycle Controller 自动清理。

---

## 5. 推荐删除顺序

### 5.1 工作节点

```bash
# 1. 在控制面节点上操作
kubectl drain <worker-node> --ignore-daemonsets --delete-emptydir-data
kubectl delete node <worker-node>

# 2. 在工作节点上操作
kubeadm reset -f

# 3. 手动清理
iptables -F && iptables -t nat -F && iptables -t mangle -F
ipvsadm -C
rm -rf /etc/cni/net.d
rm -rf $HOME/.kube
```

### 5.2 控制面节点

```bash
# 1. 在其他控制面节点上操作（或本节点，如果仍可达）
kubectl drain <cp-node> --ignore-daemonsets --delete-emptydir-data
kubectl delete node <cp-node>

# 2. 在目标控制面节点上操作
kubeadm reset -f

# 3. 确认 etcd 成员已移除（在存活的控制面节点上）
etcdctl member list

# 4. 如果 etcd 成员未自动移除
etcdctl member remove <member-id>

# 5. 手动清理
iptables -F && iptables -t nat -F && iptables -t mangle -F
rm -rf /etc/cni/net.d
rm -rf /var/lib/etcd
rm -rf $HOME/.kube
```

---

## 6. 常见问题

### 6.1 Node 处于 NotReady 但 Pod 仍在运行

Node 对象被删除后，kubelet 仍在运行。Pod 的容器进程继续运行，但不再被 Kubernetes 管理。

### 6.2 StatefulSet Pod 驱逐失败

StatefulSet Pod 如果使用 local PV，驱逐会导致数据丢失。需要先备份数据。

### 6.3 节点不可达时的处理

```bash
# 如果节点完全不可达，使用 --force 跳过 drain
kubectl delete node <unreachable-node>

# 或使用 --grace-period=0 强制删除
kubectl delete node <node> --force --grace-period=0
```

---

## 参考

- [Node Lifecycle Controller 源码](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/nodelifecycle/)
- [kubectl drain 文档](https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands#drain)
- [kubeadm reset 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/reset.go)
