---
title: 环境变量配置
description: 环境变量（Env）是 Kubernetes Pod/Container 级别的配置注入机制，通过 env/envFrom 字段将配置值、ConfigMap
  和 ...
summary: 环境变量（Env）是 Kubernetes Pod/Container 级别的配置注入机制，通过 env/envFrom 字段将配置值、ConfigMap
  和 ...
category: dictionary
tags:
- k8s
- glossary
- configuration
- env
- configmap
- secret
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 环境变量配置 是什么
- Environment Variables 详解
trigger_keywords:
- 环境变量配置
- Environment Variables
- dictionary
prerequisites:
- kubernetes
---



# 环境变量配置（Environment Variables）

## 概述

环境变量（Env）是 Kubernetes Pod/Container 级别的配置注入机制，通过 env/envFrom 字段将配置值、ConfigMap 和 Secret 注入到容器中，是 12-Factor App 的配置管理实践。

## 核心概念/原理

- **env**：逐个定义环境变量（支持 value/valueFrom）
- **envFrom**：批量导入 ConfigMap/Secret 所有键值
- **valueFrom**：引用 ConfigMapKeyRef/SecretKeyRef/FieldRef/ResourceFieldRef
- **12-Factor**：配置与代码分离的标准实践

## 关键机制或特性

- `env.name` + `env.value` 静态定义
- `env.valueFrom.configMapKeyRef` 引用 ConfigMap
- `env.valueFrom.secretKeyRef` 引用 Secret
- `env.valueFrom.fieldRef` 引用 Pod 元数据（name/namespace/ip）
- `env.valueFrom.resourceFieldRef` 引用资源限制
- `envFrom.configMapRef` 批量导入
- 环境变量变更需要重启 Pod（不同于 Volume 挂载的热更新）

## 使用场景与最佳实践

- 应用配置的外部化注入
- 数据库连接串和 API Key 的安全传递
- 多环境（dev/staging/prod）的配置差异化
- Pod 元数据注入（Downward API）
- 最佳实践：敏感信息用 Secret、批量配置用 envFrom、避免硬编码

## 参考链接

- https://kubernetes.io/docs/tasks/inject-data-application/define-environment-variable-container/
- https://kubernetes.io/docs/concepts/configuration/configmap/

## Related
- [[17-系统基础/06-知识字典/configuration/configmap.md|ConfigMap]]
- [[17-系统基础/06-知识字典/security/secret.md|Secret]]
