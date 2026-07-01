---
title: SPIFFE 身份标准
description: 'SPIFFE（Secure Production Identity Framework for Everyone）是 CNCF 毕业项目，定义了工作负载身份的标...'
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIFFE 身份标准 是什么
- SPIFFE 详解
trigger_keywords:
- SPIFFE 身份标准
- SPIFFE
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# SPIFFE 身份标准（SPIFFE）

## 概述

SPIFFE（Secure Production Identity Framework for Everyone）是 CNCF 毕业项目，定义了工作负载身份的标准规范（SPIFFE ID + SVID），为跨平台和跨组织的微服务提供统一的安全身份框架。

## 核心概念/原理

- **身份标准**：定义工作负载身份的标准格式（spiffe://trust-domain/path）
- **SVID**：SPIFFE Verifiable Identity Document（X.509 或 JWT）
- **CNCF 毕业**：经过大规模生产验证
- **平台无关**：适用于任何平台和运行时

## 关键机制或特性

- SPIFFE ID 格式：`spiffe://<trust-domain>/<workload-path>`
- X.509-SVID：基于 X.509 证书的身份文档
- JWT-SVID：基于 JWT Token 的身份文档
- Trust Bundle：信任根分发机制
- Workload API：工作负载获取身份的标准接口
- Federation：跨信任域联邦

## 使用场景与最佳实践

- 微服务间的统一身份框架
- 零信任网络中的工作负载认证
- 跨组织/跨集群的身份联邦
- 与 Istio/Envoy/SPIRE 集成
- 合规要求下的身份管理标准化

## 参考链接

- https://spiffe.io/
- https://github.com/spiffe/spiffe

## Related

- [[domain-17-system-foundation/topic-dictionary/security/spire.md|SPIRE]]
- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
- [[domain-17-system-foundation/topic-dictionary/operations/cert-manager.md|cert-manager]]
