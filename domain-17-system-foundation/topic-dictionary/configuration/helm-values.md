---
title: Helm Values 配置值
description: 'Helm Values 是 Helm Chart 的参数化配置机制，通过 values.yaml 文件定义模板变量，实现同一 Chart 在不同环境下的差异化部...'
category: dictionary
tags:
- k8s
- glossary
- configuration
- helm
- templating
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm Values 配置值 是什么
- Helm Values 详解
trigger_keywords:
- Helm Values 配置值
- Helm Values
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Helm Values 配置值（Helm Values）

## 概述

Helm Values 是 Helm Chart 的参数化配置机制，通过 values.yaml 文件定义模板变量，实现同一 Chart 在不同环境下的差异化部署，是 Helm 模板系统的核心配置入口。

## 核心概念/原理

- **参数化**：values.yaml 定义 Chart 的所有可配置参数
- **层级覆盖**：支持 --set/--values/-f 多层覆盖
- **Go Template**：在模板中通过 `.Values.xxx` 引用
- **默认值**：Chart 内置的 values.yaml 作为默认

## 关键机制或特性

- values.yaml 默认值文件
- `--set key=value` 命令行覆盖
- `--values file.yaml` 文件覆盖
- `--set-file` 从文件读取值
- 嵌套值（`global.image.tag`）
- 条件渲染（`{{ if .Values.enabled }}`）
- values.schema.json 校验

## 使用场景与最佳实践

- 多环境（dev/staging/prod）的差异化部署
- Chart 参数化复用
- 应用配置的外部化管理
- CI/CD 中的动态配置注入
- 最佳实践：默认值兜底、分层覆盖、schema 校验、避免过度嵌套

## 参考链接

- https://helm.sh/docs/chart_template_guide/values_files/
- https://helm.sh/docs/intro/using_helm/

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/configuration/configmap.md|ConfigMap]]
- [[domain-17-system-foundation/topic-dictionary/configuration/env.md|Environment Variables]]
