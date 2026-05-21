---
title: KUDIG 文档模板目录
description: '## Skill 运维技能模板（v2.0）'
category: reference
tags:
- k8s
- templates
- documentation
- skill-template
- fta-template
- best-practice-template
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 文档模板目录 是什么
- 如何 KUDIG 文档模板目录
trigger_keywords:
- KUDIG
- 文档模板目录
prerequisites:
- kubectl-basics
---

# KUDIG 文档模板目录

## Skill 运维技能模板（v2.0）

核心结构：
- **skill_id**：唯一标识（SKILL-{CATEGORY}-{SEQ}）
- **skill_name**：双语名称
- **触发条件**：故障现象
- **诊断步骤**：命令 + 预期输出 + 判断逻辑
- **修复方案**：操作 + 风险 + 回滚
- **关联文档**：FTA、最佳实践

## FTA 故障树模板

标准化故障树结构：顶层事件 → 逻辑门 → 中间事件 → 基本原因。

## 决策树模板

Mermaid 格式的诊断决策树模板，支持交互式路径选择。

## 最佳实践模板

最佳实践文档的标准结构：背景、方案、实现、验证、总结。

## 速查表模板

一页纸速查格式，覆盖命令、参数、示例。

## 其他模板

- **演示文稿模板**：技术分享标准化格式
- **MOC 地图模板**：知识域导航地图
- **FEBM 取证模板**：取证循证方法论文档
- **Domain 文章模板**：知识域标准文章
- **项目索引模板**：项目级目录结构

---

> 来源：templates/*.md（共 11 篇）

## Related

- [[kudig-templates-catalog]] — KUDIG Templates Catalog
- [[references/kudig-gitbook-system.md|kudig-gitbook-system]] — Gitbook 本地文档浏览系统与构建指南
- [[references/KUDIG Templates and Agent Prompts.md|KUDIG Templates and Agent Prompts]] — KUDIG Templates and Agent Prompts
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[references/kudig-documentation-specs|KUDIG 文档规范体系：标签字典、Frontmatter、场景分类、同义词典]] — Cross-reference
