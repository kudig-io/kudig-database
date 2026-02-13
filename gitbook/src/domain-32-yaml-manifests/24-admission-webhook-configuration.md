# 24 - Admission Webhook 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

## 目录

- [概述](#概述)
- [核心概念](#核心概念)
- [ValidatingWebhookConfiguration](#validatingwebhookconfiguration)
- [MutatingWebhookConfiguration](#mutatingwebhookconfiguration)
- [完整字段说明](#完整字段说明)
- [内部原理](#内部原理)
- [版本兼容性](#版本兼容性)
- [最佳实践](#最佳实践)
- [生产案例](#生产案例)
- [FAQ](#faq)

---

## 概述

### 什么是 Admission Webhook?

Admission Webhook 是 Kubernetes 准入控制系统的扩展机制,允许在对象持久化到 etcd 之前拦截 API 请求并进行验证或修改。

### Webhook 类型

| 类型 | 资源 | 作用 | 执行顺序 |
|------|------|------|---------|
| **Mutating** | MutatingWebhookConfiguration | 修改对象 | 先执行 |
| **Validating** | ValidatingWebhookConfiguration | 验证对象 | 后执行 |

### 调用链路

```
API 请求 → API Server
  ↓
认证 (Authentication)
  ↓
鉴权 (Authorization)
  ↓
Mutating Admission (内置 + Webhook)
  ↓
Object Schema 验证
  ↓
Validating Admission (内置 + Webhook)
  ↓
持久化到 etcd
```

---

## 核心概念

### 1. Webhook 注册

通过 `ValidatingWebhookConfiguration` 或 `MutatingWebhookConfiguration` 向 API Server 注册 Webhook 服务。

### 2. 匹配规则

- **Rules**: 匹配哪些资源和操作
- **NamespaceSelector**: 匹配哪些命名空间的对象
- **ObjectSelector**: 匹配哪些标签的对象
- **MatchConditions (v1.27+)**: 使用 CEL 表达式精确匹配

### 3. 失败策略

- **Fail**: Webhook 失败时拒绝请求(安全优先)
- **Ignore**: Webhook 失败时忽略并继续(可用性优先)

### 4. 副作用声明

- **None**: 无副作用
- **NoneOnDryRun**: DryRun 时无副作用
- **Some**: 有副作用(已弃用)
- **Unknown**: 未知副作用(已弃用)

---

## ValidatingWebhookConfiguration

### 基础示例

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: example-validating-webhook
  annotations:
    # 描述说明
    description: "验证 Pod 安全配置"
webhooks:
  - name: validate.pods.example.com  # 必须是 FQDN 格式
    admissionReviewVersions:
      - v1      # 支持的 AdmissionReview 版本
      - v1beta1
    clientConfig:
      # 方式1: 使用 Kubernetes Service
      service:
        namespace: webhook-system       # Webhook 服务所在命名空间
        name: webhook-service          # Service 名称
        path: /validate-pods           # 请求路径
        port: 443                      # 端口,默认 443
      # CA 证书用于验证 Webhook 服务器
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...  # Base64 编码的 CA 证书
    rules:
      - operations:
          - CREATE    # 创建操作
          - UPDATE    # 更新操作
        apiGroups:
          - ""        # 核心 API 组(留空)
        apiVersions:
          - v1        # API 版本
        resources:
          - pods      # 资源类型
        scope: Namespaced  # 作用域: Namespaced 或 Cluster
    failurePolicy: Fail    # 失败策略: Fail 或 Ignore
    sideEffects: None      # 副作用声明
    timeoutSeconds: 10     # 超时时间(秒),默认 10,最大 30
    matchPolicy: Equivalent  # 匹配策略: Exact 或 Equivalent
```

### 使用外部 URL

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: external-webhook
webhooks:
  - name: validate.external.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      # 方式2: 使用外部 URL
      url: https://external-webhook.example.com:443/validate
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["apps"]
        apiVersions: [v1]
        resources: [deployments]
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 15
```

### 命名空间选择器

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: namespace-scoped-webhook
webhooks:
  - name: validate.namespaced.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /validate
    rules:
      - operations: [CREATE, UPDATE, DELETE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    # 只对带有特定标签的命名空间生效
    namespaceSelector:
      matchLabels:
        environment: production
        webhook: enabled
      matchExpressions:
        - key: team
          operator: In
          values:
            - platform
            - security
    failurePolicy: Fail
    sideEffects: None
```

### 对象选择器

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: object-scoped-webhook
webhooks:
  - name: validate.objects.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /validate
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["apps"]
        apiVersions: [v1]
        resources: [deployments]
    # 只对带有特定标签的对象生效
    objectSelector:
      matchLabels:
        security-scan: required
      matchExpressions:
        - key: app.kubernetes.io/managed-by
          operator: NotIn
          values:
            - system
            - kube-system
    failurePolicy: Fail
    sideEffects: None
```

### 匹配条件 (v1.27+)

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: cel-matching-webhook
webhooks:
  - name: validate.cel.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /validate
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    # 使用 CEL 表达式精确匹配(v1.27+)
    matchConditions:
      # 条件1: 只验证特权容器
      - name: has-privileged-container
        expression: |
          object.spec.containers.exists(c, c.securityContext.privileged == true)
      # 条件2: 排除系统命名空间
      - name: not-system-namespace
        expression: |
          !object.metadata.namespace.startsWith('kube-')
      # 条件3: 检查资源请求
      - name: has-resource-limits
        expression: |
          object.spec.containers.all(c, 
            has(c.resources) && 
            has(c.resources.limits) && 
            has(c.resources.limits.memory)
          )
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
```

### 多规则配置

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: multi-rule-webhook
webhooks:
  - name: validate.multi.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /validate
    rules:
      # 规则1: 验证所有 Pod
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
        scope: Namespaced
      # 规则2: 验证所有 Deployment
      - operations: [CREATE, UPDATE]
        apiGroups: [apps]
        apiVersions: [v1]
        resources: [deployments, statefulsets]
        scope: Namespaced
      # 规则3: 验证所有 Service
      - operations: [CREATE, UPDATE, DELETE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [services]
        scope: Namespaced
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
```

---

## MutatingWebhookConfiguration

### 基础示例

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: example-mutating-webhook
  annotations:
    description: "自动注入 Sidecar 容器"
webhooks:
  - name: mutate.pods.example.com
    admissionReviewVersions: [v1, v1beta1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /mutate-pods
        port: 443
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
        scope: Namespaced
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
    # 重新调用策略(仅 Mutating Webhook 支持)
    reinvocationPolicy: Never  # Never 或 IfNeeded
```

### Sidecar 注入示例

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: sidecar-injector
  labels:
    app: sidecar-injector
webhooks:
  - name: sidecar.inject.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: sidecar-system
        name: sidecar-injector
        path: /inject
    rules:
      - operations: [CREATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    # 只在启用注入的命名空间生效
    namespaceSelector:
      matchLabels:
        sidecar-injection: enabled
    # 排除已有 Sidecar 的 Pod
    objectSelector:
      matchExpressions:
        - key: sidecar.inject.example.com/status
          operator: DoesNotExist
    failurePolicy: Fail      # Sidecar 注入失败则拒绝 Pod
    sideEffects: None
    timeoutSeconds: 15
    reinvocationPolicy: Never
```

### 重新调用策略

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: reinvocation-webhook
webhooks:
  - name: mutate.first.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /mutate-first
    rules:
      - operations: [CREATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    failurePolicy: Fail
    sideEffects: None
    # 重新调用策略
    # - Never: 只调用一次(默认)
    # - IfNeeded: 如果对象被后续 Webhook 修改,则重新调用
    reinvocationPolicy: IfNeeded
```

### 默认值注入

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: default-values-webhook
webhooks:
  - name: defaults.inject.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /inject-defaults
    rules:
      - operations: [CREATE]
        apiGroups: [apps]
        apiVersions: [v1]
        resources: [deployments]
    # 注入默认值时使用 Ignore,避免影响可用性
    failurePolicy: Ignore
    sideEffects: None
    timeoutSeconds: 5
    reinvocationPolicy: Never
```

### 标签和注解修改

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: label-mutator
webhooks:
  - name: labels.mutate.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
        path: /mutate-labels
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    # 只对生产命名空间生效
    namespaceSelector:
      matchLabels:
        environment: production
    failurePolicy: Ignore  # 标签修改失败不影响资源创建
    sideEffects: None
    timeoutSeconds: 3
    reinvocationPolicy: Never
```

---

## 完整字段说明

### Webhook 配置字段

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration  # 或 MutatingWebhookConfiguration
metadata:
  name: complete-webhook-example
  labels:
    # 标签用于管理和筛选
    app: webhook
    component: validation
  annotations:
    # 注解用于说明和配置
    description: "完整字段示例"
webhooks:
  # ============================================================
  # 基本信息
  # ============================================================
  - name: complete.example.com  # 必须是 FQDN 格式,全局唯一
    
    # ============================================================
    # 支持的 AdmissionReview 版本
    # ============================================================
    admissionReviewVersions:
      - v1       # Kubernetes 1.16+
      - v1beta1  # Kubernetes 1.9-1.15(已弃用)
    
    # ============================================================
    # 客户端配置(二选一)
    # ============================================================
    clientConfig:
      # 选项1: 使用 Kubernetes Service
      service:
        namespace: webhook-system     # 必填
        name: webhook-service        # 必填
        path: /validate              # 可选,默认 "/"
        port: 443                    # 可选,默认 443
      
      # 选项2: 使用外部 URL
      # url: https://external.example.com:443/validate
      
      # CA 证书(Base64 编码)
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    
    # ============================================================
    # 匹配规则(支持多个规则)
    # ============================================================
    rules:
      - operations:      # 匹配的操作
          - CREATE       # 创建
          - UPDATE       # 更新
          - DELETE       # 删除
          - CONNECT      # 连接
          - "*"          # 所有操作
        apiGroups:       # 匹配的 API 组
          - ""           # 核心组
          - apps         # apps 组
          - "*"          # 所有组
        apiVersions:     # 匹配的 API 版本
          - v1
          - v1beta1
          - "*"          # 所有版本
        resources:       # 匹配的资源类型
          - pods
          - pods/status  # 子资源
          - deployments
          - "*"          # 所有资源
        scope: Namespaced  # 作用域: Namespaced, Cluster, "*"
    
    # ============================================================
    # 命名空间选择器
    # ============================================================
    namespaceSelector:
      matchLabels:
        environment: production
      matchExpressions:
        - key: team
          operator: In  # In, NotIn, Exists, DoesNotExist
          values:
            - platform
            - security
    
    # ============================================================
    # 对象选择器
    # ============================================================
    objectSelector:
      matchLabels:
        webhook: enabled
      matchExpressions:
        - key: skip-webhook
          operator: DoesNotExist
    
    # ============================================================
    # 匹配条件(v1.27+,使用 CEL 表达式)
    # ============================================================
    matchConditions:
      - name: condition-name
        expression: |
          object.metadata.namespace != 'kube-system'
    
    # ============================================================
    # 失败策略
    # ============================================================
    failurePolicy: Fail  # Fail: 失败时拒绝 | Ignore: 失败时忽略
    
    # ============================================================
    # 副作用声明
    # ============================================================
    sideEffects: None  # None, NoneOnDryRun, Some(已弃用), Unknown(已弃用)
    
    # ============================================================
    # 超时时间
    # ============================================================
    timeoutSeconds: 10  # 1-30 秒,默认 10
    
    # ============================================================
    # 匹配策略
    # ============================================================
    matchPolicy: Equivalent  # Exact: 精确匹配 | Equivalent: 等效匹配
    
    # ============================================================
    # 重新调用策略(仅 MutatingWebhookConfiguration)
    # ============================================================
    # reinvocationPolicy: Never  # Never: 只调用一次 | IfNeeded: 需要时重新调用
```

### Rules 字段详解

#### Operations

```yaml
rules:
  - operations:
      - CREATE      # 创建资源
      - UPDATE      # 更新资源
      - DELETE      # 删除资源
      - CONNECT     # 连接操作(如 exec, attach, port-forward)
      - "*"         # 所有操作
```

#### API Groups

```yaml
rules:
  - apiGroups:
      - ""          # 核心 API 组(v1)
      - apps        # apps/v1
      - batch       # batch/v1
      - networking.k8s.io
      - "*"         # 所有组
```

#### Resources

```yaml
rules:
  - resources:
      - pods                # 主资源
      - pods/status         # 子资源
      - pods/log            # 子资源
      - pods/exec           # 子资源
      - deployments
      - services
      - "*"                 # 所有资源
      - "*/status"          # 所有资源的 status 子资源
```

#### Scope

```yaml
rules:
  - scope: Namespaced  # 命名空间级别资源
  # scope: Cluster     # 集群级别资源
  # scope: "*"         # 所有作用域
```

### MatchPolicy 说明

```yaml
# Exact: 精确匹配
# - 只匹配 rules 中明确指定的资源和版本
matchPolicy: Exact

# Equivalent: 等效匹配(默认,推荐)
# - 匹配等效的资源请求
# - 例如: apps/v1 Deployment 等效于 apps/v1beta2 Deployment
matchPolicy: Equivalent
```

### FailurePolicy 说明

```yaml
# Fail: 安全优先
# - Webhook 失败、超时或无法访问时拒绝请求
# - 适用于关键的验证和准入控制
failurePolicy: Fail

# Ignore: 可用性优先
# - Webhook 失败时忽略并继续处理
# - 适用于非关键的修改和默认值注入
failurePolicy: Ignore
```

### SideEffects 说明

```yaml
# None: 无副作用
# - Webhook 不会产生任何副作用
# - 可以在 DryRun 模式下安全调用
sideEffects: None

# NoneOnDryRun: DryRun 时无副作用
# - 正常请求可能有副作用
# - DryRun 请求时无副作用
sideEffects: NoneOnDryRun

# Some: 有副作用(v1.19 已弃用)
# Unknown: 未知副作用(v1.19 已弃用)
```

---

## 内部原理

### 1. 调用链路详解

```
┌─────────────────────────────────────────────────────────────┐
│                    API 请求 → API Server                     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                     认证 (Authentication)                    │
│  - X.509 客户端证书                                          │
│  - Bearer Token                                              │
│  - Bootstrap Token                                           │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      鉴权 (Authorization)                     │
│  - RBAC                                                      │
│  - ABAC                                                      │
│  - Webhook                                                   │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                  Mutating Admission Controllers               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 1. 内置 Mutating Controllers (按顺序执行)            │    │
│  │    - NamespaceLifecycle                              │    │
│  │    - LimitRanger                                     │    │
│  │    - ServiceAccount                                  │    │
│  │    - DefaultStorageClass                             │    │
│  │    - ...                                             │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↓                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 2. Mutating Webhooks (并行调用)                      │    │
│  │    - Webhook A ─┐                                    │    │
│  │    - Webhook B ─┼─→ 并行调用,最多 reinvocation 1 次 │    │
│  │    - Webhook C ─┘                                    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                     Object Schema 验证                        │
│  - 检查必填字段                                               │
│  - 验证字段类型                                               │
│  - 检查字段约束                                               │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                 Validating Admission Controllers              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 1. 内置 Validating Controllers                       │    │
│  │    - LimitRanger                                     │    │
│  │    - ResourceQuota                                   │    │
│  │    - PodSecurity                                     │    │
│  │    - ...                                             │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↓                                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ 2. Validating Webhooks (并行调用)                    │    │
│  │    - Webhook X ─┐                                    │    │
│  │    - Webhook Y ─┼─→ 并行调用                         │    │
│  │    - Webhook Z ─┘                                    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      持久化到 etcd                            │
└─────────────────────────────────────────────────────────────┘
```

### 2. Webhook 调用机制

#### 串行 vs 并行

```yaml
# Mutating Webhooks: 同类型 Webhook 并行调用
# - 多个 MutatingWebhookConfiguration 中的 Webhook 并行执行
# - 如果 reinvocationPolicy=IfNeeded,可能会重新调用
# - 调用顺序不保证

# Validating Webhooks: 并行调用
# - 所有 ValidatingWebhook 并行执行
# - 只要有一个拒绝,整个请求失败
# - 调用顺序不保证

# 示例: 3 个 Mutating Webhook 的执行
# 
# 第一轮:
#   Webhook A ─┐
#   Webhook B ─┼─→ 并行执行
#   Webhook C ─┘
#        ↓
#   对象被修改
#        ↓
# 第二轮(如果有 reinvocationPolicy=IfNeeded):
#   Webhook A ─→ 重新调用(因为对象被 B 或 C 修改)
```

#### 超时处理

```yaml
# timeoutSeconds 配置
webhooks:
  - name: webhook.example.com
    timeoutSeconds: 10  # 10 秒超时
    
    # 超时后的行为取决于 failurePolicy:
    # - Fail: 超时视为失败,拒绝请求
    # - Ignore: 超时后忽略,继续处理
    failurePolicy: Fail
```

#### 失败降级

```yaml
# 示例: 多层降级策略
webhooks:
  # 关键验证: Fail
  - name: critical-validation.example.com
    failurePolicy: Fail      # 失败必须拒绝
    timeoutSeconds: 5        # 短超时
    
  # 非关键验证: Ignore
  - name: optional-validation.example.com
    failurePolicy: Ignore    # 失败时忽略
    timeoutSeconds: 3
    
  # 默认值注入: Ignore
  - name: defaults-injection.example.com
    failurePolicy: Ignore    # 注入失败不影响资源创建
    timeoutSeconds: 2
```

### 3. Reinvocation 机制 (仅 Mutating)

```yaml
# reinvocationPolicy: IfNeeded
# 
# 场景: Webhook A 添加标签, Webhook B 修改镜像
# 
# 1. 原始对象
#    { "metadata": {}, "spec": { "image": "app:v1" } }
# 
# 2. 第一轮调用(并行)
#    Webhook A: 添加标签 → { "metadata": { "labels": { "injected": "true" } }, "spec": { "image": "app:v1" } }
#    Webhook B: 修改镜像 → { "metadata": {}, "spec": { "image": "app:v2" } }
# 
# 3. 合并结果
#    { "metadata": { "labels": { "injected": "true" } }, "spec": { "image": "app:v2" } }
# 
# 4. 第二轮调用(如果 reinvocationPolicy=IfNeeded)
#    Webhook A: 检查对象已被修改(image 变了),重新调用
#    Webhook B: 不再调用
# 
# 5. 最终结果
#    合并 Webhook A 的第二轮结果

webhooks:
  - name: webhook-a.example.com
    reinvocationPolicy: IfNeeded  # 对象被修改时重新调用
    # 最多重新调用 1 次,避免无限循环
```

### 4. AdmissionReview 请求/响应

#### 请求格式

```yaml
# API Server 发送给 Webhook 的请求
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "request": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",  # 请求 ID
    "kind": {
      "group": "",
      "version": "v1",
      "kind": "Pod"
    },
    "resource": {
      "group": "",
      "version": "v1",
      "resource": "pods"
    },
    "subResource": "",                                # 子资源,如 "status"
    "requestKind": { "group": "", "version": "v1", "kind": "Pod" },
    "requestResource": { "group": "", "version": "v1", "resource": "pods" },
    "name": "myapp",                                  # 资源名称(UPDATE/DELETE 时)
    "namespace": "default",                           # 命名空间
    "operation": "CREATE",                            # CREATE, UPDATE, DELETE, CONNECT
    "userInfo": {                                     # 请求用户信息
      "username": "admin",
      "uid": "014fbff9a07c",
      "groups": ["system:authenticated", "my-admin-group"],
      "extra": {}
    },
    "object": {                                       # 要创建/更新的对象(CREATE/UPDATE)
      "apiVersion": "v1",
      "kind": "Pod",
      "metadata": { "name": "myapp", "namespace": "default" },
      "spec": { "containers": [...] }
    },
    "oldObject": {},                                  # 旧对象(UPDATE/DELETE 时)
    "dryRun": false,                                  # 是否为 DryRun 请求
    "options": {}                                     # 请求选项
  }
}
```

#### 响应格式

```yaml
# Webhook 返回的响应

# 1. 允许请求(Validating)
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "response": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",  # 必须与请求 uid 相同
    "allowed": true                                  # 允许请求
  }
}

# 2. 拒绝请求(Validating)
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "response": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",
    "allowed": false,                                # 拒绝请求
    "status": {
      "code": 403,                                   # HTTP 状态码
      "message": "Pod security validation failed: privileged container not allowed"
    }
  }
}

# 3. 修改对象(Mutating,使用 JSONPatch)
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "response": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",
    "allowed": true,
    "patchType": "JSONPatch",                        # JSONPatch 或 MergePatch
    "patch": "W3sib3AiOiAiYWRkIiwgInBhdGgiOiAiL21ldGFkYXRhL2xhYmVscyIsICJ2YWx1ZSI6IHsiaW5qZWN0ZWQiOiAidHJ1ZSJ9fV0="
    # Base64 编码的 JSONPatch:
    # [{"op": "add", "path": "/metadata/labels", "value": {"injected": "true"}}]
  }
}

# 4. 修改对象(Mutating,使用 MergePatch)
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "response": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",
    "allowed": true,
    "patchType": "MergePatch",
    "patch": "eyJtZXRhZGF0YSI6IHsibGFiZWxzIjogeyJpbmplY3RlZCI6ICJ0cnVlIn19fQ=="
    # Base64 编码的 MergePatch:
    # {"metadata": {"labels": {"injected": "true"}}}
  }
}

# 5. 警告信息(v1.19+)
{
  "apiVersion": "admission.k8s.io/v1",
  "kind": "AdmissionReview",
  "response": {
    "uid": "705ab4f5-6393-11e8-b7cc-42010a800002",
    "allowed": true,
    "warnings": [                                    # 警告列表
      "This configuration is deprecated",
      "Consider using apps/v1 instead of apps/v1beta1"
    ]
  }
}
```

### 5. 证书管理

```yaml
# Webhook 服务器必须使用 HTTPS
# API Server 通过 caBundle 验证 Webhook 服务器证书

# 方式1: 手动管理证书
# 1. 生成 CA 证书
# 2. 生成 Webhook 服务器证书(CN=webhook-service.webhook-system.svc)
# 3. 将 CA 证书 Base64 编码后放入 caBundle

# 方式2: 使用 cert-manager 自动管理
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webhook-cert
  namespace: webhook-system
spec:
  secretName: webhook-cert-secret
  duration: 2160h  # 90 天
  renewBefore: 360h  # 提前 15 天续期
  issuerRef:
    name: selfsigned-issuer
    kind: Issuer
  dnsNames:
    - webhook-service.webhook-system.svc
    - webhook-service.webhook-system.svc.cluster.local
---
# 使用 cert-manager 的 CA Injector 自动注入 caBundle
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: webhook-with-cert-manager
  annotations:
    # cert-manager 会自动将 CA 证书注入到 caBundle
    cert-manager.io/inject-ca-from: webhook-system/webhook-cert
webhooks:
  - name: webhook.example.com
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
      # caBundle 会被自动注入,无需手动设置
```

---

## 版本兼容性

### 各版本特性

| 版本 | 新增特性 | 变更 |
|------|---------|------|
| **v1.9** | AdmissionReview v1beta1 | 初始版本 |
| **v1.14** | objectSelector | 支持按对象标签筛选 |
| **v1.15** | AdmissionReview v1beta1 稳定 | - |
| **v1.16** | AdmissionReview v1 GA | 推荐使用 v1 |
| **v1.19** | warnings 字段 | 支持返回警告信息 |
| **v1.19** | sideEffects: Some/Unknown 弃用 | 必须使用 None/NoneOnDryRun |
| **v1.20** | matchPolicy: Equivalent 默认 | - |
| **v1.22** | AdmissionReview v1beta1 弃用 | 推荐迁移到 v1 |
| **v1.25** | AdmissionReview v1beta1 移除 | 必须使用 v1 |
| **v1.27** | matchConditions (Alpha) | 支持 CEL 表达式匹配 |
| **v1.28** | matchConditions (Beta) | CEL 表达式匹配稳定 |
| **v1.30** | matchConditions (GA) | CEL 表达式匹配正式发布 |

### v1.25+ 迁移指南

```yaml
# v1.24 及以前(支持 v1beta1)
webhooks:
  - name: webhook.example.com
    admissionReviewVersions:
      - v1
      - v1beta1  # v1.25 不再支持

# v1.25+ (只支持 v1)
webhooks:
  - name: webhook.example.com
    admissionReviewVersions:
      - v1  # 必须支持 v1
    
    # 确保 Webhook 服务器支持 AdmissionReview v1
    # v1 与 v1beta1 的主要区别:
    # 1. request.object/oldObject 是 runtime.RawExtension 而非 runtime.Unknown
    # 2. patch 必须是 Base64 编码的 JSON
```

### CEL 匹配条件 (v1.27+)

```yaml
# v1.27+: Alpha,需要启用 feature gate
# v1.28+: Beta,默认启用
# v1.30+: GA

# 在 v1.27-v1.29 需要启用 feature gate:
# --feature-gates=ValidatingAdmissionPolicy=true

webhooks:
  - name: webhook.example.com
    # ... 其他配置 ...
    matchConditions:
      - name: exclude-system-pods
        expression: |
          !(object.metadata.namespace.startsWith('kube-') || 
            object.metadata.namespace == 'kube-system')
      
      - name: require-resource-limits
        expression: |
          object.spec.containers.all(c, 
            has(c.resources) && 
            has(c.resources.limits) &&
            has(c.resources.limits.cpu) &&
            has(c.resources.limits.memory)
          )
```

---

## 最佳实践

### 1. 性能优化

```yaml
# ✅ 推荐: 使用选择器减少调用次数
webhooks:
  - name: webhook.example.com
    # 只在需要的命名空间启用
    namespaceSelector:
      matchLabels:
        webhook: enabled
    # 排除不需要的对象
    objectSelector:
      matchExpressions:
        - key: skip-webhook
          operator: DoesNotExist
    # 使用 CEL 精确匹配
    matchConditions:
      - name: only-production-pods
        expression: |
          object.metadata.labels['environment'] == 'production'
    # 短超时
    timeoutSeconds: 5

# ❌ 避免: 匹配所有资源
webhooks:
  - name: webhook.example.com
    rules:
      - operations: ["*"]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    # 这会导致每个 API 请求都调用 Webhook!
```

### 2. 可靠性保障

```yaml
# ✅ 推荐: 关键验证使用 Fail,非关键使用 Ignore
webhooks:
  # 安全验证: 必须成功
  - name: security-validation.example.com
    failurePolicy: Fail
    timeoutSeconds: 5  # 短超时避免阻塞
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    
  # 默认值注入: 可以失败
  - name: defaults-injection.example.com
    failurePolicy: Ignore  # 失败不影响 Pod 创建
    timeoutSeconds: 3
    rules:
      - operations: [CREATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]

# ✅ 推荐: 为 Webhook 服务配置多副本和 PodDisruptionBudget
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webhook-server
  namespace: webhook-system
spec:
  replicas: 3  # 多副本保障可用性
  selector:
    matchLabels:
      app: webhook-server
  template:
    metadata:
      labels:
        app: webhook-server
    spec:
      affinity:
        # 分散到不同节点
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app: webhook-server
              topologyKey: kubernetes.io/hostname
      containers:
        - name: webhook
          image: webhook:v1.0.0
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8443
              scheme: HTTPS
            initialDelaySeconds: 10
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8443
              scheme: HTTPS
            initialDelaySeconds: 5
            periodSeconds: 5
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: webhook-server
  namespace: webhook-system
spec:
  minAvailable: 2  # 至少保持 2 个副本运行
  selector:
    matchLabels:
      app: webhook-server
```

### 3. 安全配置

```yaml
# ✅ 推荐: 使用 namespaceSelector 限制作用域
webhooks:
  - name: webhook.example.com
    # 只在非系统命名空间生效
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values:
            - kube-system
            - kube-public
            - kube-node-lease
            - webhook-system  # 不验证 Webhook 自己的命名空间
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]

# ✅ 推荐: 使用 matchConditions 精确匹配(v1.27+)
webhooks:
  - name: webhook.example.com
    matchConditions:
      # 排除系统组件
      - name: not-system-component
        expression: |
          !(object.metadata.labels['app.kubernetes.io/managed-by'] in [
            'kube-controller-manager',
            'kube-scheduler'
          ])
      # 只验证特定团队的资源
      - name: specific-team
        expression: |
          has(object.metadata.labels['team']) && 
          object.metadata.labels['team'] in ['platform', 'app']
```

### 4. 调试和监控

```yaml
# ✅ 推荐: 添加详细的拒绝消息
# Webhook 服务器返回:
{
  "response": {
    "allowed": false,
    "status": {
      "code": 403,
      "message": "Pod security validation failed: [ERROR-001] privileged container not allowed in namespace 'production'. See: https://docs.example.com/security/pod-security"
      # - 包含错误码
      # - 说明原因
      # - 提供文档链接
    }
  }
}

# ✅ 推荐: 使用警告而非拒绝(v1.19+)
{
  "response": {
    "allowed": true,  # 允许创建
    "warnings": [     # 但返回警告
      "This Pod configuration is deprecated and will be rejected in Kubernetes 1.32+",
      "Please update your Pod spec to use security context instead"
    ]
  }
}

# ✅ 推荐: 添加监控指标
# Webhook 服务器应该暴露 Prometheus 指标:
# - webhook_admission_requests_total{webhook="validate-pods",result="allow|deny"}
# - webhook_admission_duration_seconds{webhook="validate-pods"}
# - webhook_admission_errors_total{webhook="validate-pods",error_type="timeout|connection"}
```

### 5. Webhook 命名规范

```yaml
# ✅ 推荐: 使用 FQDN 格式的名称
webhooks:
  - name: validate.pods.security.example.com
    # 格式: <verb>.<resource>.<category>.<domain>
    # - verb: validate, mutate
    # - resource: pods, deployments, services
    # - category: security, networking, storage
    # - domain: 组织域名

# 其他示例:
# - mutate.pods.defaults.example.com
# - validate.deployments.quota.example.com
# - mutate.services.network.example.com

# ❌ 避免: 使用简单名称
webhooks:
  - name: my-webhook  # 可能与其他组织的 Webhook 冲突
```

### 6. 避免死锁

```yaml
# ❌ 危险: Webhook 验证自己命名空间的资源
webhooks:
  - name: webhook.example.com
    namespaceSelector: {}  # 匹配所有命名空间
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        resources: [pods]
    # 问题: 如果 Webhook Pod 重启,可能无法通过自己的验证!

# ✅ 推荐: 排除 Webhook 自己的命名空间
webhooks:
  - name: webhook.example.com
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values:
            - webhook-system  # 排除 Webhook 所在命名空间
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        resources: [pods]

# ✅ 推荐: 或使用 objectSelector 排除 Webhook Pod
webhooks:
  - name: webhook.example.com
    objectSelector:
      matchExpressions:
        - key: app
          operator: NotIn
          values:
            - webhook-server  # 排除 Webhook Pod
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        resources: [pods]
```

### 7. DryRun 支持

```yaml
# ✅ 推荐: 正确声明副作用
webhooks:
  - name: webhook.example.com
    # 如果 Webhook 无副作用,声明为 None
    sideEffects: None  # 可以安全地用于 kubectl --dry-run
    
    # 如果 Webhook 有副作用(如创建审计日志),声明为 NoneOnDryRun
    # sideEffects: NoneOnDryRun

# Webhook 服务器应该检查 dryRun 字段:
{
  "request": {
    "dryRun": true,  # kubectl --dry-run=server
    # ...
  }
}

# 如果 dryRun=true,Webhook 不应该产生副作用
```

---

## 生产案例

### 案例 1: OPA Gatekeeper

OPA Gatekeeper 是一个基于 Open Policy Agent 的 Kubernetes 准入控制器。

```yaml
# Gatekeeper 的 ValidatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: gatekeeper-validating-webhook-configuration
  labels:
    gatekeeper.sh/system: "yes"
webhooks:
  - name: validation.gatekeeper.sh
    admissionReviewVersions: [v1, v1beta1]
    clientConfig:
      service:
        name: gatekeeper-webhook-service
        namespace: gatekeeper-system
        path: /v1/admit
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    # 排除 Gatekeeper 自己的命名空间
    namespaceSelector:
      matchExpressions:
        - key: admission.gatekeeper.sh/ignore
          operator: DoesNotExist
    failurePolicy: Ignore  # 默认 Ignore,避免影响集群可用性
    sideEffects: None
    timeoutSeconds: 3
    matchPolicy: Equivalent

---
# Gatekeeper 的 MutatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: gatekeeper-mutating-webhook-configuration
  labels:
    gatekeeper.sh/system: "yes"
webhooks:
  - name: mutation.gatekeeper.sh
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        name: gatekeeper-webhook-service
        namespace: gatekeeper-system
        path: /v1/mutate
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    namespaceSelector:
      matchExpressions:
        - key: admission.gatekeeper.sh/ignore
          operator: DoesNotExist
    failurePolicy: Ignore
    sideEffects: None
    timeoutSeconds: 1
    reinvocationPolicy: Never

---
# 策略示例: 要求所有容器设置资源限制
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredresources
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredResources
      validation:
        openAPIV3Schema:
          type: object
          properties:
            limits:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8srequiredresources
        
        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          not container.resources.limits.cpu
          msg := sprintf("Container %v missing cpu limit", [container.name])
        }
        
        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          not container.resources.limits.memory
          msg := sprintf("Container %v missing memory limit", [container.name])
        }
---
# 应用策略
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredResources
metadata:
  name: must-have-resources
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: [Pod]
    namespaces:
      - production
      - staging
  parameters:
    limits:
      - cpu
      - memory
```

### 案例 2: Kyverno

Kyverno 是一个 Kubernetes 原生的策略引擎。

```yaml
# Kyverno 的 ValidatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: kyverno-resource-validating-webhook-cfg
  labels:
    app.kubernetes.io/name: kyverno
webhooks:
  - name: validate.kyverno.svc
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        name: kyverno-svc
        namespace: kyverno
        path: /validate
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources:
          - pods
          - deployments
          - statefulsets
          - services
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values:
            - kyverno
            - kube-system
    failurePolicy: Fail  # Kyverno 默认 Fail
    sideEffects: None
    timeoutSeconds: 10

---
# Kyverno 的 MutatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: kyverno-resource-mutating-webhook-cfg
  labels:
    app.kubernetes.io/name: kyverno
webhooks:
  - name: mutate.kyverno.svc
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        name: kyverno-svc
        namespace: kyverno
        path: /mutate
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: ["*"]
        apiVersions: ["*"]
        resources: ["*"]
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values:
            - kyverno
            - kube-system
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
    reinvocationPolicy: IfNeeded  # 支持重新调用

---
# Kyverno 策略示例 1: 添加默认标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-default-labels
spec:
  rules:
    - name: add-team-label
      match:
        any:
          - resources:
              kinds:
                - Pod
                - Deployment
      mutate:
        patchStrategicMerge:
          metadata:
            labels:
              +(team): "platform"  # + 表示如果不存在则添加
              +(managed-by): "kyverno"

---
# Kyverno 策略示例 2: 验证镜像来源
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: validate-image-registry
spec:
  validationFailureAction: enforce  # enforce 或 audit
  rules:
    - name: check-image-registry
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "Images must come from approved registries"
        pattern:
          spec:
            containers:
              - image: "registry.example.com/* | gcr.io/* | docker.io/library/*"

---
# Kyverno 策略示例 3: 自动生成 NetworkPolicy
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-network-policy
spec:
  rules:
    - name: generate-default-deny-policy
      match:
        any:
          - resources:
              kinds:
                - Namespace
      generate:
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny-all
        namespace: "{{request.object.metadata.name}}"
        synchronize: true
        data:
          spec:
            podSelector: {}
            policyTypes:
              - Ingress
              - Egress
```

### 案例 3: cert-manager Webhook

cert-manager 使用 Webhook 验证和修改证书资源。

```yaml
# cert-manager 的 ValidatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: cert-manager-webhook
  labels:
    app: webhook
    app.kubernetes.io/name: webhook
  annotations:
    cert-manager.io/inject-ca-from: cert-manager/cert-manager-webhook-ca
webhooks:
  - name: webhook.cert-manager.io
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        name: cert-manager-webhook
        namespace: cert-manager
        path: /validate
      # caBundle 由 cert-manager 自动注入
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups:
          - cert-manager.io
          - acme.cert-manager.io
        apiVersions: ["*"]
        resources:
          - certificates
          - issuers
          - clusterissuers
          - certificaterequests
          - orders
          - challenges
    failurePolicy: Fail  # 证书配置必须验证通过
    sideEffects: None
    timeoutSeconds: 10

---
# cert-manager 的 MutatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: cert-manager-webhook
  labels:
    app: webhook
  annotations:
    cert-manager.io/inject-ca-from: cert-manager/cert-manager-webhook-ca
webhooks:
  - name: webhook.cert-manager.io
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        name: cert-manager-webhook
        namespace: cert-manager
        path: /mutate
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups:
          - cert-manager.io
          - acme.cert-manager.io
        apiVersions: ["*"]
        resources:
          - certificates
          - issuers
          - clusterissuers
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
    reinvocationPolicy: Never

---
# 示例: 创建证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com-tls
  namespace: default
spec:
  secretName: example-com-tls
  duration: 2160h  # 90 天
  renewBefore: 360h  # 提前 15 天续期
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
    - example.com
    - www.example.com
  # cert-manager webhook 会验证:
  # - issuerRef 是否存在
  # - dnsNames 格式是否正确
  # - duration 和 renewBefore 是否合理
```

### 案例 4: Istio Sidecar 自动注入

Istio 使用 MutatingWebhook 自动注入 Envoy Sidecar 容器。

```yaml
# Istio 的 MutatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: istio-sidecar-injector
  labels:
    app: sidecar-injector
    istio.io/rev: default
webhooks:
  - name: namespace.sidecar-injector.istio.io
    admissionReviewVersions: [v1, v1beta1]
    clientConfig:
      service:
        name: istiod
        namespace: istio-system
        path: /inject
        port: 443
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
    rules:
      - operations: [CREATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    # 命名空间级别注入控制
    namespaceSelector:
      matchLabels:
        istio-injection: enabled  # 只在标记的命名空间注入
    # 对象级别注入控制
    objectSelector:
      matchExpressions:
        # 排除已注入的 Pod
        - key: sidecar.istio.io/inject
          operator: NotIn
          values:
            - "false"
        # 排除 HostNetwork Pod
        - key: istio.io/rev
          operator: DoesNotExist
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 10
    reinvocationPolicy: Never

---
# 启用 Sidecar 注入的命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  labels:
    istio-injection: enabled  # 启用自动注入

---
# Pod 级别控制注入
apiVersion: v1
kind: Pod
metadata:
  name: myapp
  namespace: my-app
  annotations:
    # 覆盖命名空间设置,禁用注入
    sidecar.istio.io/inject: "false"
spec:
  containers:
    - name: app
      image: myapp:v1.0.0

---
# Istio 注入效果示例
# 原始 Pod:
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  containers:
    - name: app
      image: myapp:v1.0.0
      ports:
        - containerPort: 8080

# Istio 注入后的 Pod:
apiVersion: v1
kind: Pod
metadata:
  name: myapp
  annotations:
    # Istio 添加的注解
    sidecar.istio.io/status: '{"version":"...","initContainers":["istio-init"],"containers":["istio-proxy"],...}'
spec:
  # 注入的 Init 容器(配置 iptables 规则)
  initContainers:
    - name: istio-init
      image: docker.io/istio/proxyv2:1.20.0
      args:
        - istio-iptables
        - -p
        - "15001"  # Envoy 入站端口
        - -z
        - "15006"  # Envoy 入站捕获
        - -u
        - "1337"   # Envoy 用户 ID
      securityContext:
        capabilities:
          add:
            - NET_ADMIN
            - NET_RAW
  containers:
    # 原始容器
    - name: app
      image: myapp:v1.0.0
      ports:
        - containerPort: 8080
    
    # 注入的 Sidecar 容器
    - name: istio-proxy
      image: docker.io/istio/proxyv2:1.20.0
      args:
        - proxy
        - sidecar
        - --domain
        - $(POD_NAMESPACE).svc.cluster.local
      env:
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
      ports:
        - containerPort: 15090  # Envoy Prometheus metrics
          name: http-envoy-prom
          protocol: TCP
      resources:
        limits:
          cpu: "2"
          memory: 1Gi
        requests:
          cpu: 100m
          memory: 128Mi
      volumeMounts:
        - mountPath: /etc/istio/proxy
          name: istio-envoy
  volumes:
    - emptyDir:
        medium: Memory
      name: istio-envoy
```

### 案例 5: 自定义 Webhook 服务器示例

使用 Go 实现简单的 Validating Webhook。

```yaml
# 部署 Webhook 服务器
apiVersion: apps/v1
kind: Deployment
metadata:
  name: pod-validator
  namespace: webhook-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: pod-validator
  template:
    metadata:
      labels:
        app: pod-validator
    spec:
      containers:
        - name: webhook
          image: my-registry.example.com/pod-validator:v1.0.0
          ports:
            - containerPort: 8443
              name: webhook
          volumeMounts:
            - name: webhook-certs
              mountPath: /etc/webhook/certs
              readOnly: true
          env:
            - name: TLS_CERT_FILE
              value: /etc/webhook/certs/tls.crt
            - name: TLS_KEY_FILE
              value: /etc/webhook/certs/tls.key
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8443
              scheme: HTTPS
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8443
              scheme: HTTPS
      volumes:
        - name: webhook-certs
          secret:
            secretName: pod-validator-certs

---
# Service
apiVersion: v1
kind: Service
metadata:
  name: pod-validator
  namespace: webhook-system
spec:
  selector:
    app: pod-validator
  ports:
    - port: 443
      targetPort: 8443
      name: webhook

---
# ValidatingWebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: pod-validator
  annotations:
    cert-manager.io/inject-ca-from: webhook-system/pod-validator-cert
webhooks:
  - name: validate.pods.example.com
    admissionReviewVersions: [v1]
    clientConfig:
      service:
        namespace: webhook-system
        name: pod-validator
        path: /validate-pods
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        apiVersions: [v1]
        resources: [pods]
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values:
            - kube-system
            - webhook-system
    failurePolicy: Fail
    sideEffects: None
    timeoutSeconds: 5
```

Go 服务器代码示例:

```go
// main.go
package main

import (
    "encoding/json"
    "fmt"
    "io/ioutil"
    "net/http"
    
    admissionv1 "k8s.io/api/admission/v1"
    corev1 "k8s.io/api/core/v1"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

func main() {
    http.HandleFunc("/validate-pods", validatePods)
    http.HandleFunc("/healthz", healthz)
    http.HandleFunc("/readyz", readyz)
    
    // 启动 HTTPS 服务器
    certFile := "/etc/webhook/certs/tls.crt"
    keyFile := "/etc/webhook/certs/tls.key"
    
    fmt.Println("Starting webhook server on :8443")
    if err := http.ListenAndServeTLS(":8443", certFile, keyFile, nil); err != nil {
        panic(err)
    }
}

func validatePods(w http.ResponseWriter, r *http.Request) {
    // 读取请求
    body, err := ioutil.ReadAll(r.Body)
    if err != nil {
        http.Error(w, "failed to read request body", http.StatusBadRequest)
        return
    }
    
    // 解析 AdmissionReview
    admissionReview := admissionv1.AdmissionReview{}
    if err := json.Unmarshal(body, &admissionReview); err != nil {
        http.Error(w, "failed to unmarshal request", http.StatusBadRequest)
        return
    }
    
    // 解析 Pod
    pod := corev1.Pod{}
    if err := json.Unmarshal(admissionReview.Request.Object.Raw, &pod); err != nil {
        http.Error(w, "failed to unmarshal pod", http.StatusBadRequest)
        return
    }
    
    // 验证逻辑
    allowed := true
    message := ""
    
    // 示例 1: 禁止特权容器
    for _, container := range pod.Spec.Containers {
        if container.SecurityContext != nil && 
           container.SecurityContext.Privileged != nil && 
           *container.SecurityContext.Privileged {
            allowed = false
            message = fmt.Sprintf("Privileged container not allowed: %s", container.Name)
            break
        }
    }
    
    // 示例 2: 要求设置资源限制
    if allowed {
        for _, container := range pod.Spec.Containers {
            if container.Resources.Limits == nil || 
               container.Resources.Limits.Memory().IsZero() ||
               container.Resources.Limits.Cpu().IsZero() {
                allowed = false
                message = fmt.Sprintf("Container %s must specify CPU and memory limits", container.Name)
                break
            }
        }
    }
    
    // 构造响应
    response := admissionv1.AdmissionReview{
        TypeMeta: metav1.TypeMeta{
            APIVersion: "admission.k8s.io/v1",
            Kind:       "AdmissionReview",
        },
        Response: &admissionv1.AdmissionResponse{
            UID:     admissionReview.Request.UID,
            Allowed: allowed,
        },
    }
    
    if !allowed {
        response.Response.Result = &metav1.Status{
            Code:    403,
            Message: message,
        }
    }
    
    // 返回响应
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

func healthz(w http.ResponseWriter, r *http.Request) {
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("ok"))
}

func readyz(w http.ResponseWriter, r *http.Request) {
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("ready"))
}
```

---

## FAQ

### Q1: Webhook 调用失败会怎样?

**A**: 取决于 `failurePolicy`:

```yaml
# failurePolicy: Fail
# - Webhook 失败、超时、无法连接 → 拒绝请求
# - API 请求返回错误
# - 适用于关键验证

# failurePolicy: Ignore
# - Webhook 失败 → 忽略,继续处理
# - 就像 Webhook 不存在一样
# - 适用于非关键功能
```

**最佳实践**:
- 关键安全验证: 使用 `Fail`
- 默认值注入、增强功能: 使用 `Ignore`
- 提高 Webhook 可用性: 多副本 + PDB

### Q2: 如何避免 Webhook 影响集群性能?

**A**: 多层优化策略:

```yaml
# 1. 精确匹配:减少不必要的调用
webhooks:
  - name: webhook.example.com
    # 只匹配需要的资源
    rules:
      - operations: [CREATE]  # 不是 [CREATE, UPDATE, DELETE]
        apiGroups: [""]
        resources: [pods]      # 不是 ["*"]
    
    # 使用选择器过滤
    namespaceSelector:
      matchLabels:
        webhook: enabled       # 只在特定命名空间生效
    
    objectSelector:
      matchLabels:
        validate: required     # 只验证特定对象
    
    # 使用 CEL 精确匹配(v1.27+)
    matchConditions:
      - name: only-production
        expression: object.metadata.labels['env'] == 'production'
    
    # 短超时
    timeoutSeconds: 3          # 不要 10+

# 2. Webhook 服务优化
# - 多副本(3+)
# - 充足资源
# - 快速响应(<100ms)
# - 本地缓存
# - 异步处理

# 3. 监控和告警
# - webhook_admission_duration_seconds > 1s
# - webhook_admission_errors_total > threshold
```

### Q3: Mutating 和 Validating 的区别?

**A**: 

| 方面 | Mutating Webhook | Validating Webhook |
|------|-----------------|-------------------|
| **作用** | 修改对象 | 验证对象 |
| **执行顺序** | 先执行 | 后执行 |
| **响应** | 返回 JSONPatch | 返回 allow/deny |
| **reinvocationPolicy** | 支持 | 不支持 |
| **典型用途** | Sidecar 注入、默认值、标签修改 | 安全策略、配额、合规性检查 |

```yaml
# 执行顺序示例:
# 1. Mutating Webhooks
#    - 添加 Sidecar
#    - 注入默认值
#    - 修改标签
#    ↓
# 2. Schema 验证
#    ↓
# 3. Validating Webhooks
#    - 验证安全配置
#    - 检查资源配额
#    - 合规性检查
```

### Q4: 如何调试 Webhook?

**A**: 多种方法:

```bash
# 1. 检查 Webhook 配置
kubectl get validatingwebhookconfiguration
kubectl get mutatingwebhookconfiguration
kubectl describe validatingwebhookconfiguration <name>

# 2. 查看 API Server 日志
# 搜索 webhook 相关错误
kubectl logs -n kube-system kube-apiserver-xxx | grep webhook

# 3. 查看 Webhook 服务日志
kubectl logs -n webhook-system deployment/webhook-server

# 4. 测试 Webhook 连通性
kubectl run test-pod --image=nginx --dry-run=server
# 如果 Webhook 有问题,会显示错误信息

# 5. 检查证书
kubectl get secret -n webhook-system webhook-certs -o yaml
# 检查证书是否过期:
kubectl get secret -n webhook-system webhook-certs -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates

# 6. 直接调用 Webhook (测试)
curl -k -X POST https://webhook-service.webhook-system.svc:443/validate \
  -H "Content-Type: application/json" \
  -d @test-admission-review.json

# 7. 查看事件
kubectl get events --all-namespaces | grep admission
```

**常见错误**:

```yaml
# 错误 1: x509: certificate signed by unknown authority
# 原因: caBundle 不正确
# 解决: 更新 caBundle 为正确的 CA 证书

# 错误 2: context deadline exceeded (Client.Timeout)
# 原因: Webhook 超时
# 解决: 
#   - 检查 Webhook 服务是否运行
#   - 增加 timeoutSeconds
#   - 优化 Webhook 响应速度

# 错误 3: connection refused
# 原因: Webhook 服务不可用
# 解决:
#   - 检查 Service 和 Pod 是否运行
#   - 检查端口配置
#   - 检查网络策略

# 错误 4: admission webhook "xxx" denied the request
# 原因: 验证失败
# 解决: 查看错误消息,修复违反的策略
```

### Q5: 如何处理 Webhook 证书轮换?

**A**: 推荐使用 cert-manager 自动管理:

```yaml
# 1. 安装 cert-manager
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml

# 2. 创建自签名 Issuer
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: selfsigned-issuer
  namespace: webhook-system
spec:
  selfSigned: {}

# 3. 创建 CA 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webhook-ca
  namespace: webhook-system
spec:
  secretName: webhook-ca-secret
  isCA: true
  commonName: webhook-ca
  issuerRef:
    name: selfsigned-issuer
    kind: Issuer

# 4. 创建 CA Issuer
apiVersion: cert-manager.io/v1
kind: Issuer
metadata:
  name: webhook-ca-issuer
  namespace: webhook-system
spec:
  ca:
    secretName: webhook-ca-secret

# 5. 创建 Webhook 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webhook-cert
  namespace: webhook-system
spec:
  secretName: webhook-cert-secret
  duration: 2160h      # 90 天
  renewBefore: 360h    # 提前 15 天续期
  issuerRef:
    name: webhook-ca-issuer
    kind: Issuer
  dnsNames:
    - webhook-service
    - webhook-service.webhook-system
    - webhook-service.webhook-system.svc
    - webhook-service.webhook-system.svc.cluster.local

# 6. 自动注入 CA 到 WebhookConfiguration
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: my-webhook
  annotations:
    # cert-manager 会自动注入 caBundle
    cert-manager.io/inject-ca-from: webhook-system/webhook-cert
webhooks:
  - name: webhook.example.com
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service
      # caBundle 由 cert-manager 自动注入
    # ...

# 7. Webhook Pod 挂载证书
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webhook-server
  namespace: webhook-system
spec:
  template:
    spec:
      containers:
        - name: webhook
          volumeMounts:
            - name: certs
              mountPath: /etc/webhook/certs
              readOnly: true
      volumes:
        - name: certs
          secret:
            secretName: webhook-cert-secret
            # cert-manager 自动更新 Secret
            # Webhook Pod 需要监听证书变化并重新加载
```

### Q6: Webhook 可以修改哪些字段?

**A**: Mutating Webhook 可以修改几乎所有字段,但有限制:

```yaml
# ✅ 可以修改:
# - metadata.labels
# - metadata.annotations
# - spec 的大部分字段
# - 添加容器、Volume
# - 修改资源请求/限制

# ❌ 不能修改:
# - metadata.name (CREATE 后)
# - metadata.namespace (CREATE 后)
# - metadata.uid
# - metadata.creationTimestamp
# - metadata.deletionTimestamp

# ⚠️ 慎重修改:
# - 已有容器的关键配置
# - 可能导致不一致的字段
```

**示例**:

```yaml
# 修改示例 1: 添加标签(JSONPatch)
[
  {
    "op": "add",
    "path": "/metadata/labels",
    "value": {
      "injected": "true",
      "version": "v1"
    }
  }
]

# 修改示例 2: 添加 Sidecar 容器(JSONPatch)
[
  {
    "op": "add",
    "path": "/spec/containers/-",
    "value": {
      "name": "sidecar",
      "image": "sidecar:v1.0.0",
      "ports": [{"containerPort": 9090}]
    }
  }
]

# 修改示例 3: 添加环境变量(JSONPatch)
[
  {
    "op": "add",
    "path": "/spec/containers/0/env",
    "value": [
      {
        "name": "INJECTED_VAR",
        "value": "injected-value"
      }
    ]
  }
]

# 修改示例 4: 修改镜像(JSONPatch)
[
  {
    "op": "replace",
    "path": "/spec/containers/0/image",
    "value": "my-registry.example.com/app:v1.0.0"
  }
]
```

### Q7: 如何实现 Webhook 的灰度发布?

**A**: 多种策略:

```yaml
# 策略 1: 使用 objectSelector 灰度
# 1. 部署新版本 Webhook
# 2. 创建新的 WebhookConfiguration,只匹配带有特定标签的对象

apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: webhook-v2-canary
webhooks:
  - name: webhook-v2.example.com
    clientConfig:
      service:
        namespace: webhook-system
        name: webhook-service-v2  # 新版本 Service
    objectSelector:
      matchLabels:
        webhook-version: v2  # 只验证标记的对象
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        resources: [pods]
    failurePolicy: Ignore  # 灰度期间使用 Ignore

# 2. 逐步给对象添加标签,灰度新版本
# 3. 观察新版本稳定性
# 4. 全量切换后删除旧版本

---
# 策略 2: 使用命名空间灰度
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: webhook-v2-canary
webhooks:
  - name: webhook-v2.example.com
    clientConfig:
      service:
        name: webhook-service-v2
        namespace: webhook-system
    namespaceSelector:
      matchLabels:
        webhook-canary: enabled  # 只在特定命名空间启用
    rules:
      - operations: [CREATE, UPDATE]
        apiGroups: [""]
        resources: [pods]

# 逐步给命名空间添加标签:
# kubectl label namespace dev webhook-canary=enabled
# kubectl label namespace staging webhook-canary=enabled
# kubectl label namespace prod webhook-canary=enabled

---
# 策略 3: 使用多个 Webhook,通过权重分流(需要自定义实现)
# 在 Webhook 服务器内部实现灰度逻辑
```

### Q8: Webhook 性能基准是什么?

**A**: 推荐指标:

```yaml
# 延迟:
# - P50 < 50ms    (中位数)
# - P95 < 200ms   (95 分位)
# - P99 < 500ms   (99 分位)
# - Max < 1000ms  (最大值)

# 可用性:
# - SLA > 99.9% (月度停机 < 43 分钟)
# - 错误率 < 0.1%

# 并发:
# - 支持 100+ QPS (中小集群)
# - 支持 1000+ QPS (大集群)

# 超时配置:
webhooks:
  - name: webhook.example.com
    timeoutSeconds: 5  # 推荐 3-5 秒
    # 不要超过 10 秒,除非必要

# 资源配置:
resources:
  requests:
    cpu: 100m      # 基准
    memory: 128Mi
  limits:
    cpu: 500m      # 允许突发
    memory: 256Mi

# 副本数:
replicas: 3  # 至少 2 个,推荐 3+
```

---

## 总结

Admission Webhook 是 Kubernetes 准入控制的强大扩展机制:

1. **两种类型**: Mutating(修改)和 Validating(验证)
2. **调用链路**: 认证 → 鉴权 → Mutating → Schema 验证 → Validating → 持久化
3. **关键配置**: Rules, Selectors, FailurePolicy, SideEffects, Timeout
4. **新特性**: matchConditions (v1.27+) 使用 CEL 表达式精确匹配
5. **最佳实践**: 精确匹配、短超时、多副本、cert-manager 管理证书
6. **生产案例**: OPA Gatekeeper, Kyverno, cert-manager, Istio

**关键要点**:
- 关键验证使用 `failurePolicy: Fail`
- 非关键功能使用 `failurePolicy: Ignore`
- 使用 namespaceSelector/objectSelector/matchConditions 减少调用
- 多副本 + PDB 保障可用性
- 使用 cert-manager 自动管理证书
- 监控延迟、错误率、可用性

---

**相关文档**:
- [Kubernetes 官方文档 - Dynamic Admission Control](https://kubernetes.io/docs/reference/access-authn-authz/extensible-admission-controllers/)
- [OPA Gatekeeper](https://open-policy-agent.github.io/gatekeeper/)
- [Kyverno](https://kyverno.io/)
- [cert-manager](https://cert-manager.io/)
- [Istio Sidecar Injection](https://istio.io/latest/docs/setup/additional-setup/sidecar-injection/)
