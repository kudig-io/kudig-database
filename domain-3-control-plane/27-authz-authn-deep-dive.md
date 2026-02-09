# Kubernetes 认证授权深度解析 (Authentication & Authorization Deep Dive)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 安全认证文档

---

## 目录

1. [认证机制详解](#1-认证机制详解)
2. [授权机制详解](#2-授权机制详解)
3. [准入控制机制](#3-准入控制机制)
4. [RBAC最佳实践](#4-rbac最佳实践)
5. [安全配置模板](#5-安全配置模板)
6. [故障排查指南](#6-故障排查指南)
7. [企业级安全方案](#7-企业级安全方案)

---

## 1. 认证机制详解

### 1.1 认证概述

Kubernetes认证机制负责验证请求者的身份，确定"你是谁"。

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                             Authentication Flow                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Client Request                                                                  │
│        ↓                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    Authentication Methods                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Client Certs│ │ Bearer Token│ │ Auth Proxy  │ │ Webhook Token       ││    │
│  │  │ (X509)      │ │ (ServiceAcc)│ │             │ │ Review              ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│        ↓                                                                         │
│  Identity Mapping                                                                │
│        ↓                                                                         │
│  Authenticated User                                                              │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 客户端证书认证 (X509 Client Certs)

#### 配置示例

```yaml
# kube-apiserver启动参数
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - command:
    - kube-apiserver
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    - --tls-cipher-suites=TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
```

#### 证书生成脚本

```bash
#!/bin/bash
# generate-client-cert.sh

CA_CERT="/etc/kubernetes/pki/ca.crt"
CA_KEY="/etc/kubernetes/pki/ca.key"
USER_NAME="$1"
GROUP_NAME="$2"

# 生成用户私钥
openssl genrsa -out ${USER_NAME}.key 2048

# 生成CSR
openssl req -new -key ${USER_NAME}.key -out ${USER_NAME}.csr -subj "/CN=${USER_NAME}/O=${GROUP_NAME}"

# 使用CA签名
openssl x509 -req -in ${USER_NAME}.csr -CA ${CA_CERT} -CAkey ${CA_KEY} -CAcreateserial -out ${USER_NAME}.crt -days 365
```

### 1.3 Bearer Token认证

#### ServiceAccount Tokens

```yaml
# 创建ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: monitoring-sa
  namespace: monitoring
automountServiceAccountToken: true

---
# 为ServiceAccount绑定权限
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: monitoring-rb
  namespace: monitoring
subjects:
- kind: ServiceAccount
  name: monitoring-sa
  namespace: monitoring
roleRef:
  kind: Role
  name: monitoring-role
  apiGroup: rbac.authorization.k8s.io
```

#### 静态Token文件

```bash
# /etc/kubernetes/tokens.csv
monitoring-token,monitoring-user,uid-123,"system:masters"
admin-token,admin-user,uid-456,"system:masters"
```

### 1.4 认证代理 (Auth Proxy)

```yaml
# kube-apiserver配置
- --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
- --requestheader-allowed-names=front-proxy-client
- --requestheader-extra-headers-prefix=X-Remote-Extra-
- --requestheader-group-headers=X-Remote-Group
- --requestheader-username-headers=X-Remote-User
```

### 1.5 Webhook Token认证

```yaml
# webhook-config.yaml
apiVersion: v1
kind: Config
clusters:
- name: token-reviewer
  cluster:
    certificate-authority: /path/to/ca.pem
    server: https://auth.example.com/token
users:
- name: apiserver
  user:
    client-certificate: /path/to/cert.pem
    client-key: /path/to/key.pem
contexts:
- name: webhook
  context:
    cluster: token-reviewer
    user: apiserver
current-context: webhook
```

## 2. 授权机制详解

### 2.1 授权概述

授权机制决定认证后的用户可以做什么，基于ABAC、RBAC、Webhook等方式。

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                             Authorization Flow                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Authenticated User                                                              │
│        ↓                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                     Authorization Modes                                 │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ RBAC        │ │ ABAC        │ │ Webhook     │ │ Node                ││    │
│  │  │ (推荐)      │ │ (已弃用)    │ │             │ │ (节点授权)          ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│        ↓                                                                         │
│  Access Decision                                                                 │
│        ↓                                                                         │
│  Allow/Deny                                                                      │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 RBAC核心概念

#### 基本对象关系

```
User/ServiceAccount ──RoleBinding/ClusterRoleBinding──→ Role/ClusterRole
      ↑                                                    ↓
   Subject                                            Permissions
```

#### RBAC对象详解

```yaml
# Role - 命名空间级别权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: default
  name: pod-reader
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "watch", "list"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list"]

---
# ClusterRole - 集群级别权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: secret-reader
rules:
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list"]
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]

---
# RoleBinding - 绑定到命名空间
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: default
subjects:
- kind: User
  name: jane
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io

---
# ClusterRoleBinding - 绑定到集群
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: read-secrets-global
subjects:
- kind: Group
  name: manager
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: secret-reader
  apiGroup: rbac.authorization.k8s.io
```

### 2.3 权限动词详解

```bash
# 核心权限动词
GET     - 获取资源详情
LIST    - 列出资源集合
CREATE  - 创建新资源
UPDATE  - 更新现有资源
PATCH   - 部分更新资源
DELETE  - 删除资源
DELETECOLLECTION - 批量删除
WATCH   - 监听资源变化
PROXY   - 代理请求
```

### 2.4 聚合角色

```yaml
# 系统聚合角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: aggregate-view
  labels:
    rbac.authorization.k8s.io/aggregate-to-view: "true"
rules:
- apiGroups: [""]
  resources: ["configmaps", "endpoints", "persistentvolumeclaims"]
  verbs: ["get", "list", "watch"]
```

## 3. 准入控制机制

### 3.1 准入控制概述

准入控制器在对象持久化之前拦截API请求，用于验证和修改对象。

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Admission Control Flow                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  API Request                                                                     │
│        ↓                                                                         │
│  Authentication & Authorization                                                  │
│        ↓                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                   Admission Controllers                                 │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Validating  │ │ Mutating    │ │ Built-in    │ │ Custom              ││    │
│  │  │ Webhooks    │ │ Webhooks    │ │ Controllers │ │ Controllers         ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│        ↓                                                                         │
│  Object Persistence                                                              │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 内置准入控制器

#### 常用内置控制器

```bash
# 推荐启用的准入控制器
--enable-admission-plugins= \
NamespaceLifecycle,\
LimitRanger,\
ServiceAccount,\
DefaultStorageClass,\
DefaultTolerationSeconds,\
MutatingAdmissionWebhook,\
ValidatingAdmissionWebhook,\
ResourceQuota,\
PodSecurityPolicy,\
NodeRestriction
```

#### 关键控制器说明

```yaml
# NamespaceLifecycle - 防止在终止的namespace中创建资源
# LimitRanger - 强制执行资源限制
# ServiceAccount - 自动挂载service account token
# DefaultStorageClass - 设置默认存储类
# ResourceQuota - 实施资源配额
# PodSecurityPolicy - Pod安全策略(已弃用，请使用 Pod Security Admission)
# NodeRestriction - 限制节点自我修改权限
```

### 3.3 Pod Security Admission (PSA)

作为 PSP 的替代方案，PSA 通过 Label 在命名空间级别强制执行安全标准。

#### 3.3.1 安全标准级别
- **Privileged**: 无限制，适用于系统级组件。
- **Baseline**: 最小限制，防止已知的特权提升。
- **Restricted**: 严格限制，遵循 Pod 安全最佳实践。

#### 3.3.2 配置示例
```yaml
# 在 Namespace 上启用 PSA
apiVersion: v1
kind: Namespace
metadata:
  name: restricted-ns
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.30
    pod-security.kubernetes.io/warn: baseline
```

### 3.4 ValidatingAdmissionPolicy (CEL)

Kubernetes v1.30+ 推荐使用的声明式准入控制，无需开发 Webhook。

```yaml
# CEL 策略示例：限制副本数
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: check-replicas
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: ["apps"]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["deployments"]
  validations:
    - expression: "object.spec.replicas <= 10"
      message: "Replicas must be less than or equal to 10"
---
# 绑定策略到 Namespace
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: check-replicas-binding
spec:
  policyName: check-replicas
  validationActions: [Deny]
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: production
```

### 3.5 Validating Webhook

```yaml
# validating-webhook-config.yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: validation-webhook-example
webhooks:
- name: validation.example.com
  clientConfig:
    service:
      namespace: webhook-system
      name: webhook-service
      path: "/validate"
    caBundle: <CA_BUNDLE>
  rules:
  - operations: ["CREATE", "UPDATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
  admissionReviewVersions: ["v1", "v1beta1"]
  sideEffects: None
  timeoutSeconds: 5
```

### 3.4 Mutating Webhook

```yaml
# mutating-webhook-config.yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: mutation-webhook-example
webhooks:
- name: mutation.example.com
  clientConfig:
    service:
      namespace: webhook-system
      name: webhook-service
      path: "/mutate"
    caBundle: <CA_BUNDLE>
  rules:
  - operations: ["CREATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
  admissionReviewVersions: ["v1", "v1beta1"]
  sideEffects: None
```

## 4. RBAC最佳实践

### 4.1 权限最小化原则

```yaml
# 避免过度授权的反面示例
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: bad-example
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]  # 危险！授予所有权限

---
# 推荐的最小权限示例
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: deployment-operator
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
```

### 4.2 分层权限管理

```yaml
# 管理员权限 - 严格控制
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-admin-binding
subjects:
- kind: User
  name: admin@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io

---
# 开发者权限 - 限制命名空间
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-access
  namespace: development
subjects:
- kind: Group
  name: developers
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: namespace-developer
  apiGroup: rbac.authorization.k8s.io
```

### 4.4 权限风险警示

在配置 RBAC 时，需特别警惕以下具有提权风险的权限：
- **escalate**: 允许用户创建/更新具有比自己更高权限的角色。
- **bind**: 允许用户将角色绑定到主体，可能导致越权。
- **impersonate**: 允许模拟其他用户。
- **nodes/proxy**: 允许访问节点 API，可能导致逃逸。

## 5. 安全配置模板

### 5.1 生产环境API Server配置

```yaml
# kube-apiserver安全配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - command:
    - kube-apiserver
    # 认证配置
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    - --tls-cipher-suites=TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
    
    # 授权配置
    - --authorization-mode=Node,RBAC
    - --anonymous-auth=false
    
    # 准入控制
    - --enable-admission-plugins=NamespaceLifecycle,LimitRanger,ServiceAccount,DefaultStorageClass,DefaultTolerationSeconds,MutatingAdmissionWebhook,ValidatingAdmissionWebhook,ResourceQuota,NodeRestriction
    
    # 安全增强
    - --audit-log-path=/var/log/kubernetes/audit.log
    - --audit-policy-file=/etc/kubernetes/audit-policy.yaml
    - --audit-log-maxage=30
    - --audit-log-maxbackup=10
    - --audit-log-maxsize=100
    
    # 网络安全
    - --secure-port=6443
    - --bind-address=0.0.0.0
    - --insecure-port=0
```

### 5.2 RBAC安全模板

```yaml
# 紧急响应团队权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: emergency-response
rules:
- apiGroups: [""]
  resources: ["pods", "services", "deployments"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/exec", "pods/portforward"]
  verbs: ["create"]
- apiGroups: ["events.k8s.io"]
  resources: ["events"]
  verbs: ["get", "list", "watch"]

---
# 只读审计权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: read-only-auditor
rules:
- apiGroups: [""]
  resources: ["pods", "services", "nodes", "namespaces"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["get", "list", "watch"]
```

## 6. 故障排查指南

### 6.1 常见认证问题

```bash
# 1. 证书过期检查
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -text -noout | grep -E "(Not Before|Not After)"

# 2. 证书链验证
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 3. ServiceAccount Token验证
kubectl get secret $(kubectl get sa default -o jsonpath='{.secrets[0].name}') -o jsonpath='{.data.token}' | base64 -d
```

### 6.2 RBAC权限调试

```bash
# 1. 检查用户权限
kubectl auth can-i get pods --as=system:serviceaccount:default:default

# 2. 查看RoleBinding详情
kubectl get rolebinding -A -o wide

# 3. 查看ClusterRoleBinding详情
kubectl get clusterrolebinding -o wide

# 4. 调试RBAC规则
kubectl auth reconcile -f role.yaml --remove-extra-permissions --confirm
```

### 6.3 准入控制问题

```bash
# 1. 检查准入控制器状态
kubectl get mutatingwebhookconfigurations
kubectl get validatingwebhookconfigurations

# 2. 查看Webhook调用日志
kubectl logs -n webhook-system deployment/webhook-deployment

# 3. 临时禁用Webhook进行调试
kubectl delete mutatingwebhookconfiguration <webhook-name>
```

## 7. 企业级安全方案

### 7.1 多租户安全隔离

```yaml
# 多租户命名空间隔离
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a-production
  labels:
    tenant: tenant-a
    environment: production

---
# 租户专用网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a-production
spec:
  podSelector: {}
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
```

### 7.2 零信任安全模型

```yaml
# 零信任访问控制
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: zero-trust-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: critical-app
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/app-service-account"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/*"]
    when:
    - key: request.auth.claims[groups]
      values: ["production-users"]
```

### 7.3 安全合规检查清单

```bash
#!/bin/bash
# security-audit-checklist.sh

echo "=== Kubernetes安全审计检查清单 ==="

# 1. 认证安全检查
echo "1. 认证配置检查:"
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -E "(client-ca-file|anonymous-auth)"

# 2. 授权模式检查
echo "2. 授权模式检查:"
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep "authorization-mode"

# 3. 准入控制器检查
echo "3. 准入控制器检查:"
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep "enable-admission-plugins"

# 4. RBAC配置检查
echo "4. RBAC配置检查:"
kubectl get clusterroles --no-headers | wc -l
kubectl get clusterrolebindings --no-headers | wc -l

# 5. ServiceAccount检查
echo "5. ServiceAccount配置检查:"
kubectl get serviceaccounts --all-namespaces | grep -E "(default|system)"
```

---
**文档维护**: Kusheet Security Team | **最后审查**: 2026-02 | **安全等级**: ★★★★★