---
title: HashiCorp Vault
description: '- [[synthesis/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
category: entities
tags:
- k8s
- security
- secrets
- vault
- pki
- encryption
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HashiCorp Vault 是什么
- 如何 HashiCorp Vault
trigger_keywords:
- HashiCorp
- Vault
prerequisites:
- kubectl-basics
- tls-basics
---

# HashiVault

HashiCorp Vault is the leading enterprise secrets management platform, providing comprehensive credential lifecycle management.

## Key Capabilities

| Feature | Description |
|---------|-------------|
| KV Store | Static secrets with versioning and access control |
| Dynamic Credentials | Temporary database credentials with TTL |
| PKI Engine | Internal CA for certificate issuance and rotation |
| Transit Engine | Encryption-as-a-Service (encrypt/decrypt API) |
| Auth Methods | K8s ServiceAccount, LDAP, OIDC, AppRole |
| Audit Trail | Complete audit log of all secret access |

## K8s Integration Patterns

| Pattern | How It Works | Use Case |
|---------|-------------|----------|
| Agent Sidecar | Vault Agent injects secrets into memory-only volume | Production (secrets never touch disk) |
| External Secrets Operator | Syncs Vault secrets to K8s Secret objects | GitOps compatibility |
| CSI Driver | Mounts secrets as files via CSI interface | Special compliance requirements |

## Dynamic Credentials Advantage

Instead of static passwords that persist indefinitely, Vault generates temporary database credentials on each application request. Credentials auto-expire after TTL (e.g., 1 hour), limiting the blast radius of credential theft.

## Related
- [[synthesis/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合

- [[tokenetes]] — Tokenetes
- [[external-secrets]] — External Secrets Operator
- [[domain-19-landscape-references/sandbox/radius/radius.md|radius]] — radius
- [[concepts/secrets-management.md|secrets-management]] — Secrets Management
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/secrets-management.md|Secrets Management]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[cert-manager|cert-manager]]

- [[domain-09-reliability-engineering/05-commvault-enterprise-disaster-recovery.md|05-commvault-enterprise-disaster-recovery]]
- [[domain-05-security-compliance/99-vault-k8s-secrets-guide.md|99-vault-k8s-secrets-guide]]
- [[domain-05-security-compliance/05-vault-enterprise-secrets-management.md|05-vault-enterprise-secrets-management]]
- [[domain-19-landscape-references/sandbox/bank-vaults/bank-vaults.md|bank-vaults]]