---
title: Pod 知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断 (Pod Pending Diagnosis)]]
- Pod 全面故障排查 (Pod Comprehensive Troubleshooting)
- [[19-故障诊断/04-高级排障/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]]

### FTA 故障树

- [[19-故障诊断/06-FTA故障树/list/pod-fta.md|Pod 异常 FTA 树]]

### 技能卡片

- [[19-故障诊断/08-技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]

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

- [[19-故障诊断/04-高级排障/05-workloads/02-deployment-troubleshooting.md|Deployment 故障排查指南]]
- [[19-故障诊断/04-高级排障/05-workloads/03-statefulset-troubleshooting.md|StatefulSet 故障排查指南]]
- [[19-故障诊断/04-高级排障/05-workloads/04-daemonset-troubleshooting.md|DaemonSet 故障排查指南]]
- [[19-故障诊断/04-高级排障/05-workloads/05-job-cronjob-troubleshooting.md|Job 与 CronJob 故障排查指南]]
- [[19-故障诊断/04-高级排障/05-workloads/06-configmap-secret-troubleshooting.md|ConfigMap 与 Secret 故障排查指南]]

### 控制平面

- [[19-故障诊断/04-高级排障/02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]]
- [[19-故障诊断/04-高级排障/02-node-components/03-container-runtime-troubleshooting.md|容器运行时故障排查指南]]
- [[19-故障诊断/04-高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]

### 网络

- [[19-故障诊断/04-高级排障/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[19-故障诊断/04-高级排障/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]]

### 存储

- [[19-故障诊断/04-高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]

### 安全

- [[19-故障诊断/04-高级排障/06-security-auth/03-pod-security-troubleshooting.md|Pod 安全与 SecurityContext 故障排查指南]]
- [[19-故障诊断/04-高级排障/06-security-auth/01-rbac-troubleshooting.md|RBAC 与认证故障排查指南]]

### 调度资源

- [[19-故障诊断/04-高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[19-故障诊断/04-高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|PodDisruptionBudget (PDB) 故障排查指南]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，Pod 运维可参考相关章节。

### 节点相关

- [[19-故障诊断/04-高级排障/02-node-components/04-node-troubleshooting.md|节点问题专项排查指南]]
- [[19-故障诊断/04-高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]

### 术语词典

- [[17-系统基础/06-知识字典/workloads/pods.md|Pods]]
- [[17-系统基础/06-知识字典/workloads/pod-lifecycle.md|Pod Lifecycle]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads.md|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/sidecar-containers.md|Sidecar Containers]]
- [[17-系统基础/06-知识字典/workloads/vertical-pod-autoscaling.md|Vertical Pod Autoscaling]]


<!-- risk-assessed -->
