---
title: 附录 B：工具与资源清单
description: '**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- llm
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 附录 B：工具与资源清单 是什么
- 如何 附录 B：工具与资源清单
- 附录 B：工具与资源清单 根因分析
- 附录 B：工具与资源清单 故障树
trigger_keywords:
- 附录
- B：工具与资源清单
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
---

# 附录 B：工具与资源清单

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一附录**: [附录 A：FTA 术语表](./appendix-a-glossary.md)  
> **下一附录**: [附录 C：参考文献](./appendix-c-references.md)

---

## FTA 建模工具

| 工具名称 | 类型 | 特点 | 获取方式 |
|---------|------|------|---------|
| OpenFTA | 开源 | 免费、基础 FTA 功能 | https://github.com/open-fta |
| CAFTA (EPRI) | 商业 | 核工业级定量分析 | EPRI 授权 |
| Relyence FTA | SaaS | 云平台、团队协作 | https://www.relyence.com |
| PTC Windchill FTA | 商业 | PLM 集成 | PTC 授权 |
| Isograph FaultTree+ | 商业 | 全功能 FTA/ETA | https://www.isograph.com |

## 知识图谱工具

| 工具名称 | 类型 | 用途 |
|---------|------|------|
| Neo4j Community | 开源 | FTA 图数据库存储 |
| NetworkX (Python) | 开源 | FTA 图算法分析 |
| Cypher Query Language | 查询语言 | Neo4j 图查询 |
| Apache TinkerPop/Gremlin | 开源 | 通用图数据库查询 |

## Agent 开发框架

| 框架 | 特点 | 适用场景 |
|------|------|---------|
| LangGraph | 有状态多 Agent 编排 | 复杂 FTA 导航 |
| CrewAI | 角色化 Agent 团队 | 多领域协作诊断 |
| AutoGen (Microsoft) | 多 Agent 对话 | LLM 增强推理 |
| Semantic Kernel | .NET + Python 框架 | 企业级 Agent |

## 混沌工程工具

| 工具 | 平台 | 特点 |
|------|------|------|
| Chaos Mesh | [[entities/kubernetes|kubernetes]] | CNCF 项目，K8s 原生 |
| Litmus | Kubernetes | CNCF 项目，ChaosHub |
| Gremlin | 多平台 | SaaS，企业级 |
| AWS FIS | AWS | AWS 原生故障注入 |

---

> **导航**: [<< 附录 A - FTA 术语表](./appendix-a-glossary.md) | [附录 C - 参考文献 >>](./appendix-c-references.md)
