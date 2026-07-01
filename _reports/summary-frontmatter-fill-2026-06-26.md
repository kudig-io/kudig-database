---
title: Summary 与 Frontmatter 批量补充报告（2026-06-26）
description: 为缺少 summary/frontmatter 的核心页面自动补充字段
category: reports
tags:
- wiki-lint
- summary
- frontmatter
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
---


# Summary 与 Frontmatter 批量补充报告

- **扫描页面数**: 4987
- **新增 summary**: 4937
- **补齐 frontmatter**: 41
- **权限错误**: 2

## 生成规则

1. summary 优先使用 frontmatter 中的 description 字段
2. 无 description 时提取正文第一段前 200 字符
3. frontmatter 缺失 title/category/tags/created 时按路径推断默认值

> ⚠️ 自动生成的 summary 和 category 仅为默认值，建议人工 review 关键页面。