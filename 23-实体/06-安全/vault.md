---
title: HashiCorp Vault
description: '- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
summary: '- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]] — 综合'
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




# HashiCorp Vault

> HashiCorp Vault 是企业级密钥管理平台，提供静态密钥存储、动态凭据、PKI、加密即服务、完整审计日志，是云原生零信任架构的核心组件。

## 基本信息

| 属性 | 值 |
|------|------|
| 开发 | HashiCorp (BSL 许可) |
| 语言 | Go |
| 架构 | Client-Server, Raft/Consul 存储 |
| K8s 集成 | Agent Sidecar / CSI Driver / External Secrets |
| 官网 | https://www.vaultproject.io |
| 替代方案 | Secrets Store CSI, External Secrets Operator, Sealed Secrets |

## 核心能力

| 功能 | 说明 | 场景 |
|------|------|------|
| KV Store | 静态密钥存储，版本控制 | 数据库密码、API Key |
| Dynamic Credentials | 临时数据库凭据，自动过期 | DB 访问、云 API |
| PKI Engine | 内部 CA，证书签发/轮换 | mTLS、内部服务 |
| Transit Engine | 加密即服务 (encrypt/decrypt API) | 应用数据加密 |
| Auth Methods | K8s SA, LDAP, OIDC, AppRole | 身份认证 |
| Audit Trail | 完整审计日志 | 合规审计 |
| SSH | 签发 SSH 证书/OTP | 服务器访问 |
| TotP | 生成/验证 TOTP | MFA |

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│                  Vault Server Cluster                │
│                                                      │
│  ┌────────┐  ┌────────┐  ┌────────┐           │
│  │Active  │  │Standby │  │Standby │  (HA)     │
│  │Node    │  │Node    │  │Node    │           │
│  └───┬────┘  └────────┘  └────────┘           │
│      │                                          │
│      ▼                                          │
│  ┌──────────────────────────────────────┐  │
│  │  Storage Backend (Raft/Consul/S3)    │  │
│  └──────────────────────────────────────┘  │
│      │                                          │
│      ▼                                          │
│  ┌──────────────────────────────────────┐  │
│  │  Barrier (加密层)                    │  │
│  │  所有数据经过 Barrier 加密后存储    │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────┐
│  客户端: K8s Pod / CLI / API / Terraform          │
└─────────────────────────────────────────────────────┘
```

## K8s 集成模式

| 模式 | 原理 | 适用场景 |
|------|------|----------|
| Agent Sidecar | Vault Agent 注入密钥到内存卷 | 生产环境 (密钥不落盘) |
| External Secrets Operator | 同步 Vault 密钥到 K8s Secret | GitOps 兼容 |
| CSI Driver | 通过 CSI 接口挂载密钥文件 | 特殊合规要求 |

### Agent Sidecar 示例

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
        vault.hashicorp.com/role: "app-role"
        vault.hashicorp.com/agent-inject-secret-db: "database/creds/app-role"
        vault.hashicorp.com/agent-inject-template-db: |
          {{- with secret "database/creds/app-role" -}}
          DB_USER={{ .Data.username }}
          DB_PASS={{ .Data.password }}
          {{- end }}
    spec:
      serviceAccountName: app-sa
      containers:
      - name: app
        image: myapp:latest
        volumeMounts:
        - name: vault-secrets
          mountPath: /vault/secrets
          readOnly: true
```

### Kubernetes Auth 配置

```bash
# 🟡 启用 K8s 认证
vault auth enable kubernetes

# 🟡 配置 K8s 认证
vault write auth/kubernetes/config \
  kubernetes_host="https://kubernetes.default.svc:443" \
  token_reviewer_jwt="$(kubectl get secret vault-auth -o jsonpath='{.data.token}' | base64 -d)" \
  kubernetes_ca_cert=@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt

# 🟡 创建 Role
vault write auth/kubernetes/role/app-role \
  bound_service_account_names=app-sa \
  bound_service_account_namespaces=default \
  policies=app-policy \
  ttl=1h

# 🟡 创建 Policy
vault policy write app-policy - <<EOF
path "database/creds/app-role" {
  capabilities = ["read"]
}
path "secret/data/app/*" {
  capabilities = ["read"]
}
EOF
```

## 动态凭据示例

```bash
# 🟡 配置数据库连接
vault write database/config/mydb \
  plugin_name=mysql-database-plugin \
  connection_url="{{username}}:{{password}}@tcp(mysql:3306)/" \
  allowed_roles="app-role" \
  username="vault_admin" \
  password="admin_pass"

# 🟡 创建动态凭据角色
vault write database/roles/app-role \
  db_name=mydb \
  creation_statements="CREATE USER '{{name}}'@'%' IDENTIFIED BY '{{password}}'; GRANT SELECT ON mydb.* TO '{{name}}'@'%';" \
  default_ttl="1h" \
  max_ttl="24h"

# 🟢 获取动态凭据
vault read database/creds/app-role
# 返回临时用户名/密码，1小时后自动过期
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Vault 状态
vault status
kubectl get pods -n vault

# 🟢 查看已启用引擎
vault secrets list
vault auth list

# 🟢 读取密钥
vault kv get secret/app/config
vault kv get -version=2 secret/app/config

# 🟡 写入密钥
vault kv put secret/app/config db_host="mysql" db_port="3306"

# 🟢 查看审计日志
vault audit list

# 🔴 初始化 Vault (仅首次)
vault operator init -key-shares=5 -key-threshold=3

# 🔴 解封 Vault
vault operator unseal <key-1>
vault operator unseal <key-2>
vault operator unseal <key-3>
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Vault Sealed | 重启后未解封 | 执行 unseal 或配置 auto-unseal |
| Pod 无法获取密钥 | SA/Role 配置错误 | 检查 vault auth 配置 |
| 动态凭据失败 | DB 连接失败 | 检查 database/config |
| Agent Sidecar 未注入 | Annotation 缺失 | 检查 Pod annotations |
| 性能下降 | 存储后端慢 | 检查 Raft/Consul 状态 |

## Vault vs K8s Secrets

| 特性 | Vault | K8s Secrets |
|------|-------|-------------|
| 加密 | 静态+传输 | 可选 (etcd 加密) |
| 动态凭据 | 支持 | 不支持 |
| 审计 | 完整 | 基本 (audit log) |
| 轮换 | 自动 | 手动 |
| 访问控制 | 细粒度 Policy | RBAC |
| 复杂度 | 高 | 低 |

## 检查清单

- [ ] 理解 Vault 架构 (Active/Standby, Barrier)
- [ ] 掌握 K8s Auth 配置
- [ ] 能配置 Agent Sidecar 注入
- [ ] 理解动态凭据原理
- [ ] 掌握 Seal/Unseal 流程
- [ ] 能排查密钥获取失败问题
- [ ] 了解 Vault vs K8s Secrets 选型

## Related

- [[external-secrets]] — External Secrets Operator
- [[cert-manager]] — cert-manager
- [[22-概念/05-安全/secrets-management.md|Secrets Management]]
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[22-概念/11-交叉分析/Secret 管理 × 存储模型.md|Secret 管理 × 存储模型]]

<!-- risk-assessed -->
