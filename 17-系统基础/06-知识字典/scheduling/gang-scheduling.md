---
title: Gang Scheduling
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- scheduler
- job
- gpu
- nvidia
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gang Scheduling 是什么
- 如何 Gang Scheduling
trigger_keywords:
- Gang
- Scheduling
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Gang Scheduling

## 概述

Gang Scheduling（组调度）确保一组 Pod 以"全有或全无"的方式进行调度。如果集群无法容纳整个组（或定义的最低数量），则组中没有任何 Pod 会被绑定到节点上。该特性在 [[Kubernetes|Kubernetes]] v1.35 中为 alpha 状态。

## 核心概念/原理

Gang Scheduling 依赖于 [[17-系统基础/06-知识字典/workloads/workload-api.md|Workload API]]。需要启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。

当启用 `GangScheduling` 插件时，调度器会更改属于 Workload 中 `gang` pod group 策略的 Pod 的生命周期：

1. **PreEnqueue 阶段**：调度器会 hold 住 Pod，直到：
   - 引用的 Workload 对象已创建。
   - 引用的 pod group 存在于 Workload 中。
   - 为该特定组创建的 Pod 数量至少等于 `minCount`。
   
   在这些条件满足之前，Pod 不会进入活动调度队列。

2. **调度阶段**：一旦满足法定数量（quorum），调度器尝试为组中的所有 Pod 找到放置位置。所有已分配的 Pod 在此过程中会在 `WaitOnPermit` 门处等待。

3. **绑定阶段**：如果调度器能为至少 `minCount` 个 Pod 找到有效的放置位置，则允许所有找到的 Pod 绑定到其分配的节点。如果在 5 分钟的固定超时内无法为整个组找到放置位置，则不会调度任何 Pod，而是将它们移到不可调度队列中等待集群资源释放。

## 关键机制或特性

- **Workload API 依赖**：Gang Scheduling 的核心是 Workload API 中的 `gang` pod group 策略。
- **minCount**：定义了组调度的最低 Pod 数量要求。
- **WaitOnPermit**：Pod 在找到放置位置后但在绑定前会在此等待，确保整个组满足条件后才进行绑定。
- **超时机制**：固定 5 分钟超时，超时后 Pod 会被移到不可调度队列，让其他工作负载有机会被调度。
- **Alpha 限制**：在 alpha 阶段，找到放置位置是基于逐个 Pod 调度的方式，而非单周期方式。

## 使用场景

- 分布式训练作业（如 MPI、TensorFlow、PyTorch）需要所有工作进程同时启动，否则训练无法进行。
- 需要原子性调度的批处理作业，确保资源分配的一致性。
- 多 Pod 协作的应用场景，其中部分 Pod 无法单独运行。

## 最佳实践/注意事项

- 必须启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。
- 由于当前是 alpha 特性，实现方式是基于逐个 Pod 调度，可能存在一定的调度延迟。
- 超时后 Pod 会进入不可调度队列，工作负载设计需要能容忍这种等待。

## 生产 YAML 示例

### Workload 定义（Gang Pod Group）

```yaml
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: distributed-training-job
  namespace: ml-platform
spec:
  podGroups:
    - name: workers
      policy: gang
      minCount: 4                          # 至少 4 个 worker 同时调度
      selector:
        matchLabels:
          app: pytorch-trainer
          role: worker
```

### 引用 Workload 的 Pod（PyTorch 分布式训练）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pytorch-worker-0
  namespace: ml-platform
  labels:
    app: pytorch-trainer
    role: worker
  annotations:
    scheduling.k8s.io/workload: distributed-training-job
    scheduling.k8s.io/pod-group: workers
spec:
  containers:
    - name: trainer
      image: registry.example.com/pytorch-trainer:v2.3
      resources:
        requests:
          cpu: "8"
          memory: 32Gi
          nvidia.com/gpu: "1"
        limits:
          nvidia.com/gpu: "1"
      env:
        - name: WORLD_SIZE
          value: "4"
        - name: RANK
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
  restartPolicy: Never
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 所有 Pod 卡在 PreEnqueue，不进入调度队列 | Workload 对象未创建或 Pod 数量不满足 minCount | 确认 Workload 对象存在；检查创建的 Pod 数量 >= minCount |
| Pod 在 WaitOnPermit 超时后全部被退回 | 集群资源不足，无法同时放置 minCount 个 Pod | 检查集群可用资源；考虑增加节点或降低 minCount |
| 部分 Pod 调度成功但部分失败 | 非 gang 策略的 Pod 被混入 | 确认所有相关 Pod 的 annotation 正确引用 Workload 和 pod-group |
| 特性门控未启用导致功能无效 | GenericWorkload 特性门控未开启 | 确认 apiserver 和 scheduler 启用 `GenericWorkload` 和 `scheduling.k8s.io/v1alpha1` |

## 生产检查清单

- [ ] 启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组
- [ ] 在调度器配置中启用 `GangScheduling` 插件
- [ ] 合理设置 `minCount`（不宜过大，避免长时间无法满足法定数量）
- [ ] 为分布式训练 Pod 设置合理的资源请求，避免碎片化
- [ ] 配置 Karpenter / Cluster Autoscaler 感知 gang 调度需求
- [ ] 监控 WaitOnPermit 超时事件，设置告警
- [ ] 工作负载设计需容忍 5 分钟超时后的重试

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Workload 对象
kubectl get workloads -n ml-platform

# 查看 Workload 详情
kubectl describe workload distributed-training-job -n ml-platform

# 查看 gang 组内 Pod 状态
kubectl get pods -n ml-platform -l app=pytorch-trainer,role=worker

# 查看调度器日志中 gang 相关事件
kubectl logs -n kube-system -l component=kube-scheduler | grep -i gang

# 查看 Pending Pod 的调度事件
kubectl describe pod pytorch-worker-0 -n ml-platform | grep -A 10 Events
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度周期与绑定周期
- [调度框架](./scheduling-framework.md) — PreEnqueue / Permit 扩展点
- [Pod 调度就绪性](./pod-scheduling-readiness.md) — schedulingGates 与 gang 调度互补
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — gang 组 Pod 的优先级设置
- Karpenter 自动扩缩容](./karpenter-autoscaling.md) — 为 gang 组快速准备节点

## 参考链接

- [Kubernetes 官方文档 - Gang Scheduling](https://kubernetes.io/docs/concepts/scheduling-eviction/gang-scheduling/)

## Related
- [[21-生态参考/03-领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
