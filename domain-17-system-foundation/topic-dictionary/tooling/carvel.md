---
title: Carvel K8s 工具集
description: Carvel（原 K14s）是 VMware 开源的 Kubernetes 工具集，包含 ytt（YAML 模板）、kapp（应用部署）、kbld（镜像构建）、...
summary: Carvel（原 K14s）是 VMware 开源的 Kubernetes 工具集，包含 ytt（YAML 模板）、kapp（应用部署）、kbld（镜像构建）、...
category: dictionary
tags:
- k8s
- glossary
- tooling
- configuration
- vmware
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Carvel K8s 工具集 是什么
- Carvel 详解
trigger_keywords:
- Carvel K8s 工具集
- Carvel
- dictionary
prerequisites:
- kubernetes
---



# Carvel K8s 工具集（Carvel）

## 概述

Carvel（原 K14s）是 VMware 开源的 Kubernetes 工具集，包含 ytt（YAML 模板）、kapp（应用部署）、kbld（镜像构建）、kwt（网络隧道）等一组轻量级互补工具。

## 核心概念/原理

- **工具集**：一组专注于单一功能的轻量级 CLI 工具
- **可组合**：工具间通过标准输入输出自由组合
- **VMware 开源**：Tanzu 生态的核心工具链
- **UNIX 哲学**：每个工具做好一件事

## 关键机制或特性

- ytt：YAML 模板引擎（Starlark 脚本）
- kapp：声明式应用部署（diff + apply）
- kbld：镜像构建和引用解析
- kwt：K8s 网络隧道（本地访问集群网络）
- vendir：依赖管理（下载 Helm/Git/HTTP 资源）
- imgpkg：镜像打包和分发

## 使用场景与最佳实践

- YAML 配置的模板化和复用
- K8s 应用的声明式部署
- 镜像构建和引用自动化
- 本地开发环境的网络打通
- Helm Chart 的依赖管理

## 参考链接

- https://carvel.dev/
- https://github.com/carvel-dev

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kpt.md|kpt]]
