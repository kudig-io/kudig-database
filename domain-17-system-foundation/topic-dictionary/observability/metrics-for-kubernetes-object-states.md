---
title: Kubernetes 对象状态指标（kube-state-metrics）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- prometheus
- grafana
- statefulset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 对象状态指标（kube-state-metrics） 是什么
- 如何 Kubernetes 对象状态指标（kube-state-metrics）
trigger_keywords:
- Kubernetes
- 对象状态指标
- kube-state-metrics
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] 对象状态指标（kube-state-metrics）

## 概述

kube-state-metrics 是一个 Kubernetes 插件代理，用于从 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|Kubernetes API]] 中对象的状态生成并暴露集群级指标。它连接到 API 服务器，通过 HTTP 端点暴露由集群中各个对象状态生成的指标，使运维人员能够基于对象状态进行查询和告警。

## 核心概念/原理

- **对象状态即指标**：Kubernetes 对象在 API 中的状态（如标签、注解、启动/终止时间、当前阶段等）可以被转换为 [[Prometheus|Prometheus]] 指标。
- **只读代理**：kube-state-metrics 仅读取 Kubernetes API 对象，不修改任何资源。
- **补充控制平面指标**：与控制平面组件自身暴露的运行时指标不同，kube-state-metrics 提供的是“对象语义”层面的指标。

## 关键机制或特性

### 指标示例

容器中运行的 Pod 会生成 `kube_pod_container_info` 指标，其标签包含：

- 容器名称
- Pod 名称
- 命名空间
- 容器镜像名称
- 镜像 ID
- spec 中的镜像名称
- 运行中容器的 ID
- Pod ID

### 查询示例

以下 PromQL 查询返回未就绪 Pod 的数量：

```promql
count(kube_pod_status_ready{condition="false"}) by (namespace, pod)
```

### 告警示例

以下告警规则在 Pod 处于 `Terminating` 状态超过 5 分钟时触发：

```yaml
groups:
- name: Pod state
  rules:
  - alert: PodsBlockedInTerminatingState
    expr: count(kube_pod_deletion_timestamp) by (namespace, pod) * count(kube_pod_status_reason{reason="NodeLost"} == 0) by (namespace, pod) > 0
    for: 5m
    labels:
      severity: page
    annotations:
      summary: Pod {{$labels.namespace}}/{{$labels.pod}} blocked in Terminating state.
```

### 集成方式

kube-state-metrics 本身只是一个指标暴露端点，需要配合 Prometheus 或其他支持 Prometheus 格式的抓取工具使用。抓取后可将数据存储到时序数据库，并用于 Grafana 仪表板、告警规则等。

## 使用场景

- **基于对象状态的监控**：如监控 Pod 是否处于 CrashLoopBackOff、Terminating、Pending 等异常状态。
- **容量与拓扑洞察**：通过 Deployment、StatefulSet、Node 等指标了解副本数、节点状态、资源分配情况。
- **告警构建**：利用对象状态指标构建精准的告警规则，如 Pod 卡住、节点不可用、PVC 绑定失败等。
- **多集群可视化**：将多个集群的 kube-state-metrics 数据聚合到统一的 Prometheus/Thanos 中，实现全局视图。

## 最佳实践/注意事项

- kube-state-metrics 是一个第三方项目（CNCF 项目），不由 Kubernetes 官方直接维护，但已被社区广泛采用。
- 在大规模集群中，kube-state-metrics 的内存和 CPU 消耗与集群对象数量成正比，需根据集群规模调整其资源请求和限制。
- 仅暴露对象状态指标，不暴露容器运行时级别的性能指标（如 CPU、内存使用量），后者由 cAdvisor/kubelet 提供。
- 确保 Prometheus 等抓取工具能够访问 kube-state-metrics 的暴露端口，并配置适当的抓取间隔。
- 定期更新 kube-state-metrics 版本，以获取对新 API 对象和指标的支持。

## 参考链接

- [Metrics for Kubernetes Object States - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/kube-state-metrics/)

## Related

- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
