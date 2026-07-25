---
title: kubelet 资源驱逐机制
description: '## 概述'
summary: '当节点资源（内存、磁盘、PID）不足时，kubelet 的驱逐管理器（Eviction Manager）主动终止低优先级 Pod，释放资源保护高优先级工作负载和节点稳定性。这种机制比被动等待 Linux OOM Killer 更加优雅和可控。'
category: skills
tags:
- k8s
- kubelet
- eviction
- qos
- oom
- resource-pressure
- memory-pressure
- prometheus
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 资源驱逐机制 是什么
- 如何 kubelet 资源驱逐机制
trigger_keywords:
- kubelet
- 资源驱逐机制
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[kubelet|kubelet]] 资源驱逐机制

## 概述

当节点资源（内存、磁盘、PID）不足时，kubelet 的驱逐管理器（Eviction Manager）主动终止低优先级 Pod，释放资源保护高优先级工作负载和节点稳定性。这种机制比被动等待 Linux OOM Killer 更加优雅和可控。

## 驱逐信号

kubelet 监控以下资源信号：

| 信号 | 说明 | 单位 |
|------|------|------|
| `memory.available` | 节点可用内存 | bytes / Mi / Gi |
| `nodefs.available` | 节点文件系统可用空间 | bytes / % |
| `nodefs.inodesFree` | 节点文件系统可用 inode | 数量 / % |
| `imagefs.available` | 镜像文件系统可用空间 | bytes / % |
| `imagefs.inodesFree` | 镜像文件系统可用 inode | 数量 / % |
| `pid.available` | 可用 PID 数量 | 数量 |

## 两种驱逐类型

| 类型 | 触发条件 | 行为 | 配置参数 |
|------|---------|------|---------|
| **硬驱逐（Hard Eviction）** | 资源量立即低于阈值 | 立即终止 Pod | `evictionHard` |
| **软驱逐（Soft Eviction）** | 资源量低于阈值持续一段时间 | 等待宽限期后终止 | `evictionSoft` + `evictionSoftGracePeriod` |

## 默认配置

```yaml
# /var/lib/kubelet/config.yaml

# 硬驱逐阈值
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
  nodefs.inodesFree: "5%"

# 软驱逐阈值
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"

# 软驱逐宽限期
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"

# 驱逐后最小回收量
evictionMinimumReclaim:
  memory.available: "50Mi"
  nodefs.available: "500Mi"

# 驱逐状态转换延迟（防止状态抖动）
evictionPressureTransitionPeriod: 5m
```

## QoS 驱逐优先级

```
驱逐优先级（从高到低）:
  ┌─────────────────────────────────────────────┐
  │  Guaranteed（最高优先级，最后被驱逐）          │
  │  条件: 所有容器都设置了 limits                │
  │        且 requests == limits                 │
  ├─────────────────────────────────────────────┤
  │  Burstable（中等优先级）                      │
  │  条件: 至少一个容器设置了 requests 或 limits  │
  ├─────────────────────────────────────────────┤
  │  BestEffort（最低优先级，最先被驱逐）          │
  │  条件: 所有容器都没有设置 requests 和 limits   │
  └─────────────────────────────────────────────┘
```

### QoS 判定示例

```yaml
# Guaranteed: requests == limits
resources:
  requests:
    cpu: "1"
    memory: "1Gi"
  limits:
    cpu: "1"
    memory: "1Gi"

# Burstable: requests != limits
resources:
  requests:
    cpu: "500m"
    memory: "512Mi"
  limits:
    cpu: "2"
    memory: "2Gi"

# BestEffort: 没有设置任何 requests/limits
# (不写 resources 字段)
```

## 同 QoS 等级内的驱逐顺序

在同一个 QoS 等级内，kubelet 按以下规则选择驱逐目标：

1. **实际资源使用量超过请求量最多的 Pod 优先被驱逐**
2. **使用量与请求量的比值最大的 Pod 优先被驱逐**

## Eviction vs OOM Kill

| 维度 | Eviction（kubelet） | OOM Kill（内核） |
|------|-------------------|-----------------|
| 触发者 | kubelet | Linux 内核 OOM Killer |
| 时机 | 资源接近耗尽时主动触发 | 资源已耗尽时被动触发 |
| 选择逻辑 | 按 QoS 等级和资源使用量 | 按 oom_score_adj 和内存使用量 |
| 优雅性 | 发送 SIGTERM，等待优雅终止 | 直接发送 SIGKILL，无法捕获 |
| Pod 状态 | Evicted 状态 | OOMKilled 状态 |

### OOM Score 设置

kubelet 会为不同 QoS 的 Pod 设置不同的 `oom_score_adj`：

| QoS 等级 | oom_score_adj | 说明 |
|---------|--------------|------|
| Guaranteed | -997 | 最低，最后被 OOM Kill |
| Burstable | 0~999 | 基于内存使用量动态计算 |
| BestEffort | 1000 | 最高，最先被 OOM Kill |

## 节点 Conditions

| Condition | 触发条件 | 影响 |
|-----------|---------|------|
| `MemoryPressure` | 可用内存 < 驱逐阈值 | 仅允许调度 Guaranteed Pod |
| `DiskPressure` | 磁盘空间 < 驱逐阈值 | 仅允许调度 Guaranteed Pod |
| `PIDPressure` | PID 使用量 > 阈值 | 仅允许调度 Guaranteed Pod |

## 监控与告警

```yaml
# Prometheus 告警规则
groups:
- name: eviction-alerts
  rules:
  - alert: PodEvicted
    expr: increase(kubelet_evictions_total[5m]) > 0
    labels:
      severity: warning
    annotations:
      summary: "Pod was evicted on {{ $labels.node }}"

  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.node }} is under memory pressure"
```

## 调试命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看被驱逐的 Pod
kubectl get pods --all-namespaces | grep Evicted

# 查看驱逐事件
kubectl get events --all-namespaces | grep -i evict

# 查看节点资源压力
kubectl describe node <node> | grep -A 10 Conditions

# 查看 kubelet 驱逐日志
journalctl -u kubelet | grep -i eviction

# 查看 OOM 事件
dmesg | grep -i "oom"
kubectl describe pod <pod> | grep -A 5 "Last State"
```
## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| Pod 被 OOMKilled | 容器内存超过 limit | 增加 memory limit 或优化内存使用 |
| Pod 被 Evicted | 节点资源不足 | 扩容节点或调整驱逐阈值 |
| BestEffort Pod 频繁被驱逐 | 节点资源长期紧张 | 为 Pod 设置 requests/limits |
| Guaranteed Pod 被驱逐 | 系统级内存耗尽 | 增加 `--kube-reserved` 和 `--system-reserved` |

## 相关技能

- [[skills/node-drain-and-maintenance.md|[[节点驱逐与维护|节点驱逐与维护]]]]
- [[concepts/resource-management.md|资源管理]]
- [[pod-lifecycle|Pod 生命周期]]
- [[entities/kubelet.md|kubelet]]

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[score]] — Score
- [[prometheus]] — Prometheus

- [[pod-lifecycle|pod-lifecycle]]
- [[domain-17-system-foundation/速查卡/linux.md|linux]]

<!-- risk-assessed -->
