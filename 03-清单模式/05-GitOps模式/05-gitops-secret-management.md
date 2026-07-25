---
title: GitOps 密钥管理
description: External Secrets Operator + SOPS 在 GitOps 中的密钥管理模式
summary: 使用 External Secrets Operator、Mozilla SOPS 和 Sealed Secrets 实现 GitOps 安全密钥管理
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- security
- external-secrets
- sops
- sealed-secrets
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- 安全工程师
estimated_read_time: 12min
intent_queries:
- GitOps 如何管理 Secret
- External Secrets Operator 配置
- SOPS 加密 Kubernetes Secret
trigger_keywords:
- external-secrets
- sops
- sealed-secrets
- vault
- kms
prerequisites:
- gitops-basics
- k8s-secret-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GitOps 密钥管理

## 1. 方案对比

| 方案 | 原理 | 优点 | 缺点 |
|------|------|------|------|
| **External Secrets Operator** | 从 Vault/AWS SM 拉取 | 动态轮换、不存 Git | 依赖外部服务 |
| **SOPS** | 加密 values 后存 Git | Git 即真相 | 轮换需手动 |
| **Sealed Secrets** | 非对称加密存 Git | 简单 | 私钥丢失则全暴露 |

## 2. External Secrets Operator（ESO）

### 2.1 SecretStore — 连接外部密钥源

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: my-app
spec:
  provider:
    vault:
      server: "https://vault.example.com"
      path: "kv"                  # Vault KV 引擎路径
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "my-app-role"     # Vault K8s auth role
          serviceAccountRef:
            name: my-app-sa
```

### 2.2 AWS Secrets Manager SecretStore

```yaml
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
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
            namespace: external-secrets
```

### 2.3 ExternalSecret — 拉取密钥

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
  namespace: my-app
spec:
  refreshInterval: 1h             # 自动轮换间隔
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: db-secret               # 生成的 K8s Secret 名称
    creationPolicy: Owner
    template:
      type: kubernetes.io/tls
      data:
        TLS_CRT: "{{ .tls_cert }}"
        TLS_KEY: "{{ .tls_key }}"
  data:
    - secretKey: username
      remoteRef:
        key: production/database
        property: username
    - secretKey: password
      remoteRef:
        key: production/database
        property: password
  dataFrom:
    - extract:
        key: production/database  # 批量提取所有键值对
```

### 2.4 PushSecret — 推送密钥

```yaml
apiVersion: external-secrets.io/v1alpha1
kind: PushSecret
metadata:
  name: push-db-password
  namespace: my-app
spec:
  refreshInterval: 1h
  secretStoreRefs:
    - name: aws-secrets-manager
      kind: ClusterSecretStore
  selector:
    secret:
      name: generated-db-secret   # 从 K8s Secret 读取
  data:
    - match:
        secretKey: password
        remoteRef:
          remoteKey: production/db-password
          property: password
```

## 3. Mozilla SOPS

### 3.1 加密文件

```bash
# 🟢 低风险：文件加密操作
# 使用 AWS KMS 加密
sops --encrypt --kms "arn:aws:kms:us-east-1:123:key/abc" \
  --in-place secrets/production.yaml
```

### 3.2 加密后的 YAML

```yaml
# 加密后只有 metadata 明文，data 值加密
apiVersion: v1
kind: Secret
metadata:
    name: app-secrets
    namespace: production
type: Opaque
stringData:
    DB_PASSWORD: ENC[AES256_GCM,data:abc123==,iv:xyz==,tag:def==,type:str]
    API_KEY: ENC[AES256_GCM,data:ghi456==,iv:uvw==,tag:jkl==,type:str]
sops:
    kms:
        - arn: arn:aws:kms:us-east-1:123:key/abc
          created_at: "2026-07-11T08:00:00Z"
          enc: AQICAH...
```

### 3.3 Helm Secrets + SOPS

```yaml
# HelmRelease 中引用 SOPS 加密文件
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: my-app
  namespace: production
spec:
  valuesFrom:
    - kind: Secret
      name: app-secrets          # 解密后的 Secret
      valuesKey: values.yaml
```

## 4. Sealed Secrets

### 4.1 加密

```bash
# 🟢 低风险：本地加密操作
echo -n 'my-password' | kubectl create secret generic my-secret \
  --dry-run=client --from-file=password=/dev/stdin -o yaml \
  | kubeseal --controller-namespace=kube-system \
  > sealed-secret.yaml
```

### 4.2 SealedSecret 资源

```yaml
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: my-secret
  namespace: production
spec:
  encryptedData:
    password: AgBy3i...（加密内容）
  template:
    metadata:
      name: my-secret
      namespace: production
    type: Opaque
```

## 5. 生产实践

| 实践 | 说明 |
|------|------|
| ESO 优先 | 动态轮换、审计日志、RBAC 集成 |
| 设置 `refreshInterval` | 自动轮换过期密钥 |
| 使用 IRSA/Workload Identity | 避免 IAM 长期凭证 |
| 备份加密密钥 | SOPS KMS Key / Sealed Secrets 私钥 |
| SecretStore 按命名空间隔离 | 避免跨命名空间泄露 |
| 禁止 Git 中明文 Secret | CI 中扫描（truffleHog/gitleaks） |

## Related

- [[03-清单模式/06-安全模式/06-secret-external-management|Secret 外部管理]]
- [[03-清单模式/05-GitOps模式/03-flux-kustomization-patterns|Flux Kustomization]]

## See Also

- [External Secrets Operator 文档](https://external-secrets.io/)
- [SOPS 文档](https://github.com/getsops/sops)
- [Sealed Secrets](https://github.com/bitnami-labs/sealed-secrets)

<!-- risk-assessed -->
