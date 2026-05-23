---
title: Node 知识图谱索引
description: '## Node 知识图谱'
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
created: "2026-05-23"
---

# Node 知识图谱索引

> 知识图谱：按关键字 **node** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 节点管理

- 27 - 节点与节点池管理 (Node & NodePool Management)
- [[entities/kubefleet|Kubelet 配置与调优]]]]
- [[domain-01-cluster-fundamentals/03-control-plane/15-kubelet-deep-dive|kubelet 深度解析 (kubelet Deep Dive)]]]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis|06 - [[[[节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation|节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation]] — 数字人播报脚本|Node NotReady]]dy 状态深度诊断|Node NotReady 状态深度诊断]] (Node NotReady Diagnosis)]]
- [[domain-10-troubleshooting-diagnostics/09-node-comprehensive-troubleshooting|09 - Node 全面故障排查 (Node Comprehensive Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/35-node-component-troubleshooting|35 - 节点组件故障排查 (Node Component Troubleshooting)]]

### 结构化故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting|kubelet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/02-kube-proxy-troubleshooting|kube-proxy 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting|容器运行时故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting|节点问题专项排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/05-image-registry-troubleshooting|镜像与镜像仓库故障排查指南]]

### YAML 配置参考

- 32 - Lease / Event / Node YAML 配置参考

## 关联文档 (K8s 集成)

### K8s 事件

- 06 - 节点生命周期与状态事件

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/01-node-notready|节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL|K8s Node NotReady 诊断与修复]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta|Node 异常 FTA 树]]

### 自动扩缩容

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/03-cluster-autoscaler-troubleshooting|[[Cluster Autoscaler 节点自动扩缩容故障排查指南|Cluster Autoscaler 节点自动扩缩容故障排查指南]]]]
- Karpenter 节点自动扩展实践指南

## 扩展参考

### 容器运行时生态

- containerd
- CRI-O

### 节点操作系统

- [[entities/inclavare-containers]]
- K0s
- k3s

### 硬件与设备管理

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|GPU 与设备插件故障排查指南]]
- Metal3-io
- KubeEdge

### 生产运维

- 19-集群性能调优
- [[domain-09-reliability-engineering/03-capacity-planning/24-capacity-planning-forecasting]]
- K8s FinOps 成本优化实践指南
