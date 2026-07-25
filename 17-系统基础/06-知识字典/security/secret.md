---
title: 密钥
description: Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap
  更强的安全控制机...
summary: Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap 更强的安全控制机...
category: dictionary
tags:
- k8s
- glossary
- secret
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 密钥 是什么
- Secret 详解
trigger_keywords:
- 密钥
- Secret
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 密钥

> **英文名**: Secret

## 概述

Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap 更强的安全控制机制。

## 核心概念/原理

### Secret 类型

- **Opaque**：通用 Secret（默认类型）。
- **kubernetes.io/tls**：TLS 证书和私钥。
- **kubernetes.io/dockerconfigjson**：容器镜像仓库认证。
- **kubernetes.io/basic-auth**：基本认证凭据。
- **kubernetes.io/ssh-auth**：SSH 认证密钥。
- **kubernetes.io/service-account-token**：ServiceAccount Token。

### 安全措施

- etcd 加密：启用 EncryptionConfiguration 加密 Secret 数据。
- RBAC：限制对 Secret 资源的访问权限。
- 外部密钥管理：集成 Vault、AWS Secrets Manager 等。

## 关键机制或特性

- Secret 数据以 Base64 编码存储（非加密），需配合 etcd 加密。
- Secret 大小限制 1MB。
- 使用 `stringData` 字段可以用明文方式创建 Secret（自动转换为 Base64）。
- Volume 挂载的 Secret 更新会自动传播。

## 使用场景与最佳实践

- 生产环境使用 External Secrets Operator 集成外部密钥管理系统。
- 启用 etcd 加密确保 Secret 数据安全。
- 通过 RBAC 严格控制 Secret 的访问权限。
- 避免将 Secret 硬编码在 YAML 文件中并提交到 Git。

## 参考链接

- [Secret - Official Documentation](https://kubernetes.io/docs/concepts/configuration/secret/)

## Related

[[17-系统基础/06-知识字典/configuration/secrets.md|Secrets]]


<!-- risk-assessed -->
