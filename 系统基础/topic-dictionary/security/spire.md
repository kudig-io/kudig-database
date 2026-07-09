---
title: SPIRE 身份框架
description: SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目，实现 SPIFFE 规范的生产级参考实现，为工作负载提供通用的加密身份...
summary: SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目，实现 SPIFFE 规范的生产级参考实现，为工作负载提供通用的加密身份...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- spiffe
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIRE 身份框架 是什么
- SPIRE 详解
trigger_keywords:
- SPIRE 身份框架
- SPIRE
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SPIRE 身份框架（SPIRE）

## 概述

SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目，实现 SPIFFE 规范的生产级参考实现，为工作负载提供通用的加密身份框架，自动签发和管理短期 X.509 证书和 JWT。

## 核心概念/原理

- **SPIFFE 实现**：SPIFFE 标准的生产级参考实现
- **自动身份**：基于节点和工作负载属性自动分配身份
- **短期凭证**：自动签发和轮转短期 X.509 SVID 和 JWT-SVID
- **CNCF 毕业**：经过大规模生产验证

## 关键机制或特性

- Server + Agent 分布式架构
- Node Attestation（节点证明）多种插件
- Workload Attestation（工作负载证明）
- SVID 自动签发和轮转（X.509 / JWT）
- Federation API 跨域联邦
- 支持 Kubernetes、AWS、GCP 等多平台
- 与 Envoy SDS API 集成

## 使用场景与最佳实践

- 微服务间的 mTLS 自动管理
- 零信任网络中的工作负载身份
- 多集群/多云的身份联邦
- Kubernetes 工作负载的身份认证
- 与 Istio/Envoy 集成的服务网格身份

## 参考链接

- https://spiffe.io/spire/
- https://github.com/spiffe/spire

## Related

- [[系统基础/topic-dictionary/security/spiffe-spire-identity.md|SPIFFE/SPIRE]]
- [[系统基础/topic-dictionary/operations/cert-manager.md|cert-manager]]
- [[系统基础/topic-dictionary/networking/istio.md|Istio]]


<!-- risk-assessed -->
