---
title: 难度分级索引 (Difficulty Index)
description: 'description: ''| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'''
category: general
tags:
- meta
- docker
- ingress
- gateway
- operator
- ebpf
- llm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 难度分级索引 (Difficulty Index) 是什么
- 如何 难度分级索引 (Difficulty Index)
trigger_keywords:
- 难度分级索引
- Difficulty
- Index
prerequisites:
- kubectl-basics
- ebpf-basics
---

title: 难度分级索引 (Difficulty Index)
description: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
category: general
tags:
- k8s
- docker
- ingress
- gateway
- operator
- ebpf
- llm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 难度分级索引 (Difficulty Index) 是什么
- 如何 难度分级索引 (Difficulty Index)
trigger_keywords:
- 难度分级索引
- Difficulty
- Index
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
sources: []
created: '2026-05-21'
updated: '2026-05-21'
---
# 难度分级索引 (Difficulty Index)

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
- domain-11-production-operations/topic-learn/: 1 个月学习计划（系统化路径）
- domain-17-system-foundation/topic-cheat-sheet/k8s.md: Kubernetes 速查卡
- domain-08-release-change-management/topic-deployment/01: 本地 Demo 部署
- domain-13-container-runtime/01: Docker 架构概述
- domain-17-system-foundation/09: Linux 运维基础

### 概念速查
- domain-17-system-foundation/topic-dictionary/: 运维词典

---

## intermediate 中级

### 核心技术
- domain-01-cluster-fundamentals/01-02: K8s 架构、核心组件
- domain-02-workloads-applications/10-11: 工作负载控制器、Pod 生命周期
- domain-03-networking-traffic/05,11,16: 网络架构、Service、DNS
- domain-04-storage-data/01-04: 存储架构、PV、StorageClass
- domain-05-security-compliance/01-03: 认证授权、网络安全、运行时安全

### 日常运维
- domain-10-troubleshooting-diagnostics/05-08: Pod/Node 故障诊断
- domain-18-manifests-patterns/: YAML 配置参考

---

## advanced 高级

### 深度原理
- domain-2: 设计原理全系列（18 篇）
- domain-01-cluster-fundamentals/11-23: 控制平面深度解析
- domain-03-networking-traffic/27-42: Ingress/Gateway API 深度
- domain-8: 可观测性全系列

### 生产实践
- domain-9: 平台运维全系列
- domain-18: 生产运维实践
- topic-skills: 18 个诊断-修复 Skill
- domain-10-troubleshooting-diagnostics/topic-fta/list: 36 个组件故障树

---

## expert 专家级

### 源码与架构
- domain-01-cluster-fundamentals/11: K8s 源码架构
- domain-01-cluster-fundamentals/12: Operator 开发指南
- domain-3: 控制平面源码级分析
- domain-19: 技术白皮书（26 篇）

### 方法论
- domain-10-troubleshooting-diagnostics/topic-fta/01-23: FTA 方法论体系
- topic-febm: FEBM 取证循证方法论

### 前沿技术
- domain-35: eBPF 技术
- domain-36: 平台工程
- domain-38: WebAssembly
- domain-14-ai-ml-infra/15-36: LLM/AI 基础设施

---

> 本索引为手动维护的参考分级，将随 Frontmatter 体系完善后自动化生成

---

## Obsidian 相关文档

- [[metadata/knowledge-map.md|知识图谱 (Knowledge Map)]]
- [[metadata/README.md|元数据索引 (Metadata)]]
- [[metadata/tags-index.md|标签索引 (Tags Index)]]
