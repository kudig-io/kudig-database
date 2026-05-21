---
title: 附录 D：FTA 模板参考 (历史参考)
description: — FTA 方法论与 AI Agent 实践'
category: fta
tags:
- fta
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
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 附录 D：FTA 模板参考 (历史参考) 故障排查
- 附录 D：FTA 模板参考 (历史参考) 排障步骤
- 附录 D：FTA 模板参考 (历史参考) 根因分析
trigger_keywords:
- 附录
- D：FTA
- 模板参考
- 历史参考
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
fta_id: FTA-APPENDIX_D_TEMPLATES-001
component: Appendix D Templates
severity: high
---

title: 附录 D：FTA 模板参考 (历史参考)
description: '- [../fta-methodology-and-agentic-practices.md](../fta-methodology-and-agentic-practices.md)
  — FTA 方法论与 AI Agent 实践'
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
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
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

- [templates/README.md](../../templates/README.md) — 模板体系索引
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

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-c-references.md|appendix-c-references]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-diagnosis-improvement.md|fta-diagnosis-improvement]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-execution-engine.md|fta-execution-engine]]
