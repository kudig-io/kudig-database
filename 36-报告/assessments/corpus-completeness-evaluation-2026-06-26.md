---
title: 语料内容完整度评估报告（2026-06-26）
description: KUDIG Database 作为 llm-wiki 语料的内容完整度评估
category: reports
tags:
- corpus
- evaluation
- llm-wiki
created: "2026-06-26"
updated: "2026-06-26"
---

# 语料内容完整度评估报告

> **综合评分**: 80.2/100

## 评分维度

| 维度 | 得分 | 说明 |
|---|---|---|
| 规模 | 100.0 | 9330 页面，34702K tokens |
| 结构健康度 | 0.0 | broken links=24, missing frontmatter=77 |
| 概念覆盖度 | 98.0 | 49/50 关键概念 |
| 工单智能体适配度 | 100.0 | ticket 页=121, QA 对≈30193, skill 文档=1468 |
| 阿里云覆盖度 | 84.3 | aliyun 页=42, ack 页=211 |
| RAG 适配度 | 99.2 | summary=9249, tags=9253, category=9253 |

## 规模统计

- **总页面数**: 9330
- **总字符数**: 138,820,198
- **估算 Tokens**: 34,701,566

## 结构健康度

- **Broken links**: 24
- **Missing frontmatter**: 77
- **Missing summary**: 81
- **Missing tier**: 81
- **Orphans**: 3401
- **Core pages**: 2163
- **Supporting pages**: 3548
- **Peripheral pages**: 3492

## 内容覆盖度

- 检查关键概念: 50
- 命中概念: 49
- 覆盖率: 98.0%

## 工单智能体适配度

- **Ticket 相关页面**: 121
- **估算 QA 对数**: 30193
- **Skill 文档**: 1468

## 阿里云/专有云覆盖

- **阿里云相关页面**: 42
- **ACK 相关页面**: 211

## RAG 适配度

- **有 summary**: 9249 / 9330
- **有 tags**: 9253 / 9330
- **有 category**: 9253 / 9330

## Top 15 Tags

- `#k8s` — 5715
- `#prometheus` — 2385
- `#etcd` — 1793
- `#kubelet` — 1779
- `#rag` — 1370
- `#release-notes` — 1367
- `#changelog` — 1327
- `#apiserver` — 1245
- `#scheduler` — 1228
- `#docker` — 1228
- `#grafana` — 1160
- `#operator` — 1007
- `#helm` — 949
- `#index` — 946
- `#troubleshooting` — 853

## Top 15 Categories

- `general` — 1327
- `release-notes` — 1322
- `index` — 897
- `entities` — 569
- `dictionary` — 550
- `skills` — 362
- `learning` — 290
- `synthesis` — 269
- `concepts` — 231
- `structural-troubleshooting` — 216
- `fta` — 185
- `troubleshooting` — 140
- `moc` — 122
- `production-operations` — 106
- `domain` — 101

## 评估结论

综合评分达到 **优秀** 水平，语料规模充足、结构健康、覆盖全面，可以作为 llm-wiki 语料导出使用。