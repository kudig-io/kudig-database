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

## 安装与配置

```bash
# Helm 安装
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets external-secrets/external-secrets \
  -n external-secrets --create-namespace \
  --set installCRDs=true \
  --set webhook.port=9443

# 等待 Operator 就绪
kubectl wait --for=condition=available deployment/external-secrets -n external-secrets --timeout=120s
```

```yaml
# SecretStore 配置（AWS Secrets Manager）
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: aws-secrets
  namespace: production
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
---
# ExternalSecret 配置（同步数据库密码）
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: db-credentials
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secrets
    kind: SecretStore
  target:
    name: db-password-secret
    creationPolicy: Owner
  data:
  - secretKey: username
    remoteRef:
      key: prod/database/credentials
      property: username
  - secretKey: password
    remoteRef:
      key: prod/database/credentials
      property: password
---
# Vault SecretStore 配置
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: "https://vault.company.com:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "external-secrets"
          serviceAccountRef:
            name: external-secrets-sa
            namespace: external-secrets
```

## 运维操作

```bash
# 🟢 查看 ExternalSecret 同步状态
kubectl get externalsecret -A
kubectl describe externalsecret db-credentials -n production

# 🟢 查看 SecretStore 状态
kubectl get secretstore -A
kubectl get clustersecretstore

# 🟡 强制刷新 ExternalSecret（立即拉取最新密钥）
kubectl annotate externalsecret db-credentials -n production \
  force-sync=$(date +%s) --overwrite

# 🟢 查看同步的 K8s Secret
kubectl get secret db-password-secret -n production -o yaml

# 🟡 修改刷新间隔
kubectl patch externalsecret db-credentials -n production \
  --type merge -p '{"spec":{"refreshInterval":"15m"}}'

# 🔴 删除 ExternalSecret（会删除关联的 K8s Secret）
kubectl delete externalsecret db-credentials -n production
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| ExternalSecret 状态 SecretSyncedError | 外部系统不可达或认证失败 | `kubectl describe externalsecret <name>` | 检查 SecretStore 认证配置和网络 |
| Secret 未创建 | SecretStore 未就绪或 CRD 未安装 | `kubectl get secretstore` | 确认 CRD 已安装，SecretStore Ready |
| 密钥值未更新 | refreshInterval 未到期或外部值未变 | `kubectl get externalsecret -o yaml` | 强制刷新或检查外部系统 |
| IRSA/Workload Identity 失败 | ServiceAccount 权限不足 | `kubectl logs -n external-secrets -l app=external-secrets` | 检查 IAM 角色和 SA 注解 |
| Webhook 拒绝请求 | 证书过期或 webhook 服务异常 | `kubectl get validatingwebhookconfigurations` | 重启 ESO Pod 重新生成证书 |

```
排查流程：
├── 同步失败
│   ├── kubectl describe externalsecret 查看事件
│   ├── 检查 SecretStore 状态是否 Ready
│   ├── 验证外部系统连接（网络/认证）
│   └── 查看 ESO controller 日志
├── 认证问题
│   ├── AWS: 检查 IRSA ServiceAccount 注解
│   ├── Vault: 检查 Kubernetes Auth 角色配置
│   ├── Azure: 检查 Workload Identity 配置
│   └── 确认 RBAC 允许 ESO 创建 Secret
└── Webhook 异常
    ├── 检查证书 Secret 是否存在
    ├── 确认 webhook Service 端点可达
    └── 重启 ESO Deployment
```

## 生产案例

### 案例 1：多集群密钥统一管理

- **场景**：企业 10 个 K8s 集群，密钥分散在各集群 Secret 中，轮换需要逐个集群手动更新
- **排查**：密钥轮换耗时 2 小时+，曾发生部分集群未更新导致服务中断
- **方案**：部署 ESO + Vault，所有集群通过 ExternalSecret 从 Vault 拉取，Vault 统一轮换
- **效果**：密钥轮换从 2 小时降至 5 分钟（Vault 一次更新全集群同步），密钥泄露事件归零

### 案例 2：GitOps 流水线密钥安全

- **场景**：ArgoCD GitOps 流水线中，Secret YAML 加密存储在 Git，但解密密钥管理复杂
- **排查**：Sealed Secrets 的 master key 轮换困难，新团队加入时密钥分发流程复杂
- **方案**：迁移到 ESO，Git 中只存储 ExternalSecret YAML（不含密钥值），密钥从 AWS Secrets Manager 拉取
- **效果**：Git 仓库零密钥泄露风险，新团队接入无需分发解密密钥，审计日志完整

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **ESO** | CNCF 项目、20+ 提供商、自动轮换 | 仅同步（非注入） | 多后端密钥同步 |
| Sealed Secrets | GitOps 原生、简单 | 密钥仍存储在 Git 中 | 小团队简单场景 |
| Vault CSI Provider | CSI 原生文件挂载 | 仅支持 Vault | Vault 专用场景 |
| SOPS | 加密 YAML、GitOps 友好 | 需手动或脚本解密 | 配置文件加密 |

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
