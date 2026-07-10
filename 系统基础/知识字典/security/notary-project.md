---
title: Notary Project 容器签名
description: Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安...
summary: Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安...
category: dictionary
tags:
- k8s
- glossary
- security
- supply-chain
- signing
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Notary Project 容器签名 是什么
- Notary Project 详解
trigger_keywords:
- Notary Project 容器签名
- Notary Project
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Notary Project 容器签名（Notary Project）

## 概述

Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安全的基石组件。

## 核心概念/原理

- **OCI 签名**：为容器镜像和 OCI 制品附加数字签名
- **签名验证**：在拉取和部署时验证签名的完整性和来源
- **CNCF 孵化**：Docker/Microsoft/VMware 等联合推动
- **跨 Registry**：签名与制品分离存储，支持跨 Registry 传播

## 关键机制或特性

- `notation sign` 对 OCI 制品签名
- `notation verify` 验证签名
- 支持多种密钥后端（本地文件、Azure Key Vault、AWS KMS）
- Trust Store 和 Trust Policy 管理
- 签名存储在 OCI Registry 的 Referrers API
- 与 Kyverno/OPA Gatekeeper/Ratify 集成验证

## 使用场景与最佳实践

- CI/CD Pipeline 中的镜像签名和验证
- 生产部署前的镜像来源验证
- 合规要求下的软件供应链审计
- 多环境镜像复制时的完整性保障
- Kubernetes Admission 策略中的签名验证

## 参考链接

- https://notaryproject.dev/
- https://github.com/notaryproject/notation

## Related

- [[系统基础/知识字典/security/ratify.md|Ratify]]
- [[系统基础/知识字典/security/in-toto.md|in-toto]]
- [[系统基础/知识字典/security/trivy.md|Trivy]]


<!-- risk-assessed -->
