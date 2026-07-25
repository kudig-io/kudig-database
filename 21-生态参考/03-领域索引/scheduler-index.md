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

- [[17-系统基础/06-知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler 深度解析 (Kube-Scheduler Deep Dive)]]
- 调度器配置与优化
- 动态资源分配 ([[17-系统基础/06-知识字典/scheduling/dynamic-resource-allocation.md|dynamic-resource-allocation]]
- HPA/VPA 自动伸缩配置
- 集群容量规划
- 资源管理表

### 故障排查

- [[19-故障诊断/04-高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]
- [[19-故障诊断/02-资源排障/17-hpa-vpa-troubleshooting.md|HPA/VPA 故障排查 (HPA/VPA Troubleshooting)]]
- [[19-故障诊断/02-资源排障/24-quota-limitrange-troubleshooting.md|Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)]]
- [[19-故障诊断/03-基础设施排障/28-cluster-autoscaler-troubleshooting.md|集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)]]
- [[19-故障诊断/03-基础设施排障/33-performance-bottleneck-troubleshooting.md|性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)]]
- [[19-故障诊断/04-高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[19-故障诊断/04-高级排障/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|Cluster Autoscaler 节点自动扩缩容故障排查指南]]

### 技能卡片

- [[19-故障诊断/08-技能体系/12-autoscaling-failure.md|HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation]]

### YAML 配置

- Namespace / ResourceQuota / LimitRange YAML 配置参考
- PriorityClass / RuntimeClass YAML 配置参考
- HorizontalPodAutoscaler v2 YAML 配置参考

### K8s 事件

- 调度与抢占事件
- 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)

### 技术论文

- [[21-生态参考/02-论文/12-kubernetes-scheduler-deep-optimization-custom-scheduling.md|12 kubernetes scheduler deep optimization custom scheduling]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及调度与弹性伸缩但以其他 K8s 组件为主题。

### 调度相关

- [[19-故障诊断/04-高级排障/01-control-plane/06-apf-troubleshooting.md|API 优先级与公平性 (APF) 故障排查指南]]
- [[19-故障诊断/04-高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|PodDisruptionBudget (PDB) 故障排查指南]]

### 资源管理

- [[10-平台工程/03-治理/03-capacity-planning-resource-assessment.md|03 capacity planning resource assessment]]
- [[19-故障诊断/04-高级排障/07-resources-scheduling/01-resources-quota-troubleshooting.md|资源与调度故障排查指南]]

### Pod调度

- [[19-故障诊断/04-高级排障/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]]
- [[19-故障诊断/08-技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]

### 节点调度

- [[19-故障诊断/04-高级排障/02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]]
- [[19-故障诊断/04-高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，调度与弹性伸缩可参考相关章节。

### 术语词典

- [[17-系统基础/06-知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]]
- [[17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption.md|Pod Priority and Preemption]]
- [[17-系统基础/06-知识字典/scheduling/taints-and-tolerations.md|Taints and Tolerations]]
- [[17-系统基础/06-知识字典/scheduling/pod-topology-spread-constraints.md|Pod Topology Spread Constraints]]
- [[17-系统基础/06-知识字典/scheduling/scheduling-framework.md|Scheduling Framework]]
- [[17-系统基础/06-知识字典/scheduling/scheduler-performance-tuning.md|Scheduler Performance Tuning]]
- [[17-系统基础/06-知识字典/scheduling/gang-scheduling.md|Gang Scheduling]]
- [[17-系统基础/06-知识字典/scheduling/karpenter-autoscaling.md|Karpenter 自动扩缩容]]
- [[17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes.md|Assigning Pods to Nodes]]
- [[17-系统基础/06-知识字典/scheduling/pod-scheduling-readiness.md|Pod Scheduling Readiness]]
- [[17-系统基础/06-知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]]
- [[17-系统基础/06-知识字典/scheduling/node-declared-features.md|Node Declared Features]]
- [[17-系统基础/06-知识字典/scheduling/pod-overhead.md|Pod Overhead]]
- [[17-系统基础/06-知识字典/scheduling/node-pressure-eviction.md|Node-pressure Eviction]]
- [[17-系统基础/06-知识字典/scheduling/api-initiated-eviction.md|API-initiated Eviction]]
- [[17-系统基础/06-知识字典/platform-engineering/device-plugins.md|Device Plugins]]
- [[17-系统基础/06-知识字典/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]]
- [[17-系统基础/06-知识字典/security/resource-quotas.md|Resource Quotas]]
- [[17-系统基础/06-知识字典/security/limit-ranges.md|Limit Ranges]]


<!-- risk-assessed -->
