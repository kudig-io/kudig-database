# 20 - Role / RoleBinding YAML 配置参考

## 概述

Role 和 RoleBinding 是 Kubernetes RBAC (Role-Based Access Control) 的核心资源,用于在 **namespace 级别** 定义和授予权限。Role 定义了一组权限规则,RoleBinding 将这些权限授予用户、组或 ServiceAccount。本文档覆盖 Role 和 RoleBinding 的完整 YAML 配置、内部原理和生产案例。

**适用版本**: Kubernetes v1.25 - v1.32  
**更新时间**: 2026-02

---

## 1. Role 基础配置

### 1.1 基本 Role

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  # Role 名称
  name: pod-reader
  # Role 所在的 namespace
  # Role 只能授权访问同一 namespace 的资源
  namespace: default
  labels:
    app: myapp
    purpose: readonly
  annotations:
    description: "允许读取 Pod 信息"

# rules 定义权限规则列表
rules:
# 每个规则定义一组 API 资源和允许的操作
- apiGroups:
    # apiGroups 指定资源所属的 API 组
    # "" 表示 core API 组 (如 Pod, Service, ConfigMap 等)
    # 其他常见组: apps, batch, networking.k8s.io 等
    - ""
  resources:
    # resources 指定资源类型
    # 必须是小写复数形式
    - "pods"
    # 可以使用子资源
    - "pods/log"
    - "pods/status"
  verbs:
    # verbs 指定允许的操作
    # 标准动词: get, list, watch, create, update, patch, delete, deletecollection
    - "get"      # 获取单个资源
    - "list"     # 列出资源集合
    - "watch"    # 监听资源变化
```

### 1.2 多资源类型的 Role

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-operator
  namespace: production
  labels:
    env: prod
    team: platform
  annotations:
    description: "允许操作应用相关资源"

rules:
# 规则 1: Pod 只读权限
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/log"
    - "pods/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: ConfigMap 和 Secret 只读权限
- apiGroups: [""]
  resources:
    - "configmaps"
    - "secrets"
  verbs:
    - "get"
    - "list"

# 规则 3: Deployment 完整权限
- apiGroups: ["apps"]
  resources:
    - "deployments"
    - "deployments/scale"
    - "deployments/status"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 4: Service 完整权限
- apiGroups: [""]
  resources:
    - "services"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 5: Ingress 完整权限
- apiGroups: ["networking.k8s.io"]
  resources:
    - "ingresses"
    - "ingresses/status"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"
```

### 1.3 使用 resourceNames 限制特定资源

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: config-reader
  namespace: default
  annotations:
    description: "只允许读取特定的 ConfigMap"

rules:
# 规则 1: 只能读取名为 app-config 和 db-config 的 ConfigMap
- apiGroups: [""]
  resources:
    - "configmaps"
  # resourceNames 限制可以访问的资源名称列表
  # 只对 get, delete, update, patch 动词生效
  # list 和 watch 不支持 resourceNames 过滤
  resourceNames:
    - "app-config"
    - "db-config"
  verbs:
    - "get"

# 规则 2: 可以列出所有 ConfigMap (但只能 get 指定的)
- apiGroups: [""]
  resources:
    - "configmaps"
  verbs:
    - "list"
    - "watch"
```

### 1.4 完整权限的 Role

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: namespace-admin
  namespace: dev-team
  labels:
    role: admin
  annotations:
    description: "namespace 管理员,拥有所有资源的完整权限"

rules:
# 规则 1: 所有 core API 资源
- apiGroups: [""]
  resources:
    - "*"  # 通配符,表示所有资源
  verbs:
    - "*"  # 通配符,表示所有动词

# 规则 2: apps API 组的所有资源
- apiGroups: ["apps"]
  resources: ["*"]
  verbs: ["*"]

# 规则 3: batch API 组的所有资源
- apiGroups: ["batch"]
  resources: ["*"]
  verbs: ["*"]

# 规则 4: networking.k8s.io API 组的所有资源
- apiGroups: ["networking.k8s.io"]
  resources: ["*"]
  verbs: ["*"]

# 规则 5: autoscaling API 组的所有资源
- apiGroups: ["autoscaling"]
  resources: ["*"]
  verbs: ["*"]

# 规则 6: rbac.authorization.k8s.io API 组
# 注意: 即使是 namespace 管理员,通常也不应该授予 RBAC 权限
# 这会导致权限升级风险
# - apiGroups: ["rbac.authorization.k8s.io"]
#   resources: ["*"]
#   verbs: ["*"]
```

---

## 2. RoleBinding 基础配置

### 2.1 基本 RoleBinding

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  # RoleBinding 名称
  name: read-pods
  # RoleBinding 所在的 namespace
  # 必须与 Role 在同一 namespace
  namespace: default
  labels:
    app: myapp
  annotations:
    description: "将 pod-reader 角色授予 jane 用户"

# subjects 定义权限的接收者 (被授权者)
subjects:
# Subject 可以是 User, Group 或 ServiceAccount
- kind: User
  name: jane
  # apiGroup 对于 User 和 Group 必须是 rbac.authorization.k8s.io
  apiGroup: rbac.authorization.k8s.io

# roleRef 定义要绑定的 Role
roleRef:
  # kind 可以是 Role 或 ClusterRole
  # RoleBinding 可以引用 ClusterRole (限制在 namespace 范围)
  kind: Role
  name: pod-reader
  # apiGroup 必须是 rbac.authorization.k8s.io
  apiGroup: rbac.authorization.k8s.io
```

### 2.2 授权给 ServiceAccount

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-operator-binding
  namespace: production
  labels:
    app: myapp
    env: prod
  annotations:
    description: "授予 myapp ServiceAccount 操作权限"

subjects:
# ServiceAccount 作为 subject
- kind: ServiceAccount
  # ServiceAccount 名称
  name: myapp-sa
  # ServiceAccount 所在的 namespace
  # 如果与 RoleBinding 在同一 namespace,可以省略
  namespace: production
  # ServiceAccount 的 apiGroup 必须是 ""
  # (不是 rbac.authorization.k8s.io)

roleRef:
  kind: Role
  name: app-operator
  apiGroup: rbac.authorization.k8s.io
```

### 2.3 授权给多个 Subjects

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-team-binding
  namespace: dev-team
  labels:
    team: dev
  annotations:
    description: "授予开发团队成员访问权限"

subjects:
# Subject 1: 用户 alice
- kind: User
  name: alice
  apiGroup: rbac.authorization.k8s.io

# Subject 2: 用户 bob
- kind: User
  name: bob
  apiGroup: rbac.authorization.k8s.io

# Subject 3: 开发组 (Group)
- kind: Group
  # Group 名称由身份认证系统提供
  # 例如: OIDC, LDAP, x509 证书的 O 字段
  name: developers
  apiGroup: rbac.authorization.k8s.io

# Subject 4: ServiceAccount
- kind: ServiceAccount
  name: ci-cd-sa
  namespace: dev-team

# Subject 5: 所有 ServiceAccount (不推荐)
# - kind: Group
#   name: system:serviceaccounts:dev-team
#   apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: Role
  name: namespace-admin
  apiGroup: rbac.authorization.k8s.io
```

### 2.4 RoleBinding 引用 ClusterRole

```yaml
# ClusterRole 定义 (集群范围)
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: secret-reader
  annotations:
    description: "读取 Secret 的权限"
rules:
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list", "watch"]

---
# RoleBinding 引用 ClusterRole (限制在 namespace 范围)
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-secrets-in-default
  # RoleBinding 在 default namespace
  # 只能访问 default namespace 的 secrets
  namespace: default
  annotations:
    description: "允许读取 default namespace 的 secrets"

subjects:
- kind: User
  name: dave
  apiGroup: rbac.authorization.k8s.io

roleRef:
  # 引用 ClusterRole
  kind: ClusterRole
  name: secret-reader
  apiGroup: rbac.authorization.k8s.io

---
# 另一个 RoleBinding 引用相同的 ClusterRole (在不同 namespace)
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-secrets-in-production
  # RoleBinding 在 production namespace
  # 只能访问 production namespace 的 secrets
  namespace: production
  annotations:
    description: "允许读取 production namespace 的 secrets"

subjects:
- kind: User
  name: dave
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: secret-reader
  apiGroup: rbac.authorization.k8s.io
```

---

## 3. 内部原理: RBAC Authorizer

### 3.1 RBAC 授权流程

```
┌─────────────────────────────────────────────────────────────────┐
│                         API Request                              │
│  User/ServiceAccount → kube-apiserver                            │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  Authentication (身份认证)                                       │
│  - x509 Client Cert                                              │
│  - Bearer Token (ServiceAccount)                                 │
│  - OIDC, LDAP, etc.                                              │
└─────────────────────────┬───────────────────────────────────────┘
                          │ username, groups, extra
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  Authorization (授权)                                            │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  RBAC Authorizer (默认启用)                             │    │
│  │  --authorization-mode=Node,RBAC                          │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  RBAC 权限检查流程                                               │
│                                                                   │
│  1. 提取请求信息:                                                │
│     - Username: system:serviceaccount:default:myapp-sa           │
│     - Groups: [system:serviceaccounts, ...]                      │
│     - Verb: get                                                  │
│     - Resource: pods                                             │
│     - Namespace: default                                         │
│     - Name: mypod                                                │
│                                                                   │
│  2. 查找适用的 RoleBinding / ClusterRoleBinding:                │
│     - 匹配 subjects 中的 username 或 groups                      │
│     - 在请求的 namespace 中查找 RoleBinding                      │
│     - 在集群范围查找 ClusterRoleBinding                          │
│                                                                   │
│  3. 评估绑定的 Role / ClusterRole 规则:                          │
│     - 检查 apiGroups 是否匹配                                    │
│     - 检查 resources 是否匹配                                    │
│     - 检查 resourceNames 是否匹配 (如果指定)                     │
│     - 检查 verbs 是否匹配                                        │
│                                                                   │
│  4. 授权决策:                                                    │
│     - 找到至少一条匹配的规则 → Allow                             │
│     - 没有匹配的规则 → Deny (默认拒绝)                           │
│     - RBAC 是白名单机制,没有显式授权即拒绝                       │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  Admission Control (准入控制)                                    │
│  - 仅当授权通过后执行                                            │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  Execute Request (执行请求)                                      │
│  - 持久化到 etcd                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 RBAC Authorizer 配置

kube-apiserver 启动参数:

```bash
kube-apiserver \
  # 授权模式: Node (节点授权) + RBAC
  # 顺序很重要: Node 先于 RBAC
  --authorization-mode=Node,RBAC \
  # RBAC 策略源 (从 v1.8 起已废弃,现在总是从 API 加载)
  # --authorization-rbac-super-user=admin \
  ...
```

### 3.3 权限评估示例

**请求示例**:

```bash
kubectl get pods mypod -n default
```

**RBAC 评估过程**:

```yaml
# 1. 请求信息
Subject:
  Kind: User
  Name: jane
  Groups: [developers, system:authenticated]
Request:
  Verb: get
  Resource: pods
  Namespace: default
  Name: mypod

# 2. 查找 RoleBinding
RoleBinding: read-pods (namespace: default)
  Subjects:
    - Kind: User, Name: jane  # 匹配!
  RoleRef:
    Kind: Role, Name: pod-reader

# 3. 评估 Role 规则
Role: pod-reader (namespace: default)
  Rules:
    - apiGroups: [""]        # 匹配 (pods 在 core API 组)
      resources: ["pods"]    # 匹配
      verbs: ["get", "list", "watch"]  # 匹配 (get)
      # 没有 resourceNames,所以可以 get 任何 pod

# 4. 授权决策
Result: Allow (允许)
```

### 3.4 RBAC 缓存机制

RBAC Authorizer 使用缓存优化性能:

```
┌─────────────────────────────────────────────────────────────┐
│  RBAC Authorizer Cache                                       │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Role Cache (namespace → Role)                      │    │
│  │  - TTL: 5s (可配置)                                 │    │
│  │  - 自动刷新                                         │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  RoleBinding Cache (namespace → RoleBinding)        │    │
│  │  - TTL: 5s (可配置)                                 │    │
│  │  - 自动刷新                                         │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  ClusterRole Cache                                  │    │
│  │  - TTL: 5s (可配置)                                 │    │
│  │  - 自动刷新                                         │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  ClusterRoleBinding Cache                           │    │
│  │  - TTL: 5s (可配置)                                 │    │
│  │  - 自动刷新                                         │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘

注意: 更新 RBAC 资源后,最多需要 5 秒才能生效
```

---

## 4. 常用 API 资源的 rules 配置

### 4.1 核心资源 (core API group)

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: core-resources
  namespace: default
rules:
# Pods
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/log"         # Pod 日志
    - "pods/status"      # Pod 状态
    - "pods/exec"        # Pod exec
    - "pods/portforward" # Pod 端口转发
    - "pods/attach"      # Pod attach
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# Services
- apiGroups: [""]
  resources:
    - "services"
    - "services/status"
    - "services/proxy"   # Service 代理
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# ConfigMaps
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# Secrets
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# PersistentVolumeClaims
- apiGroups: [""]
  resources:
    - "persistentvolumeclaims"
    - "persistentvolumeclaims/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# ServiceAccounts
- apiGroups: [""]
  resources: ["serviceaccounts"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# Events
- apiGroups: [""]
  resources: ["events"]
  verbs: ["get", "list", "watch", "create", "patch"]
```

### 4.2 apps API group

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: apps-resources
  namespace: default
rules:
# Deployments
- apiGroups: ["apps"]
  resources:
    - "deployments"
    - "deployments/scale"      # Deployment 扩缩容
    - "deployments/status"     # Deployment 状态
    - "deployments/rollback"   # Deployment 回滚 (v1.7+已废弃)
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# StatefulSets
- apiGroups: ["apps"]
  resources:
    - "statefulsets"
    - "statefulsets/scale"
    - "statefulsets/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# DaemonSets
- apiGroups: ["apps"]
  resources:
    - "daemonsets"
    - "daemonsets/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# ReplicaSets
- apiGroups: ["apps"]
  resources:
    - "replicasets"
    - "replicasets/scale"
    - "replicasets/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
```

### 4.3 batch API group

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: batch-resources
  namespace: default
rules:
# Jobs
- apiGroups: ["batch"]
  resources:
    - "jobs"
    - "jobs/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# CronJobs
- apiGroups: ["batch"]
  resources:
    - "cronjobs"
    - "cronjobs/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
```

### 4.4 networking.k8s.io API group

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: networking-resources
  namespace: default
rules:
# Ingresses
- apiGroups: ["networking.k8s.io"]
  resources:
    - "ingresses"
    - "ingresses/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# NetworkPolicies
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

# IngressClasses
- apiGroups: ["networking.k8s.io"]
  resources: ["ingressclasses"]
  verbs: ["get", "list", "watch"]
```

### 4.5 autoscaling API group

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: autoscaling-resources
  namespace: default
rules:
# HorizontalPodAutoscaler (HPA)
- apiGroups: ["autoscaling"]
  resources:
    - "horizontalpodautoscalers"
    - "horizontalpodautoscalers/status"
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
```

---

## 5. 生产案例

### 5.1 案例 1: 开发者只读权限

**场景**: 开发团队成员需要查看 dev namespace 的资源,但不能修改。

```yaml
---
# Role: 开发者只读
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer-readonly
  namespace: dev
  labels:
    team: dev
    permission: readonly
  annotations:
    description: "开发者只读权限: 查看 Pod, Service, ConfigMap, Deployment 等"

rules:
# 规则 1: 查看 Pods 和日志
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/log"
    - "pods/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 查看 Services
- apiGroups: [""]
  resources: ["services"]
  verbs: ["get", "list", "watch"]

# 规则 3: 查看 ConfigMaps
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list", "watch"]

# 规则 4: 查看 Secrets (可选,根据安全策略决定)
# - apiGroups: [""]
#   resources: ["secrets"]
#   verbs: ["get", "list", "watch"]

# 规则 5: 查看 Deployments, StatefulSets, DaemonSets
- apiGroups: ["apps"]
  resources:
    - "deployments"
    - "deployments/status"
    - "deployments/scale"
    - "statefulsets"
    - "statefulsets/status"
    - "daemonsets"
    - "daemonsets/status"
    - "replicasets"
    - "replicasets/status"
  verbs: ["get", "list", "watch"]

# 规则 6: 查看 Jobs, CronJobs
- apiGroups: ["batch"]
  resources:
    - "jobs"
    - "jobs/status"
    - "cronjobs"
    - "cronjobs/status"
  verbs: ["get", "list", "watch"]

# 规则 7: 查看 Ingresses
- apiGroups: ["networking.k8s.io"]
  resources:
    - "ingresses"
    - "ingresses/status"
  verbs: ["get", "list", "watch"]

# 规则 8: 查看 HPA
- apiGroups: ["autoscaling"]
  resources: ["horizontalpodautoscalers"]
  verbs: ["get", "list", "watch"]

# 规则 9: 查看 Events
- apiGroups: [""]
  resources: ["events"]
  verbs: ["get", "list", "watch"]

# 规则 10: 查看 PVCs
- apiGroups: [""]
  resources: ["persistentvolumeclaims"]
  verbs: ["get", "list", "watch"]

---
# RoleBinding: 授予开发组
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-readonly-binding
  namespace: dev
  labels:
    team: dev
  annotations:
    description: "将只读权限授予开发团队"

subjects:
# 开发组 (由 OIDC/LDAP 提供)
- kind: Group
  name: developers
  apiGroup: rbac.authorization.k8s.io

# 或单个开发者用户
- kind: User
  name: alice@example.com
  apiGroup: rbac.authorization.k8s.io

- kind: User
  name: bob@example.com
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: Role
  name: developer-readonly
  apiGroup: rbac.authorization.k8s.io
```

**验证权限**:

```bash
# 以 alice 身份测试
kubectl auth can-i get pods -n dev --as alice@example.com
# yes

kubectl auth can-i delete pods -n dev --as alice@example.com
# no

kubectl auth can-i create deployments -n dev --as alice@example.com
# no

# 查看 alice 的所有权限
kubectl auth can-i --list -n dev --as alice@example.com
```

---

### 5.2 案例 2: 运维管理权限

**场景**: 运维团队需要管理 production namespace 的所有资源,但不能修改 RBAC。

```yaml
---
# Role: 运维管理员
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: ops-admin
  namespace: production
  labels:
    team: ops
    permission: admin
  annotations:
    description: "运维管理员: 管理所有应用资源,但不包括 RBAC"

rules:
# 规则 1: core API 组的所有资源
- apiGroups: [""]
  resources: ["*"]
  verbs: ["*"]

# 规则 2: apps API 组的所有资源
- apiGroups: ["apps"]
  resources: ["*"]
  verbs: ["*"]

# 规则 3: batch API 组的所有资源
- apiGroups: ["batch"]
  resources: ["*"]
  verbs: ["*"]

# 规则 4: networking.k8s.io API 组的所有资源
- apiGroups: ["networking.k8s.io"]
  resources: ["*"]
  verbs: ["*"]

# 规则 5: autoscaling API 组的所有资源
- apiGroups: ["autoscaling"]
  resources: ["*"]
  verbs: ["*"]

# 规则 6: policy API 组 (PodDisruptionBudget, PodSecurityPolicy)
- apiGroups: ["policy"]
  resources: ["*"]
  verbs: ["*"]

# 规则 7: storage.k8s.io API 组 (StorageClass, VolumeAttachment)
# 注意: StorageClass 是集群资源,Role 无法授权
# 需要使用 ClusterRole + ClusterRoleBinding
# - apiGroups: ["storage.k8s.io"]
#   resources: ["*"]
#   verbs: ["*"]

# 不包括 rbac.authorization.k8s.io API 组
# 防止运维人员修改 RBAC,导致权限升级

---
# RoleBinding: 授予运维组
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: ops-admin-binding
  namespace: production
  labels:
    team: ops
  annotations:
    description: "将管理员权限授予运维团队"

subjects:
# 运维组
- kind: Group
  name: ops-team
  apiGroup: rbac.authorization.k8s.io

# 运维 ServiceAccount (用于自动化工具)
- kind: ServiceAccount
  name: ops-automation-sa
  namespace: production

roleRef:
  kind: Role
  name: ops-admin
  apiGroup: rbac.authorization.k8s.io

---
# ServiceAccount: 运维自动化工具
apiVersion: v1
kind: ServiceAccount
metadata:
  name: ops-automation-sa
  namespace: production
  labels:
    team: ops
    purpose: automation
  annotations:
    description: "用于运维自动化脚本的 ServiceAccount"
automountServiceAccountToken: true
```

**验证权限**:

```bash
# 运维人员可以管理应用资源
kubectl auth can-i create deployments -n production --as ops-user
# yes

kubectl auth can-i delete pods -n production --as ops-user
# yes

# 运维人员不能修改 RBAC
kubectl auth can-i create roles -n production --as ops-user
# no

kubectl auth can-i create rolebindings -n production --as ops-user
# no
```

---

### 5.3 案例 3: CI/CD 部署权限

**场景**: GitLab CI/CD 需要部署应用到 production namespace,只能操作特定资源。

```yaml
---
# ServiceAccount: GitLab CI/CD
apiVersion: v1
kind: ServiceAccount
metadata:
  name: gitlab-ci-deployer
  namespace: production
  labels:
    app: gitlab-ci
    purpose: deployment
  annotations:
    description: "GitLab CI/CD 部署专用 ServiceAccount"
automountServiceAccountToken: false  # 使用短期 token
imagePullSecrets:
  - name: harbor-registry

---
# Role: CI/CD 部署权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: cicd-deployer
  namespace: production
  labels:
    app: gitlab-ci
    purpose: deployment
  annotations:
    description: "CI/CD 部署权限: 管理 Deployment, Service, ConfigMap, Secret, Ingress"

rules:
# 规则 1: 管理 Deployments
- apiGroups: ["apps"]
  resources:
    - "deployments"
    - "deployments/status"
    - "deployments/scale"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 2: 查看 ReplicaSets (Deployment 创建)
- apiGroups: ["apps"]
  resources:
    - "replicasets"
    - "replicasets/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 3: 管理 Services
- apiGroups: [""]
  resources: ["services"]
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 4: 管理 ConfigMaps
- apiGroups: [""]
  resources: ["configmaps"]
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 5: 管理 Secrets
- apiGroups: [""]
  resources: ["secrets"]
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 6: 管理 Ingresses
- apiGroups: ["networking.k8s.io"]
  resources:
    - "ingresses"
    - "ingresses/status"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 7: 管理 HPA
- apiGroups: ["autoscaling"]
  resources: ["horizontalpodautoscalers"]
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 8: 查看 Pods (用于验证部署)
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/log"
    - "pods/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 9: 查看 Events (用于调试)
- apiGroups: [""]
  resources: ["events"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 不允许:
# - 删除 Pods (防止误删)
# - 管理 StatefulSets, DaemonSets (需要额外审批)
# - 管理 RBAC (防止权限升级)

---
# RoleBinding: 授予 CI/CD ServiceAccount
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: cicd-deployer-binding
  namespace: production
  labels:
    app: gitlab-ci
  annotations:
    description: "授予 GitLab CI/CD 部署权限"

subjects:
- kind: ServiceAccount
  name: gitlab-ci-deployer
  namespace: production

roleRef:
  kind: Role
  name: cicd-deployer
  apiGroup: rbac.authorization.k8s.io
```

**GitLab CI 配置** (.gitlab-ci.yml):

```yaml
deploy:
  stage: deploy
  image: bitnami/kubectl:1.29
  script:
    # 获取短期 token (2小时)
    - export KUBE_TOKEN=$(kubectl create token gitlab-ci-deployer --namespace production --duration 2h)
    
    # 部署应用
    - kubectl set image deployment/myapp app=myapp:${CI_COMMIT_SHA} --token="$KUBE_TOKEN" -n production
    
    # 等待 rollout 完成
    - kubectl rollout status deployment/myapp --token="$KUBE_TOKEN" -n production --timeout=5m
    
    # 验证部署
    - kubectl get pods -l app=myapp -n production --token="$KUBE_TOKEN"
  only:
    - main
```

**验证权限**:

```bash
# CI/CD 可以更新 Deployment
kubectl auth can-i update deployments -n production --as system:serviceaccount:production:gitlab-ci-deployer
# yes

# CI/CD 不能删除 Pods
kubectl auth can-i delete pods -n production --as system:serviceaccount:production:gitlab-ci-deployer
# no

# CI/CD 不能管理 StatefulSets
kubectl auth can-i create statefulsets -n production --as system:serviceaccount:production:gitlab-ci-deployer
# no
```

---

### 5.4 案例 4: 应用 ServiceAccount 最小权限

**场景**: 应用需要访问 Kubernetes API 获取自己的 Pod 信息和配置。

```yaml
---
# ServiceAccount: 应用专用
apiVersion: v1
kind: ServiceAccount
metadata:
  name: myapp-sa
  namespace: default
  labels:
    app: myapp
  annotations:
    description: "myapp 应用专用 ServiceAccount"
automountServiceAccountToken: true

---
# Role: 应用最小权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: myapp-minimal
  namespace: default
  labels:
    app: myapp
  annotations:
    description: "myapp 最小权限: 只能读取自己的 Pod 和 ConfigMap"

rules:
# 规则 1: 读取 Pods (用于服务发现)
- apiGroups: [""]
  resources: ["pods"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 读取特定的 ConfigMap
- apiGroups: [""]
  resources: ["configmaps"]
  resourceNames:
    - "myapp-config"      # 只能读取 myapp-config
    - "myapp-feature-flags"  # 只能读取 myapp-feature-flags
  verbs:
    - "get"

# 规则 3: 列出 ConfigMaps (用于发现)
- apiGroups: [""]
  resources: ["configmaps"]
  verbs:
    - "list"
    - "watch"

# 规则 4: 读取特定的 Secret
- apiGroups: [""]
  resources: ["secrets"]
  resourceNames:
    - "myapp-db-credentials"  # 只能读取 DB 凭证
  verbs:
    - "get"

# 规则 5: 创建 Events (用于应用日志)
- apiGroups: [""]
  resources: ["events"]
  verbs:
    - "create"
    - "patch"

---
# RoleBinding: 授予应用 ServiceAccount
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: myapp-minimal-binding
  namespace: default
  labels:
    app: myapp
  annotations:
    description: "授予 myapp 最小权限"

subjects:
- kind: ServiceAccount
  name: myapp-sa
  namespace: default

roleRef:
  kind: Role
  name: myapp-minimal
  apiGroup: rbac.authorization.k8s.io

---
# Deployment: 使用 ServiceAccount
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      serviceAccountName: myapp-sa
      containers:
      - name: myapp
        image: myapp:v1.0
        ports:
        - containerPort: 8080
        env:
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
```

**应用代码示例** (Go):

```go
package main

import (
    "context"
    "fmt"
    "os"
    
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/client-go/kubernetes"
    "k8s.io/client-go/rest"
)

func main() {
    // 使用 in-cluster config (ServiceAccount token)
    config, err := rest.InClusterConfig()
    if err != nil {
        panic(err)
    }
    
    clientset, err := kubernetes.NewForConfig(config)
    if err != nil {
        panic(err)
    }
    
    namespace := os.Getenv("POD_NAMESPACE")
    
    // 读取自己的 Pod 信息
    podName := os.Getenv("POD_NAME")
    pod, err := clientset.CoreV1().Pods(namespace).Get(context.TODO(), podName, metav1.GetOptions{})
    if err != nil {
        panic(err)
    }
    fmt.Printf("Pod IP: %s\n", pod.Status.PodIP)
    
    // 读取 ConfigMap
    cm, err := clientset.CoreV1().ConfigMaps(namespace).Get(context.TODO(), "myapp-config", metav1.GetOptions{})
    if err != nil {
        panic(err)
    }
    fmt.Printf("Config: %v\n", cm.Data)
    
    // 读取 Secret
    secret, err := clientset.CoreV1().Secrets(namespace).Get(context.TODO(), "myapp-db-credentials", metav1.GetOptions{})
    if err != nil {
        panic(err)
    }
    fmt.Printf("DB Password: %s\n", string(secret.Data["password"]))
}
```

---

### 5.5 案例 5: 多租户隔离

**场景**: SaaS 平台,每个租户有独立的 namespace,租户之间完全隔离。

```yaml
---
# Namespace: 租户 A
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: tenant-a
    environment: production

---
# ServiceAccount: 租户 A 管理员
apiVersion: v1
kind: ServiceAccount
metadata:
  name: tenant-a-admin
  namespace: tenant-a
  labels:
    tenant: tenant-a
    role: admin
automountServiceAccountToken: false

---
# Role: 租户管理员权限 (namespace 内完整权限)
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-admin
  namespace: tenant-a
  labels:
    tenant: tenant-a
  annotations:
    description: "租户管理员: namespace 内完整权限,但不包括 RBAC"

rules:
# 所有 core API 资源
- apiGroups: [""]
  resources: ["*"]
  verbs: ["*"]

# apps API 组
- apiGroups: ["apps"]
  resources: ["*"]
  verbs: ["*"]

# batch API 组
- apiGroups: ["batch"]
  resources: ["*"]
  verbs: ["*"]

# networking.k8s.io API 组
- apiGroups: ["networking.k8s.io"]
  resources: ["*"]
  verbs: ["*"]

# autoscaling API 组
- apiGroups: ["autoscaling"]
  resources: ["*"]
  verbs: ["*"]

# policy API 组
- apiGroups: ["policy"]
  resources: ["*"]
  verbs: ["*"]

# 不包括 rbac.authorization.k8s.io
# 租户不能修改自己的权限

---
# RoleBinding: 授予租户管理员
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-a-admin-binding
  namespace: tenant-a
  labels:
    tenant: tenant-a
  annotations:
    description: "授予租户 A 管理员权限"

subjects:
# 租户 A 的管理员用户
- kind: User
  name: admin@tenant-a.com
  apiGroup: rbac.authorization.k8s.io

# 租户 A 的 ServiceAccount
- kind: ServiceAccount
  name: tenant-a-admin
  namespace: tenant-a

roleRef:
  kind: Role
  name: tenant-admin
  apiGroup: rbac.authorization.k8s.io

---
# ResourceQuota: 限制租户资源使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a
  labels:
    tenant: tenant-a
spec:
  hard:
    requests.cpu: "10"
    requests.memory: "20Gi"
    limits.cpu: "20"
    limits.memory: "40Gi"
    persistentvolumeclaims: "10"
    services.loadbalancers: "2"
    pods: "50"

---
# LimitRange: 限制单个资源的大小
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-a-limits
  namespace: tenant-a
  labels:
    tenant: tenant-a
spec:
  limits:
  - max:
      cpu: "2"
      memory: "4Gi"
    min:
      cpu: "50m"
      memory: "64Mi"
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    type: Container
  - max:
      cpu: "4"
      memory: "8Gi"
    type: Pod

---
# NetworkPolicy: 租户网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a
  labels:
    tenant: tenant-a
spec:
  podSelector: {}  # 应用于 namespace 内所有 Pods
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许来自同一 namespace 的流量
  - from:
    - podSelector: {}
  # 允许来自 ingress controller 的流量
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
  egress:
  # 允许访问同一 namespace 的 Pods
  - to:
    - podSelector: {}
  # 允许访问 kube-dns
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    - podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
  # 允许访问外部网络 (HTTP/HTTPS)
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
```

**租户隔离验证**:

```bash
# 租户 A 管理员可以管理自己的 namespace
kubectl auth can-i create deployments -n tenant-a --as admin@tenant-a.com
# yes

# 租户 A 管理员不能访问其他 namespace
kubectl auth can-i get pods -n tenant-b --as admin@tenant-a.com
# no

# 租户 A 管理员不能修改 RBAC
kubectl auth can-i create roles -n tenant-a --as admin@tenant-a.com
# no

# 测试网络隔离
kubectl run test-pod --image=nginx -n tenant-a
kubectl exec -it test-pod -n tenant-a -- curl http://service.tenant-b.svc.cluster.local
# 应该失败 (network policy 阻止)
```

---

## 6. 最佳实践

### 6.1 权限设计原则

1. **最小权限原则 (Principle of Least Privilege)**:
   - 只授予完成任务所需的最小权限
   - 避免使用通配符 `*` (除非确实需要)
   - 使用 `resourceNames` 限制特定资源

2. **职责分离 (Separation of Duties)**:
   - 区分只读、编辑、管理员角色
   - 开发和生产环境使用不同的权限
   - 应用、运维、安全团队分离

3. **禁止权限升级**:
   - 不要授予 RBAC 资源的管理权限 (除非平台管理员)
   - 使用 `escalate` verb 控制权限升级 (v1.12+)

4. **定期审计**:
   - 定期审查 RoleBinding
   - 清理不再使用的 ServiceAccount
   - 监控异常的 API 访问

### 6.2 RBAC 配置建议

```yaml
# 好的实践
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: good-practice
  namespace: default
rules:
# 明确指定资源和动词
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "update", "patch"]

# 使用 resourceNames 限制
- apiGroups: [""]
  resources: ["secrets"]
  resourceNames: ["app-config"]
  verbs: ["get"]

---
# 不好的实践 (避免)
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: bad-practice
  namespace: default
rules:
# 过度使用通配符
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]

# 授予 RBAC 权限 (权限升级风险)
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["*"]
  verbs: ["*"]
```

### 6.3 验证权限

```bash
# 1. 检查当前用户权限
kubectl auth can-i create deployments -n default

# 2. 检查其他用户/ServiceAccount 权限
kubectl auth can-i get pods -n default --as alice@example.com
kubectl auth can-i list secrets -n default --as system:serviceaccount:default:myapp-sa

# 3. 列出所有权限
kubectl auth can-i --list -n default
kubectl auth can-i --list -n default --as alice@example.com

# 4. 模拟用户操作
kubectl get pods -n default --as alice@example.com
kubectl get pods -n default --as system:serviceaccount:default:myapp-sa

# 5. 查看 RoleBinding
kubectl get rolebindings -n default
kubectl describe rolebinding read-pods -n default

# 6. 查看 Role
kubectl get roles -n default
kubectl describe role pod-reader -n default
```

### 6.4 RBAC 审计

启用 Audit Logging 记录 RBAC 操作:

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 记录 RBAC 资源的所有操作
- level: RequestResponse
  verbs: ["create", "update", "patch", "delete"]
  resources:
  - group: rbac.authorization.k8s.io
    resources:
      - roles
      - rolebindings
      - clusterroles
      - clusterrolebindings

# 记录授权失败
- level: Request
  omitStages:
    - RequestReceived
  users: ["system:anonymous"]
  verbs: ["get", "list", "watch"]

# 记录 Secret 访问
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets"]
```

---

## 7. 常见问题排查

### 7.1 权限拒绝 (Forbidden)

**症状**:
```
Error from server (Forbidden): pods is forbidden: User "alice" cannot list resource "pods" in API group "" in the namespace "default"
```

**排查步骤**:

```bash
# 1. 验证用户身份
kubectl auth whoami

# 2. 检查是否有 RoleBinding
kubectl get rolebindings -n default -o wide

# 3. 检查 RoleBinding 的 subjects
kubectl get rolebinding <binding-name> -n default -o yaml

# 4. 检查 Role 的 rules
kubectl get role <role-name> -n default -o yaml

# 5. 模拟用户权限
kubectl auth can-i list pods -n default --as alice

# 6. 查看审计日志
# 在 kube-apiserver 日志中搜索 "forbidden"
kubectl logs -n kube-system kube-apiserver-master | grep -i forbidden
```

### 7.2 ServiceAccount 无权限

**症状**:
```
Error from server (Forbidden): pods is forbidden: User "system:serviceaccount:default:myapp-sa" cannot list resource "pods"
```

**排查步骤**:

```bash
# 1. 检查 ServiceAccount 是否存在
kubectl get sa myapp-sa -n default

# 2. 检查 RoleBinding
kubectl get rolebindings -n default | grep myapp-sa

# 3. 检查 RoleBinding 详情
kubectl describe rolebinding <binding-name> -n default

# 4. 验证 ServiceAccount 权限
kubectl auth can-i list pods -n default --as system:serviceaccount:default:myapp-sa

# 5. 检查 Pod 使用的 ServiceAccount
kubectl get pod <pod-name> -n default -o jsonpath='{.spec.serviceAccountName}'

# 6. 检查 Pod 的 token
kubectl exec <pod-name> -n default -- cat /var/run/secrets/kubernetes.io/serviceaccount/token
```

### 7.3 RBAC 更新不生效

**症状**: 更新 Role/RoleBinding 后,权限未生效。

**原因**: RBAC Authorizer 有缓存 (默认 5 秒 TTL)。

**解决方法**:

```bash
# 1. 等待 5-10 秒
sleep 10

# 2. 强制刷新: 重新创建 RoleBinding
kubectl delete rolebinding <binding-name> -n default
kubectl apply -f rolebinding.yaml

# 3. 重启 kube-apiserver (不推荐,仅测试环境)
# kubectl delete pod -n kube-system -l component=kube-apiserver
```

---

## 8. 参考资料

- [Kubernetes RBAC 官方文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [Using RBAC Authorization](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [Kubernetes API Reference - Role](https://kubernetes.io/docs/reference/kubernetes-api/authorization-resources/role-v1/)
- [Kubernetes API Reference - RoleBinding](https://kubernetes.io/docs/reference/kubernetes-api/authorization-resources/role-binding-v1/)

---

**文档版本**: v1.0  
**最后更新**: 2026-02  
**维护者**: Kubernetes 中文社区  
**适用版本**: Kubernetes v1.25 - v1.32
