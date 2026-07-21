---
title: Kubernetes Secrets Management Deep Dive
description: K8s 密钥管理深度实践 — External Secrets Operator、Sealed Secrets、Vault Agent Injector、密钥轮换自动化
summary: 企业级 Kubernetes 密钥管理全方案对比与生产实践，消除硬编码密钥风险
category: practice
tags:
- secrets
- vault
- external-secrets
- sealed-secrets
- rotation
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: security
---
# Kubernetes 密钥管理深度实践

> 消除硬编码密钥，实现密钥的自动同步、轮换与审计。

## 密钥管理方案对比

| 方案 | 外部依赖 | 加密存储 | 自动轮换 | 审计 | 适用 |
|------|----------|----------|----------|------|------|
| K8s Secrets + etcd 加密 | 无 | 静态加密 | 手动 | 审计日志 | 基础 |
| Sealed Secrets | 无 | Git 安全存储 | 手动 | Git 历史 | GitOps |
| External Secrets Operator | Vault/AWS/GCP | 外部存储 | 支持 | 外部审计 | 企业 |
| Vault Agent Injector | Vault | Vault | 支持 | Vault 审计 | 高安全 |
| SOPS + age | 无 | 文件加密 | 手动 | Git 历史 | 小团队 |

## External Secrets Operator (ESO)

### 架构

```
┌─────────────────────────────────────────────────┐
│              Kubernetes Cluster                   │
│  ┌───────────────────────────────────────────┐   │
│  │        External Secrets Operator           │   │
│  │  ┌─────────────┐  ┌───────────────────┐   │   │
│  │  │ Controller  │  │  Webhook          │   │   │
│  │  │ (Reconcile) │  │  (Validation)     │   │   │
│  │  └──────┬──────┘  └───────────────────┘   │   │
│  └─────────┼─────────────────────────────────┘   │
│            │ 创建/更新                            │
│  ┌─────────▼─────────────────────────────────┐   │
│  │         K8s Secrets (内存/etcd)            │   │
│  └───────────────────────────────────────────┘   │
│            ▲                                      │
│            │ 读取                                 │
│  ┌─────────┴─────────────────────────────────┐   │
│  │         Application Pods                   │   │
│  └───────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
             │ 同步
             ▼
┌──────────────────────────────────────────────────┐
│  External Providers                               │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────┐  │
│  │ Vault  │ │ AWS SM │ │ GCP SM │ │ Azure KV │  │
│  └────────┘ └────────┘ └────────┘ └──────────┘  │
└──────────────────────────────────────────────────┘
```

### 部署与配置

```yaml
# Helm 安装 ESO
# helm install external-secrets external-secrets/external-secrets -n external-secrets --create-namespace

# SecretStore — 连接 Vault
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-apps"
          serviceAccountRef:
            name: eso-sa
---
# ClusterSecretStore — 全局（跨命名空间）
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-global
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "eso-reader"
          serviceAccountRef:
            name: eso-sa
            namespace: external-secrets
```

### ExternalSecret 定义

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  refreshInterval: "1h"  # 同步频率
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: app-secrets
    creationPolicy: Owner
    template:
      metadata:
        labels:
          app: my-service
      data:
        DATABASE_URL: "postgres://{{ .db_user }}:{{ .db_pass }}@db:5432/prod"
        REDIS_URL: "redis://:{{ .redis_pass }}@cache:6379"
  data:
    - secretKey: db_user
      remoteRef:
        key: production/database
        property: username
    - secretKey: db_pass
      remoteRef:
        key: production/database
        property: password
    - secretKey: redis_pass
      remoteRef:
        key: production/redis
        property: password
```

### AWS Secrets Manager 集成

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: aws-sm
  namespace: production
spec:
  provider:
    aws:
      service: SecretsManager
      region: ap-southeast-1
      auth:
        jwt:
          serviceAccountRef:
            name: eso-sa  # IRSA
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: api-keys
spec:
  refreshInterval: "30m"
  secretStoreRef:
    name: aws-sm
    kind: SecretStore
  target:
    name: api-keys
  dataFrom:
    - extract:
        key: production/api-keys  # 提取整个 JSON
```

## Sealed Secrets（GitOps 友好）

```bash
# 安装 kubeseal
brew install kubeseal

# 创建 SealedSecret
kubectl create secret generic db-creds \
  --from-literal=username=admin \
  --from-literal=password=s3cret \
  --dry-run=client -o yaml | \
  kubeseal --controller-namespace sealed-secrets \
    --controller-name sealed-secrets \
    --format yaml > sealed-db-creds.yaml
```

```yaml
# sealed-db-creds.yaml — 可安全提交到 Git
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: db-creds
  namespace: production
spec:
  encryptedData:
    username: AgBy3i4OJSWK+PiTySYZZA9rO43cGDEq...
    password: AgC+L3qFZvq9YNz8JkVx5mN2pQ7rS1t...
  template:
    metadata:
      name: db-creds
      namespace: production
    type: Opaque
```

## Vault Agent Injector

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-with-vault
spec:
  template:
    metadata:
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "production-apps"
        vault.hashicorp.com/agent-inject-secret-db: "database/creds/app-role"
        vault.hashicorp.com/agent-inject-template-db: |
          {{- with secret "database/creds/app-role" -}}
          postgres://{{ .Data.username }}:{{ .Data.password }}@db:5432/prod
          {{- end }}
        vault.hashicorp.com/agent-inject-secret-tls: "pki/issue/app"
        vault.hashicorp.com/agent-inject-template-tls: |
          {{- with secret "pki/issue/app" "common_name=app.svc" -}}
          {{ .Data.certificate }}
          {{ .Data.private_key }}
          {{- end }}
    spec:
      containers:
        - name: app
          image: my-app:latest
          # 密钥自动挂载到 /vault/secrets/
          volumeMounts: []  # injector 自动添加
```

## 密钥轮换自动化

### Vault 动态密钥（自动轮换）

```hcl
# Vault 数据库密钥引擎配置
path "database/config/prod" {
  capabilities = ["create", "update"]
}

# 动态密钥 — 每次请求生成临时凭证
# vault write database/roles/app-role \
#   db_name=prod-db \
#   creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}';" \
#   default_ttl="1h" \
#   max_ttl="24h"
```

### cert-manager 证书自动轮换

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: app-tls
  namespace: production
spec:
  secretName: app-tls-cert
  duration: 2160h    # 90 天
  renewBefore: 720h  # 30 天前轮换
  dnsNames:
    - app.production.svc
    - app.production.svc.cluster.local
  issuerRef:
    name: internal-ca
    kind: ClusterIssuer
```

## 安全最佳实践

1. **永远不要**在 YAML/代码中硬编码密钥
2. 启用 etcd 静态加密（EncryptionConfiguration）
3. 使用 RBAC 限制 Secret 访问（最小权限）
4. 定期轮换所有密钥（≤ 90 天）
5. 审计 Secret 访问（K8s Audit Log + Vault Audit）
6. 使用短期/动态密钥替代长期静态密钥
7. CI/CD 中使用 OIDC 联邦身份替代长期 Token
8. 镜像中不嵌入密钥（构建时/运行时注入）

---

## 密钥泄露应急响应

### 泄露检测

```yaml
# Falco 规则: 检测异常 Secret 访问
- rule: Unusual Secret Access
  desc: 检测非预期的 Secret 读取操作
  condition: >
    ka.target.resource=secrets and
    ka.verb=get and
    not ka.user.name in (system:serviceaccount:external-secrets:eso-sa,
                         system:serviceaccount:kube-system:generic-garbage-collector)
  output: >
    Secret 被异常访问 (user=%ka.user.name secret=%ka.target.name
    namespace=%ka.target.namespace)
  priority: WARNING
```

### 泄露后应急流程

```bash
# 🔴 密钥泄露应急处理

# 1. 立即轮换泄露的密钥
# Vault:
vault kv put secret/production/database password="$(openssl rand -base64 32)"

# 2. 触发 ESO 同步
kubectl annotate externalsecret app-secrets -n production \
  force-sync=$(date +%s) --overwrite

# 3. 重启使用密钥的 Pod
kubectl rollout restart deployment/my-app -n production

# 4. 审计访问日志
# K8s Audit Log:
kubectl logs -n kube-system -l component=kube-apiserver | \
  grep "secrets" | grep -v "system:serviceaccount"

# Vault Audit:
vault audit list
# 查询 Vault 审计日志中的异常访问

# 5. 检查是否有未授权访问
kubectl get events -A --field-selector reason=FailedMount | tail -20
```

---

## 多环境密钥管理

### 环境隔离架构

```
Vault
├── secret/data/development/     # 开发环境
│   ├── database
│   └── api-keys
├── secret/data/staging/         # 预发环境
│   ├── database
│   └── api-keys
└── secret/data/production/      # 生产环境
    ├── database
    └── api-keys

Vault Policies:
├── dev-policy:   path "secret/data/development/*" { capabilities = ["read"] }
├── staging-policy: path "secret/data/staging/*" { capabilities = ["read"] }
└── prod-policy:  path "secret/data/production/*" { capabilities = ["read"] }
```

### 命名空间级 SecretStore

```yaml
# 每个命名空间独立的 SecretStore
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-production
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret/data/production"  # 环境隔离路径
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-ns"  # 命名空间级角色
          serviceAccountRef:
            name: eso-sa
---
# Vault Kubernetes Auth 角色配置
# vault write auth/kubernetes/role/production-ns \
#   bound_service_account_names=eso-sa \
#   bound_service_account_namespaces=production \
#   policies=prod-policy \
#   ttl=1h
```

---

## 密钥管理成熟度模型

| 级别 | 名称 | 特征 | 工具 | 建议时间 |
|------|------|------|------|----------|
| L1 | 硬编码 | 密钥在代码/YAML 中 | 无 | - |
| L2 | K8s Secret | 使用原生 Secret + etcd 加密 | EncryptionConfig | 1 周 |
| L3 | GitOps 加密 | Sealed Secrets / SOPS | kubeseal, sops+age | 2 周 |
| L4 | 外部管理 | ESO + Vault/云 KMS | ESO, Vault | 1 月 |
| L5 | 动态密钥 | 短期凭证、自动轮换 | Vault Dynamic, cert-manager | 3 月 |
| L6 | 零信任 | 无长期密钥、工作负载身份 | SPIFFE/SPIRE, OIDC | 6 月 |

### 快速启动路线图

```
第 1 周: 启用 etcd 加密 + 审计 Secret 访问
    ├── 配置 EncryptionConfiguration
    ├── 重新加密现有 Secrets
    └── 启用审计日志

第 2 周: 部署 External Secrets Operator
    ├── 安装 ESO
    ├── 配置 SecretStore (Vault/云 KMS)
    └── 迁移关键服务密钥

第 3-4 周: 密钥轮换自动化
    ├── Vault 动态数据库凭证
    ├── cert-manager 证书轮换
    └── 自动化轮换 CronJob

第 2 月: 全面迁移 + 监控
    ├── 所有服务迁移到 ESO
    ├── 密钥访问监控告警
    └── 泄露检测 (Falco)

第 3 月: 零信任演进
    ├── 工作负载身份 (SPIFFE)
    ├── 消除长期 Token
    └── mTLS 服务间通信
```

---

## 监控与告警

```yaml
# PrometheusRule: 密钥管理告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: secrets-alerts
  namespace: monitoring
spec:
  groups:
    - name: secrets.rules
      rules:
        # ESO 同步失败
        - alert: ExternalSecretSyncFailed
          expr: |
            externalsecret_status_condition{
              condition="Ready", status="False"
            } == 1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "ExternalSecret 同步失败: {{ $labels.namespace }}/{{ $labels.name }}"

        # 证书即将过期
        - alert: CertificateExpiringSoon
          expr: |
            certmanager_certificate_expiration_timestamp_seconds - time() < 7*24*3600
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "证书 7 天内过期: {{ $labels.name }}"

        # 证书已过期
        - alert: CertificateExpired
          expr: |
            certmanager_certificate_expiration_timestamp_seconds - time() < 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "证书已过期: {{ $labels.name }}"

        # 异常 Secret 访问
        - alert: UnusualSecretAccess
          expr: |
            increase(apiserver_request_total{
              resource="secrets", verb="get",
              code="200"
            }[5m]) > 100
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Secret 访问频率异常"
```

## Related

- [[安全/身份与访问/index.md|身份与访问]]
- [[安全/零信任架构/index.md|零信任架构]]
- [[安全/合规审计/index.md|合规审计]]
