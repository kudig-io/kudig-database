---
title: AI 语料库配置 (Corpus Config)
description: '# AI 语料库配置 (Corpus Config)'
category: general
tags:
- k8s
- rag
- agent
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 语料库配置 (Corpus Config) 是什么
- 如何 AI 语料库配置 (Corpus Config)
trigger_keywords:
- AI
- 语料库配置
- Corpus
- Config
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# AI 语料库配置 (Corpus Config)

> 面向 NotebookLM / IMA / RAG 等 AI 场景的语料配置和最佳实践

---

## 目录结构

```
corpus-config/
├── README.md                           # 本文件
├── rag-chunking-strategy.md            # RAG 分块策略指南
├── embedding-guide.md                  # Embedding 选型与配置
└── profiles/                           # 场景化语料配置
    ├── notebooklm-profile.yaml         # NotebookLM 推荐配置
    ├── rag-sre-profile.yaml            # SRE 运维 Agent 语料
    ├── rag-learning-profile.yaml       # 学习场景语料
    └── rag-full-profile.yaml           # 全量语料配置
```

## 语料特点

本知识库作为 AI 语料具备以下优势：

| 特点 | 说明 |
|:---|:---|
| **结构化** | 统一的 Markdown 格式、标题层级、表格结构 |
| **领域专精** | 聚焦 Kubernetes + AI Infra，非泛化内容 |
| **生产级** | 所有配置经过验证，非玩具示例 |
| **多粒度** | Domain 深度文档 + Cheat Sheet 速查 + FTA 推理骨架 |
| **交叉引用** | 文档间建立了关联关系，增强语义理解 |

## 推荐使用场景

| 场景 | 推荐导入 | 配置文件 |
|:---|:---|:---|
| NotebookLM 播客 | topic-fta + topic-learn | [notebooklm-profile.yaml](./profiles/notebooklm-profile.yaml) |
| SRE Agent | topic-fta + topic-skills + domain-10-troubleshooting-diagnostics | [rag-sre-profile.yaml](./profiles/rag-sre-profile.yaml) |
| K8s 学习助手 | topic-learn + topic-cheat-sheet + domain-1~6 | [rag-learning-profile.yaml](./profiles/rag-learning-profile.yaml) |
| 全知识库 | 全部目录 | [rag-full-profile.yaml](./profiles/rag-full-profile.yaml) |

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
