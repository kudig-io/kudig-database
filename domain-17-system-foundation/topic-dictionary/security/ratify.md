---
title: Ratify 准入验证
description: Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM
  和漏洞扫...
summary: Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM
  和漏洞扫...
category: dictionary
tags:
- k8s
- glossary
- security
- admission
- supply-chain
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ratify 准入验证 是什么
- Ratify 详解
trigger_keywords:
- Ratify 准入验证
- Ratify
- dictionary
prerequisites:
- kubernetes
---



# Ratify 准入验证（Ratify）

## 概述

Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM 和漏洞扫描结果等供应链元数据。

## 核心概念/原理

- **准入验证**：作为 External Data Provider 为 Gatekeeper 提供验证数据
- **多验证器**：支持 Notary 签名、Cosign 签名、SBOM 验证、漏洞扫描验证
- **可扩展**：插件式验证器架构
- **Azure 背景**：微软主导，与 Azure 生态深度集成

## 关键机制或特性

- 与 OPA Gatekeeper 的 External Data 集成
- Notation / Cosign 签名验证
- SBOM 存在性和格式验证
- 漏洞扫描结果验证（Trivy/Grype）
- Certificate Store 管理签名证书
- VerificationResult 标准化输出

## 使用场景与最佳实践

- 生产集群的镜像签名强制验证
- CI/CD 中的供应链安全检查门控
- 合规要求下的 SBOM 验证
- 多来源镜像的统一准入策略
- 与 Kyverno/Gatekeeper 配合的策略引擎

## 参考链接

- https://ratify.dev/
- https://github.com/ratify-project/ratify

## Related

- [[domain-17-system-foundation/topic-dictionary/security/notary-project.md|Notary Project]]
- [[domain-17-system-foundation/topic-dictionary/security/opa.md|OPA Gatekeeper]]
- [[domain-17-system-foundation/topic-dictionary/security/kyverno.md|Kyverno]]
