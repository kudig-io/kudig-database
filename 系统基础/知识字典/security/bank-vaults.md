---
title: Bank Vaults Vault 集成
description: Bank Vaults（vault-secrets-webhook + vault-operator）是 Banzai Cloud 开源的
  HashiCorp ...
summary: Bank Vaults（vault-secrets-webhook + vault-operator）是 Banzai Cloud 开源的 HashiCorp
  ...
category: dictionary
tags:
- k8s
- glossary
- security
- vault
- secrets
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Bank Vaults Vault 集成 是什么
- Bank Vaults 详解
trigger_keywords:
- Bank Vaults Vault 集成
- Bank Vaults
- dictionary
prerequisites:
- kubernetes
---



# Bank Vaults Vault 集成（Bank Vaults）

## 概述

Bank Vaults（vault-secrets-webhook + vault-operator）是 Banzai Cloud 开源的 HashiCorp Vault Kubernetes 集成工具集，通过 Webhook 自动注入 Vault 密钥到 Pod 环境变量和 Volume 中。

## 核心概念/原理

- **自动注入**：通过 Admission Webhook 自动从 Vault 拉取密钥
- **Vault Operator**：在 K8s 上管理 Vault 实例的生命周期
- **零改造**：应用无需修改代码即可使用 Vault 密钥
- **Banzai Cloud 出品**：活跃的 Vault K8s 集成方案

## 关键机制或特性

- vault-secrets-webhook：环境变量和 ConfigMap/Secret 的 Vault 引用替换
- vault-operator：Vault 集群的 K8s Operator（HA、备份、配置）
- 支持 Vault Agent Sidecar 注入
- 支持 Vault PKI 证书自动轮转
- 支持 Kubernetes Auth Method
- 与 External Secrets 互补使用

## 使用场景与最佳实践

- Vault 密钥的 K8s 原生集成
- 无需修改应用代码的密钥注入
- Vault 集群的自动化运维
- 合规要求下的密钥轮转和审计
- 多环境密钥管理的统一方案

## 参考链接

- https://github.com/bank-vaults/vault-secrets-webhook
- https://bank-vaults.dev/

## Related

- [[系统基础/知识字典/security/vault.md|Vault]]
- [[系统基础/知识字典/security/external-secrets.md|External Secrets]]
- [[系统基础/知识字典/security/sops.md|SOPS]]
