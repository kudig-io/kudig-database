---
title: Pod Scheduling Readiness
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- job
- rbac
- webhook
- gpu
- nvidia
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Scheduling Readiness 是什么
- 如何 Pod Scheduling Readiness
trigger_keywords:
- Pod
- Scheduling
- Readiness
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Scheduling Readiness

## 概述

Pod 调度就绪性（Pod Scheduling Readiness）允许用户通过设置或移除 Pod 的 `.spec.schedulingGates` 字段来控制 Pod 何时准备好被调度器考虑。在 [[Kubernetes|Kubernetes]] v1.30 中达到 stable 状态。

## 核心概念/原理

传统上，Pod 一旦创建就被认为是可调度就绪的。但在实际场景中，某些 Pod 可能会长期处于"缺少必要资源"的状态，不必要地消耗调度器（以及下游集成组件如 Cluster AutoScaler）的资源。

通过 `schedulingGates` 字段，可以显式控制 Pod 进入调度队列的时机。

## 关键机制或特性

- **schedulingGates 字段**：包含一个字符串列表，每个字符串代表一个条件，Pod 必须满足所有这些条件才会被视为可调度。
- **生命周期限制**：该字段只能在 Pod 创建时初始化（由客户端创建或在准入阶段修改）。创建后，可以以任意顺序移除每个 schedulingGate，但不允许添加新的调度门控。
- **Pod 状态**：带有 schedulingGates 的 Pod 会处于 `SchedulingGated` 状态。
- **可观测性**：`scheduler_pending_pods` 指标新增了 `"gated"` 标签，用于区分 Pod 是不可调度还是显式标记为未准备好调度。可以通过 `scheduler_pending_pods{queue="gated"}` 查看。
- **可变调度指令**：在 Pod 具有调度门控时，可以变更其调度指令，但只能收紧（tighten）这些指令：
  - `.spec.nodeSelector` 只允许增加。
  - `spec.affinity.nodeAffinity` 的 `requiredDuringSchedulingIgnoredDuringExecution` 中的 `NodeSelectorTerms` 为空时可以设置；不为空时只允许增加 `matchExpressions` 或 `fieldExpressions`。
  - `.preferredDuringSchedulingIgnoredDuringExecution` 允许所有更新。

## 使用场景

- 当 Pod 依赖外部资源（如持久卷、配置、密钥）尚未就绪时，暂时阻止调度器处理该 Pod。
- 与 Cluster AutoScaler 配合，避免在资源未完全准备好时触发不必要的扩容操作。
- 需要按顺序或条件启动的工作负载，可以等待某些前置条件满足后再允许调度。

## 最佳实践/注意事项

- `schedulingGates` 只能在 Pod 创建时设置，创建后只能移除不能添加。
- 移除所有 `schedulingGates` 后，Pod 才会进入正常的调度流程。
- 在变更调度指令时要确保只收紧约束，否则更新会被拒绝。

## 生产 YAML 示例

### 带 Scheduling Gate 的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ml-training-worker-0
  namespace: ml-platform
  labels:
    app: ml-training
    job-id: "20260407-001"
spec:
  schedulingGates:
    - name: "example.com/dataset-ready"     # 等待数据集下载完成
    - name: "example.com/gpu-quota-approved" # 等待 GPU 配额审批
  containers:
    - name: trainer
      image: registry.example.com/ml-trainer:v4.0
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
          nvidia.com/gpu: "1"
        limits:
          nvidia.com/gpu: "1"
  nodeSelector:
    accelerator: nvidia-a100
  restartPolicy: Never
```

### 外部控制器移除 Scheduling Gate（示例 patch）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 当数据集准备就绪时，外部控制器移除对应的 gate
kubectl patch pod ml-training-worker-0 -n ml-platform \
  --type='json' \
  -p='[{"op": "remove", "path": "/spec/schedulingGates/0"}]'
```
## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 一直处于 SchedulingGated 状态 | schedulingGates 未被移除 | `kubectl get pod -o jsonpath='{.spec.schedulingGates}'` 检查剩余 gate |
| 外部控制器 patch 失败 | RBAC 权限不足 | 确认控制器 ServiceAccount 有 [[Pods|pods]]/patch 权限 |
| Pod 进入调度后因节点不足 Pending | gate 移除时机不当 | 先确认节点资源充足，再移除 gate；或配合 Cluster AutoScaler |
| scheduler_pending_pods 指标中 gated 数量持续增长 | 外部系统问题导致 gate 未释放 | 检查外部控制器日志；设置 gated Pod 告警 |
| 尝试添加新 gate 被拒绝 | 只允许在创建时设置 gate | 使用 admission webhook 在创建时注入所有需要的 gate |

## 生产检查清单

- [ ] 仅在 Pod 创建时（或 admission webhook 阶段）设置 `schedulingGates`
- [ ] 为每个 gate 实现对应的外部控制器，负责在条件满足时移除 gate
- [ ] 配置监控告警：`scheduler_pending_pods{queue="gated"}` 超过阈值时报警
- [ ] 外部控制器具备幂等的 gate 移除逻辑
- [ ] 设置 gate 超时机制：长时间未移除的 gate 自动告警或清理
- [ ] 变更调度指令时只收紧约束（增加 nodeSelector / matchExpressions）

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有 SchedulingGated 状态的 Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending \
  -o jsonpath='{range .items[?(@.spec.schedulingGates)]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'

# 查看 Pod 的 scheduling gates
kubectl get pod <pod-name> -o jsonpath='{.spec.schedulingGates}'

# 移除指定的 scheduling gate
kubectl patch pod <pod-name> --type='json' \
  -p='[{"op": "remove", "path": "/spec/schedulingGates/0"}]'

# 查看调度器指标中 gated Pod 数量
curl -sk https://localhost:10259/metrics | grep 'scheduler_pending_pods.*gated'
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度器如何处理 gated Pod
- [[domain-17-system-foundation/topic-dictionary/scheduling/gang-scheduling.md|Gang Scheduling]]](./gang-scheduling.md) — 结合 scheduling gate 实现组调度前置检查
- [动态资源分配](./dynamic-resource-allocation.md) — DRA ResourceClaim 就绪后移除 gate
- Karpenter 自动扩缩容](./karpenter-autoscaling.md) — 避免 gated Pod 触发不必要的扩容

## 参考链接

- [Kubernetes 官方文档 - Pod Scheduling Readiness](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-scheduling-readiness/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|反亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/api-initiated-eviction.md|API-initiated Eviction]]


<!-- risk-assessed -->
