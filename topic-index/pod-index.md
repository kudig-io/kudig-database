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
---

# Pod 知识图谱索引

> 知识图谱：按主题 **Pod** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Pod 为主题或直接面向 Pod 运维场景。

### 深度技术

- [容器与 Pod 高级运维模式 (Advanced Pod Patterns)](./domain-4-workloads/12-advanced-pod-patterns.md)
- [原地 Pod 资源调整 (In-Place Pod Resize)](./domain-3-control-plane/29-in-place-pod-resize.md)
- [Pod生命周期事件表](./domain-4-workloads/11-pod-lifecycle-events.md)

### 故障排查

- [Pod Pending 状态深度诊断 (Pod Pending Diagnosis)](./domain-12-troubleshooting/05-pod-pending-diagnosis.md)
- [Pod 全面故障排查 (Pod Comprehensive Troubleshooting)](./domain-12-troubleshooting/08-pod-comprehensive-troubleshooting.md)
- [Pod 故障排查与运行机制深度指南](./topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)

### FTA 故障树

- [Pod 异常 FTA 树](./topic-fta/list/pod-fta.md)

### 技能卡片

- [Pod Pending 调度失败诊断与修复](./topic-skills/03-pod-pending.md)

### YAML 配置

- [Pod 完整规格说明书](./domain-32-yaml-manifests/03-pod-specification-complete.md)
- [Pod Security Standards (PSS/PSA) YAML 配置参考](./domain-32-yaml-manifests/23-pod-security-standards.md)
- [高级 Pod 模式与调度策略 YAML 配置参考](./domain-32-yaml-manifests/35-advanced-pod-patterns.md)

### K8s 事件

- [Pod 与容器生命周期事件](./domain-33-kubernetes-events/02-pod-container-lifecycle-events.md)

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及 Pod 但以其他 K8s 组件为主题。

### 工作负载

- [Deployment 故障排查指南](./topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting.md)
- [StatefulSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md)
- [DaemonSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/04-daemonset-troubleshooting.md)
- [Job 与 CronJob 故障排查指南](./topic-structural-trouble-shooting/05-workloads/05-job-cronjob-troubleshooting.md)
- [ConfigMap 与 Secret 故障排查指南](./topic-structural-trouble-shooting/05-workloads/06-configmap-secret-troubleshooting.md)

### 控制平面

- [kubelet 故障排查指南](./topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting.md)
- [容器运行时故障排查指南](./topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting.md)
- [Scheduler 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md)

### 网络

- [CNI 网络插件故障排查指南](./topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [Service 与 Ingress 故障排查指南](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)

### 存储

- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)

### 安全

- [Pod 安全与 SecurityContext 故障排查指南](./topic-structural-trouble-shooting/06-security-auth/03-pod-security-troubleshooting.md)
- [RBAC 与认证故障排查指南](./topic-structural-trouble-shooting/06-security-auth/01-rbac-troubleshooting.md)

### 调度资源

- [HPA 与 VPA 自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting.md)
- [PodDisruptionBudget (PDB) 故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting.md)

---

## 三、扩展参考

> 以下为 K8s 全域参考，Pod 运维可参考相关章节。

### 节点相关

- [节点故障专项排查指南](./topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting.md)
- [GPU 与设备插件故障排查指南](./topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting.md)

### 术语词典

- [Pods](./topic-dictionary/workloads/pods.md)
- [Pod Lifecycle](./topic-dictionary/workloads/pod-lifecycle.md)
- [Managing Workloads](./topic-dictionary/workloads/managing-workloads.md)
- [Sidecar Containers](./topic-dictionary/workloads/sidecar-containers.md)
- [Vertical Pod Autoscaling](./topic-dictionary/workloads/vertical-pod-autoscaling.md)
