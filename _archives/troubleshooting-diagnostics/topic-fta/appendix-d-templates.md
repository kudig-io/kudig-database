---
title: 附录 D：FTA 模板参考 (历史参考)
description: '- [../fta-methodology-and-agentic-practices.md](../fta-methodology-and-agentic-practices.md) — FTA 方法论与 AI Agent
  实践'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- prometheus
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 附录 D：FTA 模板参考 (历史参考) 是什么
- 如何 附录 D：FTA 模板参考 (历史参考)
- 附录 D：FTA 模板参考 (历史参考) 根因分析
- 附录 D：FTA 模板参考 (历史参考) 故障树
trigger_keywords:
- 附录
- D：FTA
- 模板参考
- 历史参考
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
---

# 附录 D：FTA 模板参考 (历史参考)

> **⚠️ 已废弃**: 本文件内容已合并至 `templates/fta-template.md`
> **新模板位置**: [../../templates/fta-template.md](../../templates/fta-template.md)
> **最后更新**: 2026-05

---

## 变更说明

本文件作为历史参考保留，仅用于以下目的：
1. 了解 FTA 模板的历史演进
2. 迁移过程中对照已有 FTA 文档的格式

**所有新建 FTA 文档请使用 `templates/fta-template.md` 作为模板**，该模板包含：
- 完整的顶事件/底事件 YAML 定义模板
- 完整的 FTA 评审检查表（5 大类 30+ 检查项）
- Mermaid 故障树图规范
- Prometheus 告警规则模板

---

## 快速导航

- [templates/[[domain-07-platform-engineering/topic-code-analysis/deployment-create/README|README]].md](../../templates/README.md) — 模板体系索引
- [templates/fta-template.md](../../templates/fta-template.md) — FTA 文档标准模板（现行版本）
- [../fta-methodology-and-agentic-practices.md](../fta-methodology-and-agentic-practices.md) — FTA 方法论与 AI Agent 实践
- [../README.md](../README.md) — FTA 专题总览

---

## 模板版本历史

| 版本 | 日期 | 变更 |
|:---:|:---:|:---|
| 1.0 | 2026-03 | 初始版本，作为 FTA 方法论文档的附录 |
| 2.0 | 2026-05 | **内容合并至 `templates/fta-template.md`**，本文档改为重定向说明 |

---

*本文档不再维护，请使用 `templates/fta-template.md`*