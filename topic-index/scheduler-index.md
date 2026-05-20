---
title: Scheduler 调度与弹性伸缩知识图谱索引
description: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- scheduler
- autoscaling
- hpa
- vpa
- karpenter
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Scheduler 调度与弹性伸缩知识图谱 是什么
- 如何 Scheduler 调度与弹性伸缩知识图谱
trigger_keywords:
- Scheduler
- 调度
- 弹性伸缩
- 知识图谱
- hpa
- vpa
- karpenter
---

# Scheduler 调度与弹性伸缩知识图谱索引

> 知识图谱：按主题 **Scheduler & 弹性伸缩** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Scheduler 或弹性伸缩为主题或直接面向调度运维场景。

### 深度技术

- [Kubernetes Scheduler 深度解析 (Kube-Scheduler Deep Dive)](./domain-3-control-plane/20-kube-scheduler-deep-dive.md)
- [调度器配置与优化](./domain-4-workloads/19-scheduler-configuration.md)
- [动态资源分配 (Dynamic Resource Allocation)](./domain-3-control-plane/30-dynamic-resource-allocation.md)
- [HPA/VPA 自动伸缩配置](./domain-4-workloads/21-hpa-vpa-autoscaling.md)
- [集群容量规划](./domain-4-workloads/22-cluster-capacity-planning.md)
- [资源管理表](./domain-4-workloads/23-resource-management.md)

### 故障排查

- [Scheduler 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md)
- [HPA/VPA 故障排查 (HPA/VPA Troubleshooting)](./domain-12-troubleshooting/17-hpa-vpa-troubleshooting.md)
- [Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)](./domain-12-troubleshooting/24-quota-limitrange-troubleshooting.md)
- [集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)](./domain-12-troubleshooting/28-cluster-autoscaler-troubleshooting.md)
- [性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)](./domain-12-troubleshooting/33-performance-bottleneck-troubleshooting.md)
- [HPA 与 VPA 自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting.md)
- [Cluster Autoscaler 节点自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md)

### 技能卡片

- [HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation](./topic-skills/12-autoscaling-failure.md)

### YAML 配置

- [Namespace / ResourceQuota / LimitRange YAML 配置参考](./domain-32-yaml-manifests/02-namespace-resourcequota-limitrange.md)
- [PriorityClass / RuntimeClass YAML 配置参考](./domain-32-yaml-manifests/26-priorityclass-runtimeclass.md)
- [HorizontalPodAutoscaler v2 YAML 配置参考](./domain-32-yaml-manifests/27-hpa-autoscaling-v2.md)

### K8s 事件

- [调度与抢占事件](./domain-33-kubernetes-events/05-scheduling-preemption-events.md)
- [自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)](./domain-33-kubernetes-events/12-autoscaling-events.md)

### 技术论文

- [Kubernetes 调度器深度优化与自定义调度 (Scheduler Deep Optimization and Custom Scheduling)](./domain-19-papers/12-kubernetes-scheduler-deep-optimization-custom-scheduling.md)

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及调度与弹性伸缩但以其他 K8s 组件为主题。

### 调度相关

- [API 优先级与公平性 (APF) 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/06-apf-troubleshooting.md)
- [PodDisruptionBudget (PDB) 故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting.md)

### 资源管理

- [容量规划与资源评估 (Capacity Planning & Resource Assessment)](./domain-9-platform-ops/03-capacity-planning-resource-assessment.md)
- [资源与调度故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/01-resources-quota-troubleshooting.md)

### Pod调度

- [Pod 故障排查与运行机制深度指南](./topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)
- [Pod Pending 调度失败诊断与修复](./topic-skills/03-pod-pending.md)

### 节点调度

- [kubelet 故障排查指南](./topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting.md)
- [GPU 与设备插件故障排查指南](./topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting.md)

---

## 三、扩展参考

> 以下为 K8s 全域参考，调度与弹性伸缩可参考相关章节。

### 术语词典

- [Kubernetes Scheduler](./topic-dictionary/scheduling/kubernetes-scheduler.md)
- [Pod Priority and Preemption](./topic-dictionary/scheduling/pod-priority-and-preemption.md)
- [Taints and Tolerations](./topic-dictionary/scheduling/taints-and-tolerations.md)
- [Pod Topology Spread Constraints](./topic-dictionary/scheduling/pod-topology-spread-constraints.md)
- [Scheduling Framework](./topic-dictionary/scheduling/scheduling-framework.md)
- [Scheduler Performance Tuning](./topic-dictionary/scheduling/scheduler-performance-tuning.md)
- [Gang Scheduling](./topic-dictionary/scheduling/gang-scheduling.md)
- [Karpenter 自动扩缩容](./topic-dictionary/scheduling/karpenter-autoscaling.md)
- [Assigning Pods to Nodes](./topic-dictionary/scheduling/assigning-pods-to-nodes.md)
- [Pod Scheduling Readiness](./topic-dictionary/scheduling/pod-scheduling-readiness.md)
- [Resource Bin Packing](./topic-dictionary/scheduling/resource-bin-packing.md)
- [Node Declared Features](./topic-dictionary/scheduling/node-declared-features.md)
- [Pod Overhead](./topic-dictionary/scheduling/pod-overhead.md)
- [Node-pressure Eviction](./topic-dictionary/scheduling/node-pressure-eviction.md)
- [API-initiated Eviction](./topic-dictionary/scheduling/api-initiated-eviction.md)
- [Device Plugins](./topic-dictionary/platform-engineering/device-plugins.md)
- [Dynamic Resource Allocation](./topic-dictionary/scheduling/dynamic-resource-allocation.md)
- [Resource Quotas](./topic-dictionary/security/resource-quotas.md)
- [Limit Ranges](./topic-dictionary/security/limit-ranges.md)
