---
title: KUDIG Frontmatter 规范
description: '| `title` | string | 文档标题（中文） | "Kubernetes 架构全景图" |'
category: general
tags:
- k8s
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Frontmatter 规范 是什么
- 如何 KUDIG Frontmatter 规范
trigger_keywords:
- KUDIG
- Frontmatter
- 规范
prerequisites:
- kubectl-basics
---

---
title: KUDIG Frontmatter 规范
description: KUDIG Frontmatter 规范
category: docs
tags:
- k8s
- frontmatter
- spec
- metadata
relationships:
- target: '[[docs/TAG-DICTIONARY.md|KUDIG 全局标签字典]]'
  type: related_to
- target: '[[docs/SCENARIO-TAXONOMY.md|KUDIG 场景分类体系]]'
  type: related_to
- target: '[[docs/SYNONYM-DICTIONARY.md|KUDIG 同义词与别名词典]]'
  type: related_to
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

tier: peripheral---

# KUDIG Frontmatter 规范

> 创建时间: 2026-05-20
> 用途: 定义所有文档的标准 YAML frontmatter 格式
> 适用范围: domain-*, topic-* 下所有 .md 文件 (MOC.md 和 README.md 除外)

---

## 标准 Frontmatter

每篇文档必须包含以下 YAML frontmatter:

```yaml
---
# === 基础信息 (Required) ===
title: "文档标题（中文）"
title_en: "English Title"
description: "一句话摘要（20-80 字符）"
category: "所属分类（如 domain-1-architecture-fundamentals）"
tags: [一级标签, 二级标签, 三级标签]

# === 版本信息 (Required) ===
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
last_updated: "YYYY-MM"
authors:
  - name: "作者姓名"
    role: "contributor|author|reviewer"

# === 阅读体验 (Required) ===
difficulty: "beginner"       # beginner | intermediate | advanced | expert
reading_level: "beginner"    # 同 difficulty
audience: ["SRE", "DevOps"]  # SRE / DevOps / Developer / Architect / Student
estimated_read_time: "15min" # 5min / 10min / 15min / 30min / 1h / 2h

# === 交叉引用 (Optional but Recommended) ===
prerequisites:
  - "domain-01-cluster-fundamentals"
aliases:
  - "常见别名"
intent_queries:
  - "用户可能的自然语言查询"
cross_refs:
  - type: "domain"
    path: "../domain-N-name/doc.md"
    label: "说明"
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/component-fta.md"
    label: "说明"
---
```

---

## 字段说明

### Required 字段

| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `title` | string | 文档标题（中文） | "Kubernetes 架构全景图" |
| `title_en` | string | 英文标题 | "Kubernetes Architecture Overview" |
| `description` | string | 一句话摘要 | "Kubernetes 系统架构全景图，包含控制平面、数据平面和扩展组件" |
| `category` | string | 所属目录 | "domain-01-cluster-fundamentals" |
| `tags` | list | 标签数组 | [k8s, architecture, deep-dive] |
| `k8s_versions` | list | 覆盖的 K8s 版本 | ["1.28", "1.29", "1.30", "1.31", "1.32"] |
| `last_updated` | string | 最后更新日期 | "2026-05" |
| `authors` | list | 作者列表 | [{name: "Allen Galler", role: "author"}] |
| `difficulty` | string | 难度等级 | "intermediate" |
| `reading_level` | string | 阅读等级 | "intermediate" |
| `audience` | list | 目标读者 | ["SRE", "DevOps"] |
| `estimated_read_time` | string | 预估阅读时间 | "15min" |

### Optional 字段

| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `prerequisites` | list | 前置知识依赖 | ["domain-01-cluster-fundamentals"] |
| `aliases` | list | 搜索别名 | ["k8s架构", "kubernetes架构"] |
| `intent_queries` | list | Agent 意图查询 | ["K8s架构是什么？"] |
| `cross_refs` | list | 交叉引用 | [{type: "domain", path: "...", label: "..."}] |

---

## 难度等级定义

| 等级 | 说明 | 适合人群 |
|---|---|---|
| `beginner` | 入门 — 概念介绍、基础操作 | 初学者、学生 |
| `intermediate` | 进阶 — 配置、部署、日常运维 | 运维工程师、开发者 |
| `advanced` | 高级 — 架构深度、性能调优、故障排查 | SRE、资深运维 |
| `expert` | 专家 — 源码分析、源码级调优 | 架构师、核心贡献者 |

---

## 验证规则

1. `title` 不能为空，长度 2-200 字符
2. `description` 不能为空，长度 10-300 字符
3. `tags` 至少包含 1 个标签
4. `difficulty` 和 `reading_level` 必须匹配枚举值
5. `last_updated` 格式必须为 `YYYY-MM`
6. `estimated_read_time` 必须匹配 `^\d+min$` 或 `^\d+h$`
7. `authors` 至少包含 1 个作者
8. `k8s_versions` 至少包含 1 个版本

---

## MOC 专用 Frontmatter

MOC 导航页使用扩展 frontmatter:

```yaml
---
title: "{domain/topic} MOC"
description: "{domain} 知识域导航页"
category: moc
tags: [k8s, moc, {primary-tag}]
moc_scope: "{domain-name}"
moc_type: "domain|topic|global"
moc_coverage:
  total_docs: 33
last_updated: "YYYY-MM-DD"
---
```

---

*本文档是 frontmatter 的权威定义，新增字段时应在此文件中注册。*

---

## Related

- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[references/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[KUDIG Synonym Dictionary]]
- [[docs/TAG-DICTIONARY.md|KUDIG 全局标签字典]]
