---
title: TUF 更新框架
description: The Update Framework（TUF）是 CNCF 毕业项目，为软件更新提供密码学安全框架，防止更新过程中的篡改、回滚攻击和密钥泄露，是软件供应链安...
summary: The Update Framework（TUF）是 CNCF 毕业项目，为软件更新提供密码学安全框架，防止更新过程中的篡改、回滚攻击和密钥泄露，是软件供应链安...
category: dictionary
tags:
- k8s
- glossary
- security
- supply-chain
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
- TUF 更新框架 是什么
- TUF 详解
trigger_keywords:
- TUF 更新框架
- TUF
- dictionary
prerequisites:
- kubernetes
---



# TUF 更新框架（TUF）

## 概述

The Update Framework（TUF）是 CNCF 毕业项目，为软件更新提供密码学安全框架，防止更新过程中的篡改、回滚攻击和密钥泄露，是软件供应链安全的基础设施。

## 核心概念/原理

- **安全更新**：通过签名验证和元数据机制确保软件更新的安全性
- **密钥轮转**：支持在线/离线密钥分离和定期轮转
- **CNCF 毕业**：经过大规模生产验证
- **广泛采用**：PyPI、Notary、Sigstore 等均使用 TUF

## 关键机制或特性

- 四级密钥层次（Root/Targets/Snapshot/Timestamp）
- 在线/离线密钥分离（降低密钥泄露风险）
- 版本号和过期时间管理
- 委托（Delegation）机制支持多签名者
- 参考实现（python-tuf / go-tuf / rust-tuf）
- Sigstore 的 TUF Root 信任链

## 使用场景与最佳实践

- 软件分发系统的安全更新机制
- 容器 Registry 的内容完整性保障
- OTA（Over-the-Air）更新的安全验证
- 供应链中的信任链建立
- 与 Notary/Sigstore 集成的综合安全方案

## 参考链接

- https://theupdateframework.io/
- https://github.com/theupdateframework/specification

## Related

- [[domain-17-system-foundation/topic-dictionary/security/notary-project.md|Notary Project]]
- [[domain-17-system-foundation/topic-dictionary/security/in-toto.md|in-toto]]
- [[domain-17-system-foundation/topic-dictionary/security/supply-chain-security.md|供应链安全]]
