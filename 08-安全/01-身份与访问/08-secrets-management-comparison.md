---
title: "Secrets 管理方案对比"
description: "Secrets 管理对比：Vault vs External Secrets Operator vs Sealed Secrets vs SOPS，K8s Secret 加密与密钥生命周期"
summary: "面向 SRE 与安全工程师的 Kubernetes Secrets 管理方案全面对比，覆盖 Vault、ESO、Sealed Secrets、SOPS 的选型、部署、加密与密钥生命周期管理。"
category: 安全
tags:
- secrets
- vault
- external-secrets
- sealed-secrets
- sops
- encryption
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes Secrets 管理方案如何选择"
- "Vault 和 External Secrets Operator 对比"
- "如何加密 etcd 中的 Secret"
trigger_keywords:
- secrets
- vault
- external secrets
- sealed secrets
- sops
- encryption at rest
prerequisites:
- kubectl-basics
- rbac-basics
- security-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Secrets 管理方案对比

> **适用版本**: Vault 1.16+ / External Secrets Operator 0.9+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

Kubernetes 原生 Secret 的安全模型存在根本性缺陷：它仅对数据做 Base64 编码（而非加密），默认以明文形式存储在 etcd 中。这意味着，任何拥有 etcd 直接访问权限的人（如集群管理员、备份恢复人员），或者拥有 get secret RBAC 权限的 ServiceAccount，都能轻松读取所有 Secret 的明文内容。在金融、医疗、电商等对数据安全有严格要求的行业，这种安全级别是完全不可接受的。

围绕这一痛点，业界涌现出多种 Secrets 管理方案，从重量级的 HashiCorp Vault 到轻量级的 Mozilla SOPS，各有其适用场景和取舍。选型的复杂性在于，不同方案在安全性、易用性、运维成本、功能完整度上的差异巨大，没有放之四海而皆准的最优解——一个 5 人创业团队和一个千人企业的 Secrets 管理需求截然不同。

本文系统对比四大主流方案——Vault、External Secrets Operator、Sealed Secrets、SOPS，覆盖架构原理、生产部署、etcd 静态加密、密钥生命周期管理，帮助平台与安全团队根据自身规模和需求做出合理选型。Vault 的企业级实践见 [[08-安全/01-身份与访问/05-vault-enterprise-secrets-management.md|Vault 企业级 Secrets 管理]]，密钥轮换见 [[08-安全/01-身份与访问/09-key-rotation-automation.md|密钥轮换自动化]]。

---

## 核心概念

### 1. K8s 原生 Secret 的安全缺陷

要理解为什么需要外部 Secrets 管理方案，首先要清楚原生 Secret 到底有哪些安全问题。

| 缺陷 | 说明 |
|------|------|
| 仅 Base64 编码 | 非加密，可逆 |
| etcd 明文存储 | 默认无静态加密 |
| RBAC 粗粒度 | get secret 即可读全部 key |
| 无审计 | 默认不记录 Secret 访问 |
| 无轮换 | 需手动更新 |
| GitOps 泄露 | YAML 中明文 Secret 进 Git |

Base64 编码不是加密，这是最常见的误解。Base64 只是一种编码方式，任何人都可以解码，它不提供任何安全保护。etcd 明文存储意味着，即使你没有 kubectl 权限，只要能访问 etcd 的数据文件（比如通过备份文件），就能读取所有 Secret。RBAC 的粗粒度问题在于，Kubernetes 的 RBAC 无法精确到 Secret 的某个 key——你要么能读取整个 Secret 的所有 key，要么完全不能读取。在 GitOps 流程中，如果将 Secret 的 YAML 直接提交到 Git 仓库，那么所有有仓库读权限的人都能看到明文，这是一个极其常见的安全漏洞。

### 2. 四大方案对比

| 维度 | Vault | External Secrets Operator | Sealed Secrets | SOPS |
|------|-------|--------------------------|----------------|------|
| 类型 | 独立密钥管理系统 | K8s Operator（桥接外部） | K8s Controller | CLI 加密工具 |
| 密钥存储 | Vault 服务端 | 外部 Provider（Vault/云KMS） | 加密后存 Git | 加密文件存 Git |
| 动态密钥 | ✅ 强（数据库/云） | ✅（依赖 Provider） | ❌ | ❌ |
| 自动轮换 | ✅ | ✅ | ⚠️ 有限 | ❌ 手动 |
| 部署复杂度 | 高 | 中 | 低 | 极低 |
| 运维成本 | 高（需维护 Vault） | 中 | 低 | 低 |
| 适用规模 | 大型/合规要求高 | 中大型、多云 | 中小型、GitOps | 小型、个人 |
| 学习曲线 | 陡 | 中 | 缓 | 缓 |

### 3. 方案定位与选型思路

Vault 是一个完整的密钥管理平台，它不仅仅存储静态密钥，还能动态生成数据库凭证、云 API 密钥，提供加密即服务（Encryption-as-a-Service）、完整的访问审计和细粒度策略控制。但它的代价是高昂的运维成本——Vault 本身是一个有状态的分布式系统，需要专门的团队来维护其可用性、执行 unseal 操作、管理策略。

External Secrets Operator（ESO）走了一条巧妙的路线：它本身不存储任何密钥，而是作为 Kubernetes 与外部密钥管理系统之间的桥梁。ESO 从 Vault、AWS Secrets Manager、GCP Secret Manager、Azure Key Vault 等外部 Provider 拉取密钥，并以 Kubernetes Secret 的形式注入到 Pod 中。这种设计让它能复用企业已有的密钥管理基础设施，而不需要引入新的存储层。

Sealed Secrets 的理念最为简洁：用非对称加密让 Secret 可以安全地存入 Git。开发者用 kubeseal 工具将 Secret 加密为 SealedSecret 资源，这个加密后的资源可以安全地提交到 Git 仓库，只有集群中的 Sealed Secrets Controller 持有私钥能够解密。它的局限在于不支持动态密钥和自动轮换，功能相对基础。

SOPS（Secrets OPerationS）是 Mozilla 开发的文件级加密工具，它加密文件中的值而保留键名，支持 YAML、JSON、ENV 等多种格式，加密密钥可以来自 AWS KMS、GCP KMS、Azure Key Vault 或 PGP。SOPS 通常与 GitOps 工具（如 Flux、Argo CD）配合使用，在部署时解密。

---

## 生产部署/实现

### 1. etcd 静态加密（基础必备） 🔴

无论选择哪种外部方案，启用 etcd 静态加密都是最基本的防护措施。它确保即使 etcd 数据文件被窃取，Secret 内容也无法被直接读取。

```yaml
# 🔴 高风险：修改 apiserver 加密配置，错误将导致 Secret 不可读
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  providers:
  - aescbc:
      keys:
      - name: key1
        secret: <base64-encoded-32-byte-key>
  - identity: {}      # 兜底，用于解密旧数据
```

```bash
# 🔴 高风险：启用加密需重启 apiserver 并重写所有 Secret
# 1. 配置 apiserver --encryption-provider-config
# 2. 重启控制平面
# 3. 重写所有 Secret 以触发加密
kubectl get secrets --all-namespaces -o json | kubectl replace -f -
```

EncryptionConfiguration 中 providers 的顺序至关重要：apiserver 会用第一个 provider 加密新数据，但解密时会按顺序尝试所有 provider。因此 identity（明文）provider 必须保留在列表末尾，用于解密启用加密之前创建的旧 Secret。重写所有 Secret 的操作（kubectl get | kubectl replace）会触发 apiserver 用新的加密 provider 重新写入每个 Secret，完成全量加密。这个过程在大规模集群中可能需要较长时间，且会产生大量 etcd 写入，应在业务低峰期执行。

### 2. External Secrets Operator 部署 🟡

ESO 的部署分为两部分：安装 Operator 本身，以及配置 SecretStore 和 ExternalSecret 资源。

```yaml
# 🟡 中风险：ESO 从外部拉取密钥，需配置 Provider 凭证
# 安装 ESO
# helm install external-secrets external-secrets/external-secrets -n external-secrets --create-namespace

# SecretStore（指向 Vault）
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.internal:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-apps"
          serviceAccountRef:
            name: eso-sa
---
# ExternalSecret（声明式拉取）
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  refreshInterval: "1h"
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: app-secrets
    creationPolicy: Owner
  data:
  - secretKey: db-password
    remoteRef:
      key: production/database
      property: password
  - secretKey: api-key
    remoteRef:
      key: production/api
      property: key
```

ESO 使用 Kubernetes ServiceAccount 认证到 Vault，这避免了在集群中存储 Vault 的静态凭证。SecretStore 定义了密钥的来源和认证方式，ExternalSecret 则声明了需要从外部拉取哪些密钥、映射到 Kubernetes Secret 的哪些 key。refreshInterval 控制同步频率，当外部密钥被轮换后，ESO 会在下一个同步周期自动更新 Kubernetes Secret。creationPolicy: Owner 表示 ESO 拥有它创建的 Secret 的生命周期管理权，当 ExternalSecret 被删除时，对应的 Secret 也会被清理。

### 3. Sealed Secrets（GitOps 友好） 🟢

Sealed Secrets 的使用流程极其简洁：本地加密、提交 Git、集群自动解密。

```bash
# 🟢 低风险：本地加密 Secret 为 SealedSecret
# 安装 controller 后，用 kubeseal 加密
kubectl create secret generic app-secrets \
  --from-literal=db-password='s3cr3t' \
  --dry-run=client -o yaml | \
  kubeseal --controller-namespace=kube-system \
    --controller-name=sealed-secrets \
    --format yaml > sealed-secret.yaml
# sealed-secret.yaml 可安全提交到 Git
```

```yaml
# 🟢 低风险：SealedSecret 可安全存入 Git
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  encryptedData:
    db-password: AgBy3i4OJSWK+PiTySYZZA9rO43cGDEq...
  template:
    metadata:
      name: app-secrets
      namespace: production
```

SealedSecret 中的 encryptedData 是用 Controller 的公钥加密后的密文，即使 Git 仓库被泄露，攻击者也无法从中恢复出原始密钥。只有集群中持有对应私钥的 Sealed Secrets Controller 才能解密。需要注意的是，Sealed Secrets 默认使用 strict 模式，加密时绑定了 namespace 和 name，这意味着 SealedSecret 只能在加密时指定的 namespace 和 name 下使用，无法被复制到其他位置——这是一个安全特性，防止 Secret 被未授权地复制。

---

## 运维操作

### 1. 检查 Secret 暴露面 🟢

Secret 治理的第一步是了解当前的暴露面——有多少 Secret、谁在使用它们、是否启用了加密。

```bash
# 🟢 低风险：只读审计
# 列出所有 Secret（不显示内容）
kubectl get secrets -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,TYPE:.type

# 检查哪些 Pod 挂载了 Secret
kubectl get pods -A -o json | jq '.items[] | select(.spec.volumes[]?.secret) | .metadata.name'

# 检查 etcd 加密状态
kubectl -n kube-system get pod kube-apiserver-master -o yaml | grep encryption-provider
```

### 2. Vault 动态密钥示例 🟡

动态密钥是 Vault 最强大的能力之一——它不存储静态密码，而是在每次请求时动态生成一个短生命周期的数据库用户，到期后自动删除。

```bash
# 🟡 中风险：配置 Vault 数据库动态密钥
vault write database/config/mydb \
  plugin_name=mysql-database-plugin \
  connection_url="{{username}}:{{password}}@tcp(db:3306)/" \
  allowed_roles="app-role" \
  username="vault" \
  password="vault-pass"

vault write database/roles/app-role \
  db_name=mydb \
  creation_statements="CREATE USER '{{name}}'@'%' IDENTIFIED BY '{{password}}';GRANT SELECT ON mydb.* TO '{{name}}'@'%';" \
  default_ttl="1h" \
  max_ttl="24h"
```

动态密钥从根本上消除了长期凭证泄露的风险：每个应用实例获得一个独立的、短生命周期的数据库用户，即使凭证泄露，攻击窗口也只有 1 小时（default_ttl），且可以通过 Vault 立即撤销。同时，由于每个实例使用不同的凭证，审计日志可以精确追踪到是哪个实例执行了哪些数据库操作。

### 3. 密钥轮换 🟡

```bash
# 🟡 中风险：轮换 etcd 加密密钥
# 1. 生成新密钥，置于 EncryptionConfiguration 首位
# 2. 重启 apiserver
# 3. 重写所有 Secret
kubectl get secrets --all-namespaces -o json | kubectl replace -f -
# 4. 验证后移除旧密钥
```

---

## 故障排查

### 症状 1：ESO 同步失败

```bash
# 🟢 低风险
kubectl -n production describe externalsecret app-secrets
kubectl -n external-secrets logs deploy/external-secrets
```

根因可能是 Provider 凭证过期（如 Vault 的 Kubernetes 认证 token 失效）、ESO 到 Vault 的网络不通、或者 remoteRef 中的 key/property 路径错误。处置方法是刷新 ServiceAccount token、检查网络连通性和防火墙规则、核对 Vault 中的实际路径。

### 症状 2：SealedSecret 解密失败

根因通常是 Controller 的加密密钥被更换（如重新部署时未保留密钥备份），或者 strict 模式下 namespace/name 不匹配。处置方法是从备份恢复 Controller 密钥、或者用新的 Controller 公钥重新 kubeseal 加密。这凸显了 Sealed Secrets 密钥备份的重要性——一旦丢失，所有已加密的 SealedSecret 都将无法解密。

### 症状 3：启用加密后 Secret 不可读

根因是 EncryptionConfiguration 中 provider 顺序错误（identity 不在末尾）、密钥内容不匹配、或者未执行 Secret 重写操作。处置方法是确保 identity provider 在列表末尾作为兜底、确认密钥的 base64 编码正确、执行全量 Secret 重写。

### 症状 4：Vault 动态密钥过期导致连接断开

根因是 default_ttl 设置过短，或者应用没有集成 Vault Agent 进行自动续租。处置方法是调整 default_ttl 到合理值（如 1h）、在应用中集成 Vault Agent 或 SDK 实现自动续租、或者改用 ESO 的自动同步机制。

### 排查决策树

```
Secret 异常
├── ESO 同步失败?    → 凭证/网络/路径
├── Sealed 解密失败? → 证书/namespace 匹配
├── 加密后不可读?    → 配置顺序/密钥/重写
└── 动态密钥过期?    → TTL/续租
```

---

## 最佳实践

第一，etcd 静态加密是基础中的基础，无论使用哪种外部方案都应先启用。第二，选型要匹配规模：大型企业和合规要求高的场景选 Vault，多云环境且已有外部 KMS 选 ESO，中小型团队和 GitOps 流程选 Sealed Secrets，轻量级文件加密选 SOPS。第三，RBAC 严格限制 get secret 权限，使用 resourceNames 精确到具体 Secret，用 ServiceAccount 隔离不同应用的访问。第四，数据库和云凭证优先使用 Vault 动态密钥，设置短 TTL，从根本上消除长期凭证风险。第五，所有密钥定期轮换，建立自动化轮换流程，见 [[08-安全/01-身份与访问/09-key-rotation-automation.md|密钥轮换自动化]]。第六，开启 Vault 和 ESO 的访问审计日志，记录谁在什么时候访问了哪些密钥。第七，GitOps 流程中绝不在 Git 存储明文 Secret，使用 Sealed Secrets、SOPS 或 ESO 替代。第八，Vault 的 unseal key 和 Sealed Secrets 的主密钥必须安全备份，丢失意味着灾难性后果。

```yaml
# 🟢 低风险：限制 Secret 访问的 RBAC
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: secret-reader
  namespace: production
rules:
- apiGroups: [""]
  resources: ["secrets"]
  resourceNames: ["app-secrets"]    # 仅允许特定 Secret
  verbs: ["get"]
```

---

## Related

- [[08-安全/01-身份与访问/05-vault-enterprise-secrets-management.md|Vault 企业级 Secrets 管理]]
- [[08-安全/01-身份与访问/07-secret-management-tools.md|Secret 管理工具]]
- [[08-安全/01-身份与访问/09-key-rotation-automation.md|密钥轮换自动化]]
- [[08-安全/01-身份与访问/03-service-account-token-management.md|ServiceAccount Token 管理]]
- [[08-安全/05-供应链/07-sigstore-cosign-signing.md|Sigstore Cosign 签名]]
- [[08-安全/01-身份与访问/10-vault-k8s-secrets-guide.md|Vault K8s Secrets 指南]]
