---
title: KUDIG Templates Catalog
description: '| Skill 运维技能模板 | 故障排查技能文档 | skills/ 目录下所有 Skill |'
summary: '| Skill 运维技能模板 | 故障排查技能文档 | skills/ 目录下所有 Skill |'
category: reference
tags:
- k8s
- templates
- documentation
- authoring-guide
- webhook
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Templates Catalog 是什么
- 如何 KUDIG Templates Catalog
trigger_keywords:
- KUDIG
- Templates
- Catalog
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Templates Catalog

> KUDIG 文档模板目录，所有新建文档必须遵循对应模板规范

---

## 模板列表

| 模板 | 用途 | 目标文档 |
|------|------|---------|
| Skill 运维技能模板 | 故障排查技能文档 | skills/ 目录下所有 Skill |
| FTA 故障树模板 | 组件故障树分析 | 故障诊断/topic-fta/ 目录下所有 FTA |
| FEBM 取证模板 | 法医取证分析 | 故障诊断/topic-febm/ 目录 |
| MOC 导航模板 | 内容导航索引 | 各 domain/topic 的 README |
| 领域文章模板 | 技术深度文章 | domain-*/ 目录下技术文章 |
| 最佳实践模板 | 运维最佳实践 | 生产运维/ 等实践文档 |
| 速查卡模板 | 快速参考卡片 | 系统基础/topic-cheat-sheet/ 目录 |
| 决策树模板 | 故障排查决策树 | topic-structural/ 目录 |
| 演示文稿模板 | 培训课件 | 内部培训/公开培训材料 |
| 项目索引模板 | 开源项目参考 | 第三方组件 entity 页面 |

---

## Skill 文档结构

Skill 文档是最核心的运维操作模板，包含 12 个标准章节：

1. **概述** — 覆盖范围、典型场景、前置条件
2. **症状识别** — 症状模式表、工单关键词映射、排除标准
3. **快速分级** — 影响评估、严重性分级（P0-P3）、升级触发条件
4. **诊断工作流** — Phase 1 快速检查 → Phase 2 深度检查 → Phase 3 主动探测
5. **根因分类** — 至少 8 个根因，带概率、诊断证据、FTA 映射
6. **修复操作** — 四级风险：低（自动）、中（审批）、高（指导）、严重（高级审批）
7. **验证确认** — 即时验证、短期监控、解决标准、回归检测
8. **升级协议** — 自动升级条件、消息模板、交接信息包
9. **版本兼容矩阵** — K8s 各版本功能/命令/API 差异
10. **知识进化** — 常见误诊模式、深度知识引用、改进记录
11. **云厂商特异性** — ACK/EKS/GKE/AKS 差异（可选）
12. **自动化集成接口** — 脚本入口、Webhook 回调、输出规范（可选）

---

## 命名规范

| 元素 | 格式 | 示例 |
|------|------|------|
| 文件名 | `{NN}-{kebab-case-scenario}.md` | `01-node-notready.md` |
| Skill ID | `SKILL-{CATEGORY}-{SEQ}` | `SKILL-NODE-001` |
| 根因 ID | `RC-{SEQ}` | `RC-001` |
| 修复操作 ID | `REM-{SEQ}` | `REM-001` |
| 诊断步骤 ID | `D{Phase}.{Seq}` | `D1.1`, `D2.3` |
| 症状 ID | `S{Seq}` | `S1`, `S2` |
| 版本标记 | `**[vX.XX+]**` | `**[v1.30+]**` |

---

## 相关文档

- [[实体/KUDIG Templates and Agent Prompts.md|原版模板集合]]
- [[kudig-prompts-catalog|AI Prompt 模板]]
- [[技能/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]

## Related

- [[INDEX]] — Wiki Index
- [[kudig-templates-catalog]] — KUDIG 文档模板目录
- [[README]] — FTA 故障树清单索引
- [[技能/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/kudig-documentation-specs.md|KUDIG 文档规范体系：标签字典、Frontmatter、场景分类、同义词典]] — Cross-reference


<!-- risk-assessed -->
