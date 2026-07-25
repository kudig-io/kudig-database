---
title: KUDIG Agent 规范集：工单分类、知识图谱、会话管理与诊断基准
description: '### 会话上下文管理'
summary: '### 会话上下文管理'
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
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

### Release SRE 规范副本

| 规范 | Release 副本 |
|---|---|
| CLAUDE Agent Context | [[release-sre/metadata/agent-specs/CLAUDE.md]] |
| GEMINI Agent Context | [[release-sre/metadata/agent-specs/GEMINI.md]] |
| P0-1 工单分类与意图识别 | [[release-sre/metadata/agent-specs/P0-1-ticket-classification-intent-recognition.md]] |
| P0-2 多技能协同协议 | [[release-sre/metadata/agent-specs/P0-2-multi-skill-coordination-protocol.md]] |
| P0-3 会话上下文管理 | [[release-sre/metadata/agent-specs/P0-3-session-context-management.md]] |
| P0 知识图谱 RDF 模型 | [[release-sre/metadata/agent-specs/P0-Knowledge-Graph-RDF-Model.md]] |
| P0 工具 Schema 定义 | [[release-sre/metadata/agent-specs/P0-Tool-Schema-Definition.md]] |
| P1-4 决策树 Mermaid 规范 | [[release-sre/metadata/agent-specs/P1-4-Decision-Tree-Mermaid-Spec.md]] |
| P1-4 决策树可视化 | [[release-sre/metadata/agent-specs/P1-4-decision-tree-mermaid-visualization.md]] |
| P1-5 OnCall 速查卡 | [[release-sre/metadata/agent-specs/P1-5-oncall-quick-reference-card.md]] |
| P1-6 告警到工单闭环 | [[release-sre/metadata/agent-specs/P1-6-alert-to-ticket-resolution-loop.md]] |
| P1-7 反思机制 | [[release-sre/metadata/agent-specs/P1-7-Reflection-Mechanism.md]] |
| P1-8 诊断基准测试 | [[release-sre/metadata/agent-specs/P1-8-Agent-Diagnostic-Benchmark.md]] |
| P2-7 AI/ML 工作负载排障 | [[release-sre/metadata/agent-specs/P2-7-ai-ml-workloads-troubleshooting.md]] |
| P2-8 数据库中间件排障 | [[release-sre/metadata/agent-specs/P2-8-database-middleware-troubleshooting.md]] |
| P2-9 非 K8s 基础设施排障 | [[release-sre/metadata/agent-specs/P2-9-non-k8s-infrastructure-troubleshooting.md]] |
| P3-10 云厂商专项排障 | [[release-sre/metadata/agent-specs/P3-10-cloud-vendor-specific-troubleshooting.md]] |
| P3-11 安全事件 SOP | [[release-sre/metadata/agent-specs/P3-11-security-incident-sop-compliance-checklist.md]] |
| P3-12 多集群联邦排障 | [[release-sre/metadata/agent-specs/P3-12-multi-cluster-federation-troubleshooting.md]] |
| Obsidian Wiki Agent Context | [[release-sre/metadata/agent-specs/obsidian-wiki-agent-context.md]] |

## Agent 规范使用指南

### Agent 类型分类

| Agent 类型 | 职责 | 触发条件 |
|---|---|---|
| 诊断 Agent | 故障排查 | 告警触发/手动请求 |
| 修复 Agent | 执行修复 | 诊断完成后 |
| 巡检 Agent | 日常检查 | 定时触发 |
| 学习 Agent | 知识问答 | 用户提问 |

### Agent 协作流程

```
用户请求/告警触发
    ↓
诊断 Agent (FTA 分析)
    ↓
修复 Agent (执行 Playbook)
    ↓
验证 Agent (确认恢复)
    ↓
归档 Agent (记录案例)
```

### Agent 安全约束

- 高风险操作需人工审批
- 操作范围限制在指定 namespace
- 所有操作记录审计日志
- 支持一键回滚

## 面试要点

1. **Q：AI Agent 在运维中的应用场景？**
   A：智能诊断、自动修复、日常巡检、知识问答、变更辅助、容量预测。

2. **Q：如何保证 Agent 操作的安全性？**
   A：权限最小化、操作审批、审计日志、回滚机制、范围限制、人工确认。

3. **Q：Agent 协作的设计原则？**
   A：单一职责、明确接口、可观测、可回滚、优雅降级、人工介入点。

## Related

- [[23-实体/15-参考与索引/k8s-knowledge-map.md|k8s-knowledge-map]] — Kubernetes Knowledge Map
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[cilium]] — Cilium
- [[submariner]] — Submariner


<!-- risk-assessed -->
