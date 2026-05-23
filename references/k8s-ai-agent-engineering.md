---
title: AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
description: 关键优化点：
category: reference
tags:
- k8s
- ai-agent
- rag
- multi-agent
- security
- production-deployment
- hpa
- rbac
- llm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署 是什么
- 如何 AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
trigger_keywords:
- AI
- Agent
- 工程：RAG
- Agent
- 编排
- 安全护栏与生产部署
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# AI Agent 工程

## RAG（检索增强生成）

RAG 流程：
```
查询 → Embedding → 向量检索 → Re-ranking → 上下文注入 → LLM 生成
```

关键优化点：
- **分块策略**：按 Markdown 标题分块，保持知识完整性
- **Embedding 模型**：text-embedding-3-large / bge-large-zh
- **向量库**：Milvus / Qdrant / Weaviate / Chroma
- **Re-ranking**：Cohere Rerank / BGE Reranker

## 多 Agent 编排模式

| 模式 | 描述 | 适用场景 |
|------|------|----------|
| Supervisor-Worker | 主 Agent 分配任务给子 Agent | 复杂任务分解 |
| Pipeline | Agent 串行处理链 | 流程化任务 |
| Debate | 多 Agent 辩论达成共识 | 决策场景 |
| Hierarchical | 多层级 Agent 组织 | 企业级系统 |

## 安全护栏

OWASP LLM Top 10 应对：
- Prompt 注入防御（输入过滤、隔离）
- 输出内容过滤（PII 脱敏、有害内容拦截）
- 权限控制（Agent 工具调用权限最小化）
- 审计日志（全链路追踪）

## K8s 生产部署

- Deployment + HPA 自动扩缩
- 灰度发布（金丝雀/蓝绿）
- SLA 监控（P99 延迟、成功率、吞吐量）
- 故障恢复（自动重启、断路器）

---

> 来源：.zread/wiki/drafts/18-ai-agent-gong-cheng-*.md

## Related

- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — 纵深防御 x 供应链安全
- [[synthesis/服务网格 x 零信任安全|服务网格 x 零信任安全]] — 服务网格 x 零信任安全
- [[references/kudig-rag-chunking-strategy|kudig-rag-chunking-strategy]] — RAG 分块策略指南与 Manpage 安装指南
- [[references/k8s-security-compliance|k8s-security-compliance]] — 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
- [[deployment]] — Deployment
