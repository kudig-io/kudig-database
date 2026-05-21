---
title: Kubernetes Scheduler
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- scheduler
- job
- rbac
- gpu
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Scheduler 是什么
- 如何 Kubernetes Scheduler
trigger_keywords:
- Kubernetes
- Scheduler
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

# Kubernetes Scheduler

## 概述

在 Kubernetes 中，调度（Scheduling）是指将 Pod 与节点（Node）进行匹配，以便 Kubelet 能够运行它们的过程。kube-scheduler 是 Kubernetes 的默认调度器，作为控制平面的一部分运行。它负责为新创建的或未调度的 Pod 选择最优的节点。

## 核心概念/原理

调度器会持续监视那些尚未分配节点的新建 Pod。对于每个发现的 Pod，调度器负责找到最适合运行它的节点。这个决策过程主要考虑以下两个阶段：

1. **过滤（Filtering）**：找到一组可以运行该 Pod 的可行节点（feasible nodes）。例如，`PodFitsResources` 过滤器会检查候选节点是否有足够的可用资源来满足 Pod 的资源请求。如果过滤后节点列表为空，则该 Pod 暂时不可调度。
2. **评分（Scoring）**：对通过过滤的节点进行排名，选择最合适的节点。调度器根据活跃的评分规则为每个节点分配一个分数，最终选择分数最高的节点。如果有多个节点分数相同，则随机选择一个。

调度决策会考虑的因素包括：个体和集体的资源需求、硬件/软件/策略约束、亲和性与反亲和性规范、数据本地性、工作负载间的干扰等。

## 关键机制或特性

- **Binding（绑定）**：调度器选定节点后，会通知 API server 这一决策。
- **Scheduling Policies（调度策略）**：允许配置 Predicates（用于过滤）和 Priorities（用于评分）。
- **Scheduling Profiles（调度配置文件）**：允许配置实现不同调度阶段的插件，包括 `QueueSort`、`Filter`、`Score`、`Bind`、`Reserve`、`Permit` 等。还可以配置 kube-scheduler 运行不同的配置文件。
- **可替换性**：kube-scheduler 的设计允许用户在需要时编写自己的调度组件来替代默认调度器。

## 使用场景

- 理解 Pod 为什么被放置在特定节点上。
- 计划实现自定义调度器。
- 优化大规模集群中的 Pod 放置策略。

## 最佳实践/注意事项

- 调度器在选择节点时采用两阶段操作，先过滤后评分。
- 如果没有节点满足 Pod 的调度需求，Pod 将保持未调度状态，直到有合适的节点出现。
- 可以通过配置多个调度配置文件来适应不同类型的工作负载。

## 生产 YAML 示例

### 多调度配置文件

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  # 默认配置文件 — 通用工作负载
  - schedulerName: default-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 1
          - name: ImageLocality
            weight: 1
  # 高吞吐配置文件 — 批处理作业
  - schedulerName: batch-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesFit
            weight: 2
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated
            resources:
              - name: cpu
                weight: 1
              - name: memory
                weight: 1
```

### Pod 指定调度器

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: batch-job
  namespace: data-pipeline
spec:
  schedulerName: batch-scheduler          # 使用自定义调度配置文件
  containers:
    - name: etl
      image: registry.example.com/etl:v3.2
      resources:
        requests:
          cpu: "2"
          memory: 4Gi
        limits:
          cpu: "4"
          memory: 8Gi
  restartPolicy: Never
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 长时间 Pending | 无节点满足 Filter 阶段 | `kubectl describe pod <name>` 查看 Events 中的 FailedScheduling 原因 |
| Pod 被调度到非预期节点 | Score 阶段权重不合理 | 检查 KubeSchedulerConfiguration 中的 pluginConfig 权重设置 |
| 调度器日志报错插件冲突 | 多个配置文件启用冲突插件 | `kubectl logs -n kube-system kube-scheduler-*` 查看启动日志 |
| 使用自定义 schedulerName 但 Pod 不被调度 | 调度器配置文件名不匹配 | 确认 KubeSchedulerConfiguration 中 `schedulerName` 与 Pod spec 一致 |
| 调度延迟高 | 集群规模大，评分节点过多 | 调整 `percentageOfNodesToScore` 参数，参考调度器性能调优 |

## 生产检查清单

- [ ] 确认 kube-scheduler 组件健康：`kubectl get componentstatuses`
- [ ] 为不同工作负载类型配置独立的调度配置文件（default / batch / gpu 等）
- [ ] 设置调度器 Leader Election 确保高可用
- [ ] 配置 `--v=2` 日志级别用于生产环境；调试时临时切换到 `--v=4`
- [ ] 为 kube-scheduler Pod 设置合理的 CPU / Memory requests
- [ ] 监控 `scheduler_scheduling_attempt_duration_seconds` 和 `scheduler_pending_pods` 指标
- [ ] 验证调度器 RBAC 权限最小化

## 命令快速参考

```bash
# 查看调度器组件状态
kubectl get componentstatuses | grep scheduler

# 查看 Pod 调度事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100

# 查看哪些 Pod 处于 Pending 状态
kubectl get pods --all-namespaces --field-selector=status.phase=Pending

# 查看节点可分配资源
kubectl describe node <node-name> | grep -A 10 "Allocatable"

# 查看调度器指标（需要端口转发）
kubectl port-forward -n kube-system svc/kube-scheduler 10259:10259
curl -k https://localhost:10259/metrics | grep scheduler_pending_pods
```

## 交叉引用

- [调度框架](./scheduling-framework.md) — 深入了解调度器插件架构
- [调度器性能调优](./scheduler-performance-tuning.md) — 大规模集群调度优化
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — 调度队列排序与抢占逻辑
- [资源装箱](./resource-bin-packing.md) — MostAllocated / RequestedToCapacityRatio 评分策略
- [将 Pod 分配给节点](./assigning-pods-to-nodes.md) — nodeSelector / affinity 约束

## 参考链接

- [Kubernetes 官方文档 - kube-scheduler](https://kubernetes.io/docs/concepts/scheduling-eviction/kube-scheduler/)

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
