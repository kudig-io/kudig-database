---
title: 附录 B：工具与资源清单 [domain-10-troubleshooting-diagnostics]
description: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
category: fta
tags:
- fta
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
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 附录 B：工具与资源清单 故障排查
- 附录 B：工具与资源清单 排障步骤
- 附录 B：工具与资源清单 根因分析
trigger_keywords:
- 附录
- B：工具与资源清单
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
fta_id: FTA-APPENDIX_B_TOOLS_AND_RESOURCES-001
component: Appendix B Tools And Resources
severity: high
created: "2026-05-23"
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
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# 附录 B：工具与资源清单

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一附录**: 附录 A：FTA 术语表](./appendix-a-glossary.md)  
> **下一附录**: 附录 C：参考文献](./appendix-c-references.md)

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
| Chaos Mesh | Kubernetes | CNCF 项目，K8s 原生 |
| Litmus | Kubernetes | CNCF 项目，ChaosHub |
| Gremlin | 多平台 | SaaS，企业级 |
| AWS FIS | AWS | AWS 原生故障注入 |

---

> **导航**: [<< 附录 A - FTA 术语表](./appendix-a-glossary.md) | [附录 C - 参考文献 >>](./appendix-c-references.md)

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/ack-fta-generator-v2.md|ack-fta-generator-v2]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary.md|appendix-a-glossary]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-c-references.md|appendix-c-references]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-d-templates.md|appendix-d-templates]]
