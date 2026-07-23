---
title: 元数据索引 (Metadata)
description: 元数据索引 (Metadata) — Kubernetes 生产运维知识库
summary: 元数据索引 (Metadata) — Kubernetes 生产运维知识库
category: general
tags:
- k8s
- rag
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[实体/dex.md|Dex]]
- [[index|index]]
- knowledge-map
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
