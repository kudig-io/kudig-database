---
title: 元数据索引 (Metadata)
description: 元数据索引 (Metadata) — Kubernetes 生产运维知识库
category: general
tags:
- k8s
- rag
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 元数据索引 (Metadata) 是什么
- 如何 元数据索引 (Metadata)
trigger_keywords:
- 元数据索引
- Metadata
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# 元数据索引 (Metadata)

> 文档标签、难度分级和知识图谱索引，提升检索和 RAG 分块质量

## 目录

| 文件 | 用途 |
|:---|:---|
| [tags-index.md](./tags-index.md) | 标签索引 - 按标签聚合文档 |
| [difficulty-index.md](./difficulty-index.md) | 难度分级索引 |
| [knowledge-map.md](./knowledge-map.md) | 知识图谱 - 模块间关系 |

## 用途

### 对 RAG 应用的价值
- **标签索引**：帮助 RAG 系统按主题精准检索相关文档
- **难度分级**：根据用户水平推荐合适的文档
- **知识图谱**：构建文档间的语义关联，增强上下文理解

### 对人类读者的价值
- 按主题快速定位相关文档
- 了解学习路径和文档间依赖关系
- 评估自身水平，选择合适难度的内容

## Frontmatter 规范

建议每篇文档逐步添加 YAML frontmatter：

```yaml
---
title: "文档标题"
domain: architecture    # 所属知识域
difficulty: intermediate  # beginner / intermediate / advanced / expert
tags: [kubernetes, architecture, high-availability]
k8s_versions: [v1.28, v1.29, v1.30, v1.31, v1.32]
last_updated: 2026-04-01
---
```

## Related

- [[entities/dex.md|Dex]]
- [[index|index]]
- knowledge-map
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
