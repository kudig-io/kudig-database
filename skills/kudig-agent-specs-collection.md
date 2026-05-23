---
title: KUDIG Agent 规范集：工单分类、知识图谱、会话管理与诊断基准
description: '### 会话上下文管理'
category: reference
tags:
- k8s
- agent-spec
- ticket-classification
- knowledge-graph
- session-management
- diagnostic-benchmark
- cilium
- redis
- mysql
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Agent 规范集：工单分类、知识图谱、会话管理与诊断基准 是什么
- 如何 KUDIG Agent 规范集：工单分类、知识图谱、会话管理与诊断基准
trigger_keywords:
- KUDIG
- Agent
- 规范集：工单分类
- 知识图谱
- 会话管理与诊断基准
prerequisites:
- kubectl-basics
- cilium-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# KUDIG Agent 规范集

## P0 级规范（核心能力）

### 工单分类与意图识别
- 7 大类工单分类体系（Pod/Node/Network/Storage/Security/ControlPlane/Platform）
- 意图路由规则：关键词 → 知识域 → FTA 故障树
- 语料库覆盖 200+ 常见工单模式

### 知识图谱 RDF 模型
- 命名空间：`kudig:` (`https://kudig.io/ontology/`)
- 核心类：Domain, Topic, Document, [[SKILL|Skill]], FTA, Entity
- 属性：depends_on, related_to, uses, implements
- 支持 SPARQL 查询跨域推理

### 多技能协同协议
- 多个 Skill 之间的编排与协作机制
- 支持串行/并行/条件分支执行
- 上下文传递与结果聚合

### 会话上下文管理
- 会话状态存储与检索
- 多轮对话中的上下文维护
- 工单关联的诊断历史记录

### 工具 Schema 定义
- 统一的工具调用接口规范
- Function Calling 参数验证
- 错误处理与重试策略

## P1 级规范（增强能力）

### 决策树可视化
- Mermaid 格式的诊断决策树
- 支持交互式路径选择
- 覆盖 Top 20 问题场景

### OnCall 快速参考卡
- 一页纸快速排障指南
- 高频命令速查表
- 告警响应 SOP

### 告警到工单闭环
- Alert → Ticket → Diagnosis → Fix → Verify 流程
- 自动化程度分级（L1-L4）
- SLA 指标追踪

### 反思机制（Reflection）
- Agent 自我评估诊断质量
- 从错误中学习改进
- 知识库反馈循环

### 诊断基准测试
- 标准化测试场景集
- 准确率/召回率/响应时间指标
- 持续改进评估

## P2/P3 级规范（扩展能力）

### AI/ML 工作负载排障
- GPU 问题、训练任务失败、推理延迟等场景

### 数据库中间件排障
- MySQL/PostgreSQL/Redis 在 K8s 上的常见问题

### 非 K8s 基础设施排障
- 负载均衡器、CDN、DNS 等基础设施问题

### 云厂商专项排障
- AWS/Azure/GCP/阿里云特有问题与解决方案

### 安全事件 SOP
- 安全事件响应流程
- 合规检查清单

### 多集群联邦排障
- Cluster API、Submariner、Cilium ClusterMesh 等

---

> 来源：docs/agent-specs/P0-*.md, P1-*.md, P2-*.md, P3-*.md（共 17 篇）

## Related

- [[references/k8s-knowledge-map.md|k8s-knowledge-map]] — Kubernetes Knowledge Map
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[cilium]] — Cilium
- [[submariner]] — Submariner
