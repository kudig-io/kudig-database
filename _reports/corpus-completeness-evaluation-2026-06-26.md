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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 语料内容完整度评估报告

> **综合评分**: 89.1/100

## 评分维度

| 维度 | 得分 | 说明 |
|---|---|---|
| 规模 | 97.6 | 4881 页面，17250K tokens |
| 结构健康度 | 100.0 | broken links=0, missing frontmatter=0 |
| 概念覆盖度 | 98.0 | 49/50 关键概念 |
| 工单智能体适配度 | 100.0 | ticket 页=58, QA 对≈0, skill 文档=507 |
| 阿里云覆盖度 | 39.0 | aliyun 页=18, ack 页=99 |
| RAG 适配度 | 100.0 | summary=4879, tags=4881, category=4881 |

## 规模统计

- **总页面数**: 4881
- **总字符数**: 69,008,570
- **估算 Tokens**: 17,250,307

## 结构健康度

- **Broken links**: 0
- **Missing frontmatter**: 0
- **Missing summary**: 2
- **Missing tier**: 2
- **Orphans**: 1717
- **Core pages**: 1099
- **Supporting pages**: 1330
- **Peripheral pages**: 2450

## 内容覆盖度

- 检查关键概念: 50
- 命中概念: 49
- 覆盖率: 98.0%

## 工单智能体适配度

- **Ticket 相关页面**: 58
- **估算 QA 对数**: 0
- **Skill 文档**: 507

## 阿里云/专有云覆盖

- **阿里云相关页面**: 18
- **ACK 相关页面**: 99

## RAG 适配度

- **有 summary**: 4879 / 4881
- **有 tags**: 4881 / 4881
- **有 category**: 4881 / 4881

## Top 15 Tags

- `#k8s` — 3718
- `#release-notes` — 1345
- `#changelog` — 1325
- `#prometheus` — 1303
- `#etcd` — 942
- `#rag` — 924
- `#kubelet` — 919
- `#docker` — 733
- `#grafana` — 645
- `#apiserver` — 645
- `#scheduler` — 641
- `#operator` — 561
- `#helm` — 554
- `#glossary` — 549
- `#istio` — 394

## Top 15 Categories

- `release-notes` — 1322
- `general` — 717
- `dictionary` — 548
- `entities` — 284
- `skills` — 160
- `learning` — 149
- `synthesis` — 135
- `concepts` — 117
- `structural-troubleshooting` — 72
- `fta` — 62
- `moc` — 60
- `troubleshooting` — 60
- `domain` — 50
- `reference` — 45
- `networking` — 40

## 评估结论

综合评分达到 **优秀** 水平，语料规模充足、结构健康、覆盖全面，可以作为 llm-wiki 语料导出使用。

<!-- risk-assessed -->
