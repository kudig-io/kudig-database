---
title: 难度分级索引 (Difficulty Index)
description: 按难度分级的文档索引，帮助读者选择合适的学习内容
summary: 将知识库内容按 beginner/intermediate/advanced/expert 四级分类，提供渐进式学习路径指引
category: references
tags:
- difficulty-index
- meta
- learning-path
- navigation
tier: supporting
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: beginner
audience:
- 所有工程师
- 新人入职
estimated_read_time: 5min
---

# 难度分级索引 (Difficulty Index)

> 按难度分级的文档索引，帮助读者选择合适的学习内容。难度定义见 frontmatter `difficulty` 字段。

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
- 故障诊断/FTA故障树/list: 36 个组件故障树

---

## expert 专家级

### 源码与架构
- 集群基础/11: K8s 源码架构
- 集群基础/12: Operator 开发指南
- domain-3: 控制平面源码级分析
- domain-19: 技术白皮书（26 篇）

### 方法论
- 故障诊断/FTA故障树/01-23: FTA 方法论体系
- topic-febm: FEBM 取证循证方法论

### 前沿技术
- domain-35: eBPF 技术
- domain-36: 平台工程
- domain-38: WebAssembly
- AI基础设施/15-36: LLM/AI 基础设施

---

## 学习建议

| 当前水平 | 建议起步 | 进阶方向 |
|----------|----------|----------|
| 零基础 | beginner 入门级 + 速查卡 | 4 周学习路径 |
| 1-2 年 | intermediate 核心技术 | 故障诊断实战 |
| 3-5 年 | advanced 生产实践 | FTA/FEBM 方法论 |
| 5+ 年 | expert 源码与架构 | 平台工程 + AI 基础设施 |

---

> 本索引为手动维护的参考分级，将随 Frontmatter 体系完善后自动化生成。
> 难度定义权威源：[[元数据/schema.md|Wiki Schema]] 中的 `difficulty` 字段规范。

## Related

- [[元数据/metadata/knowledge-map.md|知识图谱]] — 模块依赖与学习路径
- [[元数据/metadata/tags-index.md|标签索引]] — 按主题检索
- [[元数据/schema.md|Wiki Schema]] — 难度字段定义
