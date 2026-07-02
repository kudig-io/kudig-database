---
title: Scheduler 调度与弹性伸缩知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
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
- kubelet
- pdb
tier: core
created: '2026-05-23'
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---



# Scheduler 调度与弹性伸缩知识图谱索引

> 知识图谱：按主题 **Scheduler & 弹性伸缩** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Scheduler 或弹性伸缩为主题或直接面向调度运维场景。

### 深度技术

- [[domain-17-system-foundation/topic-dictionary/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]] 深度解析 (Kube-Scheduler Deep Dive)]]
- 调度器配置与优化
- 动态资源分配 (domain-17-system-foundation/topic-dictionary/scheduling/[[dynamic-resource-allocation]]
- HPA/VPA 自动伸缩配置
- 集群容量规划
- 资源管理表

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/17-hpa-vpa-troubleshooting.md|HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/24-quota-limitrange-troubleshooting.md|Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/28-cluster-autoscaler-troubleshooting.md|集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/33-performance-bottleneck-troubleshooting.md|性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|[[Cluster Autoscaler 节点自动扩缩容故障排查指南|Cluster Autoscaler 节点自动扩缩容故障排查指南]]]]

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/12-autoscaling-failure.md|[[HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation|HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation]]]]

### YAML 配置

- Namespace / ResourceQuota / LimitRange YAML 配置参考
- PriorityClass / RuntimeClass YAML 配置参考
- HorizontalPodAutoscaler v2 YAML 配置参考

### K8s 事件

- 调度与抢占事件
- 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)

### 技术论文

- [[domain-19-landscape-references/02-papers/12-kubernetes-scheduler-deep-optimization-custom-scheduling.md|12 kubernetes scheduler deep optimization custom scheduling]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及调度与弹性伸缩但以其他 K8s 组件为主题。

### 调度相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/06-apf-troubleshooting.md|API 优先级与公平性 (APF) 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting.md|[[PodDisruptionBudget (PDB) 故障排查指南|PodDisruptionBudget (PDB) 故障排查指南]]]]

### 资源管理

- [[domain-07-platform-engineering/governance/03-capacity-planning-resource-assessment.md|03 capacity planning resource assessment]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/01-resources-quota-troubleshooting.md|资源与调度故障排查指南]]

### Pod调度

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]

### 节点调度

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，调度与弹性伸缩可参考相关章节。

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-priority-and-preemption.md|Pod Priority and Preemption]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taints-and-tolerations.md|Taints and Tolerations]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-topology-spread-constraints.md|Pod Topology Spread Constraints]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/scheduling-framework.md|Scheduling Framework]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/scheduler-performance-tuning.md|Scheduler Performance Tuning]]
- domain-17-system-foundation/topic-dictionary/scheduling/[[gang-scheduling|Gang Scheduling]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/karpenter-autoscaling.md|Karpenter 自动扩缩容]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/assigning-pods-to-nodes.md|Assigning Pods to Nodes]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-scheduling-readiness.md|Pod Scheduling Readiness]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/resource-bin-packing.md|Resource Bin Packing]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-declared-features.md|Node Declared Features]]
- domain-17-system-foundation/topic-dictionary/scheduling/[[pod-overhead|Pod Overhead]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-pressure-eviction.md|Node-pressure Eviction]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/api-initiated-eviction.md|API-initiated Eviction]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/device-plugins.md|Device Plugins]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]]
- [[domain-17-system-foundation/topic-dictionary/security/resource-quotas.md|Resource Quotas]]
- [[domain-17-system-foundation/topic-dictionary/security/limit-ranges.md|Limit Ranges]]
