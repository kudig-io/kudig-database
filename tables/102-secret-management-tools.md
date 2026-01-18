# 102 - 密钥与敏感信息管理工具 (Secret Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 密钥管理方案对比

| 工具 (Tool) | 架构 (Architecture) | 核心优势 (Advantages) | 生产建议 |
|------------|-------------------|---------------------|---------|
| **HashiCorp Vault** | 外部密钥库 | 动态秘钥、审计、精细权限 | 金融/安全敏感场景 |
| **External Secrets Operator** | 同步控制器 | 云原生、多云支持 | 推荐 ACK 环境 |
| **Sealed Secrets** | 加密控制器 | GitOps 友好 | 适合 ArgoCD 工作流 |
| **SOPS** | 文件加密工具 | 简单、支持多种 KMS | 小团队快速上手 |

## External Secrets Operator (生产推荐)

### 1. 集成阿里云 KMS
```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: alicloud-kms
  namespace: production
spec:
  provider:
    alibaba:
      regionID: cn-hangzhou
      auth:
        secretRef:
          accessKeyID:
            name: alicloud-credentials
            key: access-key-id
          accessKeySecret:
            name: alicloud-credentials
            key: access-key-secret
```

### 2. 同步密钥到 Kubernetes
```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: alicloud-kms
    kind: SecretStore
  target:
    name: db-secret
    creationPolicy: Owner
  data:
    - secretKey: username
      remoteRef:
        key: prod/db/username
    - secretKey: password
      remoteRef:
        key: prod/db/password
```

## Vault 企业级部署

### 高可用架构
- **存储后端**: Raft (推荐) 或 Consul
- **自动解封**: Auto-unseal with Cloud KMS
- **审计日志**: 持久化到对象存储

### Vault Agent Sidecar 注入
```yaml
annotations:
  vault.hashicorp.com/agent-inject: "true"
  vault.hashicorp.com/role: "myapp"
  vault.hashicorp.com/agent-inject-secret-db: "secret/data/db/config"
  vault.hashicorp.com/agent-inject-template-db: |
    {{- with secret "secret/data/db/config" -}}
    export DB_USER="{{ .Data.data.username }}"
    export DB_PASS="{{ .Data.data.password }}"
    {{- end }}
```

## Sealed Secrets GitOps 工作流

### 1. 加密敏感信息
```bash
# 安装 kubeseal CLI
kubectl create secret generic mysecret --dry-run=client -o yaml | \
  kubeseal -o yaml > sealed-secret.yaml

# 提交到 Git
git add sealed-secret.yaml
git commit -m "Add encrypted secret"
```

### 2. 自动解密
- Sealed Secrets Controller 自动解密
- 仅集群内私钥可解密
- 支持命名空间/集群级作用域

## 安全最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **最小权限** | RBAC 限制 Secret 访问 |
| **轮换策略** | 定期轮换数据库密码 |
| **审计日志** | 记录所有密钥访问 |
| **加密存储** | 启用 etcd 加密 (EncryptionConfiguration) |
| **避免硬编码** | 禁止在代码/镜像中存放密钥 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)