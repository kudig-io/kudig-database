# Bank-Vaults

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://bank-vaults.dev/ |
| **GitHub** | https://github.com/bank-vaults/bank-vaults |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Bank-Vaults 是一套围绕 HashiCorp Vault 构建的 Kubernetes 原生密钥管理工具集。它提供 Vault Operator 自动化部署和管理 Vault 集群、Webhook 自动注入密钥到 Pod 环境变量和文件、以及多种云 KMS 后端的自动解封能力。Bank-Vaults 大幅简化了在 Kubernetes 环境中使用 Vault 进行密钥管理的复杂度。

### 核心特性

- **Vault Operator**: Kubernetes Operator 自动部署、配置和管理 Vault 集群生命周期
- **Secret Webhook**: Mutating Webhook 自动将 Vault 密钥注入 Pod 环境变量和挂载卷
- **自动解封**: 支持 AWS KMS、Azure Key Vault、GCP Cloud KMS、HSM 等多种自动解封方式
- **声明式配置**: 通过 CRD 声明式管理 Vault 策略、认证方式和密钥引擎
- **Secret Sync**: 将 Vault 密钥同步为 Kubernetes Secret 资源
- **多后端支持**: 支持 Consul、Raft、etcd、S3 等多种存储后端

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                Kubernetes Cluster                     │
│                                                       │
│  ┌──────────────────┐     ┌────────────────────────┐ │
│  │  Vault Operator   │     │  Secrets Webhook       │ │
│  │  (管理 Vault      │     │  (Mutating Admission)  │ │
│  │   集群生命周期)   │     │  注入密钥到 Pod        │ │
│  └────────┬─────────┘     └──────────┬─────────────┘ │
│           │                          │                │
│  ┌────────▼──────────────────────────▼─────────────┐ │
│  │              Vault Cluster (HA)                   │ │
│  │  ┌──────┐  ┌──────┐  ┌──────┐                   │ │
│  │  │Active│  │Standby│  │Standby│                  │ │
│  │  │Node  │  │Node   │  │Node   │                  │ │
│  │  └──┬───┘  └──┬───┘  └──┬───┘                   │ │
│  │     └──────────┼────────┘                        │ │
│  │                │ Raft                             │ │
│  └────────────────┼─────────────────────────────────┘ │
│                   │                                    │
│  ┌────────────────▼─────────────────────────────────┐ │
│  │         Auto Unseal                               │ │
│  │  ┌─────────┐ ┌──────────┐ ┌─────────────────┐   │ │
│  │  │AWS KMS  │ │Azure KV  │ │GCP Cloud KMS    │   │ │
│  │  └─────────┘ └──────────┘ └─────────────────┘   │ │
│  └──────────────────────────────────────────────────┘ │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │  Application Pods                             │    │
│  │  env:                                         │    │
│  │    DB_PASSWORD: vault:secret/data/db#password │    │
│  │  → 自动注入实际密钥值                         │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Vault Operator

```bash
# 使用 Helm 安装
helm repo add bank-vaults https://bank-vaults.dev/charts
helm install vault-operator bank-vaults/vault-operator \
  --namespace vault-system \
  --create-namespace
```

### 部署 Vault 集群

```yaml
# vault-cr.yaml
apiVersion: vault.banzaicloud.com/v1alpha1
kind: Vault
metadata:
  name: vault
  namespace: vault-system
spec:
  size: 3
  image: hashicorp/vault:1.15.4

  # 自动解封配置 (AWS KMS)
  unsealConfig:
    options:
      preFlightChecks: true
    kubernetes:
      secretNamespace: vault-system
    # AWS KMS 解封
    aws:
      kmsKeyId: "arn:aws:kms:us-east-1:123456789:key/abcd-1234"
      kmsRegion: "us-east-1"

  # Vault 配置
  config:
    storage:
      raft:
        path: /vault/data
    listener:
      tcp:
        address: "0.0.0.0:8200"
        tls_cert_file: /vault/tls/server.crt
        tls_key_file: /vault/tls/server.key
    api_addr: https://vault.vault-system:8200
    cluster_addr: https://${.Env.POD_NAME}:8201

  # 声明式配置: 认证方式、策略、密钥引擎
  externalConfig:
    policies:
      - name: app-policy
        rules: |
          path "secret/data/{{identity.entity.aliases.auth_kubernetes_*.metadata.service_account_namespace}}/*" {
            capabilities = ["read", "list"]
          }
    auth:
      - type: kubernetes
        roles:
          - name: app-role
            bound_service_account_names: ["*"]
            bound_service_account_namespaces: ["app-*"]
            policies: ["app-policy"]
            ttl: 1h
    secrets:
      - path: secret
        type: kv
        description: Application secrets
        options:
          version: 2
```

```bash
kubectl apply -f vault-cr.yaml
```

### 安装 Secrets Webhook

```bash
helm install vault-secrets-webhook bank-vaults/vault-secrets-webhook \
  --namespace vault-system
```

### 在应用中使用密钥

```yaml
# app-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: app-production
spec:
  template:
    metadata:
      annotations:
        vault.security.banzaicloud.io/vault-addr: "https://vault.vault-system:8200"
        vault.security.banzaicloud.io/vault-role: "app-role"
    spec:
      serviceAccountName: my-app
      containers:
        - name: app
          image: my-app:latest
          env:
            # Webhook 自动将 vault: 前缀的值替换为实际密钥
            - name: DB_HOST
              value: "vault:secret/data/db#host"
            - name: DB_PASSWORD
              value: "vault:secret/data/db#password"
            - name: API_KEY
              value: "vault:secret/data/api#key"
```

---

## 高级功能

### Secret 同步到 Kubernetes

```yaml
# 将 Vault 密钥同步为 K8s Secret
apiVersion: vault.banzaicloud.com/v1alpha1
kind: Vault
metadata:
  name: vault
spec:
  externalConfig:
    secrets:
      - path: secret
        type: kv
        options:
          version: 2
  # 自动同步配置
  secretSync:
    - secretPath: secret/data/tls-certs
      kubernetesSecret:
        name: app-tls
        namespace: app-production
        type: kubernetes.io/tls
        keys:
          - vaultKey: tls.crt
            k8sKey: tls.crt
          - vaultKey: tls.key
            k8sKey: tls.key
```

### 多云 KMS 解封

```yaml
# Azure Key Vault 解封
unsealConfig:
  azure:
    keyVaultName: "my-vault-unseal"
    keyName: "vault-key"
    keyVersion: "latest"

# GCP Cloud KMS 解封
# unsealConfig:
#   google:
#     kmsKeyRing: "vault-keyring"
#     kmsCryptoKey: "vault-key"
#     kmsLocation: "global"
#     kmsProject: "my-project"

# HSM (PKCS#11) 解封
# unsealConfig:
#   hsm:
#     modulePath: "/usr/lib/softhsm/libsofthsm2.so"
#     slotId: 0
#     pin: "${HSM_PIN}"
#     keyLabel: "vault-unseal"
```

### Vault 注入到 ConfigMap/文件

```yaml
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    metadata:
      annotations:
        vault.security.banzaicloud.io/vault-addr: "https://vault:8200"
        vault.security.banzaicloud.io/vault-ct-configmap: "app-config"
    spec:
      containers:
        - name: app
          volumeMounts:
            - name: vault-secrets
              mountPath: /etc/secrets
      volumes:
        - name: vault-secrets
          emptyDir: {}
```

---

## 与其他方案对比

| 特性 | Bank-Vaults | External Secrets | Sealed Secrets | CSI Driver |
|:---|:---|:---|:---|:---|
| Vault 部署管理 | Operator 全生命周期 | 不提供 | 不适用 | 不提供 |
| 自动解封 | 多云 KMS + HSM | 不适用 | 不适用 | 不适用 |
| 密钥注入方式 | Webhook (env/file) | K8s Secret 同步 | 加密 Secret | CSI Volume |
| 密钥旋转 | 自动 | 轮询同步 | 手动 | 自动 |
| 声明式配置 | 完整 Vault 配置 | 仅 Secret 映射 | 仅加密 | 仅挂载 |
| 多密钥源 | Vault | 多源 | 单源 | 多源 |

---

## 最佳实践

1. **HA 部署**: 生产环境至少 3 节点 Raft 集群，确保 Vault 高可用
2. **KMS 解封**: 使用云 KMS 自动解封，避免手动解封操作
3. **最小权限**: 每个应用使用独立的 Vault Role 和 Policy，遵循最小权限原则
4. **密钥路径规范**: 按命名空间/应用组织密钥路径，如 `secret/data/{namespace}/{app}`
5. **审计日志**: 启用 Vault 审计日志，监控密钥访问行为

---

## 参考资源

- [Bank-Vaults 官方文档](https://bank-vaults.dev/docs/)
- [Bank-Vaults GitHub](https://github.com/bank-vaults/bank-vaults)
- [Vault Operator](https://github.com/bank-vaults/vault-operator)
- [Vault Secrets Webhook](https://github.com/bank-vaults/vault-secrets-webhook)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
