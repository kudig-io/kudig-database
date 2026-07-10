---
title: OPCo 策略容器
description: Open Policy Containers（OPCo）将安全策略打包为 OCI 镜像，通过标准容器 Registry 分发和管理策略，实现策略的版本控制和跨平...
summary: Open Policy Containers（OPCo）将安全策略打包为 OCI 镜像，通过标准容器 Registry 分发和管理策略，实现策略的版本控制和跨平...
category: dictionary
tags:
- k8s
- glossary
- security
- policy
- oci
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OPCo 策略容器 是什么
- Open Policy Containers 详解
trigger_keywords:
- OPCo 策略容器
- Open Policy Containers
- dictionary
prerequisites:
- kubernetes
---



# OPCo 策略容器（Open Policy Containers）

## 概述

Open Policy Containers（OPCo）将安全策略打包为 OCI 镜像，通过标准容器 Registry 分发和管理策略，实现策略的版本控制和跨平台分发。

## 核心概念/原理

- **策略即 OCI**：将策略打包为标准 OCI 镜像
- **Registry 分发**：通过容器 Registry 管理策略
- **多引擎**：支持 OPA/Rego/Kyverno 等策略引擎
- **OCI 标准**：利用 OCI Artifact 规范

## 关键机制或特性

- `policy push/pull/sign` 管理策略镜像
- 支持 Rego/Kyverno/Cedar 策略格式
- OCI Artifact 存储策略
- 策略签名和验证（Cosign/Notation）
- 策略版本管理和标签
- 与 Gatekeeper/Kyverno 集成

## 使用场景与最佳实践

- 策略的版本控制和分发
- 多集群的策略同步
- GitOps 策略管理
- 策略的安全签名和验证
- 策略库的集中管理

## 参考链接

- https://openpolicycontainers.com/
- https://github.com/opcr-io/policy

## Related

- [[系统基础/topic-dictionary/security/opa.md|OPA]]
- [[系统基础/topic-dictionary/security/kyverno.md|Kyverno]]
- [[系统基础/topic-dictionary/security/notary-project.md|Notary Project]]
