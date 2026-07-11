---
title: Gang Scheduling
summary: Gang Scheduling 是一种调度策略，确保一组关联的 Pod 能够同时被调度到节点上。
category: concepts
tags:
- scheduling
- batch
- hpc
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备具备足够的 RBAC 权限；是否已在非生产环境验证。

# Gang Scheduling

## 概述

Gang Scheduling（成组调度）是一种调度策略，确保一组关联的 Pod **要么全部被调度成功，要么全部不被调度**。这种"全有或全无"（all-or-nothing）语义对于分布式训练、MPI 作业、Spark 任务等需要多个 Pod 协同工作的场景至关重要——如果只有部分 Pod 被调度，剩余 Pod 因资源不足而阻塞，会导致死锁和资源浪费。

## 技术原理

### 核心机制

Kubernetes 默认调度器以**单个 Pod** 为粒度进行调度决策。当一组 Pod 需要同时运行时，如果集群资源不足以调度全部 Pod，部分已调度的 Pod 会占用资源却无法执行有效工作，形成**资源死锁**。

Gang Scheduling 引入了两阶段提交模型：

```
阶段 1: 调度器评估整组 Pod 的资源需求
         → 资源充足: 进入阶段 2
         → 资源不足: 全部挂起 (Pending)，不部分调度

阶段 2: 原子性绑定所有 Pod 到节点
         → 成功: 全部启动
         → 失败: 全部回退
```

### 实现方案

| 方案 | 机制 | 状态 |
|------|------|------|
| **Volcano Scheduler** | Coscheduling 插件，min-available 语义 | 生产可用 |
| **Kueue + K8s 1.32+** | 原生 PodGroup API（alpha→beta） | 1.32 beta |
| **Kube-batch** | 早期方案，已被 Volcano 替代 | 已废弃 |
| **YuniKorn** | Apache 项目，层次化队列调度 | 生产可用 |

## 生产示例

### Volcano Job 定义

```yaml
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: distributed-training
spec:
  minAvailable: 4                    # 必须同时调度 4 个 Pod
  schedulerName: volcano              # 使用 Volcano 调度器
  policies:
    - event: PodEvicted
      action: RestartJob              # Pod 被驱逐时重启整个作业
  tasks:
    - replicas: 1
      name: master
      template:
        spec:
          containers:
            - name: tensorflow
              image: tensorflow:2.15
              command: ["python", "train.py", "--role=master"]
              resources:
                limits:
                  nvidia.com/gpu: 1
    - replicas: 3
      name: worker
      template:
        spec:
          containers:
            - name: tensorflow
              image: tensorflow:2.15
              command: ["python", "train.py", "--role=worker"]
              resources:
                limits:
                  nvidia.com/gpu: 1
```

### Kueue PodGroup（Kubernetes 1.32+）

```yaml
apiVersion: kueue.x-k8s.io/v1beta1
kind: Workload
metadata:
  name: ml-training-job
spec:
  podSets:
    - name: workers
      count: 4                        # 整组需要 4 个 Pod
      template: ...
```

## 最佳实践

- **设置合理的 minAvailable 值**：不要设置过大导致长期 Pending，也不要设置过小失去 gang 语义保障。建议 minAvailable = 总副本数的 80-100%
- **配合优先级和抢占**：为高优先级 gang 作业配置 PriorityClass，允许抢占低优先级任务释放资源。生产环境必须有至少 3 个优先级层级
- **监控调度延迟**：关注 PodGroup 的 `unschedulable` 状态持续时间，配置告警。建议 Pending 超过 10 分钟触发告警，30 分钟触发通知
- **资源预留策略**：在集群中预留专用节点池给分布式训练作业，避免与在线服务争抢资源。使用 taint/toleration 确保隔离
- **故障恢复策略**：配置 Pod 失败时的 `RestartJob` 或 `ResumeJob` 策略，避免部分 Pod 残留。对 Spot 实例场景配置更激进的恢复策略

## 常见陷阱

- **死锁风险**：多个 gang 作业同时竞争资源但都不满足 minAvailable，导致全部 Pending——需要优先级抢占机制打破死锁。Volcano 支持 `reclaimAction` 自动抢占低优先级 Pod
- **调度延迟放大**：等待整组资源就绪会显著增加调度延迟，对延迟敏感型作业不适用。在线服务不应使用 gang scheduling
- **Pod 驱逐级联**：一个 Pod 被驱逐后整组失效，在 Spot 实例场景下需特别配置 PodDisruptionBudget 和节点选择策略

## 技术深度解析

### Volcano Coscheduling 内部机制

Volcano 的 Coscheduling 插件通过两阶段提交实现原子调度：

```
阶段 1 — Reserve（预留）:
  调度器遍历 PodGroup 中的所有 Pod
  → 对每个 Pod 执行 Filter + Score
  → 如果所有 Pod 都能找到节点: 进入阶段 2
  → 如果有 Pod 找不到节点: 放弃所有预留，PodGroup 标记为 unschedulable

阶段 2 — Bind（绑定）:
  原子性地将所有 Pod 绑定到各自节点
  → 如果绑定全部成功: PodGroup 进入 Running
  → 如果部分绑定失败: 回退已绑定的 Pod
```

### 与 Kueue 的集成

Kueue（K8s 原生批处理队列管理器）在 1.32+ 版本中集成了 PodGroup API：

```yaml
# Kueue LocalQueue 配置
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: gpu-training-queue
spec:
  clusterQueue: gpu-cluster-queue
---
# 提交训练作业到队列
apiVersion: kueue.x-k8s.io/v1beta1
kind: Workload
metadata:
  name: distributed-training
spec:
  queueName: gpu-training-queue
  podSets:
    - name: workers
      count: 4
      template: ...
```

Kueue 会等待集群有足够资源时才准入 Workload，实现公平排队和资源分配。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/scheduling-algorithm.md|调度算法]] — 调度器内部机制
- [[概念/dynamic-resource-allocation.md|动态资源分配]] — DRA 与 gang 调度的协同

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
