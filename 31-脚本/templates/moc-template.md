---
title: '{'
description: '{'
category: templates
tags:
- k8s
created: '2026-05-23'
summary: '{'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
tier: peripheral
---


---
title: "{{DOMAIN_NAME}} MOC - {{TITLE}}"
title_en: "MOC - {{TITLE_EN}}"
description: "{{DOMAIN_NAME}} 知识域导航页，覆盖 {{DOC_COUNT}} 篇文档"
category: moc
tags: [k8s, moc, {{PRIMARY_TAG}}]
moc_scope: "{{DOMAIN_NAME}}"
moc_type: "domain"
moc_coverage:
  total_docs: {{DOC_COUNT}}
  difficulty_distribution:
    beginner: {{BEGINNER_COUNT}}
    intermediate: {{INTERMEDIATE_COUNT}}
    advanced: {{ADVANCED_COUNT}}
    expert: {{EXPERT_COUNT}}
last_updated: "{{YYYY-MM-DD}}"
---

# {{DOMAIN_NAME}} MOC — {{TITLE}}

> **MOC 版本**: 1.0
> **知识域**: {{DOMAIN_NAME}}
> **文档数量**: {{DOC_COUNT}} 篇
> **最后更新**: {{YYYY-MM-DD}}
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

{{DOMAIN_OVERVIEW}}

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | {{DOMAIN_NAME}} |
| **核心主题** | {{CORE_TOPICS}} |
| **目标读者** | {{AUDIENCE}} |
| **难度范围** | {{DIFFICULTY_RANGE}} |
| **前置依赖** | {{PREREQUISITES}} |

---

## 文档清单

| # | 文件 | 标题 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|---|
{{DOC_TABLE}}

---

## 知识图谱

```mermaid
graph TD
    subgraph {{DOMAIN_NAME}}
        A["{{CORE_TOPIC_1}}"]
        B["{{CORE_TOPIC_2}}"]
        C["{{CORE_TOPIC_3}}"]
    end

    subgraph 关联领域
        D["上游依赖"]
        E["下游应用"]
    end

    A -->|依赖| D
    B -->|关联| C
    C -->|应用| E

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
    style C fill:#a855f7,stroke:#6b21a8,color:#fff
```

---

## 关联知识域

| 关联类型 | 知识域 | MOC | 说明 |
|---|---|---|---|
| 上游依赖 | {{UPSTREAM_DOMAIN}} | {{UPSTREAM_NAME}} MOC | {{说明}} |
| 下游应用 | {{DOWNSTREAM_DOMAIN}} | {{DOWNSTREAM_NAME}} MOC | {{说明}} |
| 横向关联 | {{PEER_DOMAIN}} | {{PEER_NAME}} MOC | {{说明}} |

---

## 场景入口

| 场景 | 入口文档 | 说明 |
|---|---|---|
| 故障排查 | FTA 故障树 | {{DOMAIN}} 相关故障树 |
| 操作技能 | Skills 技能 | {{DOMAIN}} 相关操作技能 |
| 速查参考 | Cheat Sheet | 相关速查卡 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | {{DOC_COUNT}} |
| 平均阅读时间 | {{AVG_READ_TIME}} |
| 双向链接总数 | {{TOTAL_CROSS_REFS}} |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由脚本自动生成，请勿手动编辑。*

---

## Obsidian 相关文档

- [[31-脚本/templates/PROJECT-INDEX-TEMPLATE.md|开源项目索引模板]]
- [[31-脚本/templates/README.md|KUDIG 文档模板体系]]
- [[31-脚本/templates/best-practice-template.md|最佳实践模板]]
- [[31-脚本/templates/cheat-sheet-template.md|{{主题名称}} 速查卡]]
- [[31-脚本/templates/domain-article-template.md|{]]
- [[31-脚本/templates/febm-template.md|{{主题名称}} FEBM 法医取证分析]]
- [[31-脚本/templates/fta-template.md|{{组件名称}} 故障树分析 (FTA)]]
- [[templates/presentation-template|Kubernetes [组件/技术名称] 全栈进阶培训 (从入门到专家)]]
- [[31-脚本/templates/skill-template.md|Skill 运维技能文档模板]]
