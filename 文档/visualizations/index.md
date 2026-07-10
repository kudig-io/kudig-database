---
title: 可视化中心
description: 雷达图自我评估工具，支持本地存储。
summary: 雷达图自我评估工具，支持本地存储。
category: general
tags:
- k8s
- agent
tier: peripheral
created: '2026-07-01'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可视化中心 是什么
- 如何 可视化中心
trigger_keywords:
- 可视化中心
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 可视化中心

KUDIG 知识库可视化工具，包括知识图谱、学习方法论等领域探索工具。

## 工具列表

### 知识图谱
D3.js 力导向图展示完整知识库拓扑结构，包括 950+ 文档和 41 个领域。

- [知识图谱 (Knowledge Graph)](knowledge-graph.html){ target="_blank" }

### 领域探索器
D3.js 可视化展示 5 个核心领域及其主题和交叉链接。

- [领域探索器 (Domain Explorer)](d3-domain-explorer.html){ target="_blank" }

### 学习方法论
SVG 动画展示 Write/Read/Ask/Iterate 循环学习法。

- [学习方法论 (Learning Methodology)](learning-methodology.html){ target="_blank" }

### 自学评估
雷达图自我评估工具，支持本地存储。

- [自学评估 (Self-Learning)](self-learning.html){ target="_blank" }

<style>
.card-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 1.5rem;
  margin: 2rem 0;
}
</style>

## Related

- [[实体/k8s-glossary-index.md|K8s 术语表索引]] — Cross-reference
- [[实体/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[实体/KUDIG Templates and Agent Prompts.md|KUDIG Templates and Agent Prompts]] — Cross-reference
- [[实体/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]] — Cross-reference
- [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Cross-reference


<!-- risk-assessed -->
