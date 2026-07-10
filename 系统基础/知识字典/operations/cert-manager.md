---
title: cert-manager
description: cert-manager 是 Kubernetes 原生的证书管理工具，自动化 TLS 证书的签发、续期和吊销。它支持 Let's Encrypt、Vault、...
summary: cert-manager 是 Kubernetes 原生的证书管理工具，自动化 TLS 证书的签发、续期和吊销。它支持 Let's Encrypt、Vault、...
category: dictionary
tags:
- k8s
- glossary
- cert-manager
- certificate
- tls
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager 是什么
- cert-manager 详解
trigger_keywords:
- cert-manager
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cert-manager

> **英文名**: cert-manager

## 概述

cert-manager 是 Kubernetes 原生的证书管理工具，自动化 TLS 证书的签发、续期和吊销。它支持 Let's Encrypt、Vault、自签名 CA 等多种证书颁发源，是集群 TLS 自动化的标准方案。

## 核心概念/原理

### 核心资源

| 资源 | 功能 |
|------|------|
| Issuer | 命名空间级别的证书颁发源 |
| ClusterIssuer | 集群级别的证书颁发源 |
| Certificate | 声明式证书记义 |
| CertificateRequest | 证书签发请求 |
| Order/Challenge | ACME 协议交互 |

### ACME 流程

```
Certificate → CertificateRequest → Order → Challenge (HTTP-01/DNS-01) → Let's Encrypt → 签发证书
```

## 关键机制或特性

- **ACME 协议**：支持 Let's Encrypt 的 HTTP-01 和 DNS-01 验证。
- **自动续期**：证书到期前自动续期（默认 2/3 生命周期时触发）。
- **Vault 集成**：支持 HashiCorp Vault 作为证书颁发源。
- **istio-csr**：为 Istio 提供自动化的 mTLS 证书管理。
- **approve**：内置 RBAC 控制证书审批流程。

## 使用场景与最佳实践

- 为 Ingress 资源自动签发 Let's Encrypt 证书（配合 cert-manager annotation）。
- 生产环境使用 ClusterIssuer 统一管理证书颁发源。
- DNS-01 验证适合通配符证书（*.example.com）。
- 监控证书到期时间，设置 30 天到期告警。
- 考虑使用 step-ca 或 Vault PKI 作为内部 CA。

## 参考链接

- [cert-manager Official](https://cert-manager.io/docs/)

## Related

- [[系统基础/知识字典/security/certificate.md|Certificate]]
- [[系统基础/知识字典/security/certificate-authority.md|Certificate Authority]]
- [[系统基础/知识字典/networking/ingress.md|Ingress]]
- [[系统基础/知识字典/security/webhook.md|Webhook]]
- [[系统基础/知识字典/networking/istio.md|Istio]]


<!-- risk-assessed -->
