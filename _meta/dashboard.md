---
title: KUDIG Wiki Dashboard
description: '- 可通过解析 frontmatter 的脚本在 CI/CD 或静态站点中复现相同逻辑'
category: reference
tags:
- dashboard
- meta
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Wiki Dashboard 是什么
- 如何 KUDIG Wiki Dashboard
trigger_keywords:
- KUDIG
- Wiki
- Dashboard
prerequisites:
- kubectl-basics
---

# KUDIG Wiki Dashboard

*动态数据视图，使用 Dataview 语法。在 Obsidian 中自动渲染；纯 Markdown 环境下显示为查询源码。*

---

## 1. 全库总览（Concepts + Entities + Skills）

```dataview
TABLE WITHOUT ID
  file.link AS "页面",
  category AS "类别",
  tags AS "标签",
  summary AS "摘要",
  file.mtime AS "最后修改"
FROM "concepts" OR "entities" OR "skills"
WHERE file.name != file.folder
SORT category ASC, file.name ASC
```

---

## 2. 按标签分组综合索引

```dataview
TABLE WITHOUT ID
  rows.file.link AS "页面",
  rows.category AS "类别",
  rows.summary AS "摘要"
FROM "concepts" OR "entities" OR "skills" OR "references" OR "synthesis"
WHERE file.name != file.folder
GROUP BY tags[0] AS "首标签"
SORT key ASC
```

---

## 3. 最近更新的页面（Top 50）

```dataview
TABLE WITHOUT ID
  file.link AS "页面",
  category AS "类别",
  summary AS "摘要",
  file.mtime AS "最后修改"
FROM "concepts" OR "entities" OR "skills" OR "references" OR "synthesis"
WHERE file.name != file.folder
SORT file.mtime DESC
LIMIT 50
```

---

## 4. 陈旧内容（超过 30 天未更新）

```dataview
TABLE WITHOUT ID
  file.link AS "页面",
  category AS "类别",
  file.mtime AS "最后修改",
  (date(today) - file.mtime).days + " 天" AS "距今"
FROM "concepts" OR "entities" OR "skills" OR "references" OR "synthesis"
WHERE file.name != file.folder
WHERE (date(today) - file.mtime).days > 30
SORT (date(today) - file.mtime).days DESC
```

> *注：当前 vault 为新建状态，所有页面修改日期均为 2026-05-21。此视图在 vault 运行 30 天后将自动填充结果。*

---

## 5. 综合页卡片视图（Synthesis）

```dataview
TABLE WITHOUT ID
  file.link AS "综合页",
  summary AS "核心洞察"
FROM "synthesis"
WHERE file.name != file.folder
SORT file.name ASC
```

---

## 6. 实体卡片视图（Entities）

```dataview
TABLE WITHOUT ID
  file.link AS "实体",
  tags AS "标签",
  summary AS "摘要"
FROM "entities"
WHERE file.name != file.folder
SORT file.name ASC
LIMIT 100
```

---

## 7. 技能手册总览（Skills）

```dataview
TABLE WITHOUT ID
  file.link AS "技能页",
  tags AS "标签",
  summary AS "摘要",
  file.mtime AS "最后修改"
FROM "skills"
WHERE file.name != file.folder
SORT file.name ASC
```

---

## 8. 按类别统计

```dataview
TABLE WITHOUT ID
  key AS "类别",
  length(rows) AS "页面数",
  rows.file.link AS "页面列表"
FROM "concepts" OR "entities" OR "skills" OR "references" OR "synthesis"
WHERE file.name != file.folder
GROUP BY category AS "类别"
SORT length(rows) DESC
```

---

## 9. 参考文档总览（References）

```dataview
TABLE WITHOUT ID
  file.link AS "参考页",
  tags AS "标签",
  summary AS "摘要"
FROM "references"
WHERE file.name != file.folder
SORT file.name ASC
```

---

## 使用说明

### 在 Obsidian 中
1. 安装并启用 **Dataview** 社区插件
2. 打开本页面，所有查询块将自动渲染为动态表格
3. 点击任意单元格中的 `[[链接]]` 可直接跳转对应页面

### 纯 Markdown 环境
- 上述代码块显示为 `dataview` 语法的查询源码
- 可通过解析 frontmatter 的脚本在 CI/CD 或静态站点中复现相同逻辑
- `file.mtime` 对应文件系统修改时间，`file.link` 对应文件名（去 `.md` 后缀）

### 扩展自定义
- 修改 `FROM` 子句可限定或扩展查询范围
- 修改 `WHERE` 条件可过滤特定标签或类别
- 在 `SORT` 后追加 `LIMIT N` 可限制返回行数
