# Taints and Tolerations

## 概述

节点亲和性（Node affinity）是 Pod 的属性，用于将 Pod 吸引到一组节点（作为偏好或硬性要求）。而污点（Taints）正好相反——它们允许节点排斥一组 Pod。容忍度（Tolerations）应用于 Pod，允许调度器调度具有匹配污点的 Pod。

污点和容忍度协同工作，确保 Pod 不会被调度到不合适的节点上。

## 核心概念/原理

### Taint（污点）

通过 `kubectl taint` 命令为节点添加污点：
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

Kubernetes 处理多个污点和容忍度的方式类似于过滤器：从节点的所有污点开始，忽略 Pod 有匹配容忍度的污点；剩余的未忽略污点会对 Pod 产生相应效果。

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
- **DaemonSet 容忍度**：DaemonSet Pod 对上述两个污点的 NoExecute 容忍度没有 `tolerationSeconds`，确保它们永远不会因此被驱逐。
- **数值比较操作符**（v1.35+ alpha）：除了 `Equal` 和 `Exists`，还支持 `Gt` 和 `Lt` 用于匹配整数值的污点，适用于基于阈值的调度。
- **设备污点和容忍度**：在使用动态资源分配（DRA）管理特殊硬件时，管理员可以针对单个设备（而非整个节点）设置污点和容忍度。

## 使用场景

- **专用节点**：为特定用户组保留一组节点，通过污点和容忍度实现节点专用化。如需确保 Pod 只使用专用节点，还需结合节点亲和性。
- **特殊硬件节点**：带有 GPU 等专用硬件的节点可以设置污点，确保不需要该硬件的 Pod 不会占用这些节点资源。
- **基于污点的驱逐**：节点出现问题时自动驱逐 Pod。例如节点不可达、内存压力、磁盘压力等。

## 最佳实践/注意事项

- 如果手动指定 `.spec.nodeName`，会绕过调度器，即使节点有 `NoSchedule` 污点也会绑定。但如果节点还有 `NoExecute` 污点，kubelet 仍会驱逐该 Pod（除非有匹配的容忍度）。
- 控制平面限制了向节点添加新污点的速率，以管理大量节点同时不可达时触发的驱逐数量。
- 从 v1.29 开始，基于污点的驱逐实现已从节点控制器移到了独立的 `taint-eviction-controller` 组件中。可以通过 `--controllers=-taint-eviction-controller` 禁用基于污点的驱逐。
- 当使用 `Gt`/`Lt` 操作符时，容忍度和污点的值都必须是有效的有符号 64 位整数。

## 参考链接

- [Kubernetes 官方文档 - Taints and Tolerations](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)
