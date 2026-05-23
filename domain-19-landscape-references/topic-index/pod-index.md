---
title: Pod 知识图谱索引
description: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- pod
- container
- workload
- kubelet
- scheduler
- hpa
- vpa
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Pod 知识图谱索引 是什么
- 如何 Pod 知识图谱索引
trigger_keywords:
- Pod
- 知识图谱
- container
- workload
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
created: "2026-05-23"
---

# Pod 知识图谱索引

> 知识图谱：按主题 **Pod** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Pod 为主题或直接面向 Pod 运维场景。

### 深度技术

- 容器与 Pod 高级运维模式 (Advanced Pod Patterns)
- 原地 Pod 资源调整 (In-Place Pod Resize)
- Pod生命周期事件表

### 故障排查

- [[domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis|[[Pod Pending 状态深度诊断|Pod Pending 状态深度诊断]] (Pod Pending Diagnosis)]]
- troubleshooting|Pod 全面故障排查 (Pod Comprehensive Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting|[[Pod 故障排查与运行机制深度指南|Pod 故障排查与运行机制深度指南]]]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta|Pod 异常 FTA 树]]

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/03-pod-pending|[[Pod Pending 调度失败诊断与修复|Pod Pending 调度失败诊断与修复]]]]

### YAML 配置

- Pod 完整规格说明书
- Pod Security Standards (PSS/PSA) YAML 配置参考
- 高级 Pod 模式与调度策略 YAML 配置参考

### K8s 事件

- Pod 与容器生命周期事件

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及 Pod 但以其他 K8s 组件为主题。

### 工作负载

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting|Deployment 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting|StatefulSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/04-daemonset-troubleshooting|DaemonSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/05-job-cronjob-troubleshooting|Job 与 CronJob 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/06-configmap-secret-troubleshooting|[[ConfigMap 与 Secret 故障排查指南|ConfigMap 与 Secret 故障排查指南]]]]

### 控制平面

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting|kubelet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting|容器运行时故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting|Scheduler 故障排查指南]]

### 网络

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|[[Service 与 Ingress 故障排查指南|Service 与 Ingress 故障排查指南]]]]

### 存储

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting|PV/PVC 存储深度排查与持久化治理指南]]

### 安全

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/06-security-auth/03-pod-security-troubleshooting|[[Pod 安全与 SecurityContext 故障排查指南|Pod 安全与 SecurityContext 故障排查指南]]]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/06-security-auth/01-rbac-troubleshooting|RBAC 与认证故障排查指南]]

### 调度资源

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting|[[PodDisruptionBudget (PDB) 故障排查指南|PodDisruptionBudget (PDB) 故障排查指南]]]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，Pod 运维可参考相关章节。

### 节点相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting|节点故障专项排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|GPU 与设备插件故障排查指南]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/workloads/pods|Pods]]
- [[domain-17-system-foundation/topic-dictionary/workloads/pod-lifecycle|Pod Lifecycle]]
- [[domain-17-system-foundation/topic-dictionary/workloads/managing-workloads|Managing Workloads]]
- [[domain-17-system-foundation/topic-dictionary/workloads/sidecar-containers|Sidecar Containers]]
- [[domain-17-system-foundation/topic-dictionary/workloads/vertical-pod-autoscaling|[[Vertical Pod Autoscaling|Vertical Pod Autoscaling]]]]
