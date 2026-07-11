---
title: 供应链安全、YAML 配置清单与速查表
description: '# 供应链安全、YAML 配置清单与速查表'
summary: '1. **SBOM（Software Bill of Materials）**：软件物料清单'
category: reference
tags:
- k8s
- supply-chain-security
- sbom
- slsa
- sigstore
- yaml
- cheat-sheet
- docker
- ingress
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 供应链安全、YAML 配置清单与速查表 是什么
- 如何 供应链安全、YAML 配置清单与速查表
trigger_keywords:
- 供应链安全
- YAML
- 配置清单与速查表
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 供应链安全、YAML 配置清单与速查表

> **CNCF 状态**: 参考文档 | **类别**: Supply Chain Security | **主要语言**: YAML

## 概述

Kubernetes 供应链安全 YAML 速查表是一份涵盖 K8s 供应链安全各环节配置的快速参考文档。它整合了 SLSA 框架、Sigstore 签名验证、SBOM 生成、镜像策略准入、GitOps 安全配置等关键供应链安全实践的 YAML 配置示例。该文档为 DevSecOps 团队提供从代码提交到生产部署全链条的安全配置参考，帮助实施 SLSA Level 1-4 的供应链安全控制。

## Key Features（核心能力）

- **SLSA 合规配置**：SLSA Build Level 1-4 的构建来源验证和完整性证明配置
- **镜像签名验证**：Cosign/Notation 的镜像签名和 Admission Controller 验证策略
- **SBOM 生成**：Synergy/CycloneDX 的 SBOM 生成和存储配置
- **策略准入**：Kyverno/Cosign Gatekeeper 的镜像安全策略 YAML
- **GitOps 安全**：ArgoCD/Flux 的安全配置和签名验证策略
- **密钥管理**：Sealed Secrets/SOPS 的加密配置 YAML

## 架构与工作原理

供应链安全配置覆盖四个阶段：Source（代码来源验证、提交签名）、Build（构建来源证明、SBOM 生成、镜像签名）、Package（Registry 策略、镜像扫描）、Deploy（准入验证、策略执行）。每个阶段的 YAML 配置通过 K8s CRD 或控制器配置实施，形成从代码到生产的安全链条。

## K8s 集成

在 K8s 中，供应链安全通过多种 API 对象实施：ValidatingWebhookConfiguration 执行镜像签名验证策略；ClusterPolicy（Kyverno CRD）定义镜像来源限制；ConfigMap 承载 Cosign 公钥和验证规则；Secret 存储签名密钥。CI/CD 流水线中的 Cosign 签名步骤和 K8s 部署时的验证策略通过共享的公钥/策略配置关联。

## 生产用例

- **DevSecOps 流水线**：在 CI/CD 中实施镜像签名和安全扫描
- **合规要求实施**：满足 SLSA、NIST SSDF 供应链安全框架要求
- **镜像来源控制**：限制集群仅部署经过签名的可信镜像
- **安全审计准备**：快速配置和验证供应链安全控制措施

## 安装与快速开始

```bash
# Cosign 镜像签名
cosign sign --key cosign.key my-registry/app:v1

# Kyverno 镜像签名验证策略
kubectl apply -f https://raw.githubusercontent.com/kyverno/policies/main/verify_images/verify-image-signatures/verify-image-signatures.yaml
```

## 对比替代方案

相比传统供应链安全方案，K8s 原生方案利用 Admission Controller 实现运行时强制验证。相比商业方案，开源方案（Sigstore + Kyverno）更灵活但需要更多配置工作。

## Related

- [[概念/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — 纵深防御 x 供应链安全
- [[docker]] — Docker
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[实体/trivy.md|trivy]] — Trivy


<!-- risk-assessed -->
