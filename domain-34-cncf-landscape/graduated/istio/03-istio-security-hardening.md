---
title: Istio 安全加固
description: Istio 安全配置指南，涵盖 mTLS、认证授权、证书管理、安全策略和合规配置
category: cncf-landscape
tags:
- k8s
- cncf
- istio
- security
- mtls
- authorization
- certificates
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_reading_time: 10min
intent_queries:
- Istio mTLS 配置
- Istio 授权策略
- Istio 安全加固
trigger_keywords:
- Istio
- 安全
- mTLS
- Authorization
---

# Istio 安全加固

> **适用版本**: Istio 1.20+ | **最后更新**: 2026-05

---

## 1. mTLS 双向认证

### 1.1 PeerAuthentication 配置

```yaml
# 全局 STRICT 模式（推荐生产环境）
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
```

```yaml
# 命名空间级别配置
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: production
spec:
  mtls:
    mode: STRICT
```

```yaml
# Pod 级别配置
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: frontend-mtls
spec:
  selector:
    matchLabels:
      app: frontend
  mtls:
    mode: STRICT
```

### 1.2 mTLS 模式对比

| 模式 | 说明 | 适用场景 |
|:-----|:-----|:---------|
| **STRICT** | 强制 mTLS，不允许明文 | 生产环境 |
| **PERMISSIVE** | 允许 mTLS 和明文 | 迁移期间 |
| **DISABLE** | 禁用 mTLS | 测试环境 |
| **UNSET** | 继承父级配置 | 默认行为 |

### 1.3 mTLS 迁移策略

```yaml
# 阶段 1: PERMISSIVE
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: PERMISSIVE

---
# 阶段 2: 监控日志，确认所有服务使用 mTLS
# istioctl ps

---
# 阶段 3: 切换到 STRICT
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
```

---

## 2. 授权策略 (AuthorizationPolicy)

### 2.1 默认拒绝策略

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  {}
```

### 2.2 命名空间级别授权

```yaml
# 允许同一命名空间内访问
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: allow-namespace
  namespace: production
spec:
  action: ALLOW
  rules:
  - from:
    - source:
        principals: []
        namespaces: ["production"]
    to:
    - operation:
        methods: ["GET"]
```

### 2.3 服务级别授权

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: frontend-ingress
  namespace: production
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/frontend"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/*"]
  - from:
    - source:
        namespaces: ["istio-system"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/health", "/ready"]
```

### 2.4 基于 JWT 的授权

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: require-jwt
  namespace: production
spec:
  selector:
    matchLabels:
      app: api
  action: ALLOW
  rules:
  - from:
    - source:
        requestPrincipals: ["*"]
    when:
    - key: request.auth.claims[iss]
      values: ["https://auth.example.com"]
    - key: request.auth.claims[aud]
      values: ["api-service"]
```

### 2.5 条件授权

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: conditional-access
  namespace: production
spec:
  selector:
    matchLabels:
      app: api
  action: ALLOW
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/gateway"]
    when:
    - key: source.ip
      values: ["10.0.0.0/8"]
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/internal"]
    when:
    - key: connection.src_ip
      notValues: ["10.0.0.1"]
```

---

## 3. 证书管理

### 3.1 证书自动轮换

```yaml
# 检查当前证书过期时间
istioctl pc secret <pod-name> -o json | jq '.[].Secret.days_to_expire'

# 强制轮换证书
istioctl x pane rotate-cert

# 查看 Citadel 证书状态
istioctl cs check
```

### 3.2 自定义 CA

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: istio-system
---
apiVersion: v1
kind: Secret
metadata:
  name: cacerts
  namespace: istio-system
type: Opaque
data:
  ca-cert.pem: <base64-encoded-cert>
  ca-key.pem: <base64-encoded-key>
  cert-chain.pem: <base64-encoded-chain>
  root-cert.pem: <base64-encoded-root>
```

### 3.3 外部 CA (Vault) 集成

```yaml
# 安装 Istio + Vault integration
istioctl install --set values.global.externalCaPem=VaultCA.pem

# 配置 Vault Agent Injector
apiVersion: v1
kind: ConfigMap
metadata:
  name: istio
  namespace: istio-system
data:
  vault_addr: "https://vault.example.com:8200"
  vault_role: "istio-ca"
  vault_auth_method: "kubernetes"
```

### 3.4 证书监控

```yaml
# PrometheusRule for Istio certificate expiration
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: istio-cert-expiry
  namespace: istio-system
spec:
  groups:
    - name: istio.certificate
      rules:
        - alert: IstioCertificateExpiring
          expr: |
            avg(istiod_cert_chain_expire_time_seconds) - time() < 604800
          for: 1m
          labels:
            severity: warning
          annotations:
            summary: "Istio certificate expiring in 7 days"
```