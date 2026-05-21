---
title: Secrets Management
description: External Secrets Operator 将外部密钥管理系统（如 Vault）的 Secret 同步到 Kubernetes Secret 对象 ^[inferred]：
category: concepts
tags:
- k8s
- security
- secrets
- vault
- encryption
- certificates
- pki
- etcd
- apiserver
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Secrets Management 是什么
- 如何 Secrets Management
trigger_keywords:
- Secrets
- Management
prerequisites:
- kubectl-basics
- etcd-basics
- tls-basics
---

# Secrets Management

## K8s Secrets Limitations

Kubernetes Secrets store data as base64-encoded strings in etcd, which is NOT encryption. Any user with etcd access can retrieve secret plaintext. Secrets also lack:
- Automatic rotation
- Audit trails
- Dynamic credential generation
- Fine-grained access control

## HashiCorp Vault Integration

Vault provides enterprise-grade secrets management with three K8s integration patterns:

| Pattern | How It Works | Best For |
|---------|-------------|----------|
| Agent Sidecar | Vault Agent injects secrets into memory-only volume | Production (secrets never touch disk) |
| External Secrets Operator | Syncs Vault secrets to K8s Secret objects | GitOps compatibility |
| CSI Driver | Mounts secrets as files via CSI interface | Special compliance requirements |

### Dynamic Credentials

Vault generates temporary database credentials on demand:
- App authenticates via K8s ServiceAccount token
- Vault generates DB credentials with short TTL (e.g., 1 hour)
- Credentials auto-expire and auto-rotate
- Even if stolen, credentials have limited lifetime

### PKI Certificate Management

Vault PKI engine acts as a private CA:
- Issues X.509 certificates for service-to-service mTLS
- Automatic certificate rotation before expiry
- Certificate revocation via CRL/OCSP

## cert-manager

cert-manager automates TLS certificate management in K8s:
- **ACME**: Automatic certificate issuance from Let's Encrypt or other ACME CAs
- **Private CA**: Integration with Vault PKI or self-signed issuers
- **Auto-renewal**: Certificates renewed before expiry (default 30 days before)
- **Gateway API support**: Native integration with Gateway API for TLS

## External Secrets Operator

External Secrets Operator 将外部密钥管理系统（如 Vault）的 Secret 同步到 Kubernetes Secret 对象 ^[inferred]：

- `SecretStore`：定义外部密钥后端连接（Vault server URL、认证方式）^[inferred]
- `ExternalSecret`：定义同步映射关系（远程密钥路径 -> K8s Secret key）^[inferred]
- `refreshInterval`：设置同步间隔（如 1h）^[inferred]
- 认证推荐使用 K8s ServiceAccount + Vault Kubernetes Auth ^[inferred]

## 密钥轮换策略

生产环境密钥应定期轮换 ^[inferred]：

- **Vault 动态凭证**：每次请求生成唯一短期凭证，自动过期 ^[inferred]
- **CronJob 轮换**：定时任务更新外部密钥管理系统中的密钥，然后同步到 K8s ^[inferred]
- 轮换后应验证服务正常运行，避免因密钥更新导致中断 ^[inferred]

## 访问控制

使用 RBAC 限制 Secret 访问 ^[inferred]：

- 创建 Role 仅允许 `get`/`list` 特定 Secret（`resourceNames` 限制）
- 绑定到应用专用 ServiceAccount
- 避免所有 Pod 都能访问所有 Secret ^[inferred]

## Encryption at Rest

Enable etcd encryption to protect K8s Secrets:
```yaml
# API Server configuration
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources: ["secrets"]
    providers:
      - aescbc:
          keys:
            - name: key1
              secret: <base64-encoded-key>
      - identity: {}  # Fallback (no encryption)
```

## Related
- [[synthesis/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合
- [[synthesis/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合
- [[synthesis/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合
- [[synthesis/Deployment × Secret 管理|Deployment × Secret 管理]] — 综合

- [[external-secrets]] — External Secrets Operator
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[entities/vault.md|vault]] — HashiCorp Vault
- [[cert-manager]] — cert-manager
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[entities/vault.md|HashiCorp Vault]]
- [[cert-manager|cert-manager]]

- [[domain-05-security-compliance/05-vault-enterprise-secrets-management.md|05-vault-enterprise-secrets-management]]
- [[entities/metal3-io|Metal3]] — Cross-reference
