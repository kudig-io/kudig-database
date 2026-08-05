---
title: CI/CD 中的 Secret 轮转
description: 'CI/CD Pipeline 中的 Secret 管理与自动轮转：External Secrets Operator、Sealed Secrets、Vault Agent Injector 完整方案'
summary: 'CI/CD Pipeline 中的 Secret 管理与自动轮转：External Secrets Operator、Sealed Secrets、Vault Agent Injector 完整方案'
category: release-change-management
tags:
- secrets
- external-secrets
- vault
- sealed-secrets
- security
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- CI/CD Secret 轮转 是什么
- 如何配置 External Secrets Operator
- Sealed Secrets 怎么用
trigger_keywords:
- secret-rotation
- external-secrets
- vault
- sealed-secrets
- cicd-security
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# CI/CD 中的 Secret 轮转

## 1. Secret 管理挑战

Kubernetes 原生 Secret 以 base64 编码存储在 etcd 中，存在以下问题：
- 无加密（base64 不是加密）
- 无法审计访问
- 无自动轮转
- GitOps 工作流中难以管理（不能提交到 Git）
- 无法跨集群同步

生产环境需要外部 Secret 管理系统，并与 CI/CD Pipeline 集成实现自动轮转。

## 2. External Secrets Operator (ESO)

### 2.1 架构概述

External Secrets Operator 从外部密钥管理系统（AWS Secrets Manager、Azure Key Vault、GCP Secret Manager、HashiCorp Vault 等）同步 Secret 到 Kubernetes。

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────┐
│           External Secrets Operator          │
│  ┌──────────────┐  ┌───────────────────┐    │
│  │ SecretStore  │  │ ExternalSecret    │    │
│  │ Controller   │  │ Controller        │    │
│  └──────┬───────┘  └────────┬──────────┘    │
│         │                   │                │
│         ▼                   ▼                │
│  ┌──────────────┐  ┌───────────────────┐    │
│  │ AWS/Azure/   │  │ K8s Secret 同步   │    │
│  │ GCP/Vault    │  │ (创建/更新/删除)  │    │
│  └──────────────┘  └───────────────────┘    │
└─────────────────────────────────────────────┘
```
### 2.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 Helm 安装
helm repo add external-secrets https://charts.external-secrets.io
helm repo update

helm install external-secrets external-secrets/external-secrets \
  --namespace external-secrets \
  --create-namespace \
  --set installCRDs=true \
  --set webhook.port=9443

# 验证安装
kubectl get pods -n external-secrets
```
### 2.3 ClusterSecretStore（AWS Secrets Manager）

```yaml
# AWS 凭证 Secret
apiVersion: v1
kind: Secret
metadata:
  name: aws-credentials
  namespace: external-secrets
type: Opaque
stringData:
  access-key: "AKIAIOSFODNN7EXAMPLE"
  secret-key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
---
# ClusterSecretStore（集群级）
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: aws-secrets-manager
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        secretRef:
          accessKeyIDSecretRef:
            name: aws-credentials
            namespace: external-secrets
            key: access-key
          secretAccessKeySecretRef:
            name: aws-credentials
            namespace: external-secrets
            key: secret-key
```

### 2.4 Azure Key Vault

```yaml
# Azure 凭证 Secret
apiVersion: v1
kind: Secret
metadata:
  name: azure-credentials
  namespace: external-secrets
type: Opaque
stringData:
  tenant-id: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  client-id: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  client-secret: "your-client-secret"
---
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: azure-keyvault
spec:
  provider:
    azurekv:
      tenantId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
      vaultUrl: "https://my-keyvault.vault.azure.net"
      authSecretRef:
        clientId:
          name: azure-credentials
          namespace: external-secrets
          key: client-id
        clientSecret:
          name: azure-credentials
          namespace: external-secrets
          key: client-secret
```

### 2.5 GCP Secret Manager

```yaml
# GCP Service Account Secret
apiVersion: v1
kind: Secret
metadata:
  name: gcp-credentials
  namespace: external-secrets
type: Opaque
stringData:
  secret-access-credentials: |
    {
      "type": "service_account",
      "project_id": "my-project",
      "private_key_id": "...",
      "private_key": "...",
      "client_email": "...",
      "client_id": "..."
    }
---
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: gcp-secret-manager
spec:
  provider:
    gcpsm:
      projectID: my-project
      auth:
        secretRef:
          secretAccessKey:
            name: gcp-credentials
            namespace: external-secrets
            key: secret-access-credentials
```

### 2.6 HashiCorp Vault

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "external-secrets"
          serviceAccountRef:
            name: "external-secrets-sa"
            namespace: "external-secrets"
---
# Vault Kubernetes Auth 配置
# vault auth enable kubernetes
# vault write auth/kubernetes/config \
#   kubernetes_host="https://kubernetes.default.svc"
# vault write auth/kubernetes/role/external-secrets \
#   bound_service_account_names=external-secrets-sa \
#   bound_service_account_namespaces=external-secrets \
#   policies=external-secrets
```

### 2.7 ExternalSecret CRD

```yaml
# 单个 Secret 同步
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-database-creds
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secrets-manager
    kind: ClusterSecretStore
  target:
    name: app-database-creds
    creationPolicy: Owner
    deletionPolicy: Retain
    # 模板化（可选）
    template:
      type: kubernetes.io/tls
      data:
        tls.crt: "{{ .certificate }}"
        tls.key: "{{ .private_key }}"
  data:
  # 从 AWS Secrets Manager 同步
  - secretKey: username
    remoteRef:
      key: production/database
      property: username
  - secretKey: password
    remoteRef:
      key: production/database
      property: password
---
# 多字段 Secret
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-config
  namespace: production
spec:
  refreshInterval: 30m
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: app-config
  data:
  - secretKey: api-key
    remoteRef:
      key: secret/data/production/api
      property: key
  - secretKey: api-secret
    remoteRef:
      key: secret/data/production/api
      property: secret
  # 从 JSON 提取
  - secretKey: db-host
    remoteRef:
      key: secret/data/production/database
      property: host
```

## 3. Sealed Secrets

### 3.1 工作原理

Sealed Secrets 允许将加密后的 Secret 存储在 Git 仓库中。只有集群内的 Sealed Secrets Controller 能解密。

```
开发者 → kubeseal 加密 → 提交到 Git → ArgoCD 同步
    → Sealed Secrets Controller 解密 → 创建 K8s Secret
```

### 3.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Controller
kubectl apply -f https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.27.0/controller.yaml

# 安装 kubeseal CLI
brew install kubeseal  # macOS
# 或
wget https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.27.0/kubeseal-0.27.0-linux-amd64.tar.gz
tar -xzf kubeseal-0.27.0-linux-amd64.tar.gz
sudo mv kubeseal /usr/local/bin/
```
### 3.3 使用流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建普通 Secret
kubectl create secret generic db-creds \
  --from-literal=username=admin \
  --from-literal=password=secretpass \
  --dry-run=client -o yaml > secret.yaml

# 加密为 SealedSecret
kubeseal --format yaml < secret.yaml > sealed-secret.yaml

# 查看加密结果
cat sealed-secret.yaml
```
```yaml
# sealed-secret.yaml 示例
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: db-creds
  namespace: production
spec:
  encryptedData:
    password: AgBY3...（加密数据）
    username: AgCx7...（加密数据）
  template:
    metadata:
      name: db-creds
      namespace: production
```

### 3.4 GitOps 集成

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 完整工作流
# 1. 创建 Secret
kubectl create secret generic app-secrets \
  --from-literal=jwt-secret=$(openssl rand -base64 32) \
  --from-literal=session-key=$(openssl rand -base64 32) \
  --dry-run=client -o yaml | \
  kubeseal --format yaml \
    --controller-name=sealed-secrets-controller \
    --controller-namespace=kube-system > sealed-app-secrets.yaml

# 2. 提交到 Git
git add sealed-app-secrets.yaml
git commit -m "feat: add sealed application secrets"
git push

# 3. ArgoCD 自动同步（SealedSecret → 解密为 Secret）
```
## 4. Vault Agent Injector

### 4.1 工作原理

Vault Agent Injector 通过 Sidecar 注入方式，将 Vault 中的 Secret 直接挂载到 Pod 中，无需应用感知 Vault。

```
Pod 注入 → Vault Agent Sidecar → Vault Server
    → 获取 Secret → 写入共享 Volume → 应用读取
```

### 4.2 安装与配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Vault Agent Injector
helm repo add hashicorp https://helm.releases.hashicorp.com
helm install vault hashicorp/vault \
  --namespace vault \
  --create-namespace \
  --set "injector.enabled=true" \
  --set "server.dev.enabled=true"  # 仅开发环境

# 生产环境 Vault 配置
helm install vault hashicorp/vault \
  --namespace vault \
  --create-namespace \
  --set "injector.enabled=true" \
  --set "server.ha.enabled=true" \
  --set "server.ha.replicas=3" \
  --set "server.auditStorage.enabled=true"
```
### 4.3 Vault 策略配置

```bash
# 启用 KV 引擎
vault secrets enable -path=secret kv-v2

# 写入测试数据
vault kv put secret/production/database \
  username=admin \
  password=supersecret \
  host=db.production.svc \
  port=5432

# 创建策略
vault policy write production-app - <<EOF
path "secret/data/production/*" {
  capabilities = ["read"]
}
path "secret/metadata/production/*" {
  capabilities = ["list"]
}
EOF

# 配置 Kubernetes Auth
vault auth enable kubernetes
vault write auth/kubernetes/config \
  kubernetes_host="https://kubernetes.default.svc"

vault write auth/kubernetes/role/production-app \
  bound_service_account_names=production-app \
  bound_service_account_namespaces=production \
  policies=production-app \
  ttl=1h
```

### 4.4 Pod 注入注解

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-app
  namespace: production
spec:
  template:
    metadata:
      annotations:
        # 启用 Vault 注入
        vault.hashicorp.com/agent-inject: "true"
        # Vault 角色
        vault.hashicorp.com/role: "production-app"
        # 注入 Secret
        vault.hashicorp.com/agent-inject-secret-db-creds: "secret/data/production/database"
        # Secret 模板
        vault.hashicorp.com/agent-inject-template-db-creds: |
          {{- with secret "secret/data/production/database" -}}
          DB_USERNAME={{ .Data.data.username }}
          DB_PASSWORD={{ .Data.data.password }}
          DB_HOST={{ .Data.data.host }}
          DB_PORT={{ .Data.data.port }}
          {{- end }}
        # TLS Secret
        vault.hashicorp.com/agent-inject-secret-tls: "secret/data/production/tls"
        vault.hashicorp.com/agent-inject-template-tls: |
          {{- with secret "secret/data/production/tls" -}}
          {{ .Data.data.crt }}
          {{- end }}
        vault.hashicorp.com/agent-inject-file-tls: "tls.crt"
        # Agent 配置
        vault.hashicorp.com/agent-inject-token: "false"
        vault.hashicorp.com/agent-pre-populate-only: "true"
    spec:
      serviceAccountName: production-app
      containers:
      - name: app
        image: my-app:latest
        env:
        - name: DB_CREDS_FILE
          value: /vault/secrets/db-creds
        volumeMounts:
        - name: tls-certs
          mountPath: /etc/tls
      volumes:
      - name: tls-certs
        secret:
          secretName: tls-certs
```

## 5. Secret 自动轮转策略

### 5.1 ESO 自动轮转

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: rotating-db-creds
spec:
  # 刷新间隔（轮转周期）
  refreshInterval: 15m
  secretStoreRef:
    name: aws-secrets-manager
    kind: ClusterSecretStore
  target:
    name: rotating-db-creds
    # 秘密版本策略
    creationPolicy: Owner
    deletionPolicy: Retain
  data:
  - secretKey: password
    remoteRef:
      key: production/database
      property: password
      # 版本管理
      version: AWSCURRENT
```

### 5.2 Vault 动态 Secret

```bash
# 启用数据库引擎
vault secrets enable database

# 配置 MySQL 连接
vault write database/config/my-mysql \
  plugin_name=mysql-database-plugin \
  connection_url="{{username}}:{{password}}@tcp(db:3306)/" \
  allowed_roles="production-app" \
  username="vault" \
  password="vault-password"

# 创建角色（动态凭证）
vault write database/roles/production-app \
  db_name=my-mysql \
  creation_statements="CREATE USER '{{name}}'@'%' IDENTIFIED BY '{{password}}'; \
    GRANT SELECT, INSERT, UPDATE ON mydb.* TO '{{name}}'@'%';" \
  default_ttl="1h" \
  max_ttl="24h"
```

```yaml
# Vault Agent 动态 Secret 注入
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-app
spec:
  template:
    metadata:
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "production-app"
        vault.hashicorp.com/agent-inject-secret-db: "database/creds/production-app"
        vault.hashicorp.com/agent-inject-template-db: |
          {{- with secret "database/creds/production-app" -}}
          DB_USERNAME={{ .Data.username }}
          DB_PASSWORD={{ .Data.password }}
          {{- end }}
```

### 5.3 CI Pipeline 集成

```yaml
# GitLab CI 示例
stages:
  - deploy

rotate-secrets:
  stage: deploy
  image: hashicorp/vault:latest
  script:
  # 从 Vault 获取新密码
  - NEW_PASSWORD=$(vault kv get -field=password secret/production/rotating)
  # 更新到 AWS Secrets Manager
  - aws secretsmanager update-secret \
      --secret-id production/database \
      --secret-string "{\"password\":\"$NEW_PASSWORD\"}"
  # 触发 ESO 刷新
  - kubectl annotate externalsecret rotating-db-creds \
      -n production \
      force-sync=$(date +%s) --overwrite
  rules:
  - if: $CI_PIPELINE_SOURCE == "schedule"
```

```yaml
# GitHub Actions 示例
name: Secret Rotation
on:
  schedule:
    - cron: '0 2 * * 1'  # 每周一凌晨 2 点
  workflow_dispatch:

jobs:
  rotate:
    runs-on: ubuntu-latest
    steps:
    - name: Generate new secret
      run: echo "NEW_SECRET=$(openssl rand -base64 32)" >> $GITHUB_ENV

    - name: Update AWS Secrets Manager
      uses: aws-actions/configure-aws-credentials@v4
      with:
        aws-access-key-id: ${{ secrets.AWS_ACCESS_KEY_ID }}
        aws-secret-access-key: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
        aws-region: us-east-1

    - run: |
        aws secretsmanager put-secret-value \
          --secret-id production/api-key \
          --secret-string "{\"api-key\":\"$NEW_SECRET\"}"

    - name: Force ESO refresh
      run: |
        kubectl annotate externalsecret api-key-secret \
          -n production \
          force-sync=$(date +%s) --overwrite
```

## 6. 方案对比

| 特性 | External Secrets | Sealed Secrets | Vault Agent |
|------|-----------------|----------------|-------------|
| Git 友好 | 否（引用外部） | 是（加密存储） | 否 |
| 自动轮转 | 是（refreshInterval） | 否（需重新加密） | 是（动态 Secret） |
| 多后端支持 | 是 | 否 | 是 |
| 安全等级 | 中 | 中 | 高 |
| 运维复杂度 | 低 | 低 | 高 |
| 推荐场景 | 多云环境 | GitOps 简单场景 | 高安全要求 |

## 7. 最佳实践

| 实践 | 建议 |
|------|------|
| 最小权限 | 每个应用独立 ServiceAccount 和 IAM 策略 |
| 轮转周期 | 密码 30-90 天，API Key 90-180 天 |
| 审计 | 启用 Vault 审计日志、CloudTrail |
| 备份 | 定期备份 Vault 数据和加密密钥 |
| 监控 | 监控 Secret 刷新失败、过期告警 |
| 回滚 | 保留前 N 个版本的 Secret |

## Related

- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-08-release-change-management/01-gitops/05-gitops-security-compliance|GitOps 安全合规]]
- [[domain-08-release-change-management/变更管理/01-change-window-and-approval|变更窗口与审批]]

## See Also

- [External Secrets Operator 文档](https://external-secrets.io/)
- [Sealed Secrets 文档](https://sealed-secrets.netlify.app/)
- [Vault Agent Injector 文档](https://developer.hashicorp.com/vault/docs/platform/k8s/injector)


<!-- risk-assessed -->
