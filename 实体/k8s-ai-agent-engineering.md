---
title: AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
description: 关键优化点：
summary: 关键优化点：
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
tier: core
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI Agent 工程

## 概述

AI Agent 工程涵盖了在 Kubernetes 上构建、部署和管理生产级 AI Agent 应用所需的关键技术，包括 RAG（检索增强生成）、多 Agent 编排模式、安全护栏和 K8s 生产部署最佳实践。随着 LLM 技术的成熟，将 AI Agent 从原型推向生产需要系统化的工程能力。

## RAG（检索增强生成）

RAG 流程为知识密集型 AI 应用提供外部知识注入能力：

```
查询 → Embedding → 向量检索 → Re-ranking → 上下文注入 → LLM 生成
```

关键优化点：
- **分块策略**：按 Markdown 标题/段落分块，保持知识完整性；固定大小分块时重叠 10-20%
- **Embedding 模型**：text-embedding-3-large（通用）/ bge-large-zh（中文）/ Cohere embed v3
- **向量数据库**：Milvus（大规模）/ Qdrant（Rust 高性能）/ Weaviate / Chroma（轻量）
- **Re-ranking**：Cohere Rerank / BGE Reranker / Cross-encoder 模型，显著提升检索精度
- **评估指标**：Recall@K、MRR（Mean Reciprocal Rank）、Faithfulness

## 多 Agent 编排模式

| 模式 | 描述 | 适用场景 | 框架 |
|------|------|----------|------|
| Supervisor-Worker | 主 Agent 分配任务给子 Agent | 复杂任务分解 | AutoGen, CrewAI |
| Pipeline | Agent 串行处理链 | 流程化任务 | LangChain |
| Debate | 多 Agent 辩论达成共识 | 决策场景 | AutoGen |
| Hierarchical | 多层级 Agent 组织 | 企业级系统 | MetaGPT |

编排框架对比：LangGraph（图结构灵活）、AutoGen（微软对话编排）、CrewAI（角色驱动）、MetaGPT（SOP 驱动）。

## 安全护栏

OWASP LLM Top 10 应对策略：
- **Prompt 注入防御**：输入过滤、系统提示隔离、结构化输出约束
- **输出内容过滤**：PII 脱敏、有害内容拦截（NeMo Guardrails / Guardrails AI）
- **权限控制**：Agent 工具调用权限最小化，RBAC 限制可执行操作
- **审计日志**：全链路追踪（LangSmith / Langfuse），记录每次工具调用和 LLM 交互
- **速率限制**：防止资源滥用和成本失控
- **Human-in-the-loop**：破坏性操作需人工确认

## K8s 生产部署

在 Kubernetes 上部署 AI Agent 的最佳实践：

- **部署架构**：Deployment + HPA 自动扩缩（基于 QPS 和 GPU 利用率）
- **灰度发布**：金丝雀（流量比例）或蓝绿部署，通过 Istio/Linkerd 控制流量
- **SLA 监控**：P99 延迟、Token 吞吐量、成功率、Tool 调用成功率
- **GPU 资源管理**：使用 HAMi/GPU Operator 管理共享 GPU，KEDA 事件驱动缩放
- **模型管理**：KServe 或 vLLM 部署推理服务，模型存储在 S3/PVC
- **向量数据库**：Milvus/Qdrant 以 Operator 模式部署，PVC 持久化
- **故障恢复**：自动重启、断路器（Circuit Breaker）、降级策略（fallback 到简单模型）
- **配置管理**：LLM 参数（temperature、max_tokens）通过 ConfigMap/Secret 动态管理

## 成本优化

- 使用本地模型（Ollama/vLLM）替代 GPT-4 降低 API 成本
- 缓存常见查询结果，减少重复 LLM 调用
- 使用 LLM Router 根据复杂度路由到不同模型
- GPU 时间分片和 Spot 实例降低推理成本

---

> 来源：.zread/wiki/drafts/18-ai-agent-gong-cheng-*.md

## Related

- [[概念/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — 纵深防御 x 供应链安全
- [[概念/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — 服务网格 x 零信任安全
- [[实体/kudig-rag-chunking-strategy.md|kudig-rag-chunking-strategy]] — RAG 分块策略指南与 Manpage 安装指南
- [[实体/k8s-security-compliance.md|k8s-security-compliance]] — 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
- [[deployment]] — Deployment


<!-- risk-assessed -->
