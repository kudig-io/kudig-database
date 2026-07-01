---
title: Operator Framework 框架
description: 'Operator Framework 是 Red Hat 开源的 Kubernetes Operator 开发和管理框架，包含 Operator SDK、OLM...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- operator
- sdk
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator Framework 框架 是什么
- Operator Framework 详解
trigger_keywords:
- Operator Framework 框架
- Operator Framework
- dictionary
prerequisites:
- kubernetes
created: 2026-06
created: 2026-05
---

# Operator Framework 框架（Operator Framework）

## 概述

Operator Framework 是 Red Hat 开源的 Kubernetes Operator 开发和管理框架，包含 Operator SDK、OLM（Operator Lifecycle Manager）和 OperatorHub，是 Operator 开发的行业标准工具链。

## 核心概念/原理

- **Operator SDK**：Go/Ansible/Helm/Python 多语言 Operator 开发脚手架
- **OLM**：Operator 的生命周期管理（安装/升级/卸载）
- **OperatorHub**：Operator 的发现和分发市场
- **Red Hat 主导**：OpenShift 生态的核心工具链

## 关键机制或特性

- `operator-sdk init` 初始化项目（Go/Ansible/Helm/Java）
- `operator-sdk generate` 代码和 CRD 生成
- OLM Catalog 管理 Operator 版本和更新通道
- Scorecard 测试框架
- Bundle Format 打包标准
- OperatorHub.io 社区市场

## 使用场景与最佳实践

- Kubernetes Operator 的标准化开发
- Operator 的版本管理和自动升级
- 企业内部 Operator 的分发和管理
- Red Hat OpenShift 的 Operator 认证
- Operator 生态的集成和发布

## 参考链接

- https://operatorframework.io/
- https://github.com/operator-framework/operator-sdk

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubevela.md|KubeVela]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane.md|Crossplane]]
