---
title: Tokenetes
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- opa
- crd
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
- Tokenetes 是什么
- 如何 Tokenetes
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Tokenetes
- cncf
- landscape
---


# Tokenetes

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/DaspawnW/vault-crd |
| **官网** | https://vault.koudingspawn.de/ |
| **许可证** | Apache-2.0 |
| **开发语言** | Java / Go |
| **CNCF 分类** | Security / Secrets Management |
| **兼容版本** | Kubernetes 1.21+ / Vault 1.9+ |

---

## 项目概述

Tokenetes（也称为 Vault CRD Operator）是一个 Kubernetes Operator，用于将 HashiCorp Vault 中的密钥自动同步到 Kubernetes Secrets。它通过自定义资源 (CRD) 简化了 Vault 与 Kubernetes 的集成，支持多种认证方式和密钥类型，让开发者能够以声明式方式管理敏感数据。

### 核心价值

- **声明式管理**: 使用 CRD 定义密钥同步规则
- **自动同步**: Vault 密钥变更自动更新 K8s Secrets
- **多认证支持**: Token、Kubernetes、AppRole 等认证方式
- **PKI 集成**: 自动管理 TLS 证书生命周期
- **安全审计**: 完整的操作日志和事件记录

---

## 核心特性

### 密钥同步模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    Tokenetes Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                  HashiCorp Vault                           │  │
│  │                                                            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │  │
│  │  │   KV    │  │   PKI   │  │  Transit │  │  Database │  │  │
│  │  │ Engine  │  │ Engine  │  │  Engine  │  │  Engine   │  │  │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬─────┘  │  │
│  │       │            │            │            │         │  │
│  └───────│────────────│────────────│────────────│─────────┘  │
│          │            │            │            │             │
│          └────────────┴────────────┴────────────┘             │
│                              │                                 │
│                              ▼                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              Tokenetes Operator                            │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │               Controller Manager                     │  │  │
│  │  │                                                      │  │  │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐          │  │  │
│  │  │  │   KV    │  │   PKI   │  │ Database │          │  │  │
│  │  │  │Controller│  │Controller│  │Controller│          │  │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘          │  │  │
│  │  │                                                      │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                 │
│                              ▼                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                 Kubernetes Secrets                         │  │
│  │                                                            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │  │
│  │  │db-creds │  │tls-cert  │  │api-keys  │  │ssh-keys  │  │  │
│  │  │ Secret  │  │ Secret   │  │ Secret   │  │ Secret   │  │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │  │
│  │                                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 支持的密钥类型

| 类型 | 描述 | CRD |
|:---|:---|:---|
| **KV Secrets** | 键值对密钥 | `Vault` |
| **PKI Certificates** | TLS 证书 | `VaultPKISecret` |
| **Database Credentials** | 动态数据库凭证 | `VaultDatabaseSecret` |
| **SSH Keys** | SSH 密钥对 | `VaultSSHSecret` |

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                        Tokenetes System                            │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                   Kubernetes Cluster                          │ │
│  │                                                                │ │
│  │  ┌────────────────────────────────────────────────────────┐  │ │
│  │  │              Tokenetes Operator Pod                     │  │ │
│  │  │                                                          │  │ │
│  │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │  │ │
│  │  │  │  Reconciler  │  │ Vault Client │  │   Metrics    │ │  │ │
│  │  │  │              │  │              │  │   Exporter   │ │  │ │
│  │  │  └──────┬───────┘  └──────┬───────┘  └──────────────┘ │  │ │
│  │  │         │                 │                            │  │ │
│  │  └─────────│─────────────────│────────────────────────────┘  │ │
│  │            │                 │                                │ │
│  │            │                 │                                │ │
│  │            ▼                 ▼                                │ │
│  │  ┌─────────────────┐  ┌─────────────────────────────────┐   │ │
│  │  │  Custom         │  │       HashiCorp Vault           │   │ │
│  │  │  Resources      │  │                                 │   │ │
│  │  │                 │  │  ┌─────────┐  ┌─────────┐      │   │ │
│  │  │  - Vault        │◀─│─▶│   KV    │  │   PKI   │      │   │ │
│  │  │  - VaultPKI     │  │  │  v1/v2  │  │ Engine  │      │   │ │
│  │  │  - VaultDB      │  │  └─────────┘  └─────────┘      │   │ │
│  │  │                 │  │                                 │   │ │
│  │  └────────┬────────┘  └─────────────────────────────────┘   │ │
│  │           │                                                  │ │
│  │           ▼                                                  │ │
│  │  ┌─────────────────────────────────────────────────────────┐│ │
│  │  │                Kubernetes Secrets                        ││ │
│  │  │                                                          ││ │
│  │  │  Secret: app-database-creds                              ││ │
│  │  │  ├── username: admin                                     ││ │
│  │  │  └── password: ****                                      ││ │
│  │  │                                                          ││ │
│  │  │  Secret: app-tls-cert                                    ││ │
│  │  │  ├── tls.crt: ...                                        ││ │
│  │  │  └── tls.key: ...                                        ││ │
│  │  │                                                          ││ │
│  │  └─────────────────────────────────────────────────────────┘│ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装部署

```bash
# 使用 Helm 安装
helm repo add vault-crd https://daspawnw.github.io/vault-crd-helm-chart/
helm repo update

# 安装 Operator
helm install vault-crd vault-crd/vault-crd \
  --namespace vault-crd \
  --create-namespace \
  --set vault.address=https://vault.example.com:8200 \
  --set vault.authMethod=kubernetes

# 验证安装
kubectl get pods -n vault-crd

# 输出:
# NAME                        READY   STATUS    RESTARTS   AGE
# vault-crd-xxxxxxxxx-xxxxx   1/1     Running   0          1m
```

### 配置 Vault 认证

```bash
# 在 Vault 中启用 Kubernetes 认证
vault auth enable kubernetes

# 配置 Kubernetes 认证
vault write auth/kubernetes/config \
  kubernetes_host="https://kubernetes.default.svc:443" \
  kubernetes_ca_cert=@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
  token_reviewer_jwt=@/var/run/secrets/kubernetes.io/serviceaccount/token

# 创建策略
vault policy write vault-crd-policy - <<EOF
path "secret/data/*" {
  capabilities = ["read", "list"]
}
path "pki/issue/*" {
  capabilities = ["create", "update"]
}
path "database/creds/*" {
  capabilities = ["read"]
}
EOF

# 创建角色
vault write auth/kubernetes/role/vault-crd \
  bound_service_account_names=vault-crd \
  bound_service_account_namespaces=vault-crd \
  policies=vault-crd-policy \
  ttl=1h
```

### 创建 KV 密钥同步

```yaml
# vault-secret.yaml
apiVersion: vault.koudingspawn.de/v1
kind: Vault
metadata:
  name: database-credentials
  namespace: default
spec:
  # Vault KV 路径
  path: secret/data/myapp/database
  
  # KV 版本 (1 或 2)
  kvVersion: 2
  
  # 目标 Secret 类型
  type: Opaque
  
  # 字段映射
  mapping:
    - secretKey: username
      vaultKey: db_username
    - secretKey: password
      vaultKey: db_password
    - secretKey: host
      vaultKey: db_host
```

```bash
# 首先在 Vault 中创建密钥
vault kv put secret/myapp/database \
  db_username=admin \
  db_password=secretpassword \
  db_host=db.example.com

# 应用 CRD
kubectl apply -f vault-secret.yaml

# 验证 Secret 创建
kubectl get secret database-credentials -o yaml
```

---

## 高级功能

### PKI 证书管理

```yaml
# vault-pki.yaml
apiVersion: vault.koudingspawn.de/v1
kind: VaultPKISecret
metadata:
  name: app-tls-certificate
  namespace: default
spec:
  # PKI 后端路径
  pkiPath: pki
  
  # 角色名称
  role: web-server
  
  # 证书参数
  commonName: app.example.com
  altNames:
    - app.example.com
    - www.example.com
  ipSans:
    - 10.0.0.1
  ttl: 720h  # 30 天
  
  # 目标 Secret
  secretName: app-tls-cert
  secretType: kubernetes.io/tls
  
  # 自动续期
  autoRenewal:
    enabled: true
    # 到期前 24 小时续期
    renewBeforeExpiration: 24h
```

### 动态数据库凭证

```yaml
# vault-database.yaml
apiVersion: vault.koudingspawn.de/v1
kind: VaultDatabaseSecret
metadata:
  name: postgres-dynamic-creds
  namespace: default
spec:
  # Database 后端路径
  databasePath: database
  
  # 角色名称
  role: readonly
  
  # 目标 Secret
  secretName: postgres-creds
  
  # 凭证 TTL
  ttl: 1h
  
  # 自动续期
  autoRenewal:
    enabled: true
    renewBeforeExpiration: 10m
```

### 多环境配置

```yaml
# vault-multi-env.yaml
apiVersion: vault.koudingspawn.de/v1
kind: Vault
metadata:
  name: app-config
  namespace: production
spec:
  # 使用命名空间作为环境标识
  path: secret/data/production/myapp/config
  kvVersion: 2
  
  # 完整映射
  mapping:
    - secretKey: API_KEY
      vaultKey: api_key
    - secretKey: API_SECRET
      vaultKey: api_secret
    - secretKey: DATABASE_URL
      vaultKey: database_url

---
# 开发环境
apiVersion: vault.koudingspawn.de/v1
kind: Vault
metadata:
  name: app-config
  namespace: development
spec:
  path: secret/data/development/myapp/config
  kvVersion: 2
  mapping:
    - secretKey: API_KEY
      vaultKey: api_key
    - secretKey: API_SECRET
      vaultKey: api_secret
    - secretKey: DATABASE_URL
      vaultKey: database_url
```

### 同步到特定版本

```yaml
# vault-versioned.yaml
apiVersion: vault.koudingspawn.de/v1
kind: Vault
metadata:
  name: config-v3
spec:
  path: secret/data/myapp/config
  kvVersion: 2
  
  # 指定 KV 版本号
  version: 3
  
  mapping:
    - secretKey: config
      vaultKey: data
```

---

## 认证方式

### Kubernetes Auth

```yaml
# values.yaml
vault:
  address: https://vault.example.com:8200
  authMethod: kubernetes
  kubernetes:
    role: vault-crd
    serviceAccountName: vault-crd
```

### AppRole Auth

```yaml
# values.yaml
vault:
  address: https://vault.example.com:8200
  authMethod: approle
  approle:
    roleId: your-role-id
    secretId:
      secretName: vault-approle-secret
      key: secret-id
```

### Token Auth

```yaml
# values.yaml
vault:
  address: https://vault.example.com:8200
  authMethod: token
  token:
    secretName: vault-token-secret
    key: token
```

---

## 监控与运维

### Prometheus 指标

```yaml
# ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: vault-crd
  namespace: vault-crd
spec:
  selector:
    matchLabels:
      app: vault-crd
  endpoints:
    - port: metrics
      interval: 30s
```

### 可用指标

| 指标 | 类型 | 描述 |
|:---|:---|:---|
| `vault_crd_secrets_total` | Gauge | 管理的 Secret 总数 |
| `vault_crd_sync_success_total` | Counter | 同步成功次数 |
| `vault_crd_sync_failure_total` | Counter | 同步失败次数 |
| `vault_crd_sync_duration_seconds` | Histogram | 同步耗时 |
| `vault_crd_certificate_expiry_seconds` | Gauge | 证书过期剩余时间 |

### 故障排查

```bash
# 查看 Operator 日志
kubectl logs -n vault-crd -l app=vault-crd

# 查看 CRD 状态
kubectl describe vault database-credentials

# 检查事件
kubectl get events --field-selector involvedObject.name=database-credentials

# 验证 Vault 连接
kubectl exec -n vault-crd deploy/vault-crd -- vault status
```

---

## 最佳实践

### 安全配置

```yaml
# 最小权限策略
vault policy write app-secrets - <<EOF
# 只读特定路径
path "secret/data/myapp/*" {
  capabilities = ["read"]
}

# PKI 证书签发
path "pki/issue/web-server" {
  capabilities = ["create", "update"]
}

# 禁止删除和列表
path "secret/metadata/*" {
  capabilities = ["deny"]
}
EOF
```

### 高可用部署

```yaml
# values.yaml
replicaCount: 3

resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 500m
    memory: 512Mi

affinity:
  podAntiAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: vault-crd
          topologyKey: kubernetes.io/hostname
```

---

## 参考资源

- [GitHub 仓库](https://github.com/DaspawnW/vault-crd)
- [Helm Chart](https://github.com/DaspawnW/vault-crd-helm-chart)
- [官方文档](https://vault.koudingspawn.de/)
- [HashiCorp Vault](https://www.vaultproject.io/)
- [Vault Kubernetes Auth](https://developer.hashicorp.com/vault/docs/auth/kubernetes)
- [CNCF Sandbox](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
