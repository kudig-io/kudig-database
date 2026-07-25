---
title: Keylime 远程证明
description: Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验...
summary: Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验...
category: dictionary
tags:
- k8s
- glossary
- security
- attestation
- tpm
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keylime 远程证明 是什么
- Keylime 详解
trigger_keywords:
- Keylime 远程证明
- Keylime
- dictionary
prerequisites:
- kubernetes
---



# Keylime 远程证明（Keylime）

## 概述

Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验证远程系统的完整性和可信状态。

## 核心概念/原理

- **远程证明**：验证远程系统的启动和运行状态
- **TPM 基础**：利用 TPM 2.0 硬件信任根
- **CNCF Sandbox**：MITRE 主导
- **Linux 专注**：为 Linux 系统设计

## 关键机制或特性

- Agent（被测系统）+ Verifier（验证者）架构
- TPM Quote 采集和验证
- IMA（Integrity Measurement Architecture）日志
- 可信启动链验证
- 密钥分发和绑定
- 证书管理
- REST API 和 CLI

## 使用场景与最佳实践

- 服务器启动完整性验证
- 边缘设备的信任验证
- 合规要求的系统完整性监控
- 零信任架构的硬件信任根
- 机密计算的远程证明

## 参考链接

- https://keylime.dev/
- https://github.com/keylime/keylime

## Related

- [[17-系统基础/06-知识字典/security/confidential-containers.md|Confidential Containers]]
- [[17-系统基础/06-知识字典/security/parsec.md|PARSEC]]
- [[17-系统基础/06-知识字典/security/spire.md|SPIRE]]
