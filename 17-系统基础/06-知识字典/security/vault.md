---
title: HashiCorp Vault
description: HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes
  环境中，Vault...
summary: HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes 环境中，Vault...
category: dictionary
tags:
- k8s
- glossary
- vault
- secrets-management
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HashiCorp Vault 是什么
- Vault 详解
trigger_keywords:
- HashiCorp Vault
- Vault
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HashiCorp Vault

> **英文名**: Vault

## 概述

HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes 环境中，Vault 是集中式密钥管理的标准方案。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Secret Engine | 密钥存储和管理（KV、数据库、PKI 等） |
| Auth Method | 身份认证（K8s、LDAP、AppRole 等） |
| Policy | 访问控制策略 |
| Transit | 加密即服务（Encryption as a Service） |
| PKI | 动态证书签发和吊销 |

### K8s 集成方式

- **Vault Agent Sidecar**：自动注入密钥到 Pod。
- **Vault CSI Provider**：通过 CSI 卷挂载密钥。
- **External Secrets Operator**：同步 Vault 密钥到 K8s Secret。

## 关键机制或特性

- **动态凭证**：按需生成短生命周期的数据库凭证、AWS 凭证等。
- **Kubernetes Auth**：使用 ServiceAccount Token 认证 Pod 身份。
- **Auto-Unseal**：使用云 KMS 自动解封 Vault。
- **审计日志**：记录所有密钥访问操作。
- **Secret Rotation**：自动轮转数据库密码和 API 密钥。

## 使用场景与最佳实践

- 生产环境使用 Vault 替代 K8s Secret 管理敏感信息。
- 启用 K8s Auth Method 实现 Pod 级别的密钥访问。
- 使用 Vault Agent Sidecar 自动注入密钥（无需修改应用代码）。
- 配置短期凭证（TTL < 1h）减少密钥泄露风险。
- 启用审计日志满足合规要求。

## 架构深度解析

### Vault 核心架构与 K8s 集成

```
┌──────────────────────────────────────────────────────────────┐
│  Vault Cluster（HA，3 副本）                                  │
│  ├─ Storage Backend：Raft（内置）/ Consul / 云对象存储         │
│  ├─ Seal/Unseal：Shamir 分片（5 取 3）/ 云 KMS 自动解封        │
│  ├─ 认证方法（Auth Methods）：                                 │
│  │   ├─ Kubernetes：SA Token → Vault Token（JWT 校验）        │
│  │   ├─ OIDC / LDAP / AppRole / Token                         │
│  ├─ 密钥引擎（Secrets Engines）：                              │
│  │   ├─ KV v2：版本化静态密钥                                  │
│  │   ├─ Database：动态数据库凭证（自动轮转）                   │
│  │   ├─ PKI：动态证书签发（TLS 即用即签）                      │
│  │   └─ Transit：加密即服务（KMS 能力）                        │
│  └─ 策略（Policy）：HCL 表达式 + 路径前缀授权                  │
│                                                                 │
│  K8s 集成链路：                                                 │
│  SA Token → Kubernetes Auth（校验 JWT）→ Vault Token            │
│   → 读取 kv/db-creds → 动态凭证注入应用（sidecar/CSI/ESO）      │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（hashicorp/vault）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 核心 | `vault/core.go` | 请求路由与核心逻辑 |
| K8s 认证 | `builtin/credential/kubernetes/` | SA Token 校验与映射 |
| 数据库引擎 | `builtin/logical/database/` | 动态凭证生成与租约 |
| 策略 | `sdk/helper/policyutil/` | HCL 策略解析合并 |

### 流程步骤

1. Pod 内应用（或 sidecar）用 SA Token 调用 Vault Kubernetes Auth。
2. Vault 通过 TokenReview 校验 Token 有效性，并按角色映射策略。
3. 应用获得短期 Vault Token（默认 1h，可续期），按策略读取路径。
4. 动态引擎（Database/PKI）按租约签发凭证，到期自动吊销/轮转。
5. 应用通过 agent sidecar / CSI provider / ESO 无感消费密钥。

## 生产案例

### 案例 1：Vault 被封（sealed）导致全部应用密钥读取失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 机房断电，Vault 集群全部节点重启 |
| T+5min | Vault 进入 sealed 状态，应用密钥读取全部失败 |
| T+20min | 大量应用 CrashLoopBackOff，生产大面积故障 |
| T+1h | 运维手动执行 unseal（Shamir 分片），恢复 |
| T+2d | 改造：接入云 KMS 自动解封 + unseal 监控告警 |

- **根因分析**：Vault 重启后必须 unseal 才能服务；人工 unseal 流程（分片持有人 + 顺序）在故障场景下成为恢复瓶颈。
- **修复命令**：
```bash
# 1. 检查 seal 状态（只读）
vault status
# 2. 手动 unseal（🔴 高风险：需分片持有人在场，3 片即可）
vault operator unseal <shard-1>
vault operator unseal <shard-2>
vault operator unseal <shard-3>
# 3. 配置云 KMS 自动解封（长期修复，Terraform 示例）
resource "aws_kms_key" "vault" { deletion_window_in_days = 7 }
# Vault 配置 seal "awskms" { region = "us-east-1" kms_key_id = ... }
```

### 案例 2：动态数据库凭证轮转引发连接风暴

| 时间 | 事件 |
| --- | --- |
| T+0 | 应用接入 Database 动态凭证，租约 1h |
| T+30min | 数据库连接池爆满，慢查询激增 |
| T+2h | 定位：应用连接池未回收租约，Vault 轮转时旧连接全部失效，池内瞬间重建 |
| T+6h | 优化：连接池校验（connection validation）+ 租约续期逻辑 |
| T+1d | 恢复稳定，数据库 CPU 回落 |

- **根因分析**：动态凭证的轮转与连接池生命周期不匹配：凭证轮转使存量连接失效，若连接池未做校验会集体重建，形成连接风暴。
- **修复命令**：
```bash
# 1. 检查租约与动态角色（只读）
vault lease list database/creds/my-role
vault read database/creds/my-role
# 2. 调大租约 TTL 缓解（🟡 中风险）
vault write database/roles/my-role default_ttl=2h max_ttl=24h
# 3. 应用侧：连接池开启 connection test（伪代码）
# HikariCP: connection-test-query="SELECT 1"; maximum-pool-size=20
```

## 对比评测

| 维度 | Vault | AWS Secrets Manager | ESO + KMS | 原生 Secret |
| --- | --- | --- | --- | --- |
| 动态凭证 | 支持（DB/PKI） | 部分（轮转） | 不支持 | 不支持 |
| 多云 | 是 | 仅 AWS | 多云 | - |
| 密钥引擎 | KV/DB/PKI/Transit | KV | KV | KV |
| 运维成本 | 高（HA/unseal） | 低（托管） | 中 | 低 |
| 合规审计 | 完整 | 完整 | 依赖源 | 审计日志 |

**选型建议**：需要动态凭证/多集群统一用 Vault；单云简单场景用云托管 KMS/SM；仅做 Secret 同步用 ESO。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| permission denied | 策略缺失/路径前缀错误 | `vault token capabilities self` 查看权限 |
| sealed 状态 | 节点重启未 unseal | `vault status`；配置自动解封 |
| K8s 认证失败 | SA 无 TokenReview 权限 | 检查 `system:auth-delegator` ClusterRoleBinding |
| 动态凭证失效 | 租约到期/DB 凭据被回收 | `vault lease renew` 或应用重连 |
| 性能瓶颈 | 单点/未启用 HA | 检查 leader 与副本状态 |

## 生产部署清单

- [ ] HA 部署（≥3 副本 Raft）+ 云 KMS 自动解封
- [ ] 分片持有人登记表（Shamir 分片归属与联系方式）
- [ ] Kubernetes Auth 最小权限（每应用独立 Vault 角色）
- [ ] 动态凭证（DB/PKI）租约监控与连接池校验
- [ ] 备份：Raft snapshot 定期 + 恢复演练

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 人工 unseal 且分片持有人不可靠 | 立即接云 KMS 自动解封 |
| P1 | 应用长期静态密钥未迁移动态凭证 | 优先迁移 DB/PKI 动态引擎 |
| P2 | 单副本部署 | 扩容 HA + 迁移 Raft |

## 面试要点

1. **Q：Vault 为什么需要 unseal？Seal/Unseal 机制是什么？**
   A：Vault 启动时内存中无加密密钥，处于 sealed 状态无法提供任何服务；unseal 通过解密存储的密钥来恢复服务。密钥用 Shamir 秘密共享分片（N 取 M，如 5 取 3），或由云 KMS 自动解封。该机制保证"存储介质泄露 ≠ 密钥泄露"，代价是可用性依赖 unseal 流程。
2. **Q：Vault 的动态凭证（Dynamic Secrets）与静态密钥区别？**
   A：静态密钥（KV）是存量的固定值；动态凭证是"即用即签"：应用请求时 Vault 通过数据库/PKI 引擎临时生成凭证（带租约 TTL），到期自动吊销。优点是泄露窗口极小、无需人工轮转；代价是应用需适配租约续期与连接池校验，否则轮转会造成连接风暴。
3. **Q：Vault 与 Kubernetes 集成的认证流程？**
   A：应用 Pod 用自身 SA Token 请求 Vault 的 Kubernetes Auth：Vault 调用 apiserver 的 TokenReview 校验 Token 有效性 → 按 Vault 角色（绑定 SA 名称/命名空间）映射策略 → 签发短期 Vault Token → 应用按策略访问密钥路径。集成方式有 agent sidecar、CSI provider、ESO 三种主流方案。

## 运维要点

- unseal 演练：季度性执行 sealed → unseal 恢复演练，验证分片持有人流程。
- 租约监控：动态凭证租约续期率与吊销量纳入监控，识别连接风暴前兆。
- 备份演练：Raft snapshot 恢复演练季度执行，备份加密存储。
- 版本升级：Vault 升级前阅读 upgrade guide，避免协议不兼容。
- 排障入口：先 `vault status`（seal/HA）→ `vault token capabilities`（权限）→ 应用日志（集成层）。

## 参考链接

- [Vault Official](https://www.vaultproject.io/)

## Related

- [[17-系统基础/06-知识字典/security/secret.md|Secret]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]
- [[17-系统基础/06-知识字典/security/certificate-authority.md|Certificate Authority]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/security/service-account.md|Service Account]]


<!-- risk-assessed -->
