---
title: Taints and Tolerations
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- daemonset
- operator
- webhook
- gpu
- nvidia
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Taints and Tolerations 是什么
- 如何 Taints and Tolerations
trigger_keywords:
- Taints
- and
- Tolerations
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---



# Taints and Tolerations

## 概述

节点亲和性（Node affinity）是 Pod 的属性，用于将 Pod 吸引到一组节点（作为偏好或硬性要求）。而污点（Taints）正好相反——它们允许节点排斥一组 Pod。容忍度（Tolerations）应用于 Pod，允许调度器调度具有匹配污点的 Pod。

污点和容忍度协同工作，确保 Pod 不会被调度到不合适的节点上。

## 核心概念/原理

### Taint（污点）

通过 `kubectl taint` 命令为节点添加污点：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
kubectl taint nodes node1 key1=value1:NoSchedule
```

污点由 key、value 和 effect 组成。除非 Pod 具有匹配的容忍度，否则无法调度到带有污点的节点上。

### Toleration（容忍度）

在 PodSpec 中指定容忍度。以下两种容忍度都匹配上面的污点：
```yaml
tolerations:
- key: "key1"
  operator: "Equal"
  value: "value1"
  effect: "NoSchedule"
```
```yaml
tolerations:
- key: "key1"
  operator: "Exists"
  effect: "NoSchedule"
```

容忍度匹配污点的规则：key 和 effect 相同，并且：
- operator 为 `Exists`（此时不应指定 value），或
- operator 为 `Equal` 且 values 相等。

### Effect 类型

- **NoSchedule**：没有匹配容忍度的 Pod 不会调度到该节点，但正在运行的 Pod 不会被驱逐。
- **PreferNoSchedule**：软版本的 NoSchedule，控制平面会尽量避免将没有匹配容忍度的 Pod 放置到该节点，但不保证。
- **NoExecute**：
  - 没有匹配容忍度的 Pod 会立即被驱逐。
  - 有匹配容忍度但没有指定 `tolerationSeconds` 的 Pod 会永远保留。
  - 有匹配容忍度并指定了 `tolerationSeconds` 的 Pod 会在指定时间后被驱逐。

### 多污点/多容忍度处理

[[Kubernetes|Kubernetes]] 处理多个污点和容忍度的方式类似于过滤器：从节点的所有污点开始，忽略 Pod 有匹配容忍度的污点；剩余的未忽略污点会对 Pod 产生相应效果。

## 关键机制或特性

- **内置污点**：节点控制器在特定条件下会自动为节点添加污点，包括：
  - `node.kubernetes.io/not-ready`
  - `node.kubernetes.io/unreachable`
  - `node.kubernetes.io/memory-pressure`
  - `node.kubernetes.io/disk-pressure`
  - `node.kubernetes.io/pid-pressure`
  - `node.kubernetes.io/network-unavailable`
  - `node.kubernetes.io/unschedulable`
  - `node.cloudprovider.kubernetes.io/uninitialized`
- **自动容忍度**：Kubernetes 自动为 Pod 添加 `node.kubernetes.io/not-ready` 和 `node.kubernetes.io/unreachable` 的容忍度，`tolerationSeconds=300`（5分钟）。
- **[[DaemonSet|DaemonSet]] 容忍度**：DaemonSet Pod 对上述两个污点的 NoExecute 容忍度没有 `tolerationSeconds`，确保它们永远不会因此被驱逐。
- **数值比较操作符**（v1.35+ alpha）：除了 `Equal` 和 `Exists`，还支持 `Gt` 和 `Lt` 用于匹配整数值的污点，适用于基于阈值的调度。
- **设备污点和容忍度**：在使用动态资源分配（DRA）管理特殊硬件时，管理员可以针对单个设备（而非整个节点）设置污点和容忍度。

## 使用场景

- **专用节点**：为特定用户组保留一组节点，通过污点和容忍度实现节点专用化。如需确保 Pod 只使用专用节点，还需结合节点亲和性。
- **特殊硬件节点**：带有 GPU 等专用硬件的节点可以设置污点，确保不需要该硬件的 Pod 不会占用这些节点资源。
- **基于污点的驱逐**：节点出现问题时自动驱逐 Pod。例如节点不可达、内存压力、磁盘压力等。

## 最佳实践/注意事项

- 如果手动指定 `.spec.nodeName`，会绕过调度器，即使节点有 `NoSchedule` 污点也会绑定。但如果节点还有 `NoExecute` 污点，[[kubelet|kubelet]] 仍会驱逐该 Pod（除非有匹配的容忍度）。
- 控制平面限制了向节点添加新污点的速率，以管理大量节点同时不可达时触发的驱逐数量。
- 从 v1.29 开始，基于污点的驱逐实现已从节点控制器移到了独立的 `taint-eviction-controller` 组件中。可以通过 `--controllers=-taint-eviction-controller` 禁用基于污点的驱逐。
- 当使用 `Gt`/`Lt` 操作符时，容忍度和污点的值都必须是有效的有符号 64 位整数。

## 生产 YAML 示例

### GPU 专用节点污点 + 容忍度

```yaml
# 1. 为 GPU 节点设置污点
# kubectl taint nodes gpu-node-01 nvidia.com/gpu=present:NoSchedule

# 2. GPU 工作负载 Pod — 容忍 GPU 污点 + nodeSelector
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ml-inference
  namespace: ml-platform
spec:
  replicas: 2
  selector:
    matchLabels:
      app: ml-inference
  template:
    metadata:
      labels:
        app: ml-inference
    spec:
      tolerations:
        - key: "nvidia.com/gpu"
          operator: "Equal"
          value: "present"
          effect: "NoSchedule"
      nodeSelector:
        accelerator: nvidia-a100           # 结合 nodeSelector 确保只调度到 GPU 节点
      containers:
        - name: inference
          image: registry.example.com/inference:v3.0
          resources:
            requests:
              cpu: "4"
              memory: 16Gi
              nvidia.com/gpu: "1"
            limits:
              nvidia.com/gpu: "1"
```

### 专用节点隔离（租户隔离）

```yaml
# 为租户 A 专用节点设置污点
# kubectl taint nodes tenant-a-node-01 dedicated=tenant-a:NoSchedule
# kubectl label nodes tenant-a-node-01 dedicated=tenant-a

apiVersion: apps/v1
kind: Deployment
metadata:
  name: tenant-a-app
  namespace: tenant-a
spec:
  replicas: 3
  selector:
    matchLabels:
      app: tenant-a-app
  template:
    metadata:
      labels:
        app: tenant-a-app
    spec:
      tolerations:
        - key: "dedicated"
          operator: "Equal"
          value: "tenant-a"
          effect: "NoSchedule"
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: dedicated
                    operator: In
                    values: ["tenant-a"]   # 确保只在专用节点运行
      containers:
        - name: app
          image: registry.example.com/tenant-a/app:v1.0
          resources:
            requests:
              cpu: "250m"
              memory: 256Mi
```

### NoExecute 容忍度（限时容忍）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tolerant-pod
  namespace: production
spec:
  tolerations:
    - key: "node.kubernetes.io/not-ready"
      operator: "Exists"
      effect: "NoExecute"
      tolerationSeconds: 120               # 节点 NotReady 后最多等待 2 分钟
    - key: "node.kubernetes.io/unreachable"
      operator: "Exists"
      effect: "NoExecute"
      tolerationSeconds: 120
  containers:
    - name: app
      image: registry.example.com/app:v1.0
      resources:
        requests:
          cpu: "100m"
          memory: 128Mi
```

## Effect 类型对比

| Effect | 对新 Pod | 对运行中 Pod | 典型场景 |
|--------|----------|-------------|----------|
| `NoSchedule` | 阻止调度 | 不驱逐 | GPU 节点、专用节点 |
| `PreferNoSchedule` | 尽量避免调度 | 不驱逐 | 软性偏好，如旧节点即将退役 |
| `NoExecute` | 阻止调度 | 驱逐（除非有容忍度） | 节点问题、维护模式 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod Pending，节点有空闲资源 | 节点有 NoSchedule 污点但 Pod 缺少容忍度 | `kubectl describe node <node>` 检查 Taints；`kubectl get pod -o yaml` 检查 tolerations |
| Pod 被意外驱逐 | 节点添加了 NoExecute 污点 | `kubectl get events --field-selector reason=TaintManagerEviction` |
| DaemonSet Pod 被驱逐 | DaemonSet 缺少必要的容忍度 | 检查 DaemonSet spec 中的 tolerations 列表 |
| 使用 nodeName 后 Pod 仍被驱逐 | 节点有 NoExecute 污点 | nodeName 绕过调度器但不绕过 kubelet 驱逐；添加匹配容忍度 |
| 大量节点同时 NotReady 导致 Pod 雪崩 | 控制平面速率限制不足 | 检查 `--node-eviction-rate` 和 `--secondary-node-eviction-rate` 参数 |

## 生产检查清单

- [ ] GPU / 特殊硬件节点设置 `NoSchedule` 污点
- [ ] 专用节点同时使用污点 + nodeAffinity（双重保证）
- [ ] DaemonSet Pod 配置必要的容忍度（monitoring、logging、CNI）
- [ ] 为关键 Pod 调整 `tolerationSeconds`（默认 300s 可能过长或过短）
- [ ] 了解内置污点列表（not-ready、unreachable、memory-pressure 等）
- [ ] 多租户集群使用 admission webhook 自动注入租户容忍度
- [ ] 监控 `taint-eviction-controller` 的驱逐速率

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
# 为节点添加污点
kubectl taint nodes <node-name> key=value:NoSchedule

# 移除节点污点
kubectl taint nodes <node-name> key=value:NoSchedule-

# 查看节点污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints[*].key'

# 查看节点详细污点信息
kubectl describe node <node-name> | grep -A 5 Taints

# 查看 Pod 的容忍度
kubectl get pod <pod-name> -o jsonpath='{.spec.tolerations}' | jq .

# 查看因污点驱逐的事件
kubectl get events --field-selector reason=TaintManagerEviction --all-namespaces

# 将节点标记为不可调度（添加 NoSchedule 污点）
kubectl cordon <node-name>

```

## 交叉引用

- [将 Pod 分配给节点](./assigning-pods-to-nodes.md) — nodeSelector / nodeAffinity 与污点互补
- [节点压力驱逐](./node-pressure-eviction.md) — kubelet 自动添加压力相关污点
- [API 发起驱逐](./api-initiated-eviction.md) — `kubectl drain` 与污点驱逐的区别
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — 高优先级 Pod 仍需容忍节点污点
- [Karpenter 自动扩缩容](./karpenter-autoscaling.md) — Karpenter NodePool 中的 taints 配置

## 参考链接

- [Kubernetes 官方文档 - Taints and Tolerations](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]

```