---
title: PARSEC 机密计算
description: PARSEC（Platform AbstRaction for SECurity）是 CNCF Sandbox 项目，为应用提供统一的加密和安全服务
  API，屏...
summary: PARSEC（Platform AbstRaction for SECurity）是 CNCF Sandbox 项目，为应用提供统一的加密和安全服务
  API，屏...
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
- PARSEC 机密计算 是什么
- PARSEC 详解
trigger_keywords:
- PARSEC 机密计算
- PARSEC
- dictionary
prerequisites:
- kubernetes
---



# PARSEC 机密计算（PARSEC）

## 概述

PARSEC（Platform AbstRaction for SECurity）是 CNCF Sandbox 项目，为应用提供统一的加密和安全服务 API，屏蔽底层 TEE（可信执行环境）和 HSM 的差异，简化机密计算的集成。

## 核心概念/原理

- **安全 API 抽象**：统一的加密/签名/认证 API
- **TEE 无关**：支持 Intel SGX/TDX、ARM TrustZone、TPM 等
- **CNCF Sandbox**：Arm/Intel 联合推动
- **简化集成**：应用无需关心底层安全硬件

## 关键机制或特性

- Parsec API 定义标准安全操作接口
- 多种后端 Provider（PKCS#11/TPM/Mbed Crypto/Trusted Service）
- 密钥管理（创建/使用/删除）
- 加密/解密/签名/验证
- 认证和证明
- SDK 支持 Rust/C/Go/Python/Java

## 使用场景与最佳实践

- 机密计算应用的快速集成
- 多云/多硬件的安全抽象
- IoT 设备的安全服务
- 密钥管理的统一接口
- TEE 应用的开发和部署

## 参考链接

- https://parallaxsecond.github.io/parsec/
- https://github.com/parallaxsecond/parsec

## Related

- [[17-系统基础/06-知识字典/security/confidential-containers.md|Confidential Containers]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
