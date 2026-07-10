---
title: HashiCorp Vault
description: '- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
summary: '- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HashiVault

HashiCorp Vault is the leading enterprise [[Secrets|secrets]]ts Management|secrets management]] platform, providing comprehensive credential lifecycle management.

## Key Capabilities

| Feature | Description |
|---------|-------------|
| KV Store | Static secrets with versioning and access control |
| Dynamic Credentials | Temporary database credentials with TTL |
| PKI Engine | Internal CA for certificate issuance and rotation |
| Transit Engine | Encryption-as-a-[[Service|Service]] (encrypt/decrypt API) |
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
- [[概念/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合

- [[tokenetes]] — Tokenetes
- [[external-secrets]] — External Secrets Operator
- radius — radius
- [[概念/secrets-management.md|secrets-management]] — Secrets Management
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/secrets-management.md|Secrets Management]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[cert-manager|cert-manager]]

- 05-commvault-enterprise-disaster-recovery
- 99-vault-k8s-secrets-guide
- 05-vault-enterprise-secrets-management
- bank-vaults

<!-- risk-assessed -->
