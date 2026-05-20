---
title: External Secrets Operator
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- opa
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- External Secrets Operator 是什么
- 如何 External Secrets Operator
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- External
- Secrets
- Operator
- cncf
- landscape
---

# External Secrets Operator

> **成熟度**: Sandbox | **加入时间**: 2022-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://external-secrets.io |
| **GitHub** | https://github.com/external-secrets/external-secrets |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security & Secrets Management |

---

## 项目概述

External Secrets Operator (ESO) 将外部密钥管理系统（如 AWS Secrets Manager、HashiCorp Vault、Azure Key Vault）的密钥同步到 Kubernetes Secrets，实现安全的密钥管理和自动轮换。

## 核心特性

- **多后端支持**: AWS、Azure、GCP、Vault、1Password 等 20+ 提供商
- **自动同步**: 定期从外部系统同步密钥到 K8s
- **密钥模板**: 支持模板化生成 Secret 内容
- **密钥轮换**: 自动检测和同步密钥更新
- **多租户**: 支持命名空间级别的隔离
- **推送模式**: 支持将 K8s Secret 推送到外部系统

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│               External Secrets Operator Architecture             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                  Kubernetes Cluster                        │ │
│  │                                                            │ │
│  │  ┌──────────────────┐     ┌───────────────────────────┐  │ │
│  │  │ ExternalSecret   │────▶│   External Secrets        │  │ │
│  │  │ (CR)             │     │   Operator                │  │ │
│  │  └──────────────────┘     └─────────────┬─────────────┘  │ │
│  │                                         │                 │ │
│  │  ┌──────────────────┐                   │                 │ │
│  │  │ SecretStore /    │◀──────────────────┘                 │ │
│  │  │ ClusterSecretStore│                                    │ │
│  │  └────────┬─────────┘                                    │ │
│  │           │                                               │ │
│  │           │           ┌───────────────────────────────┐  │ │
│  │           │           │      Kubernetes Secret        │  │ │
│  │           │           │      (Synced)                 │  │ │
│  │           │           └───────────────────────────────┘  │ │
│  └───────────┼──────────────────────────────────────────────┘ │
│              │                                                  │
│              │  Fetch Secrets                                   │
│              ▼                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                External Secret Providers                  │  │
│  │  ┌──────────┐ ┌──────────┐ ┌────────┐ ┌──────────────┐  │  │
│  │  │   AWS    │ │  Azure   │ │  GCP   │ │  HashiCorp   │  │  │
│  │  │ Secrets  │ │Key Vault │ │Secret  │ │    Vault     │  │  │
│  │  │ Manager  │ │          │ │Manager │ │              │  │  │
│  │  └──────────┘ └──────────┘ └────────┘ └──────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# Helm 安装
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets external-secrets/external-secrets \
  --namespace external-secrets \
  --create-namespace
```

### 配置 AWS Secrets Manager

```yaml
# 创建认证 Secret
apiVersion: v1
kind: Secret
metadata:
  name: aws-secret
  namespace: external-secrets
type: Opaque
stringData:
  access-key: "AKIAXXXXXXXX"
  secret-access-key: "xxxxxxxxxx"
---
# ClusterSecretStore
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: aws-secretsmanager
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        secretRef:
          accessKeyIDSecretRef:
            name: aws-secret
            namespace: external-secrets
            key: access-key
          secretAccessKeySecretRef:
            name: aws-secret
            namespace: external-secrets
            key: secret-access-key
---
# ExternalSecret
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secretsmanager
    kind: ClusterSecretStore
  target:
    name: db-secret
    creationPolicy: Owner
  data:
    - secretKey: username
      remoteRef:
        key: prod/database
        property: username
    - secretKey: password
      remoteRef:
        key: prod/database
        property: password
```

### HashiCorp Vault 配置

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: "https://vault.example.com:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "external-secrets"
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: vault-secret
spec:
  refreshInterval: 5m
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: app-secret
  data:
    - secretKey: api-key
      remoteRef:
        key: apps/myapp
        property: api_key
```

---

## 高级用法

### 模板化 Secret

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: templated-secret
spec:
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: config-secret
    template:
      type: Opaque
      data:
        config.yaml: |
          database:
            host: {{ .host }}
            port: {{ .port }}
            username: {{ .username }}
            password: {{ .password }}
  data:
    - secretKey: host
      remoteRef:
        key: database/config
        property: host
    - secretKey: port
      remoteRef:
        key: database/config
        property: port
```

### 推送 Secret（PushSecret）

```yaml
apiVersion: external-secrets.io/v1alpha1
kind: PushSecret
metadata:
  name: push-to-vault
spec:
  secretStoreRefs:
    - name: vault-backend
      kind: SecretStore
  selector:
    secret:
      name: local-secret
  data:
    - match:
        secretKey: api-key
        remoteRef:
          remoteKey: apps/myapp
          property: api_key
```

---

## 最佳实践

1. **最小权限**: 为 ESO 配置最小必需的云平台权限
2. **命名空间隔离**: 使用 SecretStore 而非 ClusterSecretStore 实现隔离
3. **刷新间隔**: 根据安全需求设置合理的 refreshInterval
4. **密钥轮换**: 在外部系统轮换密钥后，ESO 会自动同步
5. **监控**: 监控同步状态和错误

---

## 参考资源

- [官方文档](https://external-secrets.io/latest/)
- [GitHub Repo](https://github.com/external-secrets/external-secrets)
- [Provider 列表](https://external-secrets.io/latest/provider/)

---

**维护者**: Kudig Team | **许可证**: MIT
