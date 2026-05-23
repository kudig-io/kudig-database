---
title: Kubernetes Difficulty Index
description: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
category: reference
tags:
- k8s
- difficulty-level
- learning-path
- document-index
- cilium
- docker
- ingress
- gateway
- crd
- operator
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Difficulty Index 是什么
- 如何 Kubernetes Difficulty Index
trigger_keywords:
- Kubernetes
- Difficulty
- Index
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
created: "2026-05-23"
---

# Kubernetes Difficulty Index

> 按难度分级的文档索引，帮助读者选择合适的学习内容

---

## 难度说明

| 级别 | 标识 | 说明 | 适合人群 |
|:---|:---:|:---|:---|
| **入门** | beginner | 基础概念、入门操作 | 初学者、转岗人员 |
| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |
| **高级** | advanced | 深度原理、生产实践 | 3-5 年经验 |
| **专家** | expert | 源码级分析、架构设计 | 5+ 年资深工程师 |

---

## beginner 入门级

### 推荐起步
- [[MOC|1 个月学习计划]]
- [[MOC|Kubernetes 速查卡]]
- [[concepts/docker-architecture|Docker 架构概述]]
- [[concepts/linux-container-foundation|Linux 运维基础]]

### 概念速查
- [[references/KUDIG Tag Dictionary|运维词典]]

---

## intermediate 中级

### 核心技术
- [[references/k8s-architecture-domain-guide|K8s 架构、核心组件]]
- [[pod-lifecycle|工作负载控制器、Pod 生命周期]]
- [[concepts/service-networking|网络架构、Service、DNS]]
- [[concepts/storage-model|存储架构、PV、StorageClass]]
- [[concepts/security-defense-depth|认证授权、网络安全、运行时安全]]

### 日常运维
- [[skills/troubleshoot-pod-issues|Pod 故障诊断]]
- [[skills/troubleshoot-node-issues|Node 故障诊断]]

---

## advanced 高级

### 深度原理
- [[references/k8s-architecture-domain-guide|设计原理全系列]]
- [[concepts/kubernetes-architecture-overview|控制平面深度解析]]
- [[concepts/cilium-ebpf-networking|Ingress/Gateway API 深度]]
- [[concepts/observability-pillars|可观测性全系列]]

### 生产实践
- [[references/kubectl Scenario Quick Reference|平台运维全系列]]
- [[skills/FTA Methodology and Core Principles|FTA 方法论体系]]
- [[skills/Kubernetes Diagnostic Skills Overview|18 个诊断-修复 Skill]]

---

## expert 专家级

### 源码与架构
- [[concepts/kubernetes-architecture-overview|K8s 源码架构]]
- [[skills/develop-crd-operator|Operator 开发指南]]
- [[concepts/kubernetes-architecture-overview|控制平面源码级分析]]

### 方法论
- [[skills/FTA Methodology and Core Principles|FTA 方法论体系]]

### 前沿技术
- [[concepts/cilium-ebpf-networking|eBPF 技术]]
- [[concepts/platform-engineering-idp|平台工程]]
- [[references/k8s-ai-infra-domain-guide|LLM/AI 基础设施]]

---

## 相关索引

- [[references/k8s-knowledge-map|知识图谱]]
- [[MOC|学习路径导航]]
- [[references/KUDIG Tag Dictionary|标签索引]]

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/storage-model|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[concepts/cilium-ebpf-networking|cilium-ebpf-networking]] — Cilium eBPF Networking
