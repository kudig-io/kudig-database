---
title: 机密容器
description: Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保...
summary: Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保...
category: dictionary
tags:
- k8s
- glossary
- security
- tee
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 机密容器 是什么
- Confidential Containers 详解
trigger_keywords:
- 机密容器
- Confidential Containers
- dictionary
prerequisites:
- kubernetes
---



# 机密容器（Confidential Containers）

## 概述

Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保护容器内的数据和代码，即使基础设施提供者也无法访问。

## 核心概念/原理

- **硬件 TEE**：利用 Intel SGX/TDX、AMD SEV、ARM CCA 等硬件安全扩展
- **Kubernetes 集成**：通过 RuntimeClass 透明使用机密容器
- **零信任**：保护数据在使用中的机密性（Data in Use）
- **CNCF Sandbox**：Intel/IBM/微软等联合推动

## 关键机制或特性

- Kata Containers + TEE 后端（Guest attestation）
- 远程证明（Remote Attestation）验证运行环境
- Peer Pods 支持裸金属和云 VM
- 机密计算友好的密钥管理（密钥只在 TEE 内可用）
- CoCo Operator 简化部署和配置
- 与 Key Broker Service（KBS）集成

## 使用场景与最佳实践

- 多方数据协作（数据可用但不可见）
- 金融/医疗等高敏感数据处理
- 多租户环境下的强隔离
- 云环境中保护租户工作负载
- 合规要求下的数据加密计算

## 参考链接

- https://confidentialcontainers.org/
- https://github.com/confidential-containers

## Related

- [[系统基础/知识字典/fundamentals/kata-containers.md|Kata Containers]]
- [[系统基础/知识字典/security/vault.md|Vault]]
- [[系统基础/知识字典/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
