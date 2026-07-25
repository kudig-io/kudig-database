---
title: "密钥轮换自动化"
description: "密钥轮换自动化：证书轮换（cert-manager）、API Key 轮换、etcd 加密密钥轮换、零停机轮换策略"
summary: "面向 SRE 与安全工程师的密钥轮换自动化完整指南，覆盖 TLS 证书自动轮换、API Key 轮换、etcd 加密密钥轮换与零停机轮换策略设计。"
category: 安全
tags:
- key-rotation
- cert-manager
- certificates
- etcd
- automation
- security
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
- "如何用 cert-manager 自动轮换证书"
- "etcd 加密密钥如何轮换"
- "如何实现零停机密钥轮换"
trigger_keywords:
- key rotation
- cert-manager
- certificate renewal
- etcd encryption
- api key rotation
- zero downtime
prerequisites:
- kubectl-basics
- tls-basics
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

# 密钥轮换自动化

> **适用版本**: cert-manager 1.14+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

密钥和证书都是有生命周期的——TLS 证书会过期、API Key 可能泄露、加密密钥需要定期更换以满足合规要求（如 PCI-DSS 要求密钥至少每年轮换一次，某些行业标准要求更频繁）。然而，在我们的生产事故复盘中，"证书过期导致全站 HTTPS 不可用"和"密钥轮换导致服务中断"是出现频率最高的两类事故。这些事故的根因几乎无一例外是：轮换操作依赖人工执行，而人工操作既容易遗忘（直到证书过期才想起来），又容易出错（轮换过程中操作顺序不当导致服务中断）。

密钥轮换自动化的目标是彻底消除人为因素：让证书、密钥、令牌的轮换在到期前自动完成，且全程零停机、无需人工干预。这不仅是一个效率问题，更是一个安全问题——一个长期不轮换的密钥，其泄露风险随时间线性增长；而一个频繁自动轮换的密钥，即使泄露，攻击窗口也极其有限。

本文覆盖三大核心轮换场景：TLS 证书自动轮换（基于 cert-manager）、API Key 和 Secret 轮换（配合 External Secrets Operator 和应用热加载）、etcd 加密密钥轮换（Kubernetes 控制平面级别），并详解零停机轮换的通用策略设计。Secrets 管理方案见 [[08-安全/01-身份与访问/12-secrets-management-comparison.md|Secrets 管理方案对比]]，Vault 实践见 [[08-安全/01-身份与访问/05-vault-enterprise-secrets-management.md|Vault 企业级 Secrets 管理]]。

---

## 核心概念

### 1. 需要轮换的密钥类型

不同类型的密钥有不同的生命周期特征和轮换风险，需要采用不同的自动化策略。

| 类型 | 典型生命周期 | 轮换风险 | 自动化方案 |
|------|------------|---------|-----------|
| TLS 证书 | 90 天（Let's Encrypt）/ 1 年 | 高（HTTPS 中断） | cert-manager |
| API Key | 30-90 天 | 中（认证失败） | Vault/ESO + 应用热加载 |
| etcd 加密密钥 | 90-365 天 | 高（Secret 不可读） | 分阶段重写 |
| ServiceAccount Token | 自动轮换 | 低 | K8s 内置 |
| 数据库密码 | 30-90 天 | 中（连接失败） | Vault 动态密钥 |
| CA 根证书 | 5-10 年 | 极高 | 提前规划、双 CA 过渡 |

TLS 证书轮换是自动化程度最高的场景，cert-manager 可以完全自动地完成签发、续期和部署，几乎不需要人工干预。API Key 轮换的难点在于应用侧的配合——如果应用不支持热加载新密钥，轮换就意味着重启，而重启可能导致短暂的服务中断。etcd 加密密钥轮换是风险最高的操作，因为一旦操作失误，所有 Secret 都将不可读，影响面是整个集群。CA 根证书轮换虽然频率极低（5-10 年一次），但其影响面最大、操作最复杂，需要提前一年以上开始规划。

### 2. 零停机轮换的核心原则

零停机轮换不是简单地"替换密钥"，而是需要精心设计过渡机制，确保在新旧密钥切换的过程中服务不中断。

双密钥并存是最核心的原则：在轮换过程中，新旧密钥应该同时有效一段时间（overlap window），让所有使用旧密钥的组件有足够时间切换到新密钥。如果旧密钥在新密钥生效的瞬间就失效，那么任何还持有旧密钥的组件都会立即失败。

热加载是零停机的技术基础：应用必须能够在不重启的情况下加载新密钥。对于 TLS 证书，这意味着应用需要监听证书文件的变化并重新加载；对于 API Key，这意味着应用需要定期从 Secret 或 Vault 重新读取凭证。如果应用不支持热加载，就需要通过滚动重启来间接实现"零停机"（利用 Kubernetes 的滚动更新机制，逐个替换 Pod）。

渐进切换和可回滚是安全网：先验证新密钥确实可用（如用新证书建立测试连接），确认无误后再废弃旧密钥。整个过程中，如果新密钥出现问题，应该能快速回退到旧密钥，而不是陷入"新旧都不可用"的绝境。

### 3. 轮换策略对比

| 策略 | 说明 | 适用 | 复杂度 |
|------|------|------|--------|
| 硬切换 | 直接替换，旧密钥立即失效 | 内部短连接 | 低 |
| 重叠轮换 | 新旧并存一段时间 | 大多数场景 | 中 |
| 双写双读 | 同时用新旧密钥加解密 | 数据加密密钥 | 高 |
| 动态密钥 | 每次请求生成短时效密钥 | 数据库/云凭证 | 高（需 Vault） |

硬切换只适用于对短暂中断不敏感的场景（如内部批处理任务）。重叠轮换是最通用的策略，适用于绝大多数 TLS 证书和 API Key 的轮换。双写双读用于数据加密密钥的轮换——在过渡期，新数据用新密钥加密，读取时先尝试新密钥解密，失败则用旧密钥解密，直到所有旧数据都被重新加密后才能废弃旧密钥。动态密钥是最彻底的方案，它从根本上消除了长期密钥的概念，每次访问都生成一个短时效的临时凭证。

---

## 生产部署/实现

### 1. cert-manager 证书自动轮换 🟡

cert-manager 是 Kubernetes 生态中证书管理的事实标准，它实现了证书的自动签发、续期和部署。

```yaml
# 🟡 中风险：cert-manager 自动签发与轮换证书
# Issuer（Let's Encrypt）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: sre@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
    - http01:
        ingress:
          class: nginx
---
# Certificate（自动轮换，到期前 30 天续期）
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: app-tls
  namespace: production
spec:
  secretName: app-tls-secret
  duration: 2160h          # 90 天
  renewBefore: 720h        # 到期前 30 天自动续期
  subject:
    organizations: ["example"]
  commonName: app.example.com
  dnsNames:
  - app.example.com
  - www.app.example.com
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  usages:
  - server auth
  - digital signature
  - key encipherment
```

这个配置的核心是 duration 和 renewBefore 的配合。duration 2160h（90 天）定义了证书的有效期，renewBefore 720h（30 天）定义了续期窗口——cert-manager 会在证书到期前 30 天自动发起续期请求。这意味着证书的实际使用周期是 60 天（90 天减去 30 天的续期窗口），留出了充足的续期缓冲时间。即使某次续期因为网络问题失败，cert-manager 会在接下来的 30 天内持续重试，大大降低了证书过期的风险。

内部服务 mTLS 证书（自签 CA）：

```yaml
# 🟡 中风险：内部 CA 自动轮换
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: internal-ca
  namespace: cert-manager
spec:
  secretName: internal-ca-secret
  duration: 87600h        # 10 年
  renewBefore: 8760h      # 1 年前续期
  isCA: true
  commonName: internal-ca
  issuerRef:
    name: selfsigned
    kind: ClusterIssuer
---
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: internal-ca-issuer
  namespace: production
spec:
  ca:
    secretName: internal-ca-secret
```

内部 CA 的设计遵循"长根短叶"原则：CA 证书的有效期设为 10 年（减少 CA 轮换的频率和风险），而由该 CA 签发的服务端证书可以设置较短的有效期（如 90 天）并频繁轮换。这种分层设计将高风险的 CA 轮换频率降到最低，同时通过短周期的叶子证书保证安全性。

### 2. etcd 加密密钥轮换 🔴

etcd 加密密钥轮换是 Kubernetes 控制平面级别的高风险操作，需要严格按照分阶段流程执行。

```bash
# 🔴 高风险：etcd 加密密钥轮换，操作错误将导致所有 Secret 不可读
# 步骤 1：生成新密钥
NEW_KEY=$(head -c 32 /dev/urandom | base64)
echo "新密钥: $NEW_KEY"

# 步骤 2：更新 EncryptionConfiguration，新密钥置首，保留旧密钥
cat > encryption-config.yaml <<EOF
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources: ["secrets"]
  providers:
  - aescbc:
      keys:
      - name: key2-new
        secret: $NEW_KEY
      - name: key1-old
        secret: <旧密钥base64>
  - identity: {}
EOF

# 步骤 3：滚动重启 apiserver（多 master 逐个重启）
# 步骤 4：重写所有 Secret 以用新密钥加密
kubectl get secrets --all-namespaces -o json | kubectl replace -f -

# 步骤 5：验证所有 Secret 可读
kubectl get secrets -A -o name | while read s; do
  kubectl get $s -o jsonpath='{.metadata.name}' >/dev/null 2>&1 || echo "FAILED: $s"
done

# 步骤 6：确认无误后，移除旧密钥（再重启 apiserver）
```

这个流程的关键在于"新密钥置首、旧密钥保留"的设计。apiserver 用第一个密钥加密新写入的数据，但解密时会按顺序尝试所有密钥。因此，在步骤 2 中，新写入的 Secret 会用 key2-new 加密，而旧的 Secret 仍然可以用 key1-old 解密。步骤 4 的重写操作将所有 Secret 读出（用旧密钥解密）再写回（用新密钥加密），完成全量迁移。只有在步骤 5 验证所有 Secret 都可读之后，才能在步骤 6 中安全地移除旧密钥。跳过任何一步或颠倒顺序都可能导致灾难性后果。

### 3. API Key / Secret 轮换（配合 ESO + 应用热加载） 🟡

API Key 轮换的挑战在于应用侧的配合——如何让应用在不重启的情况下使用新密钥。

```yaml
# 🟡 中风险：ESO 自动同步轮换后的密钥
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: api-credentials
  namespace: production
spec:
  refreshInterval: "15m"      # 高频检查，及时同步轮换
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: api-credentials
    creationPolicy: Owner
    template:
      metadata:
        annotations:
          # 触发依赖 Pod 滚动（配合 reloader）
          reloader.stakater.com/match: "true"
  data:
  - secretKey: api-key
    remoteRef:
      key: production/api
      property: key
---
# 使用 Stakater Reloader 在 Secret 变更时自动滚动 Pod
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-consumer
  namespace: production
  annotations:
    secret.reloader.stakater.com/reload: "api-credentials"
spec:
  template:
    spec:
      containers:
      - name: app
        image: registry.example.com/app:v1.0
        env:
        - name: API_KEY
          valueFrom:
            secretKeyRef:
              name: api-credentials
              key: api-key
```

这个方案的工作原理是：当 Vault 中的 API Key 被轮换后，ESO 在下一个 refreshInterval（15 分钟）内检测到变化并更新 Kubernetes Secret；Stakater Reloader 监控到 Secret 变化后，自动触发依赖该 Secret 的 Deployment 进行滚动更新。滚动更新过程中，旧 Pod 继续服务直到新 Pod 就绪，因此实现了"准零停机"的密钥轮换。对于支持热加载的应用（如通过文件系统监听 Secret 变化），可以完全避免重启，实现真正的零停机。

---

## 运维操作

### 1. 证书状态检查 🟢

```bash
# 🟢 低风险：只读
kubectl get certificates -A
kubectl get certificaterequests -A
kubectl describe certificate app-tls -n production

# 检查证书到期时间
kubectl get secret app-tls-secret -n production -o jsonpath='{.data.tls\.crt}' | \
  base64 -d | openssl x509 -noout -dates

# 强制提前续期（测试用）
kubectl cert-manager renew app-tls -n production
```

定期检查证书状态是防止证书过期事故的基本运维动作。kubectl get certificates 会显示每个证书的 Ready 状态和剩余有效期。对于即将到期的证书，应该检查是否有对应的 CertificateRequest 正在处理中，以及是否有续期失败的错误信息。

### 2. 轮换演练 🟡

```bash
# 🟡 中风险：在非生产验证轮换流程
# 1. 缩短证书周期测试自动续期
# 2. 验证应用热加载新证书
kubectl -n staging rollout status deploy/app
# 3. 验证旧连接平滑过渡
```

轮换演练应该在非生产环境定期进行。演练内容包括：缩短证书有效期验证自动续期是否正常工作、验证应用在 Secret 变更后是否能正确加载新密钥、验证在轮换过程中已建立的连接是否能平滑过渡。

### 3. 监控证书到期 🟢

```bash
# 🟢 低风险
# cert-manager 暴露指标
kubectl -n cert-manager port-forward svc/cert-manager 9402:9402
curl -s http://localhost:9402/metrics | grep certmanager_certificate_expiration_timestamp_seconds
```

---

## 故障排查

### 症状 1：证书未自动续期

```bash
# 🟢 低风险
kubectl describe certificaterequest -n production | tail -20
kubectl -n cert-manager logs deploy/cert-manager
```

根因可能是 ACME challenge 验证失败（HTTP01 路径不可达或 DNS01 记录未正确配置）、Issuer 配置错误（如 ACME server URL 错误）、或者 renewBefore 设置不当导致续期窗口太短。处置方法是检查 challenge 的可达性（如确认 Ingress 正确路由了 /.well-known/acme-challenge/ 路径）、核对 Issuer 配置、调整 renewBefore 到合理值。

### 症状 2：etcd 密钥轮换后 Secret 不可读

这是最严重的故障场景。根因通常是旧密钥被过早移除（在 Secret 重写完成之前就删除了旧密钥）、apiserver 未全部重启（部分 apiserver 仍使用旧配置）、或者密钥的 base64 编码有误。处置方法是立即恢复旧密钥到 EncryptionConfiguration、重启所有 apiserver、重新执行 Secret 重写和验证流程。

### 症状 3：API Key 轮换后应用认证失败

根因是应用不支持热加载，仍在使用旧 Key；或者 ESO 的 refreshInterval 过长，尚未同步新密钥。处置方法是使用 Stakater Reloader 在 Secret 变更时自动滚动 Pod、或者改造应用实现密钥热加载（如定期重新读取 Secret 文件）、缩短 ESO 的 refreshInterval。

### 症状 4：证书续期导致短暂中断

根因是应用不支持证书热加载，在证书文件更新后仍使用内存中的旧证书；或者 Ingress Controller 未自动重新加载证书。处置方法是使用支持证书热加载的 Ingress Controller（如 nginx-ingress 会自动监控 Secret 变化并重新加载）、或者在应用层面实现证书文件监听和热加载。

### 排查决策树

```
轮换异常
├── 证书未续期?   → challenge/Issuer/renewBefore
├── Secret 不可读? → 旧密钥/apiserver 重启
├── 认证失败?     → 热加载/ESO 同步
└── 续期中断?     → 热加载/Ingress 更新
```

---

## 最佳实践

第一，证书管理全面交给 cert-manager，设置 renewBefore 为 duration 的三分之一，并监控证书到期指标。第二，采用短生命周期策略，证书 90 天、API Key 30-90 天，缩短泄露窗口。第三，零停机轮换依赖重叠窗口加热加载（Stakater Reloader 或应用内文件监听），避免硬切换。第四，etcd 密钥轮换严格遵循分阶段流程，保留旧密钥直到所有 Secret 重写并验证完成。第五，数据库和云凭证优先使用 Vault 动态密钥，从根本上消除长期静态密钥。第六，定期在非生产环境演练轮换流程，验证零停机能力。第七，建立证书到期告警（14 天内到期告警）和轮换失败告警。第八，CA 规划遵循"长根短叶"原则，根 CA 轮换提前一年以上规划，采用双 CA 过渡策略。

```yaml
# 🟢 低风险：证书到期告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cert-alerts
spec:
  groups:
  - name: certificates
    rules:
    - alert: CertificateExpiringSoon
      expr: certmanager_certificate_expiration_timestamp_seconds - time() < 14*24*3600
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "证书 {{ $labels.name }} 将在 14 天内到期"
    - alert: CertificateRenewalFailed
      expr: certmanager_certificate_ready_status{condition="True"} == 0
      for: 30m
      labels:
        severity: critical
```

---

## Related

- [[08-安全/01-身份与访问/12-secrets-management-comparison.md|Secrets 管理方案对比]]
- [[08-安全/01-身份与访问/05-vault-enterprise-secrets-management.md|Vault 企业级 Secrets 管理]]
- [[08-安全/01-身份与访问/03-service-account-token-management.md|ServiceAccount Token 管理]]
- [[08-安全/01-身份与访问/99-vault-k8s-secrets-guide.md|Vault K8s Secrets 指南]]
- [[05-网络/01-K8s网络核心/22-ingress-tls-certificate.md|Ingress TLS 证书]]
- [[05-网络/01-K8s网络核心/18-network-encryption-mtls.md|网络加密与 mTLS]]
