---
title: 难度分级索引 (Difficulty Index) [metadata]
description: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
summary: '| **中级** | intermediate | 原理理解、日常运维 | 1-2 年经验 |'
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
tier: peripheral
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- 生产运维/topic-learn/: 1 个月学习计划（系统化路径）
- 系统基础/topic-cheat-sheet/k8s.md: [[实体/kubernetes.md|kubernetes]] 速查卡
- 发布变更/topic-deployment/01: 本地 Demo 部署
- 容器运行时/01: Docker 架构概述
- 系统基础/09: Linux 运维基础

### 概念速查
- 系统基础/topic-dictionary/: 运维词典

---

## intermediate 中级

### 核心技术
- 集群基础/01-02: K8s 架构、核心组件
- 工作负载/10-11: 工作负载控制器、Pod 生命周期
- 网络/05,11,16: 网络架构、Service、DNS
- 存储/01-04: 存储架构、PV、StorageClass
- 安全/01-03: 认证授权、网络安全、运行时安全

### 日常运维
- 故障诊断/05-08: Pod/Node 故障诊断
- 清单模式/: YAML 配置参考

---

## advanced 高级

### 深度原理
- domain-2: 设计原理全系列（18 篇）
- 集群基础/11-23: 控制平面深度解析
- 网络/27-42: Ingress/Gateway API 深度
- domain-8: 可观测性全系列

### 生产实践
- domain-9: 平台运维全系列
- domain-18: 生产运维实践
- topic-skills: 18 个诊断-修复 Skill
- 故障诊断/topic-fta/list: 36 个组件故障树

---

## expert 专家级

### 源码与架构
- 集群基础/11: K8s 源码架构
- 集群基础/12: Operator 开发指南
- domain-3: 控制平面源码级分析
- domain-19: 技术白皮书（26 篇）

### 方法论
- 故障诊断/topic-fta/01-23: FTA 方法论体系
- topic-febm: FEBM 取证循证方法论

### 前沿技术
- domain-35: eBPF 技术
- domain-36: 平台工程
- domain-38: WebAssembly
- AI基础设施/15-36: LLM/AI 基础设施

---

> 本索引为手动维护的参考分级，将随 Frontmatter 体系完善后自动化生成


<!-- risk-assessed -->
