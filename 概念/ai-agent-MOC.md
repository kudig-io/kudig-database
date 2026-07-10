---
title: 02-ai-agents MOC
description: 'summary: "AI Agent — AI 智能体架构、工具调用、Agent 工作流"02-ai-agents MOC""'
summary: 'summary: "AI Agent — AI 智能体架构、工具调用、Agent 工作流"02-ai-agents MOC""'
category: general
tags:
- k8s
- llm
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 02-ai-agents MOC 是什么
- 如何 02-ai-agents MOC
trigger_keywords:
- 02-ai-agents
- MOC
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "02-ai-agents MOC"
category: concepts
summary: "AI Agent — AI 智能体架构、工具调用、Agent 工作流"02-ai-agents MOC""
tags: k8s, ai-agent]
sources: ["AI基础设施/02-ai-agents/MOC.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# 02-ai-agents MOC

> **MOC 版本**: 1.0
> **专题**: 02-ai-agents
> **文档数量**: 57 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

AI Agent — AI 智能体架构、工具调用、Agent 工作流

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | 02-ai-agents |
| **文档数量** | 57 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | AI Agent 基础与核心架构 |  | ai, ai-agent |  |
| 2 | LLM 基座模型选型与评估 |  | ai, ai-agent |  |
| 3 | [[03-agent-frameworks-comparison|主流 Agent 框架深度对比]] |  | ai, ai-agent |  |
| 4 | RAG 检索增强生成深度指南 |  | ai, ai-agent |  |
| 5 | Tool Use & Function Calling 设计规范 |  | ai, ai-agent |  |
| 6 | 多 Agent 编排与协作架构 |  | ai, ai-agent |  |
| 7 | 记忆管理与上下文窗口工程 |  | ai, ai-agent |  |
| 8 | [[08-agent-evaluation-observability|Agent 评测体系与可观测性]] |  | ai, ai-agent, observability |  |
| 9 | [[09-production-deployment-guide|生产部署指南：K8s 上运行 Agent 服务]] |  | ai, ai-agent, deployment |  |
| 10 | 安全护栏、提示注入防护与合规 |  | ai, ai-agent, security |  |
| 11 | 成本与延迟优化策略 |  | ai, ai-agent, cost-optimization |  |
| 12 | 企业级实战案例 |  | ai, ai-agent, case-study |  |
| 13 | [[13-trusted-agent-system-fiscal-plan|可信智能体体系 — 运维智能体财年规划]] |  | ai, ai-agent |  |
| 14 | [[14-agent-kudig-design-strategy|Agent 作为技术赋能新方式：设计思路与落地路径]] |  | ai, ai-agent |  |
| 15 | Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ |  | ai, ai-agent |  |
| 16 | [[16-agentscope-overview-installation|AgentScope 概述与安装入门]] |  | ai, ai-agent, deep-dive |  |
| 17 | AgentScope 核心概念与基础操作 |  | ai, ai-agent |  |
| 18 | AgentScope 工具系统与 MCP 集成 |  | ai, ai-agent |  |
| 19 | AgentScope 记忆管理与上下文工程 |  | ai, ai-agent |  |
| 20 | [[20-agentscope-multi-agent-orchestration|AgentScope 多 Agent 编排与工作流]] |  | ai, ai-agent |  |
| 21 | [[21-agentscope-advanced-features|AgentScope 高级特性与扩展开发]] |  | ai, ai-agent |  |
| 22 | [[22-agentscope-production-deployment|AgentScope 生产部署与可观测性]] |  | ai, ai-agent, deployment |  |
| 23 | Agent CLI 基础概念与架构模式 |  | ai, ai-agent |  |
| 24 | 主流 Agent CLI 工具全景对比 |  | ai, ai-agent |  |
| 25 | Agent CLI 与 MCP 协议深度集成 |  | ai, ai-agent |  |
| 26 | [[26-agent-cli-development-workflow|Agent CLI 开发工作流与最佳实践]] |  | ai, ai-agent |  |
| 27 | [[27-agent-cli-security-governance|Agent CLI 安全治理与权限模型]] |  | ai, ai-agent, security |  |
| 28 | [[28-agent-cli-enterprise-automation|Agent CLI 企业级自动化与 CI/CD 集成]] |  | ai, ai-agent |  |
| 29 | [[29-agentscope-studio-skill-demo|AgentScope Studio 与 Agent Skill 实战指南]] |  | ai, ai-agent, daily-ops |  |
| 30 | Agent Harness 工程：从模型包装到生产级 Agent 系统设计 |  | ai, ai-agent |  |
| 31 | [[31-agent-harness-loop-execution|Agent Harness Loop 与执行引擎深度设计]] |  | ai, ai-agent |  |
| 32 | [[32-agent-harness-tool-engineering|Agent Harness 工具工程：从设计到精简的完整实践]] |  | ai, ai-agent |  |
| 33 | [[33-agent-harness-context-memory|Agent Harness 上下文与记忆工程]] |  | ai, ai-agent |  |
| 34 | [[34-agent-harness-verification-quality|Agent Harness 验证与质量门禁]] |  | ai, ai-agent |  |
| 35 | [[35-agent-harness-security-constraints|Agent Harness 安全与约束工程]] |  | ai, ai-agent, security |  |
| 36 | [[36-agent-harness-observability|Agent Harness 可观测性体系]] |  | ai, ai-agent, observability |  |
| 37 | Agent Harness 多 Agent 编排 |  | ai, ai-agent |  |
| 38 | [[38-agent-harness-performance-cost|Agent Harness 性能与成本优化]] |  | ai, ai-agent, performance |  |
| 39 | [[39-agent-harness-testing-benchmark|Agent Harness 测试与基准评测]] |  | ai, ai-agent, performance |  |
| 40 | [[40-agent-harness-production-maturity|Agent Harness 生产运维与成熟度模型]] |  | ai, ai-agent, production |  |
| 41 | [[41-react-harness-identification-guide|ReAct Agent 与 Harness 识别指南]] |  | ai, ai-agent, guide |  |
| 42 | [[42-model-harness-compatibility-matrix|模型 × Harness 兼容性矩阵（2025-2026）]] |  | ai, ai-agent |  |
| 43 | [[43-openclaw-framework-integration|OpenClaw File-First 架构与 Agent Harness 集成指南]] |  | ai, ai-agent |  |
| 44 | OpenClaw SOUL 机制深度解析 |  | ai, ai-agent |  |
| 45 | OpenClaw USER 机制深度解析 |  | ai, ai-agent |  |
| 46 | OpenClaw AGENTS 机制深度解析 |  | ai, ai-agent |  |
| 47 | OpenClaw TOOLS 机制深度解析 |  | ai, ai-agent |  |
| 48 | OpenClaw SKILL 机制深度解析 |  | ai, ai-agent, daily-ops |  |
| 49 | OpenClaw MEMORY 机制深度解析 |  | ai, ai-agent |  |
| 50 | [[50-openclaw-identity-mechanism|OpenClaw IDENTITY 机制深度解析]] |  | ai, ai-agent |  |
| ... | 共 57 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 57 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## 相关概念

- AI Agent 基础
- LLM 基础模型

## Related

- [[skill-k8s-node-notready-SKILL]] — Skill
- [[deployment]] — Deployment
- [[概念/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference

## 相关合成分析

- [[概念/gpu-scheduling-ai-workloads.md|gpu-scheduling-ai-workloads]]


<!-- risk-assessed -->
