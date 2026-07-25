---
title: Postmortem & Incident Review
description: 事后复盘知识域 — Blameless Postmortem 模板、复盘文化、事故时间线、改进行动跟踪
category: subdomain
tags:
- postmortem
- blameless
- incident-review
- sre-culture
- action-items
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 事后复盘 Postmortem

> 无指责复盘文化，从事故中学习，持续改进系统韧性。

## 复盘流程

| 阶段 | 活动 | 输出 |
|------|------|------|
| 事故响应 | 实时记录时间线 | 事件日志 |
| 复盘准备 | 收集数据/指标/日志 | 事实汇总 |
| 复盘会议 | 无指责讨论根因 | 复盘报告 |
| 行动跟踪 | 执行改进项 | 修复 PR/工单 |
| 知识沉淀 | 分享与归档 | 知识库更新 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[12-可靠性/05-事后复盘/01-blameless-postmortem-template.md\|复盘模板]] | Blameless Postmortem 结构化模板 | intermediate |
| [[12-可靠性/05-事后复盘/02-postmortem-culture-guide.md\|复盘文化指南]] | 建立无指责复盘文化 | intermediate |

## 复盘报告核心要素

- 事故摘要（影响范围、持续时间、严重级别）
- 时间线（检测、响应、恢复各节点）
- 根因分析（5 Whys / 鱼骨图）
- 改进行动（具体、可跟踪、有负责人）
- 经验教训（可复用的组织知识）

## Related

- [[12-可靠性/04-混沌工程/index.md|混沌工程]]
- [[12-可靠性/02-灾难恢复/index.md|灾难恢复]]
- [[13-生产运维/index.md|生产运维]]
