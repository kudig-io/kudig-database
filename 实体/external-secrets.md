---
title: External Secrets Operator (entities)
description: '## 概述'
summary: 'External Secrets Operator (ESO) 将外部密钥管理系统（如 AWS Secrets Manager、HashiCorp Vault、Azure Key Vault）的密钥同步到 Kubernetes Secrets，实现安全的密钥管理和自动轮换。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- external-secrets
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- External Secrets Operator 是什么
- 如何 External Secrets Operator
trigger_keywords:
- External
- Secrets
- Operator
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# External Secretsts|Secrets]] Operator

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

External Secrets Operator（ESO）是一个 Kubernetes 原生的密钥同步工具，2021 年加入 CNCF Sandbox。它将外部密钥管理系统（如 AWS Secrets Manager、HashiCorp Vault、Azure Key Vault、Google Secret Manager、1Password 等 20+ 提供商）的密钥自动同步到 Kubernetes Secrets，实现安全的密钥管理和自动轮换。ESO 解决了将密钥硬编码在 K8s Secret 或 Git 仓库中的安全问题。

## 核心特性

- **20+ 后端**: AWS、Azure、GCP、Vault、1Password、CyberArk、Pulumi 等
- **声明式同步**: 通过 SecretStore 和 ExternalSecret CRD 声明密钥映射
- **自动轮换**: 定期从外部系统拉取最新密钥值并更新 K8s Secret
- **模板化生成**: 支持 templated output，动态生成 Secret 内容
- **多租户隔离**: 命名空间级 SecretStore 和集群级 ClusterSecretStore
- **推送模式**: 支持将 K8s Secret 推送到外部系统（PushSecret）

## 架构

ESO 以 Operator 模式运行。Controller（Deployment）监听 ExternalSecret 和 SecretStore CRD。SecretStore 定义外部密钥系统的连接配置和认证凭证。ExternalSecret 定义从外部系统到 K8s Secret 的映射（哪个外部密钥的哪个字段 → K8s Secret 的哪个 key）。Controller 定期（refreshInterval）调用 Provider API 拉取密钥值，创建或更新对应的 Kubernetes Secret。Provider 实现标准接口（GetSecret、GetSecretMap），每个 Provider 对接一个外部系统。

## Kubernetes 集成

ESO 通过 CRD 声明式管理密钥同步。SecretStore CRD 定义连接信息，存储在命名空间中（命名空间级隔离）。ExternalSecret CRD 定义映射规则，Controller 根据规则创建标准 K8s Secret。创建的 Secret 与手动创建的 Secret 行为完全一致，Pod 可正常挂载为环境变量或卷。支持 Secret 推送（PushSecret），将 K8s Secret 同步到外部系统。通过 RBAC 和 SecretStore 作用域实现多租户隔离。

## 生产使用场景

1. **云密钥管理集成**: 将 AWS Secrets Manager 的密钥自动同步到 K8s Secret
2. **Vault 密钥同步**: 不修改应用代码，将 Vault 中的密钥同步为 K8s Secret
3. **密钥自动轮换**: 在云平台轮换密钥后，ESO 自动更新集群中的 Secret
4. **GitOps 兼容**: SecretStore/ExternalSecret YAML 可安全提交到 Git（不含密钥值）

## 安装

```bash
# Helm 安装
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets external-secrets/external-secrets \
  -n external-secrets --create-namespace
# 配置密钥源
kubectl apply -f - <<EOF
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata: { name: aws-secrets }
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef: { name: external-secrets-sa }
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata: { name: db-password }
spec:
  refreshInterval: 1h
  secretStoreRef: { name: aws-secrets, kind: SecretStore }
  target: { name: db-password-secret }
  data:
  - secretKey: password
    remoteRef: { key: prod/db/password }
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **ESO** | CNCF 项目、20+ 提供商 | 仅同步（非注入） |
| Sealed Secrets | GitOps 原生、简单 | 密钥仍存储在 Git 中 |
| Vault CSI Provider | CSI 原生文件挂载 | 仅支持 Vault |
| SOPS | 加密 YAML、GitOps 友好 | 需手动或脚本解密 |

## 架构定位

在 CNCF 生态中，ESO 属于 **Supply Chain / Security** 类别，是密钥管理标准化同步的领先方案。它与 Vault、AWS Secrets Manager、Sealed Secrets 等互补。

## 参考链接

- [[实体/vault.md|vault]]
- [[operator-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[cni]] — CNI (Container Network Interface)
- [[实体/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- external-secrets
- [[实体/ratify.md|Ratify]]
- [[实体/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
