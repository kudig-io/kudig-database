---
title: 密钥管理深度指南
description: '# 密钥管理深度指南'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- falco
- ingress
- gateway
- rbac
- networkpolicy
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 密钥管理深度指南 是什么
- 如何 密钥管理深度指南
trigger_keywords:
- 密钥管理深度指南
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- tls-basics
created: "2026-05-23"
---

# 密钥管理深度指南

## 概述

在 [[Kubernetes|Kubernetes]] 环境中，[[Secrets|Secrets]]（如数据库密码、API 密钥、TLS 证书、OAuth Token）是攻击者最觊觎的目标。2026 年的安全最佳实践认为，**单纯依赖 Kubernetes 原生的 Secret 资源已不足以应对企业级安全要求**。现代密钥管理需要结合**外部密钥管理系统（KMS）、自动轮转、最小权限访问、审计日志和硬件安全模块（HSM）**，构建端到端的 Secret 生命周期管理体系。

## 核心概念/原理

### 1. Kubernetes Secret 的局限

原生 Secret 存在以下安全风险：
- **默认未加密存储**：[[etcd|etcd]] 中的 Secret 默认以 Base64 编码存储，若 etcd 被攻破则 Secret 泄露
- **访问控制粗粒度**：任何具有 Pod 创建权限的用户都可能读取同一 Namespace 中的 Secret
- **无自动轮转**：Kubernetes 本身不提供 Secret 的自动过期和更新机制
- **缺乏审计**：无法追踪谁、在何时、以何种方式使用了 Secret

> **注意**：Kubernetes 1.25+ 支持 etcd 加密（Encryption at Rest），这是生产环境的最低要求，但仍有上述其他局限。

### 2. 外部密钥管理系统（KMS）

企业级 KMS 将 Secret 的存储和管理从 Kubernetes 中剥离，提供更高的安全保证：

| 方案 | 特点 | 适用场景 |
|------|------|----------|
| **HashiCorp Vault** | 功能最全面，支持动态 Secret、PKI、数据库凭证 | 金融、大型企业 |
| **AWS Secrets Manager** | 与 AWS IAM 深度集成，支持自动轮转 | AWS 原生环境 |
| **Azure Key Vault** | 与 Azure AD、Managed Identity 集成 | Azure 原生环境 |
| **GCP Secret Manager** | 与 GCP IAM、Cloud KMS 集成 | GCP 原生环境 |
| **Doppler / 1Password Secrets Automation** | SaaS 化，开发者体验好 | 中小企业、初创公司 |

### 3. External Secrets Operator（ESO）

**ESO** 是 2026 年 Kubernetes Secret 管理的主流方案，它在集群中运行 Controller，自动从外部 KMS 同步 Secret 到 Kubernetes Secret 资源中：

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: db-password
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    kind: ClusterSecretStore
    name: vault-backend
  target:
    name: database-secret
  data:
    - secretKey: password
      remoteRef:
        key: secret/data/db
        property: password
```

**优势**：
- 开发团队仍使用熟悉的 Kubernetes Secret 语义
- 实际密钥由 Vault/AWS/Azure 管理，支持审计、轮转、细粒度访问控制
- Secret 变更后自动同步到集群

### 4. Sealed Secrets

**Bitnami Sealed Secrets** 提供了一种将 Secret 安全存入 Git 的方案：
- 运维人员使用 `kubeseal` CLI 将普通 Secret 加密为 `SealedSecret` 自定义资源
- `SealedSecret` 可以安全地提交到公共 Git 仓库
- 集群中的 Controller 是唯一能够解密的实体
- 适合 GitOps 工作流，但不如 ESO 灵活（不支持动态 Secret 和自动轮转）

```bash
# 加密 Secret
echo -n 'my-password' | kubectl create secret generic my-secret \
  --dry-run=client --from-file=password=/dev/stdin -o yaml | \
  kubeseal --controller-namespace=kube-system --format yaml > my-sealed-secret.yaml
```

### 5. [[domain-19-landscape-references/01-cncf-landscape/graduated/cert-manager/cert-manager|cert-manager]]

**cert-manager** 是 Kubernetes 上自动化 TLS 证书管理的 CNCF 项目：
- 自动从 Let's Encrypt、Vault PKI、AWS PCA 等 CA 申请和更新证书
- 将证书以 Secret 形式注入 Ingress / Gateway / Pod
- 支持证书过期前自动续期

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com
  namespace: default
spec:
  secretName: example-com-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
    - example.com
    - www.example.com
```

## 关键机制或特性

### Secret 访问的零信任模型

2026 年的最佳实践要求：
- **按 Pod/ServiceAccount 授权**：使用 SPIFFE/SPIRE 或 Vault Kubernetes Auth 将 Secret 访问绑定到特定身份
- **短期动态凭证**：数据库密码不应是静态的，而应由 Vault 按需生成、设置 TTL、用完后自动吊销
- **Secret 审计日志**：记录每次 Secret 读取和使用的操作者、时间、IP 地址
- **网络隔离**：Vault/KMS 控制平面应与业务 Pod 运行在不同网络段，通过严格 NetworkPolicy 隔离

### Secret 轮转策略

| 轮转类型 | 说明 | 实现方式 |
|----------|------|----------|
| **手动轮转** | 安全事件后人工更新 | kubectl apply / Vault UI |
| **定时轮转** | 每 30/60/90 天自动更新 | Vault Rotation / AWS Secret Manager |
| **事件驱动轮转** | 员工离职、凭证泄露后触发 | Webhook / CI Pipeline |
| **动态短期凭证** | 每次连接使用不同密码 | Vault Database Secrets Engine |

### Secret 注入模式

| 模式 | 优点 | 缺点 |
|------|------|------|
| **环境变量注入** | 最简单、最兼容 | Secret 可能在进程列表中泄露 |
| **Volume 挂载注入** | Secret 作为文件，更安全 | 应用需支持文件读取 |
| **Sidecar 注入** | 动态获取和更新 Secret | 增加复杂度 |
| **运行时 SDK 直取** | 直接从 Vault/KMS 读取 | 需要应用改造 |

## 使用场景

1. **多租户集群的 Secret 隔离**：每个团队通过 ESO 连接到自己的 Vault Namespace，互不干扰
2. **数据库动态密码**：应用启动时从 Vault 获取有效期为 1 小时的数据库密码，无需硬编码
3. **GitOps 中的 Secret 管理**：使用 Sealed Secrets 将加密的 Secret 与应用配置一起存入 Git
4. **TLS 证书全自动化**：Ingress 证书通过 cert-manager 自动申请和续期，运维零介入
5. **密钥泄露应急响应**：一旦怀疑 Secret 泄露，通过 Vault 立即吊销旧凭证并自动分发新凭证到所有 Pod

## 最佳实践/注意事项

- **启用 etcd 加密 at Rest**：所有生产集群必须配置 `--encryption-provider-config` 对 Secret 进行 AES-GCM 加密
- **限制 Secret 的 RBAC 权限**：仅授予真正需要读取 Secret 的 ServiceAccount，禁止 human user 直接访问
- **避免将 Secret 存入 Git**：除非使用 Sealed Secrets 或 SOPS 加密，否则永远不要将明文 Secret 提交到 Git
- **使用动态 Secret 替代静态密码**：对于数据库、云服务 API，优先使用 Vault 的动态凭证能力
- **监控 Secret 访问异常**：通过 Falco 或审计日志检测异常的 Secret 读取行为
- **定期执行 Secret 扫描**：使用工具（如 GitLeaks、TruffleHog）扫描代码库，防止开发者误提交密钥
- **最小化 Secret 作用域**：按 Namespace 或按 Pod 拆分 Secret，避免"万能 Secret"模式
- **Secret 更新后滚动重启**：当 Secret 内容变更时，需要通过 Restart Annotation 或 Operator 触发 Pod 滚动更新

## 参考链接

- [External Secrets Operator Documentation](https://external-secrets.io/latest/)
- [HashiCorp Vault on Kubernetes](https://developer.hashicorp.com/vault/docs/platform/k8s)
- [Bitnami Sealed Secrets](https://sealed-secrets.netlify.app/)
- [cert-manager Documentation](https://cert-manager.io/docs/)
- [Kubernetes Secret Encryption at Rest](https://kubernetes.io/docs/tasks/administer-cluster/encrypt-data/)

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
