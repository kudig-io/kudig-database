---
title: Service Account Token 管理
description: 'SA Token 管理：Bound SA Token (TokenRequest API)、短期 Token 自动轮转、Projected ServiceAccountVolume、与 Vault 集成的外部 Token'
summary: 'Bound SA Token、短期 Token 轮转、Projected Volume 与 Vault 集成'
category: security-compliance
tags:
- service-account
- token
- authentication
- vault
- bound-token
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Service Account Token 管理是什么
- 如何配置 SA Token 自动轮转
trigger_keywords:
- Service Account Token
- TokenRequest
- Bound Token
- Projected Volume
- Vault
prerequisites:
- kubectl-basics
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


# Service Account Token 管理

## 概述

传统 Kubernetes Service Account Token 是长期有效的 Secret，一旦泄露风险极大。现代 Kubernetes 提供了 Bound SA Token（TokenRequest API）、Projected ServiceAccountVolume 和自动轮转机制，大幅提升 Token 安全性。

## 1. 传统 SA Token 的问题

```yaml
# 传统方式：长期 Token 存储在 Secret 中
# 问题：
# 1. Token 永不过期（除非手动删除）
# 2. Token 可以跨命名空间使用
# 3. Token 不绑定到特定 Pod
# 4. 泄露后难以检测和撤销

apiVersion: v1
kind: Secret
metadata:
  name: default-token-xxx
  namespace: default
  annotations:
    kubernetes.io/service-account.name: default
type: kubernetes.io/service-account-token
```

## 2. Bound SA Token（TokenRequest API）

### 2.1 核心概念

Bound SA Token 通过 TokenRequest API 创建，具有以下安全特性：
- 限定 Audience（受众）
- 限定有效期
- 绑定到特定 Pod（通过 ServiceAccountTokenVolumeProjection）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动创建 Bound Token
kubectl create token my-service-account \
  --audience=https://kubernetes.default.svc \
  --duration=1h \
  -n my-namespace

# 输出：eyJhbGciOiJSUzI1NiIsImtpZCI6IiJ9...
```
### 2.2 TokenRequest API 配置

```yaml
# 使用 kubectl 手动调用 TokenRequest API
apiVersion: authentication.k8s.io/v1
kind: TokenRequest
metadata:
  name: my-token
spec:
  audiences:
  - https://kubernetes.default.svc
  - https://my-api.example.com
  expirationSeconds: 3600  # 1 小时
  boundObjectRef:
    apiVersion: v1
    kind: Pod
    name: my-pod
    uid: <pod-uid>
```

### 2.3 自动 Bound Token（1.24+）

Kubernetes 1.24+ 默认不再自动创建长期 Token Secret：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查是否有自动创建的 Token
kubectl get secrets -n <namespace> | grep service-account-token

# 手动创建长期 Token（仅用于特殊场景）
kubectl apply -f - <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: my-sa-token
  namespace: default
  annotations:
    kubernetes.io/service-account.name: my-service-account
type: kubernetes.io/service-account-token
EOF
```
## 3. Projected ServiceAccountVolume

### 3.1 基本配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-app
  namespace: production
spec:
  serviceAccountName: my-app-sa
  containers:
  - name: app
    image: my-app:latest
    volumeMounts:
    - name: sa-token
      mountPath: /var/run/secrets/tokens
      readOnly: true
  volumes:
  - name: sa-token
    projected:
      sources:
      - serviceAccountToken:
          path: sa-token
          expirationSeconds: 3600
          audience: https://kubernetes.default.svc
```

### 3.2 多 Audience Token

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-audience-app
spec:
  serviceAccountName: my-app-sa
  containers:
  - name: app
    volumeMounts:
    - name: k8s-token
      mountPath: /var/run/secrets/kubernetes
    - name: api-token
      mountPath: /var/run/secrets/api
  volumes:
  - name: k8s-token
    projected:
      sources:
      - serviceAccountToken:
          path: token
          expirationSeconds: 7200
          audience: https://kubernetes.default.svc
  - name: api-token
    projected:
      sources:
      - serviceAccountToken:
          path: token
          expirationSeconds: 3600
          audience: https://api.example.com
```

### 3.3 Token 自动刷新

kubelet 会在 Token 过期前自动刷新（默认在过期前 10%~20% 的时间窗口内）：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Token 刷新配置
kubectl get cm -n kube-system kubelet-config -o yaml | grep -i token

# kubelet 配置参数
# serviceAccountTokenAutoRotate: true（默认）
# serviceAccountTokenGracePeriod: 默认 10% 的 expirationSeconds
```
## 4. 短期 Token 自动轮转

### 4.1 kubelet Token 轮转

```yaml
# kubelet 配置（/var/lib/kubelet/config.yaml）
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
# Token 自动轮转
rotateCertificates: true
serverTLSBootstrap: true
# SA Token 轮转
serviceAccountTokenAutoRotate: true
serviceAccountTokenGracePeriod: 2m0s
```

### 4.2 Service Account Token Projection 控制器

```yaml
# 使用 MutatingWebhook 自动注入 Token Projection
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: token-projection
webhooks:
- name: token-projection.k8s.io
  admissionReviewVersions: ["v1"]
  sideEffects: None
  timeoutSeconds: 5
  clientConfig:
    service:
      name: token-projection-webhook
      namespace: kube-system
      path: /mutate
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE"]
    resources: ["pods"]
  namespaceSelector:
    matchExpressions:
    - key: kubernetes.io/metadata.name
      operator: NotIn
      values: ["kube-system", "kube-public"]
```

### 4.3 Token 有效期管理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前 Token 有效期
kubectl get serviceaccount -n <namespace> <sa-name> -o yaml

# 检查 Pod 挂载的 Token 有效期
kubectl exec <pod> -- cat /var/run/secrets/tokens/sa-token | \
  cut -d. -f2 | base64 -d 2>/dev/null | jq -r '.exp'

# 计算 Token 剩余时间
EXPIRE=$(kubectl exec <pod> -- cat /var/run/secrets/tokens/sa-token | \
  cut -d. -f2 | base64 -d 2>/dev/null | jq -r '.exp')
NOW=$(date +%s)
echo "Token expires in: $(( EXPIRE - NOW )) seconds"
```
## 5. Vault 集成外部 Token

### 5.1 Vault Kubernetes Auth

```yaml
# Vault Kubernetes Auth 配置
# Step 1: 启用 Kubernetes Auth 方法
# vault auth enable kubernetes
# vault write auth/kubernetes/config \
#   kubernetes_host=https://kubernetes.default.svc

# Step 2: 创建 Vault Policy
# vault policy write my-app-policy - <<EOF
# path "secret/data/my-app/*" {
#   capabilities = ["read"]
# }
# EOF

# Step 3: 创建 Kubernetes Auth Role
# vault write auth/kubernetes/role/my-app \
#   bound_service_account_names=my-app-sa \
#   bound_service_account_namespaces=production \
#   policies=my-app-policy \
#   ttl=1h
```

### 5.2 Vault Agent Sidecar

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
spec:
  replicas: 1
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "my-app"
        vault.hashicorp.com/agent-inject-secret-config: "secret/data/my-app/config"
        vault.hashicorp.com/agent-inject-template-config: |
          {{- with secret "secret/data/my-app/config" -}}
          API_KEY={{ .Data.data.api_key }}
          DB_PASSWORD={{ .Data.data.db_password }}
          {{- end }}
    spec:
      serviceAccountName: my-app-sa
      containers:
      - name: app
        image: my-app:latest
        env:
        - name: CONFIG_PATH
          value: "/vault/secrets/config"
```

### 5.3 CSI Secret Store 与 Vault

```yaml
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-database
  namespace: production
spec:
  provider: vault
  parameters:
    roleName: "my-app"
    vaultAddress: "https://vault.example.com:8200"
    vaultCACertSecret: "vault-tls"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/my-app/database"
        secretKey: "password"
      - objectName: "api-key"
        secretPath: "secret/data/my-app/api"
        secretKey: "key"
  secretObjects:
  - secretName: app-secrets
    type: Opaque
    data:
    - objectName: db-password
      key: DB_PASSWORD
    - objectName: api-key
      key: API_KEY
---
apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  serviceAccountName: my-app-sa
  containers:
  - name: app
    image: my-app:latest
    volumeMounts:
    - name: secrets-store
      mountPath: "/mnt/secrets"
      readOnly: true
    env:
    - name: DB_PASSWORD
      valueFrom:
        secretKeyRef:
          name: app-secrets
          key: DB_PASSWORD
  volumes:
  - name: secrets-store
    csi:
      driver: secrets-store.csi.k8s.io
      readOnly: true
      volumeAttributes:
        secretProviderClass: vault-database
```

## 6. 最佳实践

### 6.1 Token 安全策略

```
SA Token 安全检查清单：

□ 禁用自动创建长期 Token（1.24+ 默认行为）
□ 使用 Projected ServiceAccountVolume 挂载 Token
□ 设置合理的 Token 有效期（建议 1h）
□ 使用 Audience 限定 Token 使用范围
□ 启用 kubelet Token 自动轮转
□ 定期审计 Service Account 权限
□ 使用最小权限原则配置 RBAC
□ 外部服务使用 Vault 集成
□ 监控 Token 使用异常
```

### 6.2 监控 Token 异常

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sa-token-monitoring
spec:
  groups:
  - name: sa-token.rules
    rules:
    - alert: SATokenExpired
      expr: |
        kubelet_expired_token_total > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "SA token expiration detected"

    - alert: SATokenRequestHighRate
      expr: |
        sum(rate(rest_client_requests_total{
          code="401",
          verb="POST",
          url=~".*/serviceaccounts/.*/token"
        }[5m])) > 0.1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "High rate of failed SA token requests"
```

## Related

- [[domain-05-security-compliance/01-identity-access/07-rbac-matrix-configuration|RBAC 最佳实践]]
- [[domain-05-security-compliance/01-identity-access/04-oidc-identity-provider-integration|OIDC 身份集成]]

## See Also

- [TokenRequest API 文档](https://kubernetes.io/docs/reference/kubernetes-api/authentication-resources/token-request-v1/)
- [Vault Kubernetes Auth](https://developer.hashicorp.com/vault/docs/auth/kubernetes)


<!-- risk-assessed -->
