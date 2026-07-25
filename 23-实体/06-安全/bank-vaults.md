---
title: Bank-Vaults (entities)
description: '## 概述'
summary: 'Bank-Vaults 是一套围绕 HashiCorp Vault 构建的 Kubernetes 原生密钥管理工具集。它提供 Vault Operator 自动化部署和管理 Vault 集群、Webhook 自动注入密钥到 Pod 环境变量和文件、以及多种云 KMS 后端的自动解封能力。'
category: entities
tags:
- k8s
- cncf
- security
- bank-vaults
- etcd
- prometheus
- crd
- operator
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Bank-Vaults 是什么
- 如何 Bank-Vaults
trigger_keywords:
- Bank-Vaults
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Bank-Vaults

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Bank-Vaults 是由 Banzai Cloud（现 Cisco）开源的 Kubernetes 原生 HashiCorp Vault 管理工具集，2019 年加入 CNCF Sandbox。它围绕 Vault 构建了三个核心组件：Vault Operator（自动化部署和管理 Vault 集群）、Vault Webhook（将密钥自动注入到 Pod 环境变量和文件中）和 Vault Secrets Webhook（Secret 挂载转换）。Bank-Vaults 大幅简化了在 Kubernetes 环境中使用 Vault 进行密钥管理的复杂度。

## 核心特性

- **Vault Operator**: 通过 CRD 声明式部署和管理 Vault 集群，支持 HA Raft
- **自动解封**: 支持云 KMS（AWS KMS、GCP KMS、Azure Key Vault）自动解封
- **密钥注入**: Mutating Webhook 自动将 Vault 密钥注入 Pod 环境变量和文件
- **多后端存储**: 支持 Consul、etcd、Raft、文件系统等多种存储后端
- **监控集成**: 内置 Prometheus 指标导出
- **配置即代码**: 使用 YAML 文件声明式管理 Vault 配置和策略

## 架构

Bank-Vaults 由多个组件构成。Vault Operator 监听 Vault CRD，管理 Vault Pod 的部署、配置、扩缩容和升级。每个 Vault 实例配置 Unseal Config（KMS 或 Kubernetes Secrets 自动解封）。Vault Webhook 作为 Mutating Admission Controller 拦截 Pod 创建请求，将 Vault 密钥注入环境变量或 CSI 卷。配置管理器（bank-vaults configure）持续同步外部配置文件到 Vault，实现配置即代码。

## Kubernetes 集成

Bank-Vaults 通过 CRD（Vault、VaultSecret）声明式管理 Vault 生命周期。Vault Webhook 通过 Mutating Admission Webhook 在 Pod 创建时自动注入密钥，无需应用代码感知 Vault。支持 Kubernetes Service Account Token 认证到 Vault，实现 Pod 身份与 Vault 角色的绑定。Operator 管理 PodDisruptionBudget、Services 和 Ingress，确保 Vault 集群高可用。

## 生产使用场景

1. **集中式密钥管理**: 使用 Vault 统一管理数据库密码、API Keys 和证书
2. **动态密钥**: 为每个 Pod 动态生成短期数据库凭证
3. **自动解封**: 利用云 KMS 实现 Vault 自动解封，避免人工干预
4. **密钥注入**: 通过 Webhook 自动将密钥注入应用，无需 SDK 集成

## 安装与配置

```bash
helm repo add banzaicloud-stable https://kubernetes-charts.banzaicloud.com
helm install vault-operator banzaicloud-stable/vault-operator \
  -n vault --create-namespace
# 创建 Vault 集群
kubectl apply -f - <<EOF
apiVersion: vault.banzaicloud.com/v1alpha1
kind: Vault
metadata:
  name: vault
  namespace: vault
spec:
  size: 3
  image: hashicorp/vault:1.17
  serviceAccount: vault
  unsealConfig:
    aws:
      kmsKeyId: <key-id>
      region: us-east-1
  config:
    storage:
      raft:
        path: /vault/data
    listener:
      tcp:
        address: "0.0.0.0:8200"
        tls_cert_file: /vault/tls/server.crt
        tls_key_file: /vault/tls/server.key
EOF
```

### Webhook 密钥注入配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  annotations:
    # 启用 Vault 密钥注入
    vault.security.banzaicloud.io/vault-addr: "https://vault:8200"
    vault.security.banzaicloud.io/vault-role: "my-app-role"
spec:
  template:
    spec:
      containers:
        - name: app
          image: myapp:latest
          env:
            - name: DB_PASSWORD
              value: "vault:secret/data/myapp#password"
            - name: API_KEY
              value: "vault:secret/data/myapp#api_key"
```

## 运维操作

```bash
# 🟢 查看 Vault 集群状态
kubectl get vault -n vault
kubectl exec -n vault vault-0 -- vault status

# 🟢 查看 Vault Operator 日志
kubectl logs -n vault deploy/vault-operator

# 🟢 检查 Webhook 状态
kubectl get mutatingwebhookconfigurations | grep vault

# 🟡 初始化 Vault（首次部署）
kubectl exec -n vault vault-0 -- vault operator init -key-shares=5 -key-threshold=3

# 🟡 配置 Vault 策略
kubectl exec -n vault vault-0 -- vault policy write my-app - <<EOF
path "secret/data/myapp/*" {
  capabilities = ["read"]
}
EOF

# 🔴 密封 Vault（紧急场景）
kubectl exec -n vault vault-0 -- vault operator seal

# 🟢 查看密钥注入日志
kubectl logs -n vault deploy/vault-webhook
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Vault Pod CrashLoop | 存储后端不可用 | `kubectl logs vault-0 -n vault` | 检查 Raft/Consul 连接 |
| 自动解封失败 | KMS 权限不足 | `kubectl logs vault-operator -n vault` | 检查 IAM 角色和 KMS Key 策略 |
| 密钥注入失败 | Webhook 未拦截 | `kubectl get mutatingwebhookconfigurations` | 检查 namespaceSelector 匹配 |
| Pod 无法读取密钥 | Vault Role 未配置 | `vault read auth/kubernetes/role/my-app` | 创建 Kubernetes Auth Role |
| TLS 连接失败 | 证书过期 | `openssl s_client -connect vault:8200` | 更新 Vault TLS 证书 |

### 排查流程

```
Bank-Vaults 异常
├─ Vault 集群不可用？
│  ├─ Pod 未运行 → 检查 PVC/存储后端
│  ├─ Sealed 状态 → 检查自动解封配置
│  └─ Raft 集群异常 → vault operator raft list-peers
├─ 密钥注入失败？
│  ├─ Webhook 未触发 → 检查注解和 namespaceSelector
│  ├─ Vault 认证失败 → 检查 ServiceAccount 和 Role
│  └─ 密钥路径错误 → 验证 vault:secret/data/... 路径
└─ Operator 异常？
   ├─ CRD 未注册 → 重新安装 Operator
   └─ 配置同步失败 → 检查 RBAC 权限
```

## 生产案例

### 案例 1: 金融企业动态数据库凭证

**场景**: 某证券公司需为每个微服务生成独立的短期数据库凭证，有效期 1 小时。

**方案**:
1. 部署 Bank-Vaults + Vault HA 集群（Raft 存储）
2. 配置 Vault Database Secret Engine 动态生成 PostgreSQL 凭证
3. Webhook 自动注入动态凭证到 Pod
4. 凭证 1 小时后自动过期

**效果**: 消除静态密码泄露风险，审计日志可追踪每个服务的数据库访问。

### 案例 2: 多团队密钥自助管理

**场景**: 50+ 开发团队需要安全管理各自的 API Keys 和证书。

**方案**:
1. 按团队划分 Vault 命名空间
2. 配置 Kubernetes Auth 绑定 ServiceAccount
3. 团队通过 Vault UI 自助管理密钥
4. Webhook 自动注入，无需代码修改

**效果**: 密钥管理工单减少 90%，开发者无需感知 Vault API。

## 对比与替代方案

| 维度 | Bank-Vaults | External Secrets Operator | Vault CSI Provider | Sealed Secrets |
|------|-------------|--------------------------|--------------------|----------------|
| Vault 集群管理 | ✅ | ❌ | ❌ | ❌ |
| 自动解封 | ✅ | N/A | N/A | N/A |
| 密钥注入 | Webhook | CRD 同步 | CSI 卷 | Controller |
| 动态密钥 | ✅ | 部分 | ❌ | ❌ |
| GitOps 友好 | 中 | ✅ | 中 | ✅ |
| 多后端 | 仅 Vault | AWS/Azure/GCP/Vault | 仅 Vault | 仅 K8s |

## 检查清单

- [ ] Vault HA 集群已部署（3 节点 Raft）
- [ ] 自动解封已配置（云 KMS）
- [ ] TLS 证书已配置并自动轮换
- [ ] Kubernetes Auth 已启用
- [ ] Webhook 已部署并测试注入
- [ ] 密钥访问策略已按团队/应用划分
- [ ] 审计日志已启用并发送到 SIEM
- [ ] 备份策略已配置（Raft Snapshot）

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Bank-Vaults** | 功能全面、Webhook 注入 | 仅支持 Vault、社区较小 |
| External Secrets Operator | 多后端（AWS/Azure/GCP/Vault） | 无集群部署能力，仅同步 |
| Vault CSI Provider | CSI 原生集成 | 功能单一、仅文件挂载 |
| Sealed Secrets | 简单易用、GitOps 友好 | 无动态密钥和集中管理 |

## 架构定位

在 CNCF 生态中，Bank-Vaults 属于 **Security** 类别，是 HashiCorp Vault 在 Kubernetes 上的最佳实践工具集。它与 External Secrets Operator 互补，前者管理 Vault 本身，后者同步密钥。

## 参考链接

- [[etcd]]
- [[23-实体/06-安全/vault.md|vault]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[submariner]] — Submariner
- [[03-prometheus-ha-deployment]] — [[Prometheus|Prometheus]]us 高可用部署|Prometheus 高可用部署]]
- [[inclavare-containers]] — Inclavare Containers
- [[23-实体/06-安全/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bank-vaults
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
