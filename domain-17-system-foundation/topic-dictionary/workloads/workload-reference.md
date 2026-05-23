---
title: Workload Reference
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- job
- gpu
- cuda
- nvidia
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Workload Reference 是什么
- 如何 Workload Reference
trigger_keywords:
- Workload
- Reference
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Workload Reference

## 概述
Workload Reference 是 [[entities/kubernetes|[[Kubernetes|kubernetes]]]] v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控）。它允许将 Pod 链接到一个 Workload 对象，使调度器能够按组进行协同调度决策，而不是将 Pod 视为独立个体。

## 核心概念/原理
- **Workload 引用**：在 Pod 的 `spec.workloadRef` 字段中指定同一命名空间下的 Workload 对象名称和 Pod 组名称。
- **Pod 组副本（podGroupReplicaKey）**：通过 `podGroupReplicaKey` 可将单个 Pod 组复制为多个独立的调度单元。例如，设置不同的 replica key 可创建多个逻辑子组。
- **行为**：
  - 若引用的组使用 `basic` 策略，workloadRef 主要起分组标签作用。
  - 若引用 `gang` 策略（需启用 `GangScheduling`），Pod 将进入 gang 调度生命周期，等待组内其他 Pod 就绪后一起绑定到节点。
- **缺失引用处理**：若 Pod 引用的 Workload 或 Pod 组不存在，Pod 将保持 Pending，不会被调度。

## 关键机制或特性
- **协同调度**：适用于紧耦合应用（如分布式训练 Job），需要一组 Pod 同时启动才能正常工作。
- **调度器验证**：调度器在做出放置决策前会验证 `workloadRef` 的有效性。

## 使用场景
- 大规模机器学习训练任务（如 MPI、PyTorch），需要所有 worker 同时运行。
- 需要 gang 调度的批处理作业，避免部分启动导致死锁或资源浪费。
- 将 Pod 按应用组进行逻辑归类，便于可观测性和管理。

## 最佳实践/注意事项
- 使用该特性前需确保集群启用了 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组。
- 使用 `gang` 策略时，还需启用 `GangScheduling` 特性门控。
- 确保 Workload 对象和 Pod 组在 Pod 被调度前已存在，否则 Pod 将无限期 Pending。

## 生产 YAML 示例

### Workload 资源 + Pod 引用

```yaml
# 1. 创建 Workload 资源（定义 Pod 组和调度策略）
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: distributed-training
  namespace: ml-team
spec:
  controllerRef:
    apiGroup: batch
    kind: Job
    name: pytorch-ddp-training
  podGroups:
  - name: workers
    policy:
      gang:
        minCount: 4          # 至少 4 个 worker 同时调度才启动
  - name: driver
    policy:
      basic: {}              # driver 独立调度
---
# 2. Pod 中通过 workloadRef 引用
apiVersion: v1
kind: Pod
metadata:
  name: worker-0
  namespace: ml-team
spec:
  workloadRef:
    name: distributed-training    # 引用上面的 Workload
    podGroupName: workers         # 属于 workers 组
    podGroupReplicaKey: "0"       # 副本标识
  containers:
  - name: pytorch-worker
    image: pytorch/pytorch:2.4.0-cuda12.1-cudnn9-runtime
    resources:
      requests:
        nvidia.com/gpu: "1"
        memory: "16Gi"
        cpu: "4"
      limits:
        nvidia.com/gpu: "1"
        memory: "16Gi"
  restartPolicy: Never
```

### 使用 podGroupReplicaKey 创建多副本组

```yaml
# 每个 replicaKey 创建独立的调度单元
# 适用于多组并行训练（如超参数搜索）
apiVersion: v1
kind: Pod
metadata:
  name: experiment-a-worker-0
spec:
  workloadRef:
    name: hyperparameter-search
    podGroupName: workers
    podGroupReplicaKey: "experiment-a"    # 实验 A 的副本组
  # ...
---
apiVersion: v1
kind: Pod
metadata:
  name: experiment-b-worker-0
spec:
  workloadRef:
    name: hyperparameter-search
    podGroupName: workers
    podGroupReplicaKey: "experiment-b"    # 实验 B 的副本组，独立调度
  # ...
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 长期 Pending，Events 显示 workloadRef 无效 | 引用的 Workload 对象不存在或名称拼写错误 | `kubectl get workloads -n <ns>` 确认对象存在 |
| Gang 组部分 Pod 被调度，其余卡住 | `minCount` 设置不合理或集群资源不足 | 检查 `minCount` 与实际 Pod 数是否匹配；`kubectl describe node` 检查可用资源 |
| Pod 调度后被意外驱逐 | gang 组中某个 Pod 失败导致整组回滚 | 查看 Events 和调度器日志，确认 gang 生命周期行为 |
| 特性门控启用后 API 调用 404 | `scheduling.k8s.io/v1alpha1` API 组未注册 | `kubectl api-resources --api-group=scheduling.k8s.io` 验证 API 是否可用 |

## 生产检查清单

- [ ] 集群已启用 `GenericWorkload` 特性门控
- [ ] `scheduling.k8s.io/v1alpha1` API 组已注册
- [ ] 若使用 gang 策略，已同时启用 `GangScheduling` 特性门控
- [ ] Workload 对象在 Pod 创建之前已存在
- [ ] `podGroupReplicaKey` 在同一组内唯一
- [ ] 集群资源足够满足 `minCount` 要求的并发 Pod
- [ ] 监控 Pending Pod 数量，设置告警阈值
- [ ] 已在非生产环境验证 gang 调度行为

## 命令快速参考

```bash
# 确认特性门控启用状态
kubectl get --raw /apis/scheduling.k8s.io/v1alpha1 | jq .

# 列出命名空间内所有 Workload 对象
kubectl get workloads -n <namespace>

# 查看 Workload 详情
kubectl describe workload <name> -n <namespace>

# 检查 Pod 的 workloadRef 字段
kubectl get pod <name> -o jsonpath='{.spec.workloadRef}'

# 查看 Pending 状态 Pod（可能因 workloadRef 问题卡住）
kubectl get pods --field-selector=status.phase=Pending -n <namespace>

# 查看调度器日志中的 gang 调度信息
kubectl logs -n kube-system -l component=kube-scheduler --tail=100 | grep -i gang
```

## 交叉引用

- [[domain-17-system-foundation/topic-dictionary/workloads/workload-api.md|Workload API]]](workload-api.md) — Workload 资源的 API 定义和结构
- [[domain-17-system-foundation/topic-dictionary/workloads/pod-group-policies.md|Pod Group Policies]]](pod-group-policies.md) — basic 和 gang 策略详解
- [[domain-17-system-foundation/topic-dictionary/workloads/jobs.md|Jobs]]](jobs.md) — Job 控制器与 Workload 配合使用
- [调度与驱逐](../scheduling/) — 调度器行为和 Pod 放置决策

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/workload-reference/
