---
title: Consul
description: Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy
  sidecar）等功能...
summary: Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy sidecar）等功能...
category: dictionary
tags:
- k8s
- glossary
- consul
- service-mesh
- service-discovery
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Consul 是什么
- Consul 详解
trigger_keywords:
- Consul
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Consul

> **英文名**: Consul

## 概述

Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy sidecar）等功能，支持多数据中心和多云部署。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Service Discovery | 服务注册和 DNS/HTTP 发现 |
| Health Checking | 多维度健康检查 |
| KV Store | 分布式键值存储 |
| Service Mesh | 基于 Envoy 的 L7 流量管理 |
| Multi-DC | 多数据中心联邦 |

### 与 K8s Service 对比

Consul 可补充 K8s 的服务发现：跨集群、非 K8s 服务、多数据中心场景。

## 关键机制或特性

- **Consul Connect**：基于 Envoy 的 mTLS 服务网格。
- **Catalog Sync**：K8s Service 与 Consul Catalog 双向同步。
- **Intentions**：声明式的服务间访问控制策略。
- **Mesh Gateway**：跨数据中心的服务网格通信。
- 支持 Terraform 管理 Consul 配置。

## 使用场景与最佳实践

- 混合云/多云场景使用 Consul 统一服务发现。
- 非 K8s 服务（VM、裸金属）需要纳入服务网格时使用 Consul。
- 使用 Consul KV 存储应用配置。
- 配合 Vault 实现服务间证书管理。
- 使用 `consul-k8s` CLI 安装到 Kubernetes。

## 参考链接

- [Consul Official](https://www.consul.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/istio.md|Istio]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/coredns.md|CoreDNS]]
- [[domain-17-system-foundation/topic-dictionary/security/vault.md|Vault]]


<!-- risk-assessed -->
