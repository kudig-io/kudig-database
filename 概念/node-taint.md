---
title: 节点污点
summary: 节点污点：Kubernetes 节点污点（Taint）是一种让节点排斥某些 Pod 的机制。
category: concepts
tags:
- core-concept
- k8s
- scheduling
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# 节点污点（Taint）

## 概述

Taint（污点）和 Toleration（容忍）是 Kubernetes 调度器的一对孪生机制：**Taint 标记节点"排斥"某些 Pod，Toleration 让特定 Pod 能够"容忍"该 Taint 从而被允许调度**。二者配合实现节点级专用化与节点状态隔离——例如把 GPU 节点只给需要 GPU 的 Pod、把 master 节点排除普通业务、在节点磁盘不足/不可达时自动驱逐 Pod。这是与 nodeAffinity（正向吸引）互补的反向控制手段。

## 架构与工作原理

```
节点：node-gpu-1
  Taint: dedicated=gpu:NoSchedule        ← 标记：拒绝不带容忍的 Pod

普通 Pod（无 toleration）
  → 调度器过滤掉该节点（NoSchedule）

GPU 任务 Pod
  tolerations:
  - key: dedicated, value: gpu, effect: NoSchedule
  → 允许调度到 node-gpu-1
```

**Taint 三要素**：`key=value:effect`
- **key / value**：标签式键值，value 可省。
- **effect**（效果）：
  - `NoSchedule`：**硬性**拒绝新 Pod 调度（已运行的 Pod 不受影响）。
  - `PreferNoSchedule`：**软性**尽量避免调度，但资源紧张时仍可调度。
  - `NoExecute`：**驱逐**——新 Pod 不调度，且已运行但无对应 toleration 的 Pod **被驱逐**。可配 `tolerationSeconds` 延迟驱逐，用于节点临时故障（如网络分区）的优雅迁移。

**节点自动打的 Taint**（controller-manager 自动维护）：
- `node-role.kubernetes.io/control-plane:NoSchedule` —— master/control-plane 节点（1.24+ 替代旧的 `node-role.kubernetes.io/master`）。
- `node.kubernetes.io/not-ready:NoExecute` —— 节点 NotReady。
- `node.kubernetes.io/unreachable:NoExecute` —— 节点失联。
- `node.kubernetes.io/disk-pressure`、`memory-pressure`、`pid-pressure`、`network-unavailable` —— 资源压力。

## 关键组件与特性

| 元素 | 说明 |
|------|------|
| `kubectl taint` | 命令行打/去污点 |
| `spec.tolerations` | Pod 声明容忍哪些 Taint |
| `effect` | NoSchedule / PreferNoSchedule / NoExecute |
| `tolerationSeconds` | NoExecute 下延迟驱逐秒数 |
| `operator` | Equal（精确匹配）/ Exists（仅匹配 key） |
| 自动 Taint | 节点状态变化时由节点控制器自动添加 |

## 配置示例

```bash
# 1. 给 GPU 节点打专用污点
kubectl taint node node-gpu-1 dedicated=gpu:NoSchedule
# 2. 节点维护前驱逐
kubectl taint node node-1 node.kubernetes.io/maintenance=true:NoExecute
# 3. 移除污点（key-effect 末尾加减号）
kubectl taint node node-gpu-1 dedicated=gpu:NoSchedule-
```

```yaml
---
# 4. GPU Pod 容忍污点
apiVersion: apps/v1
kind: Deployment
metadata: {name: train-job, namespace: ml}
spec:
  replicas: 2
  selector: {matchLabels: {app: train}}
  template:
    metadata: {labels: {app: train}}
    spec:
      nodeSelector: {hardware/gpu: nvidia}     # 正向限定 GPU 节点
      tolerations:
      - key: dedicated
        value: gpu
        effect: NoSchedule
      containers:
      - {name: train, image: train:v1, resources: {limits: {nvidia.com/gpu: 1}}}
---
# 5. 系统组件容忍所有 control-plane 与 not-ready（DaemonSet 必备）
spec:
  tolerations:
  - {key: node-role.kubernetes.io/control-plane, effect: NoSchedule, operator: Exists}
  - {key: node.kubernetes.io/not-ready,          effect: NoExecute,  operator: Exists, tolerationSeconds: 300}
  - {key: node.kubernetes.io/unreachable,        effect: NoExecute,  operator: Exists, tolerationSeconds: 300}
  - {key: node.kubernetes.io/disk-pressure,      effect: NoSchedule, operator: Exists}
---
# 6. 全容忍（谨慎，仅系统级）
tolerations:
- operator: Exists
```

## 常用操作与命令

```bash
# 查看节点 Taint
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints
kubectl describe node node-1 | grep -iA10 taint

# 打/去污点
kubectl taint node node-1 role=dedicated:NoSchedule
kubectl taint node node-1 role=dedicated:NoSchedule-

# 检查 Pod 是否因为 Taint Pending
kubectl describe pod <pod> | grep -A5 Taints

# 维护流程：cordon + drain（自动打 NoSchedule + 驱逐）
kubectl cordon node-1
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --timeout=120s
# ... 维护操作 ...
kubectl uncordon node-1

# 排查被 NoExecute 驱逐的 Pod
kubectl get events -n production --field-selector reason=TaintManagerEviction
```

## 最佳实践

1. **专用节点配 Taint + nodeSelector 双保险**：只用 nodeSelector 仍可能被无 toleration 的 Pod 占用，Taint 才是硬隔离。
2. **control-plane 保留 NoSchedule**：默认 Taint 不要去除，避免业务 Pod 抢占控制平面资源。
3. **DaemonSet 容忍 control-plane 与 not-ready**：监控、日志、网络 Agent 必须容忍才能覆盖所有节点。
4. **生产应用配 tolerationSeconds**：对 not-ready/unreachable 设 300s 左右，给短暂网络抖动留缓冲。
5. **维护用 cordon + drain**：自动处理 Taint + 遵守 PDB，比手动 taint 更安全。
6. **不要用全容忍（Exists）**：等于绕过 Taint 隔离，除非是系统组件。

## 常见陷阱

- **Pod 一直 Pending 且 events 提示 Taint**：忘记加对应 toleration，或 effect 类型写错（NoSchedule vs NoExecute）。
- **control-plane 跑业务 Pod**：去掉了 master 的 Taint，业务 Pod 抢资源影响控制平面稳定。
- **节点抖动反复驱逐**：unreachable NoExecute 无 tolerationSeconds，Pod 被频繁重建；加延迟缓冲。
- **drain 卡住**：有 PodDisruptionBudget 或无 controller 的裸 Pod 阻止驱逐，加 `--disable-eviction` 或清理裸 Pod。
- **Taint key 拼写错误**：`control-plane`（1.24+）与旧的 `master` 不通用，容忍要覆盖实际生效的 key。
- **PreferNoSchedule 误用为硬约束**：期望"绝对隔离"却用了 PreferNoSchedule，资源紧张时仍被调度。

## 参见

- [[kubernetes]] — k8s 领域核心页面
- [[概念/pods.md|Pod]]
- [[概念/pod-affinity.md|Pod 亲和性]] — 正向调度控制
- [[概念/daemonset.md|DaemonSet]] — 需容忍多种 Taint
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
