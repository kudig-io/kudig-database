---
title: 附录 D：FTA 模板参考 (历史参考) [topic-fta]
description: 附录 D：FTA 模板参考（历史参考）— 已废弃，模板迁移至 31-脚本/templates/fta-template.md
summary: 附录 D：FTA 模板参考（历史参考）— 已废弃，模板迁移至 31-脚本/templates/fta-template.md
category: fta
tags:
- fta
- troubleshooting
- prometheus
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-08
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 附录 D：FTA 模板参考 (历史参考)

> **⚠️ 已废弃**: 本文件内容已合并至 `templates/fta-template.md`
> **新模板位置**: [../../templates/fta-template.md](../../31-%E8%84%9A%E6%9C%AC/templates/fta-template.md)
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

- [templates/README.md](../../31-%E8%84%9A%E6%9C%AC/templates/README.md) — 模板体系索引
- [templates/fta-template.md](../../31-%E8%84%9A%E6%9C%AC/templates/fta-template.md) — FTA 文档标准模板（现行版本）
- [../fta-methodology-and-agentic-practices.md](fta-methodology-and-agentic-practices.md) — FTA 方法论与 AI Agent 实践
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

- [[19-故障诊断/06-FTA故障树/MOC.md|topic-fta [[README|MOC]]]]
- [[19-故障诊断/06-FTA故障树/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[19-故障诊断/06-FTA故障树/01-fta-origin-and-evolution.md|[[19-故障诊断/06-FTA故障树/01-fta-origin-and-evolution|第一章：FTA 起源与发展史]]]]
- [[19-故障诊断/06-FTA故障树/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[19-故障诊断/06-FTA故障树/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[19-故障诊断/06-FTA故障树/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[19-故障诊断/06-FTA故障树/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[19-故障诊断/06-FTA故障树/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[19-故障诊断/06-FTA故障树/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[19-故障诊断/06-FTA故障树/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[19-故障诊断/06-FTA故障树/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[19-故障诊断/06-FTA故障树/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[19-故障诊断/06-FTA故障树/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]]
- [[19-故障诊断/06-FTA故障树/appendix-c-references.md|appendix-c-references]]
- [[19-故障诊断/06-FTA故障树/fta-diagnosis-improvement.md|fta-diagnosis-improvement]]
- [[19-故障诊断/06-FTA故障树/fta-execution-engine.md|fta-execution-engine]]


<!-- risk-assessed -->
