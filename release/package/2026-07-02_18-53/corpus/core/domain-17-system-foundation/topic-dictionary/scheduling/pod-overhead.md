---
title: Pod Overhead
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- scheduler
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Overhead 是什么
- 如何 Pod Overhead
trigger_keywords:
- Pod
- Overhead
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Overhead

## 概述

Pod Overhead（Pod 开销）是 [[Kubernetes|Kubernetes]] 中一种用于核算 Pod 基础设施所消耗系统资源的方式。这些资源是容器内部运行所需资源之外的额外开销。Pod 的开销在准入时根据 Pod 的 RuntimeClass 相关联的开销进行设置。

## 核心概念/原理

当在节点上运行 Pod 时，Pod 本身会占用一定量的系统资源。在 Kubernetes 中，Pod Overhead 用于在容器请求和限制之外，额外计入 Pod 基础设施消耗的资源。

Pod 的 overhead 在调度 Pod 时会被考虑在内：调度器会将 Pod 的 overhead 与容器资源请求之和一起计算。同样，[[kubelet|kubelet]] 在调整 Pod cgroup 大小以及执行 Pod 驱逐排序时也会包含 Pod overhead。

## 关键机制或特性

- **RuntimeClass 配置**：需要使用定义了 `overhead` 字段的 `RuntimeClass`。
- **准入控制器修改**：RuntimeClass 准入控制器会在准入时更新工作负载的 PodSpec，加入 `overhead` 字段。如果 PodSpec 已经定义了该字段，Pod 将被拒绝。
- **资源配额计算**：如果定义了 ResourceQuota，容器请求之和以及 `overhead` 字段都会被计入。
- **调度考虑**：调度器在决定哪个节点应该运行新 Pod 时，会将 Pod 的 `overhead` 与容器请求之和相加。
- **cgroup 限制**：kubelet 设置 Pod cgroup 的上限时，会基于容器限制之和加上 PodSpec 中定义的 `overhead`。
- **CPU shares**：对于 Guaranteed 或 Burstable QoS 的 Pod，kubelet 会根据容器请求之和加上 `overhead` 来设置 `cpu.shares`。
- **可观测性**：kube-state-metrics 提供了 `kube_pod_overhead_*` 指标来帮助识别 Pod overhead 的使用情况。

## 使用场景

- 使用虚拟化容器运行时（如 Kata Containers 结合 Firecracker）时，每个 Pod 需要为虚拟机和客户操作系统预留额外资源（如 120MiB 内存、250m CPU）。
- 需要精确计算节点资源使用情况，确保调度决策和 cgroup 限制都包含 Pod 级别的基础设施开销。

## 最佳实践/注意事项

- 确保使用的 RuntimeClass 正确定义了 `overhead` 字段。
- PodSpec 中不应预先定义 `overhead` 字段，否则会被准入控制器拒绝。
- 使用 `kubectl get pod <name> -o jsonpath='{.spec.overhead}'` 可以检查 Pod 的 overhead 值。
- 在节点描述中观察到的资源请求会包含 Pod overhead，这是预期的行为。

## 生产 YAML 示例

### RuntimeClass 配置 Kata Containers 开销

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
scheduling:
  nodeSelector:
    katacontainers.io/kata-runtime: "true"
```

### 使用 RuntimeClass 的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-workload
  namespace: production
spec:
  runtimeClassName: kata-containers       # 引用上面的 RuntimeClass
  containers:
    - name: app
      image: registry.example.com/app:v2.1
      resources:
        requests:
          cpu: "500m"                     # 实际调度请求 = 500m + 250m(overhead) = 750m
          memory: "256Mi"                 # 实际调度请求 = 256Mi + 120Mi(overhead) = 376Mi
        limits:
          cpu: "1"
          memory: "512Mi"
  restartPolicy: Always
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 被拒绝创建（Admission error） | PodSpec 手动定义了 overhead 字段 | 检查 Pod YAML，移除 `spec.overhead`，由 RuntimeClass 准入控制器自动填充 |
| Pod Pending，资源不足 | overhead 未计入预估 | `kubectl get pod -o jsonpath='{.spec.overhead}'` 确认 overhead 值；确认节点有足够额外资源 |
| ResourceQuota 报错 | 配额未计入 overhead | 重新计算配额上限：容器 requests + overhead |
| 节点描述中资源请求比 Pod 容器请求更高 | overhead 被加入节点资源计算 | 正常行为；`kubectl describe node` 看到的 Requests 包含 overhead |
| kube-state-metrics 中看不到 overhead 指标 | kube-state-metrics 版本过低 | 升级 kube-state-metrics 至 v2.0+，检查 `kube_pod_overhead_*` 指标 |

## 生产检查清单

- [ ] 为每种运行时（runc / kata / gVisor）创建对应 RuntimeClass
- [ ] 在 RuntimeClass 中准确设置 `overhead.podFixed` CPU 和 Memory 值
- [ ] 确认 PodSpec 中未手动设置 `overhead` 字段
- [ ] 重新评估 ResourceQuota 和 LimitRange，将 overhead 纳入计算
- [ ] 验证调度结果：`kubectl describe node` 中 Requests 列应包含 overhead
- [ ] 启用 kube-state-metrics 的 `kube_pod_overhead_*` 指标进行监控
- [ ] 节点容量规划时预留 overhead 开销（尤其 Kata / gVisor 节点）

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群中的 RuntimeClass
kubectl get runtimeclasses

# 查看 RuntimeClass 详情（含 overhead）
kubectl get runtimeclass kata-containers -o yaml

# 查看 Pod 的 overhead 值
kubectl get pod <pod-name> -o jsonpath='{.spec.overhead}'

# 查看节点资源分配（含 overhead）
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 查看 kube-state-metrics overhead 指标
curl -s http://kube-state-metrics:8080/metrics | grep kube_pod_overhead
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度器如何将 overhead 纳入资源计算
- [动态资源分配](./dynamic-resource-allocation.md) — DRA 设备的额外资源开销
- [节点压力驱逐](./node-pressure-eviction.md) — kubelet 驱逐排序如何考虑 overhead
- Karpenter 自动扩缩容](./karpenter-autoscaling.md) — 节点容量规划需计入 overhead

## 参考链接

- [Kubernetes 官方文档 - Pod Overhead](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-overhead/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|反亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/api-initiated-eviction.md|API-initiated Eviction]]


<!-- risk-assessed -->
