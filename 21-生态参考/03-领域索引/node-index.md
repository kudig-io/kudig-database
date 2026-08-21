---
title: Node 知识图谱索引
description: '## Node 知识图谱'
summary: '## Node 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- node
- kubelet
- node-pool
- containerd
- cri-o
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Node 知识图谱 是什么
- Node 节点 相关文档
trigger_keywords:
- Node
- 知识图谱
- index
- kubelet
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Node 知识图谱索引

> 知识图谱：按关键字 **node** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 节点管理

- 27 - 节点与节点池管理 (Node & NodePool Management)
- [[23-实体/09-编排调度/kubefleet.md|Kubelet 配置与调优]]
- [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|kubelet 深度解析 (kubelet Deep Dive)]]

### 故障排查

- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|06 - 节点 NotReady 状态深度诊断 (Node NotReady Diagnosis)]]
- [[19-故障诊断/02-资源排障/01-node-comprehensive-troubleshooting.md|09 - Node 全面故障排查 (Node Comprehensive Troubleshooting)]]
- [[19-故障诊断/04-高级排障/01-node-component-troubleshooting.md|35 - 节点组件故障排查 (Node Component Troubleshooting)]]

### 结构化故障排查

- [[19-故障诊断/04-高级排障/structural-02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/02-kube-proxy-troubleshooting.md|kube-proxy 故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/03-container-runtime-troubleshooting.md|容器运行时故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/04-node-troubleshooting.md|节点问题专项排查指南]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/05-image-registry-troubleshooting.md|镜像与镜像仓库故障排查指南]]

### YAML 配置参考

- 32 - Lease / Event / Node YAML 配置参考

## 关联文档 (K8s 集成)

### K8s 事件

- 06 - 节点生命周期与状态事件

### 技能卡片

- [[19-故障诊断/08-技能体系/01-node-notready.md|节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复]]

### FTA 故障树

- [[19-故障诊断/06-FTA故障树/list/node-fta.md|Node 异常 FTA 树]]

### 自动扩缩容

- [[19-故障诊断/04-高级排障/structural-07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|Cluster Autoscaler 节点自动扩缩容故障排查指南]]
- Karpenter 节点自动扩展实践指南

## 扩展参考

### 容器运行时生态

- containerd
- CRI-O

### 节点操作系统

- [[17-系统基础/01-Linux/16-k8s-node-os-support-matrix.md|K8s 节点 OS 支持矩阵]]
- [[17-系统基础/01-Linux/17-k8s-node-os-issues.md|K8s 节点 OS 问题全清单]]
- [[23-实体/03-运行时/inclavare-containers.md|inclavare containers]]
- K0s
- k3s

### 硬件与设备管理

- [[19-故障诊断/04-高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]
- Metal3-io
- KubeEdge

### 生产运维

- 19-集群性能调优
- [[12-可靠性/03-容量规划/06-capacity-planning-forecasting.md|24 capacity planning forecasting]]
- K8s FinOps 成本优化实践指南


<!-- risk-assessed -->
