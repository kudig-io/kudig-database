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
- kubelet
- pdb
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

- [[domain-01-cluster-fundamentals/20-kube-scheduler-deep-dive|Kubernetes Scheduler 深度解析 (Kube-Scheduler Deep Dive)]]
- [[domain-02-workloads-applications/19-scheduler-configuration|调度器配置与优化]]
- [[domain-01-cluster-fundamentals/30-dynamic-resource-allocation|动态资源分配 (Dynamic Resource Allocation)]]
- [[domain-02-workloads-applications/21-hpa-vpa-autoscaling|HPA/VPA 自动伸缩配置]]
- [[domain-02-workloads-applications/22-cluster-capacity-planning|集群容量规划]]
- [[domain-02-workloads-applications/23-resource-management|资源管理表]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting|Scheduler 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/17-hpa-vpa-troubleshooting|HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/24-quota-limitrange-troubleshooting|Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/28-cluster-autoscaler-troubleshooting|集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/33-performance-bottleneck-troubleshooting|性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting|Cluster Autoscaler 节点自动扩缩容故障排查指南]]

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/12-autoscaling-failure|HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation]]

### YAML 配置

- [[domain-18-manifests-patterns/02-namespace-resourcequota-limitrange|Namespace / ResourceQuota / LimitRange YAML 配置参考]]
- [[domain-18-manifests-patterns/26-priorityclass-runtimeclass|PriorityClass / RuntimeClass YAML 配置参考]]
- [[domain-18-manifests-patterns/27-hpa-autoscaling-v2|HorizontalPodAutoscaler v2 YAML 配置参考]]

### K8s 事件

- [[domain-17-system-foundation/05-scheduling-preemption-events|调度与抢占事件]]
- [[domain-17-system-foundation/12-autoscaling-events|自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]

### 技术论文

- [[domain-19-landscape-references/12-kubernetes-scheduler-deep-optimization-custom-scheduling|Kubernetes 调度器深度优化与自定义调度 (Scheduler Deep Optimization and Custom Scheduling)]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及调度与弹性伸缩但以其他 K8s 组件为主题。

### 调度相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/06-apf-troubleshooting|API 优先级与公平性 (APF) 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting|PodDisruptionBudget (PDB) 故障排查指南]]

### 资源管理

- [[domain-07-platform-engineering/03-capacity-planning-resource-assessment|容量规划与资源评估 (Capacity Planning & Resource Assessment)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/01-resources-quota-troubleshooting|资源与调度故障排查指南]]

### Pod调度

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting|Pod 故障排查与运行机制深度指南]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/03-pod-pending|Pod Pending 调度失败诊断与修复]]

### 节点调度

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting|kubelet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|GPU 与设备插件故障排查指南]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，调度与弹性伸缩可参考相关章节。

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/scheduling/kubernetes-scheduler|Kubernetes Scheduler]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taints-and-tolerations|Taints and Tolerations]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-topology-spread-constraints|Pod Topology Spread Constraints]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/scheduling-framework|Scheduling Framework]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/scheduler-performance-tuning|Scheduler Performance Tuning]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/gang-scheduling|Gang Scheduling]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/karpenter-autoscaling|Karpenter 自动扩缩容]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-scheduling-readiness|Pod Scheduling Readiness]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/resource-bin-packing|Resource Bin Packing]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-declared-features|Node Declared Features]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-overhead|Pod Overhead]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-pressure-eviction|Node-pressure Eviction]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/api-initiated-eviction|API-initiated Eviction]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/device-plugins|Device Plugins]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/dynamic-resource-allocation|Dynamic Resource Allocation]]
- [[domain-17-system-foundation/topic-dictionary/security/resource-quotas|Resource Quotas]]
- [[domain-17-system-foundation/topic-dictionary/security/limit-ranges|Limit Ranges]]
