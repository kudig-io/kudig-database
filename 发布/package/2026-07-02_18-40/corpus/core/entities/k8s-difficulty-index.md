---
title: Kubernetes Difficulty Index
description: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
summary: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- [[concepts/docker-architecture.md|Docker 架构概述]]
- [[concepts/linux-container-foundation.md|Linux 运维基础]]

### 概念速查
- [[entities/KUDIG Tag Dictionary.md|运维词典]]

---

## intermediate 中级

### 核心技术
- [[entities/k8s-architecture-domain-guide.md|K8s 架构、核心组件]]
- [[pod-lifecycle|工作负载控制器、Pod 生命周期]]
- [[concepts/service-networking.md|网络架构、Service、DNS]]
- [[concepts/storage-model.md|存储架构、PV、StorageClass]]
- [[concepts/security-defense-depth.md|认证授权、网络安全、运行时安全]]

### 日常运维
- [[skills/troubleshoot-pod-issues.md|Pod 故障诊断]]
- [[skills/troubleshoot-node-issues.md|Node 故障诊断]]

---

## advanced 高级

### 深度原理
- [[entities/k8s-architecture-domain-guide.md|设计原理全系列]]
- [[concepts/kubernetes-architecture-overview.md|控制平面深度解析]]
- [[concepts/cilium-ebpf-networking.md|Ingress/Gateway API 深度]]
- [[concepts/observability-pillars.md|可观测性全系列]]

### 生产实践
- [[entities/kubectl Scenario Quick Reference.md|平台运维全系列]]
- [[skills/FTA Methodology and Core Principles.md|FTA 方法论体系]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|18 个诊断-修复 Skill]]

---

## expert 专家级

### 源码与架构
- [[concepts/kubernetes-architecture-overview.md|K8s 源码架构]]
- [[skills/develop-crd-operator.md|Operator 开发指南]]
- [[concepts/kubernetes-architecture-overview.md|控制平面源码级分析]]

### 方法论
- [[skills/FTA Methodology and Core Principles.md|FTA 方法论体系]]

### 前沿技术
- [[concepts/cilium-ebpf-networking.md|eBPF 技术]]
- [[concepts/platform-engineering-idp.md|平台工程]]
- [[entities/k8s-ai-infra-domain-guide.md|LLM/AI 基础设施]]

---

## 相关索引

- [[entities/k8s-knowledge-map.md|知识图谱]]
- [[MOC|学习路径导航]]
- [[entities/KUDIG Tag Dictionary.md|标签索引]]

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking


<!-- risk-assessed -->
