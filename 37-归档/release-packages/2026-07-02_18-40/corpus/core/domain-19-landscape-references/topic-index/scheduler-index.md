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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Scheduler 调度与弹性伸缩知识图谱索引

> 知识图谱：按主题 **Scheduler & 弹性伸缩** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Scheduler 或弹性伸缩为主题或直接面向调度运维场景。

### 深度技术

- [[domain-17-system-foundation/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler 深度解析 (Kube-Scheduler Deep Dive)]]
- 调度器配置与优化
- 动态资源分配 ([[domain-17-system-foundation/知识字典/scheduling/dynamic-resource-allocation.md|dynamic-resource-allocation]]
- HPA/VPA 自动伸缩配置
- 集群容量规划
- 资源管理表

### 故障排查

- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/02-hpa-vpa-troubleshooting|HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/10-quota-limitrange-troubleshooting|Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/02-cluster-autoscaler-troubleshooting|集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/06-performance-bottleneck-troubleshooting|性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|Cluster Autoscaler 节点自动扩缩容故障排查指南]]

### 技能卡片

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/topic-skills/02-autoscaling-failure|HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation]]

### YAML 配置

- Namespace / ResourceQuota / LimitRange YAML 配置参考
- PriorityClass / RuntimeClass YAML 配置参考
- HorizontalPodAutoscaler v2 YAML 配置参考

### K8s 事件

- 调度与抢占事件
- 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)

### 技术论文

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-19-landscape-references/02-papers/05-kubernetes-scheduler-deep-optimization-custom-scheduling|12 kubernetes scheduler deep optimization custom scheduling]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及调度与弹性伸缩但以其他 K8s 组件为主题。

### 调度相关

- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/06-apf-troubleshooting.md|API 优先级与公平性 (APF) 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|PodDisruptionBudget (PDB) 故障排查指南]]

### 资源管理

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-07-platform-engineering/governance/01-capacity-planning-resource-assessment|03 capacity planning resource assessment]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/01-resources-quota-troubleshooting.md|资源与调度故障排查指南]]

### Pod调度

- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]]
- [[domain-10-troubleshooting-diagnostics/技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]

### 节点调度

- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，调度与弹性伸缩可参考相关章节。

### 术语词典

- [[domain-17-system-foundation/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]]
- [[domain-17-system-foundation/知识字典/scheduling/pod-priority-and-preemption.md|Pod Priority and Preemption]]
- [[domain-17-system-foundation/知识字典/scheduling/taints-and-tolerations.md|Taints and Tolerations]]
- [[domain-17-system-foundation/知识字典/scheduling/pod-topology-spread-constraints.md|Pod Topology Spread Constraints]]
- [[domain-17-system-foundation/知识字典/scheduling/scheduling-framework.md|Scheduling Framework]]
- [[domain-17-system-foundation/知识字典/scheduling/scheduler-performance-tuning.md|Scheduler Performance Tuning]]
- [[domain-17-system-foundation/知识字典/scheduling/gang-scheduling.md|Gang Scheduling]]
- [[domain-17-system-foundation/知识字典/scheduling/karpenter-autoscaling.md|Karpenter 自动扩缩容]]
- [[domain-17-system-foundation/知识字典/scheduling/assigning-pods-to-nodes.md|Assigning Pods to Nodes]]
- [[domain-17-system-foundation/知识字典/scheduling/pod-scheduling-readiness.md|Pod Scheduling Readiness]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]]
- [[domain-17-system-foundation/知识字典/scheduling/node-declared-features.md|Node Declared Features]]
- [[domain-17-system-foundation/知识字典/scheduling/pod-overhead.md|Pod Overhead]]
- [[domain-17-system-foundation/知识字典/scheduling/node-pressure-eviction.md|Node-pressure Eviction]]
- [[domain-17-system-foundation/知识字典/scheduling/api-initiated-eviction.md|API-initiated Eviction]]
- [[domain-17-system-foundation/知识字典/platform-engineering/device-plugins.md|Device Plugins]]
- [[domain-17-system-foundation/知识字典/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]]
- [[domain-17-system-foundation/知识字典/security/resource-quotas.md|Resource Quotas]]
- [[domain-17-system-foundation/知识字典/security/limit-ranges.md|Limit Ranges]]


<!-- risk-assessed -->
