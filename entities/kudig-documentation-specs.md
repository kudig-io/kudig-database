---
title: KUDIG 文档规范体系：标签字典、Frontmatter、场景分类、同义词典
description: '| 错误状态 | FTA 入口 | 快速排查 |'
summary: '| 错误状态 | FTA 入口 | 快速排查 |'
category: reference
tags:
- k8s
- documentation
- tag-dictionary
- frontmatter
- scenario-taxonomy
- synonym-dictionary
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 文档规范体系：标签字典、Frontmatter、场景分类、同义词典 是什么
- 如何 KUDIG 文档规范体系：标签字典、Frontmatter、场景分类、同义词典
trigger_keywords:
- KUDIG
- 文档规范体系：标签字典
- Frontmatter
- 场景分类
- 同义词典
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 文档规范体系

本页面汇总 docs/ 目录下 7 篇文档规范的核心内容。各规范已有独立 references 页面的，此处仅提供索引和补充信息。

## 已有独立页面的规范

- **标签字典** → `references/KUDIG Tag Dictionary`
- **Frontmatter 规范** → `references/KUDIG Frontmatter Spec`
- **场景分类体系** → `references/KUDIG Scenario Taxonomy`

## 错误码 → FTA 映射

将常见 K8s 错误状态映射到 FTA 故障树：

| 错误状态 | FTA 入口 | 快速排查 |
|----------|----------|----------|
| CrashLoopBackOff | pod-fta | `kubectl logs <pod>` |
| ImagePullBackOff | pod-fta | 检查 image 和 secret |
| Pending | node-fta | `kubectl describe pod` |
| Unschedulable | node-fta | 检查资源/亲和性/污点 |
| OOMKilled | pod-fta | 检查 limits 设置 |
| CreateContainerConfigError | pod-fta | 检查 ConfigMap/Secret |

## 命令 → 文档映射

将 kubectl 命令映射到相关知识文档，辅助 Agent 命令推荐。

## API → 文档映射

将 K8s API 资源类型映射到详细文档，支持 API 级别的知识检索。

## 同义词与别名词典

统一术语表达，解决同一概念多名称的检索问题：
- `Pod` = `容器组` = `豆荚`
- `Service` = `服务` = `svc`
- `ConfigMap` = `配置映射` = `cm`

---

> 来源：docs/TAG-DICTIONARY.md, docs/FRONTMATTER-SPEC.md, docs/SCENARIO-TAXONOMY.md, docs/SYNONYM-DICTIONARY.md, docs/ERROR-FTA-MAP.md, docs/COMMAND-DOC-MAP.md, docs/API-DOC-MAP.md

## Related

- [[entities/kudig-gitbook-system.md|kudig-gitbook-system]] — Gitbook 本地文档浏览系统与构建指南
- [[kudig-templates-catalog]] — KUDIG 文档模板目录
- [[entities/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]] — KUDIG Frontmatter Specification
- [[故障诊断/FTA故障树/list/pod-fta.md|pod-fta]] — pod-fta
- [[故障诊断/FTA故障树/list/node-fta.md|node-fta]] — node-fta


<!-- risk-assessed -->
