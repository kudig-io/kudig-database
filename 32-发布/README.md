---
title: 发布 Release
description: 发布产物目录 — 语料导出（corpus、metadata、qa），冻结快照
summary: 发布产物冻结目录，包含 corpus 语料包、metadata 和 qa 数据
category: domain
tags:
- release
- corpus
- frozen
tier: peripheral
created: '2026-08-05'
---

# 发布 Release

> ⚠️ **冻结目录**。本目录存放语料导出发布产物（corpus、metadata、qa），供 Agent 加载使用。
> 内容从知识库生成，不直接编辑。

## 子目录

| 子目录 | 内容 |
|--------|------|
| package/ | 发布包（按时间戳版本化） |
| scripts/ | 发布与导出脚本 |

## 发布流程

发布产物由 `31-脚本/corpus-generator/` 下的脚本从知识源文档生成，按时间戳版本化存储。

## 跨域导航

- [[32-发布/index.md\|发布索引]]
- [[31-脚本/README.md\|脚本工具]]
- [[35-元数据/README.md\|元数据]]
