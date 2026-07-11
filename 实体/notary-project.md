---
title: Notary Project (entities)
description: '## 概述'
summary: 'Notary Project 提供容器镜像和 OCI 制品的签名、验证规范与工具。它是软件供应链安全的关键组件，通过数字签名确保制品的完整性和来源可信。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- notary-project
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Notary Project 是什么
- 如何 Notary Project
trigger_keywords:
- Notary
- Project
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Notary Project

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: Go

## 概述

Notary Project 提供容器镜像和 OCI 制品的签名、验证规范与工具，是软件供应链安全的关键组件。Notary Project 由 Docker 团队创建，2017 年加入 CNCF 孵化。它通过数字签名确保制品的完整性和来源可信，防止镜像在分发过程中被篡改。Notary Project V2（基于 notation CLI）采用 OCI 兼容的签名方案，签名作为独立的 manifest 存储在 Registry 中，不修改原始镜像。与 Cosign（Sigstore）不同，Notary 使用基于 X.509 证书的 PKI 体系，支持企业 CA、KMS（AWS KMS、Azure Key Vault、HashiCorp Vault）和硬件令牌（HSM）等多种密钥管理方案。Notary 是 SLSA（Supply Chain Levels for Software Artifacts）框架的关键工具。

## 核心能力

- **OCI 兼容签名规范**: 基于 OCI Image Spec 的签名格式，签名存储为独立 manifest
- **Notation CLI**: 签名和验证的命令行工具，支持多平台
- **多种密钥源**: 本地密钥、KMS（AWS KMS、Azure Key Vault、HashiCorp Vault）、硬件令牌（HSM）
- **信任策略**: 灵活的签名验证策略配置（trust policy，strict/permissive）
- **插件架构**: 支持第三方 KMS 和签名服务扩展
- **无侵入签名**: 签名存储在 OCI manifest，不修改原始镜像内容

## 架构

Notary Project V2 采用 OCI 原生签名架构：

- **Notation CLI**: 客户端工具，执行 `notation sign` 和 `notation verify` 操作
- **Signature Manifest**: OCI 1.1 兼容的签名 manifest，引用原始镜像 manifest 并包含签名数据
- **Signing Key**: X.509 证书 + 私钥，存储在 KMS/HSM 或本地
- **Trust Policy**: JSON 配置文件，定义信任的身份（证书 CN/O）和 Registry
- **Trust Store**: 存储受信任的根 CA 证书
- **Plugin**: 密钥管理插件（PKCS#11、AWS KMS、Azure Key Vault 等）

签名流程：`notation sign → 获取镜像 digest → 生成签名 → 推送 signature manifest → Registry`

## K8s 集成

Notary Project 在 Kubernetes 生态中通过策略控制器实现镜像准入验证。Kyverno 和 Ratify（由 Microsoft 开源）等策略引擎可以集成 Notary 验证——在 Pod 创建时自动验证镜像签名，拒绝未签名或不信任的镜像。通过 Validating Webhook 拦截 Pod 创建请求，调用 notation verify 验证镜像签名，验证通过才允许部署。签名密钥通过 Kubernetes Secret 或外部 KMS 管理。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的准入控制（Admission Controller）深度集成，实现"零信任镜像"策略。

## 生产场景

1. **零信任镜像部署**: 只允许签名验证通过的生产镜像部署到集群
2. **CI/CD 签名流水线**: 在 CI 中自动签名构建产物，确保只有签名镜像可部署
3. **合规供应链**: 满足 SLSA Level 3 要求，制品签名+验证+审计闭环
4. **多团队镜像安全**: 不同团队使用不同证书签名，通过信任策略控制

## 安装

```bash
# 安装 Notation CLI
curl -L https://github.com/notaryproject/notation/releases/latest/download/notation_$(uname -s)_$(uname -m).tar.gz | tar xz
mv notation /usr/local/bin/

# 生成签名证书
notation cert generate-test --default "my-cert"

# 签名 OCI 镜像
notation sign myregistry.io/myapp:v1.0.0

# 验证签名
notation verify myregistry.io/myapp:v1.0.0

# 配置 Kubernetes 镜像验证（通过 Ratify）
helm install ratify ratify/ratify --namespace gatekeeper-system
kubectl apply -f - <<EOF
apiVersion: config.ratify.deislabs.io/v1alpha1
kind: Store
metadata:
  name: store-notary
spec:
  name: notaryv2
EOF
```

## 对比

| 特性 | Notary V2 | Cosign (Sigstore) | Docker Content Trust |
|------|-----------|-------------------|---------------------|
| 签名格式 | OCI Manifest | OCI Manifest | TUF |
| 密钥体系 | X.509 PKI | KMS/Keyless | TUF PKI |
| Keyless | ❌ | ✅ (Fulcio) | ❌ |
| 透明日志 | ❌ | ✅ (Rekor) | ❌ |

## 架构定位

在 CNCF 生态中，Notary Project 属于 **Supply Chain** 类别，为云原生应用提供制品签名和验证能力。

## 参考链接

- [[实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[kyverno]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[open-cluster-management]] — [[实体/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[cloud-custodian]] — Cloud Custodian
- [[kuadrant]] — Kuadrant
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- notary-project
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
