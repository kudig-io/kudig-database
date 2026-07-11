---
title: Secret 外部管理模式
description: External Secrets Operator 集成 Vault/AWS SM/GCP SM 的密钥管理
summary: 使用 External Secrets Operator 从 Vault、AWS Secrets Manager、GCP Secret Manager 拉取密钥并自动轮换
category: manifests-patterns
tags:
- k8s
- manifests
- security
- external-secrets
- vault
- secret-management
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- External Secrets Operator 配置
- Vault Kubernetes 集成
- Secret 自动轮换
trigger_keywords:
- external-secrets
- vault
- secret-store
- secret-management
- rotation
prerequisites:
- k8s-secret-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Secret 外部管理模式

## 1. 为什么不用原生 Secret

| 问题 | 原生 Secret | External Secrets |
|------|-------------|------------------|
| 存储加密 | 依赖 etcd 加密 | Git 中不存储密钥 |
| 轮换 | 手动 | 自动 `refreshInterval` |
| 审计 | 有限 | 外部系统完整审计 |
| 多源聚合 | 不支持 | 同时从多个源拉取 |

## 2. ESO 架构

```
External Secret (CR)
      ↓
SecretStore (连接配置) → Vault / AWS SM / GCP SM / Azure KV
      ↓
生成 K8s Secret（自动刷新）
      ↓
Pod 挂载使用
```

## 3. Vault 集成

### 3.1 SecretStore

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.internal.example.com"
      path: "secret"              # KV 引擎挂载路径
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-reader"
          serviceAccountRef:
            name: external-secrets-sa
            namespace: external-secrets
```

### 3.2 ExternalSecret

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  refreshInterval: 1h             # 每小时自动刷新
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: db-secret               # 生成的 K8s Secret 名称
    creationPolicy: Owner
    template:
      type: Opaque
      data:
        DATABASE_URL: "postgresql://{{ .username }}:{{ .password }}@db.internal:5432/myapp"
  data:
    - secretKey: username
      remoteRef:
        key: production/database  # Vault 路径
        property: username
    - secretKey: password
      remoteRef:
        key: production/database
        property: password
```

## 4. AWS Secrets Manager 集成

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: aws-sm-store
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
            namespace: external-secrets
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: stripe-keys
  namespace: production
spec:
  refreshInterval: 30m
  secretStoreRef:
    name: aws-sm-store
    kind: ClusterSecretStore
  target:
    name: stripe-secret
  dataFrom:
    - extract:
        key: production/stripe    # AWS SM 中的密钥名
```

## 5. GCP Secret Manager 集成

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: gcp-store
  namespace: production
spec:
  provider:
    gcpsm:
      projectID: my-project-123
      auth:
        workloadIdentity:
          serviceAccountRef:
            name: external-secrets-sa
          clusterLocation: us-central1
          clusterName: prod-cluster
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: api-keys
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: gcp-store
    kind: SecretStore
  target:
    name: api-keys-secret
  data:
    - secretKey: JWT_SECRET
      remoteRef:
        key: jwt-secret
        version: latest
```

## 6. 动态密钥（Vault Database Engine）

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: dynamic-db-creds
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: dynamic-db-secret
    template:
      data:
        DATABASE_URL: "postgresql://{{ .username }}:{{ .password }}@db.internal:5432/myapp"
  dataFrom:
    - extract:
        key: database/creds/myapp-role  # Vault 动态凭证路径
```

## 7. PushSecret — 同步 K8s Secret 到外部

```yaml
apiVersion: external-secrets.io/v1alpha1
kind: PushSecret
metadata:
  name: sync-generated-password
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRefs:
    - name: aws-sm-store
      kind: ClusterSecretStore
  selector:
    secret:
      name: generated-password  # 从 K8s Secret 读取
  data:
    - match:
        secretKey: password
        remoteRef:
          remoteKey: production/generated-password
          property: password
```

## 8. 多源聚合

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-config-secrets
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: app-config
  data:
    # 从 Vault
    - secretKey: DB_PASSWORD
      remoteRef:
        key: production/database
        property: password
    # 从 AWS SM
    - secretKey: STRIPE_KEY
      storeRef:
        name: aws-sm-store
        kind: ClusterSecretStore
      remoteRef:
        key: production/stripe
        property: secret_key
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 设置合理的 `refreshInterval` | 敏感密钥 30m，普通 1h |
| 使用 Workload Identity | 避免静态凭证 |
| SecretStore 按 Namespace 隔离 | 避免跨 Namespace 访问 |
| 监控 ExternalSecret 状态 | `kubectl get es` 检查同步状态 |
| 密钥最小化 | 只拉取应用需要的 key |
| 使用 `template` 组合密钥 | 避免在应用中拼接 URL |

## 10. 紧急回滚

```bash
# 🟡 中风险：密钥操作
# 手动触发刷新
kubectl annotate externalsecret db-secret -n production \
  force-sync=$(date +%s) --overwrite

# 检查同步状态
kubectl get externalsecret -n production
```

## Related

- [[清单模式/04-gitops-patterns/05-gitops-secret-management|GitOps 密钥管理]]
- [[清单模式/05-security-patterns/04-rbac-least-privilege|RBAC 最小权限]]

## See Also

- [External Secrets Operator 文档](https://external-secrets.io/)
- [Vault Kubernetes Auth](https://developer.hashicorp.com/vault/docs/auth/kubernetes)

<!-- risk-assessed -->
