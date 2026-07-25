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
- [[22-概念/15-运行时与系统/docker-architecture.md|Docker 架构概述]]
- [[22-概念/15-运行时与系统/linux-container-foundation.md|Linux 运维基础]]

### 概念速查
- [[23-实体/15-参考与索引/KUDIG Tag Dictionary.md|运维词典]]

---

## intermediate 中级

### 核心技术
- [[23-实体/15-参考与索引/k8s-architecture-domain-guide.md|K8s 架构、核心组件]]
- [[pod-lifecycle|工作负载控制器、Pod 生命周期]]
- [[22-概念/03-网络/service-networking.md|网络架构、Service、DNS]]
- [[22-概念/04-存储/storage-model.md|存储架构、PV、StorageClass]]
- [[22-概念/05-安全/security-defense-depth.md|认证授权、网络安全、运行时安全]]

### 日常运维
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障诊断]]
- [[26-技能/03-节点/node/诊断排障/troubleshoot-node-issues.md|Node 故障诊断]]

---

## advanced 高级

### 深度原理
- [[23-实体/15-参考与索引/k8s-architecture-domain-guide.md|设计原理全系列]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|控制平面深度解析]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Ingress/Gateway API 深度]]
- [[22-概念/06-可观测性/observability-pillars.md|可观测性全系列]]

### 生产实践
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference.md|平台运维全系列]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论体系]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|18 个诊断-修复 Skill]]

---

## expert 专家级

### 源码与架构
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|K8s 源码架构]]
- [[26-技能/02-控制面/crd-operator/运维操作/develop-crd-operator.md|Operator 开发指南]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|控制平面源码级分析]]

### 方法论
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论体系]]

### 前沿技术
- [[22-概念/03-网络/cilium-ebpf-networking.md|eBPF 技术]]
- [[22-概念/09-平台与发布/platform-engineering-idp.md|平台工程]]
- [[23-实体/15-参考与索引/k8s-ai-infra-domain-guide.md|LLM/AI 基础设施]]

---

## 相关索引

- [[23-实体/15-参考与索引/k8s-knowledge-map.md|知识图谱]]
- [[MOC|学习路径导航]]
- [[23-实体/15-参考与索引/KUDIG Tag Dictionary.md|标签索引]]

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/04-存储/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking


<!-- risk-assessed -->
