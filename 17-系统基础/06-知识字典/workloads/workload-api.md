---
title: Workload API
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- job
- cronjob
- gpu
- nvidia
- llm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Workload API 是什么
- 如何 Workload API
trigger_keywords:
- Workload
- API
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Workload API

## 概述
Workload API 是 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] v1.35 引入的 Alpha 特性（默认禁用，需启用 `GenericWorkload` 特性门控和 `scheduling.k8s.io/v1alpha1` API 组）。它提供了一种结构化的、机器可读的多 Pod 应用调度需求定义，补充了现有工作负载控制器的运行时行为。

## 核心概念/原理
- **Workload 资源**：属于 `scheduling.k8s.io/v1alpha1` API 组，用于定义一组 Pod 的调度策略和放置约束。
- **与控制器分离**：Workload 资源决定 Pod 组应如何被调度，而 Job 等控制器决定运行什么。
- **结构组成**：
  - `podGroups`：定义工作负载的多个组件（如机器学习任务中的 driver 和 worker）。
  - `controllerRef`：链接到上层控制器对象（如 Job），用于可观测性和工具集成，不参与调度。

## 关键机制或特性
- **Pod 组（Pod Groups）**：每个组必须具有唯一的名称和一个调度策略（`basic` 或 `gang`）。
- **Gang 调度**：通过 `gang` 策略实现“全有或全无”调度，确保紧耦合工作负载的所有 Pod 能够同时调度，避免部分启动导致的死锁或资源浪费。
- **Pod 引用**：Pod 通过 `spec.workloadRef` 链接到 Workload 对象中的具体 Pod 组。

## 使用场景
- 大规模分布式训练（如 MPI、PyTorch、TensorFlow）需要 gang 调度。
- 批处理作业中多个 worker 必须同时启动才能协同工作。
- 为调度器提供显式的 Pod 分组信息，优化放置决策和集群可观测性。

## 最佳实践/注意事项
- 使用本特性前需确认集群启用了对应的特性门控和 API 组。
- `controllerRef` 仅用于工具和可观测性，调度器不会读取该字段。
- 若 Pod 引用了不存在的 Workload 或 Pod 组，Pod 将保持 Pending 状态。
- Gang 调度策略需同时启用 `GangScheduling` 特性门控。

## 生产 YAML 示例

### 完整 Workload 定义（分布式训练场景）

```yaml
apiVersion: scheduling.k8s.io/v1alpha1
kind: Workload
metadata:
  name: llm-training-job
  namespace: ml-platform
spec:
  # 链接到上层控制器（仅用于可观测性，不影响调度）
  controllerRef:
    apiGroup: batch
    kind: Job
    name: llm-finetune-v2
  podGroups:
  # Driver 组：负责协调训练（单副本，独立调度即可）
  - name: driver
    policy:
      basic: {}
  # Worker 组：执行训练计算（gang 调度，必须全部就绪）
  - name: workers
    policy:
      gang:
        minCount: 8           # 至少 8 个 GPU worker 同时调度
```

### 对应的 Job 与 Pod 模板

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: llm-finetune-v2
  namespace: ml-platform
spec:
  completions: 8
  parallelism: 8
  completionMode: Indexed
  template:
    spec:
      workloadRef:
        name: llm-training-job
        podGroupName: workers
      containers:
      - name: trainer
        image: registry.example.com/ml/llm-trainer:v2.1
        env:
        - name: WORLD_SIZE
          value: "8"
        - name: RANK
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        resources:
          requests:
            nvidia.com/gpu: "1"
            memory: "32Gi"
            cpu: "8"
          limits:
            nvidia.com/gpu: "1"
            memory: "32Gi"
      restartPolicy: Never
  backoffLimit: 3

```

## Workload API 字段对照表

| 字段 | 必填 | 说明 |
|------|------|------|
| `spec.podGroups[].name` | 是 | Pod 组唯一名称，被 Pod 的 `workloadRef.podGroupName` 引用 |
| `spec.podGroups[].policy` | 是 | `basic` 或 `gang`，只能指定一种 |
| `spec.podGroups[].policy.gang.minCount` | gang 时必填 | 满足调度条件的最小 Pod 数 |
| `spec.controllerRef` | 否 | 上层控制器引用，仅用于工具和可观测性 |
| `spec.controllerRef.apiGroup` | 是 | 控制器的 API 组（如 `batch`） |
| `spec.controllerRef.kind` | 是 | 控制器类型（如 `Job`） |
| `spec.controllerRef.name` | 是 | 控制器对象名称 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| `kubectl apply` 报 404 或 unknown resource | API 组未启用 | 确认 `--runtime-config=scheduling.k8s.io/v1alpha1=true` 已配置 |
| Pod 组中部分 Pod 调度成功，其余 Pending | minCount 设置过低 | 调整 `minCount` 到实际需要的最小并发数 |
| Workload 创建成功但 Pod 未关联 | Pod 的 `workloadRef.name` 或 `podGroupName` 不匹配 | `kubectl get pod -o yaml` 检查 workloadRef 字段 |
| Gang 组 Pod 长时间等待 | 集群 GPU/内存资源不足以满足 minCount | `kubectl describe nodes` 检查可分配资源总量 |

## 生产检查清单

- [ ] 启用 `GenericWorkload` 和 `GangScheduling` 特性门控
- [ ] API 组 `scheduling.k8s.io/v1alpha1` 在 API Server 中已注册
- [ ] 每个 podGroup 名称在 Workload 内唯一
- [ ] gang 策略的 `minCount` 与实际 parallelism 设置一致
- [ ] controllerRef 正确指向关联的 Job/控制器（可选但推荐）
- [ ] Workload 对象先于 Pod 创建（Pod 依赖 Workload 存在）
- [ ] 配置 ResourceQuota 防止 gang 组过度占用集群资源

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 API 组是否可用
kubectl api-versions | grep scheduling.k8s.io

# 创建 Workload 资源
kubectl apply -f workload.yaml

# 查看 Workload 详情
kubectl get workload llm-training-job -n ml-platform -o yaml

# 验证 Pod 关联的 Workload
kubectl get pods -n ml-platform -o custom-columns='NAME:.metadata.name,WORKLOAD:.spec.workloadRef.name,GROUP:.spec.workloadRef.podGroupName'

# 检查调度器对 gang 组的处理
kubectl logs -n kube-system -l component=kube-scheduler | grep -i "workload|gang"
```
## 交叉引用

- [[17-系统基础/06-知识字典/workloads/workload-reference.md|Workload Reference]]](workload-reference.md) — Pod 端的 workloadRef 字段说明
- [[17-系统基础/06-知识字典/workloads/pod-group-policies.md|Pod Group Policies]]](pod-group-policies.md) — basic 和 gang 策略的详细行为
- [Jobs](jobs.md) — 批处理 Job 与 Workload API 的集成
- [CronJob](cronjob.md) — 周期性任务场景下的 Workload 使用

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/workload-api/

## Related

- [[17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[17-系统基础/06-知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[17-系统基础/06-知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]

```

<!-- risk-assessed -->
