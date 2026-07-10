---
title: 云凭证管理 CCO
description: Cloud Credential Operator（CCO）是 Red Hat 开源的 K8s Operator，自动管理云提供商凭证（IAM
  Roles/Se...
summary: Cloud Credential Operator（CCO）是 Red Hat 开源的 K8s Operator，自动管理云提供商凭证（IAM Roles/Se...
category: dictionary
tags:
- k8s
- glossary
- multi-cloud
- credentials
- openshift
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云凭证管理 CCO 是什么
- Cloud Credential Operator 详解
trigger_keywords:
- 云凭证管理 CCO
- Cloud Credential Operator
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云凭证管理 CCO（Cloud Credential Operator）

## 概述

Cloud Credential Operator（CCO）是 Red Hat 开源的 K8s Operator，自动管理云提供商凭证（IAM Roles/Service Accounts），为集群组件和 Operator 安全分发最小权限的云访问凭证。

## 核心概念/原理

- **凭证自动化**：自动创建和管理云凭证
- **最小权限**：为每个组件生成精确的 IAM 策略
- **多 Provider**：AWS/Azure/GCP/OpenStack
- **OpenShift 核心**：OpenShift 安装流程的核心组件

## 关键机制或特性

- CredentialsRequest CRD 声明云凭证需求
- Mint 模式（自动创建 IAM 用户/角色）
- Passthrough 模式（使用共享凭证）
- Manual 模式（管理员手动配置）
- STS/Workload Identity 集成
- 凭证轮转和审计
- ccoctl 命令行工具

## 使用场景与最佳实践

- 集群组件的云权限自动管理
- 最小权限原则的执行
- 多账户/多项目的凭证隔离
- 合规要求下的凭证审计
- 最佳实践：使用 STS/Workload Identity、定期审计、最小权限

## 参考链接

- https://github.com/openshift/cloud-credential-operator
- https://docs.openshift.com/container-platform/latest/authentication/managing_cloud_provider_credentials/

## Related

- [[系统基础/topic-dictionary/multi-cloud/cluster-api.md|Cluster API]]
- [[系统基础/topic-dictionary/security/spiffe.md|SPIFFE]]
- [[系统基础/topic-dictionary/security/vault.md|Vault]]


<!-- risk-assessed -->
