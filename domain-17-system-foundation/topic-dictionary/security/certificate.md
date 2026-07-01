---
title: 证书
description: Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server
  的 HTTPS 端点和...
summary: Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server 的 HTTPS
  端点和...
category: dictionary
tags:
- k8s
- glossary
- security
- certificate
- tls
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 证书 是什么
- Certificate 详解
trigger_keywords:
- 证书
- Certificate
- dictionary
prerequisites:
- kubectl-basics
---



# 证书

> **英文名**: Certificate

## 概述

Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server 的 HTTPS 端点和 Ingress TLS 终止都依赖证书。

## 核心概念/原理

### 证书用途

- **组件间通信**：API Server、etcd、kubelet 之间的 mTLS。
- **API Server HTTPS**：对外提供 HTTPS 服务。
- **Ingress TLS**：终止 HTTPS 流量。
- **Webhook**：准入 Webhook 的 TLS 认证。

### 证书管理方式

- **kubeadm 自动生成**：集群初始化时自动生成所有证书。
- **cert-manager**：CNCF 项目，自动化证书生命周期管理。
- **手动管理**：使用 cfssl 或 openssl 生成。

## 关键机制或特性

- Kubernetes 使用 PKI（Public Key Infrastructure）管理证书。
- CA（Certificate Authority）是根证书，签发其他证书。
- 证书有有效期，需要定期轮转。
- cert-manager 支持 Let's Encrypt、Vault 等多种 Issuer。

## 使用场景与最佳实践

- 使用 cert-manager 自动化证书管理（推荐）。
- 监控证书过期时间，设置过期前告警。
- 为 Ingress TLS 使用 Let's Encrypt 免费证书。
- 定期检查证书链完整性。

## 参考链接

- [Certificate - Official Documentation](https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|Rbac]]
- [[domain-17-system-foundation/topic-dictionary/security/role.md|Role]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrole.md|Clusterrole]]
- [[domain-17-system-foundation/topic-dictionary/security/rolebinding.md|Rolebinding]]
- [[domain-17-system-foundation/topic-dictionary/security/clusterrolebinding.md|Clusterrolebinding]]
