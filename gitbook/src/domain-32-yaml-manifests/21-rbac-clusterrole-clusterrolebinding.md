# 21 - ClusterRole / ClusterRoleBinding YAML 配置参考

## 概述

ClusterRole 和 ClusterRoleBinding 是 Kubernetes RBAC 的**集群级别**资源,用于授予跨 namespace 或集群资源的权限。与 Role/RoleBinding 不同,ClusterRole 可以授权访问集群范围的资源(如 Node、PersistentVolume)和非资源 URL(如 /healthz)。本文档覆盖 ClusterRole、ClusterRoleBinding、内建角色、权限审查 API 的完整配置。

**适用版本**: Kubernetes v1.25 - v1.32  
**更新时间**: 2026-02

---

## 1. ClusterRole 基础配置

### 1.1 基本 ClusterRole

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  # ClusterRole 名称 (集群唯一)
  name: cluster-pod-reader
  labels:
    app: monitoring
    purpose: readonly
  annotations:
    description: "集群级别的 Pod 只读权限"
  # ClusterRole 没有 namespace 字段

# rules 定义权限规则列表
rules:
# 规则 1: 读取所有 namespace 的 Pods
- apiGroups:
    # "" 表示 core API 组
    - ""
  resources:
    - "pods"
    - "pods/log"
    - "pods/status"
  verbs:
    - "get"      # 获取单个 Pod
    - "list"     # 列出所有 Pods
    - "watch"    # 监听 Pod 变化

# 规则 2: 读取所有 namespace 的 Services
- apiGroups: [""]
  resources: ["services"]
  verbs: ["get", "list", "watch"]
```

### 1.2 访问集群资源的 ClusterRole

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-resource-reader
  labels:
    purpose: cluster-resources
  annotations:
    description: "读取集群级别资源 (Node, PV, StorageClass)"

rules:
# 规则 1: 读取 Nodes
- apiGroups: [""]
  resources:
    - "nodes"
    - "nodes/status"
    - "nodes/proxy"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 读取 PersistentVolumes
- apiGroups: [""]
  resources:
    - "persistentvolumes"
    - "persistentvolumes/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 3: 读取 StorageClasses
- apiGroups: ["storage.k8s.io"]
  resources: ["storageclasses"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 4: 读取 Namespaces
- apiGroups: [""]
  resources: ["namespaces"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 5: 读取 ClusterRoles 和 ClusterRoleBindings
- apiGroups: ["rbac.authorization.k8s.io"]
  resources:
    - "clusterroles"
    - "clusterrolebindings"
  verbs:
    - "get"
    - "list"
    - "watch"
```

### 1.3 访问非资源 URL 的 ClusterRole

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-health-checker
  labels:
    purpose: monitoring
  annotations:
    description: "访问集群健康检查和指标 API"

rules:
# 规则 1: 访问健康检查 URLs
- nonResourceURLs:
    # /healthz 及其子路径
    - "/healthz"
    - "/healthz/*"
    # /livez (v1.16+)
    - "/livez"
    - "/livez/*"
    # /readyz (v1.16+)
    - "/readyz"
    - "/readyz/*"
  verbs:
    - "get"

# 规则 2: 访问 metrics API
- nonResourceURLs:
    # /metrics (kube-apiserver 指标)
    - "/metrics"
    # /metrics/cadvisor (容器指标)
    - "/metrics/cadvisor"
  verbs:
    - "get"

# 规则 3: 访问 version API
- nonResourceURLs:
    - "/version"
  verbs:
    - "get"

# 注意: nonResourceURLs 只能用于 ClusterRole,不能用于 Role
# nonResourceURLs 必须以 / 开头
# 支持通配符 * 表示子路径
```

### 1.4 使用 aggregationRule 聚合 ClusterRole

```yaml
---
# 父 ClusterRole: 使用 aggregationRule 聚合
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-aggregate
  labels:
    app: monitoring
  annotations:
    description: "聚合所有带有 rbac.example.com/aggregate-to-monitoring=true 标签的 ClusterRole"

# aggregationRule 定义聚合规则
# 使用标签选择器聚合其他 ClusterRole 的 rules
aggregationRule:
  clusterRoleSelectors:
  # 选择所有带有此标签的 ClusterRole
  - matchLabels:
      rbac.example.com/aggregate-to-monitoring: "true"

# rules 字段由控制器自动填充 (只读)
# 不要手动编辑 rules,它们会被覆盖
rules: []

---
# 子 ClusterRole 1: Pod 读取权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-pods
  labels:
    # 此标签使其被聚合到 monitoring-aggregate
    rbac.example.com/aggregate-to-monitoring: "true"
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "pods/status"]
  verbs: ["get", "list", "watch"]

---
# 子 ClusterRole 2: Service 读取权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-services
  labels:
    # 此标签使其被聚合到 monitoring-aggregate
    rbac.example.com/aggregate-to-monitoring: "true"
rules:
- apiGroups: [""]
  resources: ["services", "endpoints"]
  verbs: ["get", "list", "watch"]

---
# 子 ClusterRole 3: Node 读取权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: monitoring-nodes
  labels:
    # 此标签使其被聚合到 monitoring-aggregate
    rbac.example.com/aggregate-to-monitoring: "true"
rules:
- apiGroups: [""]
  resources: ["nodes", "nodes/status"]
  verbs: ["get", "list", "watch"]

# 最终 monitoring-aggregate 的 rules 包含:
# - monitoring-pods 的 rules
# - monitoring-services 的 rules
# - monitoring-nodes 的 rules
```

---

## 2. ClusterRoleBinding 基础配置

### 2.1 基本 ClusterRoleBinding

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  # ClusterRoleBinding 名称 (集群唯一)
  name: read-pods-global
  labels:
    app: monitoring
  annotations:
    description: "授予 jane 用户读取所有 namespace 的 Pods 权限"
  # ClusterRoleBinding 没有 namespace 字段

# subjects 定义权限的接收者 (被授权者)
subjects:
# Subject 可以是 User, Group 或 ServiceAccount
- kind: User
  name: jane
  # apiGroup 对于 User 和 Group 必须是 rbac.authorization.k8s.io
  apiGroup: rbac.authorization.k8s.io

# roleRef 定义要绑定的 ClusterRole
roleRef:
  # kind 必须是 ClusterRole
  # ClusterRoleBinding 不能引用 Role
  kind: ClusterRole
  name: cluster-pod-reader
  # apiGroup 必须是 rbac.authorization.k8s.io
  apiGroup: rbac.authorization.k8s.io
```

### 2.2 授权给 ServiceAccount

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: monitoring-sa-global
  labels:
    app: prometheus
  annotations:
    description: "授予 Prometheus ServiceAccount 集群级别的监控权限"

subjects:
# ServiceAccount 作为 subject
- kind: ServiceAccount
  # ServiceAccount 名称
  name: prometheus
  # ServiceAccount 所在的 namespace (必须指定)
  namespace: monitoring
  # ServiceAccount 的 apiGroup 必须是 ""

roleRef:
  kind: ClusterRole
  name: monitoring-aggregate
  apiGroup: rbac.authorization.k8s.io
```

### 2.3 授权给多个 Subjects

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-admins
  labels:
    role: admin
  annotations:
    description: "授予平台管理员集群管理员权限"

subjects:
# Subject 1: 管理员用户
- kind: User
  name: admin@example.com
  apiGroup: rbac.authorization.k8s.io

# Subject 2: 管理员组
- kind: Group
  # Group 名称由身份认证系统提供
  # 例如: OIDC 的 groups claim, x509 证书的 O 字段
  name: cluster-admins
  apiGroup: rbac.authorization.k8s.io

# Subject 3: 管理员 ServiceAccount
- kind: ServiceAccount
  name: admin-sa
  namespace: kube-system

# Subject 4: 所有认证用户 (不推荐用于高权限角色)
# - kind: Group
#   name: system:authenticated
#   apiGroup: rbac.authorization.k8s.io

# Subject 5: 所有 ServiceAccounts (非常危险,不推荐)
# - kind: Group
#   name: system:serviceaccounts
#   apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io
```

### 2.4 授权特定 Namespace 的 ServiceAccount 访问集群资源

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: node-reader-for-monitoring
  labels:
    app: monitoring
  annotations:
    description: "允许 monitoring namespace 的 node-exporter SA 读取 Node 信息"

subjects:
# 只授权 monitoring namespace 的 node-exporter ServiceAccount
- kind: ServiceAccount
  name: node-exporter
  namespace: monitoring

roleRef:
  kind: ClusterRole
  name: cluster-resource-reader
  apiGroup: rbac.authorization.k8s.io
```

---

## 3. 内建 ClusterRole

Kubernetes 预定义了一些内建 ClusterRole,位于 `kube-system` namespace。

### 3.1 cluster-admin (超级管理员)

```yaml
# 预定义的 cluster-admin ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-admin
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  annotations:
    rbac.authorization.kubernetes.io/autoupdate: "true"
    description: "超级管理员权限,可以执行任何操作"

rules:
# 规则 1: 所有 API 组的所有资源的所有操作
- apiGroups:
    - "*"  # 所有 API 组
  resources:
    - "*"  # 所有资源
  verbs:
    - "*"  # 所有动词

# 规则 2: 所有非资源 URL
- nonResourceURLs:
    - "*"  # 所有非资源 URL
  verbs:
    - "*"
```

**使用场景**:
- 集群初始化和引导
- 平台管理员
- 紧急故障排查

**安全警告**: 
- ⚠️ 此角色拥有完全的集群控制权
- ⚠️ 可以修改 RBAC 配置,导致权限升级
- ⚠️ 可以访问所有 Secret,包括 ServiceAccount token
- ⚠️ 谨慎授予,建议使用审计和 MFA

```bash
# 授予用户 cluster-admin 权限
kubectl create clusterrolebinding admin-binding \
  --clusterrole=cluster-admin \
  --user=admin@example.com
```

---

### 3.2 admin (Namespace 管理员)

```yaml
# 预定义的 admin ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: admin
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  annotations:
    rbac.authorization.kubernetes.io/autoupdate: "true"
    description: "Namespace 管理员权限,可以管理 namespace 内的大部分资源"

rules:
# 规则 1: 读写大部分资源
- apiGroups:
    - ""
    - "apps"
    - "batch"
    - "extensions"
    - "networking.k8s.io"
    - "policy"
    - "autoscaling"
  resources:
    - "*"
  verbs:
    - "*"

# 规则 2: 可以读取和修改 Roles 和 RoleBindings
- apiGroups:
    - "rbac.authorization.k8s.io"
  resources:
    - "roles"
    - "rolebindings"
  verbs:
    - "*"

# 注意: admin 不能修改 ResourceQuota 和 LimitRange
# 不能修改 Namespace 本身
```

**使用场景**:
- Namespace 所有者
- 团队管理员
- 需要管理 RBAC 的用户

**限制**:
- 只能通过 RoleBinding 授予 (namespace 范围)
- 不能修改 ResourceQuota、LimitRange
- 不能删除 Namespace

```bash
# 授予用户 namespace 管理员权限
kubectl create rolebinding namespace-admin \
  --clusterrole=admin \
  --user=alice@example.com \
  --namespace=production
```

---

### 3.3 edit (编辑者)

```yaml
# 预定义的 edit ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: edit
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  annotations:
    rbac.authorization.kubernetes.io/autoupdate: "true"
    description: "允许读写大部分资源,但不能修改 RBAC"

rules:
# 规则 1: 读写大部分资源 (与 admin 类似)
- apiGroups:
    - ""
    - "apps"
    - "batch"
    - "extensions"
    - "networking.k8s.io"
    - "policy"
    - "autoscaling"
  resources:
    - "*"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# 规则 2: 可以读取 Secrets (但不能创建 ServiceAccount token)
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

# 注意: edit 不能修改 Roles 和 RoleBindings
# 不能修改 ResourceQuota 和 LimitRange
```

**使用场景**:
- 开发者
- 运维人员
- CI/CD 系统

**限制**:
- 不能修改 RBAC (Roles, RoleBindings)
- 不能修改 ResourceQuota、LimitRange
- 不能创建 ServiceAccount token (v1.24+)

```bash
# 授予用户编辑权限
kubectl create rolebinding developer \
  --clusterrole=edit \
  --user=dev@example.com \
  --namespace=dev
```

---

### 3.4 view (查看者)

```yaml
# 预定义的 view ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: view
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  annotations:
    rbac.authorization.kubernetes.io/autoupdate: "true"
    description: "只读权限,可以查看大部分资源"

rules:
# 规则 1: 只读大部分资源
- apiGroups:
    - ""
    - "apps"
    - "batch"
    - "extensions"
    - "networking.k8s.io"
    - "policy"
    - "autoscaling"
  resources:
    - "*"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 可以读取 ConfigMaps
- apiGroups: [""]
  resources: ["configmaps"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 注意: view 不能读取 Secrets (安全考虑)
# 不能读取 Roles 和 RoleBindings
```

**使用场景**:
- 只读用户
- 监控系统
- 审计人员
- 实习生/临时员工

**限制**:
- 不能读取 Secrets
- 不能读取 RBAC 资源
- 不能修改任何资源

```bash
# 授予用户只读权限
kubectl create rolebinding readonly-user \
  --clusterrole=view \
  --user=viewer@example.com \
  --namespace=production
```

---

### 3.5 内建 ClusterRole 对比表

| ClusterRole | 权限范围 | Secrets | RBAC | ResourceQuota | 使用场景 |
|-------------|----------|---------|------|---------------|----------|
| **cluster-admin** | 所有资源 + 非资源 URL | ✅ 读写 | ✅ 读写 | ✅ 读写 | 超级管理员 |
| **admin** | Namespace 内大部分资源 | ✅ 读写 | ✅ 读写 Roles/RoleBindings | ❌ 只读 | Namespace 管理员 |
| **edit** | Namespace 内大部分资源 | ✅ 读写 | ❌ 只读 | ❌ 只读 | 开发者/运维 |
| **view** | Namespace 内大部分资源 | ❌ 无权限 | ❌ 无权限 | ❌ 只读 | 只读用户 |

---

### 3.6 其他常用内建 ClusterRole

```yaml
# system:node (Kubelet 权限)
# 用于 Kubelet 访问 API server
kind: ClusterRole
metadata:
  name: system:node
rules:
- apiGroups: [""]
  resources: ["nodes", "nodes/status"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: [""]
  resources: ["pods", "pods/status"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["events"]
  verbs: ["create", "patch", "update"]

---
# system:kube-controller-manager (控制器管理器权限)
kind: ClusterRole
metadata:
  name: system:kube-controller-manager
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]

---
# system:kube-scheduler (调度器权限)
kind: ClusterRole
metadata:
  name: system:kube-scheduler
rules:
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch", "update"]
- apiGroups: [""]
  resources: ["bindings", "pods/binding"]
  verbs: ["create"]

---
# system:kube-proxy (kube-proxy 权限)
kind: ClusterRole
metadata:
  name: system:kube-proxy
rules:
- apiGroups: [""]
  resources: ["services", "endpoints"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get"]
```

---

## 4. SubjectAccessReview (权限审查 API)

### 4.1 SubjectAccessReview (检查其他用户权限)

```yaml
apiVersion: authorization.k8s.io/v1
kind: SubjectAccessReview
metadata:
  # SubjectAccessReview 是无状态的,不需要 name
  creationTimestamp: null
spec:
  # resourceAttributes 定义要检查的资源访问权限
  resourceAttributes:
    # namespace 指定资源所在的 namespace
    # 如果是集群资源,省略此字段
    namespace: default
    
    # verb 指定操作类型
    verb: get
    
    # group 指定 API 组
    # "" 表示 core API 组
    group: ""
    
    # resource 指定资源类型
    resource: pods
    
    # subresource 指定子资源 (可选)
    # subresource: log
    
    # name 指定资源名称 (可选)
    # 用于检查对特定资源的访问权限
    name: mypod
    
    # version 指定 API 版本 (可选)
    # version: v1
  
  # user 指定要检查的用户
  user: alice@example.com
  
  # groups 指定用户所属的组 (可选)
  groups:
    - developers
    - system:authenticated
  
  # uid 指定用户 UID (可选)
  # uid: "12345678-1234-1234-1234-123456789abc"
  
  # extra 包含额外的用户属性 (可选)
  # extra:
  #   scopes:
  #     - "email"
  #     - "profile"

# status 字段由 API server 填充 (只读)
status:
  # allowed 表示是否允许访问
  allowed: true
  
  # denied 表示是否明确拒绝 (可选)
  # denied: false
  
  # reason 包含允许或拒绝的原因 (可选)
  reason: "RBAC: allowed by ClusterRoleBinding 'read-pods-global' of ClusterRole 'cluster-pod-reader' to User 'alice@example.com'"
  
  # evaluationError 包含评估错误信息 (可选)
  # evaluationError: ""
```

**使用 kubectl 检查权限**:

```bash
# 检查用户是否可以执行操作
kubectl auth can-i get pods -n default --as alice@example.com
# yes

# 检查 ServiceAccount 权限
kubectl auth can-i list nodes --as system:serviceaccount:monitoring:prometheus
# yes

# 使用 kubectl create 提交 SubjectAccessReview
kubectl create -f - <<EOF
apiVersion: authorization.k8s.io/v1
kind: SubjectAccessReview
spec:
  resourceAttributes:
    namespace: default
    verb: get
    resource: pods
    name: mypod
  user: alice@example.com
  groups:
    - developers
EOF
```

---

### 4.2 SelfSubjectAccessReview (检查自己的权限)

```yaml
apiVersion: authorization.k8s.io/v1
kind: SelfSubjectAccessReview
metadata:
  creationTimestamp: null
spec:
  # resourceAttributes 定义要检查的资源访问权限
  resourceAttributes:
    namespace: default
    verb: delete
    group: ""
    resource: pods
    name: mypod
  
  # 不需要指定 user, groups, uid
  # API server 会从当前请求的身份中提取

# status 字段由 API server 填充 (只读)
status:
  allowed: false
  reason: "RBAC: denied by default"
```

**使用 kubectl 检查自己的权限**:

```bash
# 检查当前用户是否可以执行操作
kubectl auth can-i create deployments -n production
# no

kubectl auth can-i get pods --all-namespaces
# yes

# 列出当前用户的所有权限
kubectl auth can-i --list -n default

# 输出示例:
# Resources                                       Non-Resource URLs   Resource Names   Verbs
# selfsubjectaccessreviews.authorization.k8s.io   []                  []               [create]
# selfsubjectrulesreviews.authorization.k8s.io    []                  []               [create]
# pods                                            []                  []               [get list watch]
# services                                        []                  []               [get list watch]
# configmaps                                      []                  [app-config]     [get]
```

---

### 4.3 SelfSubjectRulesReview (列出自己的所有权限)

```yaml
apiVersion: authorization.k8s.io/v1
kind: SelfSubjectRulesReview
metadata:
  creationTimestamp: null
spec:
  # namespace 指定要检查的 namespace
  # 如果省略,返回集群级别的权限
  namespace: default

# status 字段由 API server 填充 (只读)
status:
  # resourceRules 列出所有资源权限
  resourceRules:
  # 规则 1: Pods
  - apiGroups:
      - ""
    resources:
      - "pods"
      - "pods/log"
      - "pods/status"
    verbs:
      - "get"
      - "list"
      - "watch"
  
  # 规则 2: ConfigMaps (限制特定名称)
  - apiGroups:
      - ""
    resources:
      - "configmaps"
    resourceNames:
      - "app-config"
    verbs:
      - "get"
  
  # 规则 3: Services
  - apiGroups:
      - ""
    resources:
      - "services"
    verbs:
      - "get"
      - "list"
      - "watch"
  
  # nonResourceRules 列出所有非资源 URL 权限
  nonResourceRules:
  - nonResourceURLs:
      - "/healthz"
      - "/healthz/*"
    verbs:
      - "get"
  
  # incomplete 表示权限列表是否完整
  # true: 权限太多,列表被截断
  # false: 列表完整
  incomplete: false
```

**使用 kubectl 列出自己的权限**:

```bash
# 列出当前用户在 default namespace 的所有权限
kubectl auth can-i --list -n default

# 列出集群级别的权限
kubectl auth can-i --list
```

---

### 4.4 LocalSubjectAccessReview (Namespace 范围的权限检查)

```yaml
apiVersion: authorization.k8s.io/v1
kind: LocalSubjectAccessReview
metadata:
  # LocalSubjectAccessReview 必须指定 namespace
  namespace: default
spec:
  # resourceAttributes 定义要检查的资源访问权限
  resourceAttributes:
    # namespace 字段被忽略,使用 metadata.namespace
    verb: delete
    group: ""
    resource: pods
    name: mypod
  
  # user 指定要检查的用户
  user: bob@example.com
  
  # groups 指定用户所属的组
  groups:
    - ops-team
    - system:authenticated

# status 字段由 API server 填充 (只读)
status:
  allowed: true
  reason: "RBAC: allowed by RoleBinding 'ops-admin-binding' of Role 'ops-admin' to Group 'ops-team'"
```

---

## 5. 生产案例

### 5.1 案例 1: 集群级别监控系统权限

**场景**: Prometheus 需要监控整个集群的资源和指标。

```yaml
---
# Namespace: 监控系统
apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
  labels:
    name: monitoring

---
# ServiceAccount: Prometheus
apiVersion: v1
kind: ServiceAccount
metadata:
  name: prometheus
  namespace: monitoring
  labels:
    app: prometheus
  annotations:
    description: "Prometheus 监控系统 ServiceAccount"
automountServiceAccountToken: true

---
# ClusterRole: Prometheus 监控权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: prometheus-monitoring
  labels:
    app: prometheus
  annotations:
    description: "Prometheus 集群监控权限: 读取所有资源和指标"

rules:
# 规则 1: 读取所有 namespace 的 Pods
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 读取所有 namespace 的 Services 和 Endpoints
- apiGroups: [""]
  resources:
    - "services"
    - "endpoints"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 3: 读取 Nodes
- apiGroups: [""]
  resources:
    - "nodes"
    - "nodes/proxy"
    - "nodes/metrics"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 4: 读取 Ingresses
- apiGroups: ["networking.k8s.io", "extensions"]
  resources:
    - "ingresses"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 5: 访问 metrics API
- nonResourceURLs:
    - "/metrics"
    - "/metrics/cadvisor"
  verbs:
    - "get"

# 规则 6: 读取 ConfigMaps (用于服务发现)
- apiGroups: [""]
  resources:
    - "configmaps"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 7: 读取 Namespaces
- apiGroups: [""]
  resources:
    - "namespaces"
  verbs:
    - "get"
    - "list"
    - "watch"

---
# ClusterRoleBinding: 授予 Prometheus ServiceAccount
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: prometheus-monitoring-binding
  labels:
    app: prometheus
  annotations:
    description: "授予 Prometheus 集群监控权限"

subjects:
- kind: ServiceAccount
  name: prometheus
  namespace: monitoring

roleRef:
  kind: ClusterRole
  name: prometheus-monitoring
  apiGroup: rbac.authorization.k8s.io

---
# Deployment: Prometheus
apiVersion: apps/v1
kind: Deployment
metadata:
  name: prometheus
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: prometheus
  template:
    metadata:
      labels:
        app: prometheus
    spec:
      serviceAccountName: prometheus
      containers:
      - name: prometheus
        image: prom/prometheus:v2.45.0
        args:
          - "--config.file=/etc/prometheus/prometheus.yml"
          - "--storage.tsdb.path=/prometheus"
          - "--web.console.libraries=/etc/prometheus/console_libraries"
          - "--web.console.templates=/etc/prometheus/consoles"
        ports:
        - containerPort: 9090
          name: http
        volumeMounts:
        - name: config
          mountPath: /etc/prometheus
        - name: storage
          mountPath: /prometheus
      volumes:
      - name: config
        configMap:
          name: prometheus-config
      - name: storage
        emptyDir: {}
```

**Prometheus 配置** (kubernetes_sd_configs):

```yaml
# prometheus-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-config
  namespace: monitoring
data:
  prometheus.yml: |
    global:
      scrape_interval: 15s
    
    scrape_configs:
    # 抓取 Kubernetes API server
    - job_name: 'kubernetes-apiservers'
      kubernetes_sd_configs:
      - role: endpoints
      scheme: https
      tls_config:
        ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
      relabel_configs:
      - source_labels: [__meta_kubernetes_namespace, __meta_kubernetes_service_name, __meta_kubernetes_endpoint_port_name]
        action: keep
        regex: default;kubernetes;https
    
    # 抓取 Nodes
    - job_name: 'kubernetes-nodes'
      kubernetes_sd_configs:
      - role: node
      scheme: https
      tls_config:
        ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
    
    # 抓取 Pods
    - job_name: 'kubernetes-pods'
      kubernetes_sd_configs:
      - role: pod
      relabel_configs:
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
        action: keep
        regex: true
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_path]
        action: replace
        target_label: __metrics_path__
        regex: (.+)
      - source_labels: [__address__, __meta_kubernetes_pod_annotation_prometheus_io_port]
        action: replace
        regex: ([^:]+)(?::\d+)?;(\d+)
        replacement: $1:$2
        target_label: __address__
```

---

### 5.2 案例 2: 集群管理员权限分级

**场景**: 大型组织需要多级管理员: 平台管理员、集群管理员、Namespace 管理员。

```yaml
---
# 1. 平台管理员 (cluster-admin)
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: platform-admins
  labels:
    role: platform-admin
  annotations:
    description: "平台管理员: 完全的集群控制权"

subjects:
# 平台管理员组
- kind: Group
  name: platform-admins
  apiGroup: rbac.authorization.k8s.io

# 关键个人 (备用)
- kind: User
  name: superadmin@example.com
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io

---
# 2. 集群管理员 (自定义 ClusterRole)
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-operator
  labels:
    role: cluster-operator
  annotations:
    description: "集群运维: 管理集群资源,但不能修改 RBAC"

rules:
# 所有 namespace 范围的资源
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]

# 集群资源
- apiGroups: [""]
  resources:
    - "nodes"
    - "nodes/*"
    - "persistentvolumes"
    - "namespaces"
  verbs: ["*"]

- apiGroups: ["storage.k8s.io"]
  resources: ["*"]
  verbs: ["*"]

- apiGroups: ["certificates.k8s.io"]
  resources: ["*"]
  verbs: ["*"]

# 非资源 URLs
- nonResourceURLs: ["*"]
  verbs: ["get"]

# 不包括 rbac.authorization.k8s.io
# 集群管理员不能修改 RBAC

---
# ClusterRoleBinding: 授予集群管理员
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-operators
  labels:
    role: cluster-operator

subjects:
- kind: Group
  name: cluster-operators
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: cluster-operator
  apiGroup: rbac.authorization.k8s.io

---
# 3. Namespace 管理员 (使用内建 admin ClusterRole)
# 为每个团队的 namespace 创建 RoleBinding

# 团队 A - production namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-admins
  namespace: team-a-production
  labels:
    team: team-a
    role: admin

subjects:
- kind: Group
  name: team-a-admins
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: admin
  apiGroup: rbac.authorization.k8s.io

---
# 团队 B - production namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-b-admins
  namespace: team-b-production
  labels:
    team: team-b
    role: admin

subjects:
- kind: Group
  name: team-b-admins
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: admin
  apiGroup: rbac.authorization.k8s.io

---
# 4. 只读审计员 (集群级别)
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-auditor
  labels:
    role: auditor
  annotations:
    description: "审计员: 集群级别只读,包括 Secrets 和 RBAC"

rules:
# 所有资源的只读权限
- apiGroups: ["*"]
  resources: ["*"]
  verbs:
    - "get"
    - "list"
    - "watch"

# 非资源 URLs
- nonResourceURLs: ["*"]
  verbs: ["get"]

---
# ClusterRoleBinding: 授予审计员
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-auditors
  labels:
    role: auditor

subjects:
- kind: Group
  name: auditors
  apiGroup: rbac.authorization.k8s.io

roleRef:
  kind: ClusterRole
  name: cluster-auditor
  apiGroup: rbac.authorization.k8s.io
```

**权限验证**:

```bash
# 平台管理员: 可以做任何事
kubectl auth can-i "*" "*" --as superadmin@example.com
# yes

# 集群管理员: 可以管理节点和存储,但不能修改 RBAC
kubectl auth can-i get nodes --as cluster-operator@example.com
# yes
kubectl auth can-i create clusterrolebindings --as cluster-operator@example.com
# no

# Namespace 管理员: 只能管理自己的 namespace
kubectl auth can-i create deployments -n team-a-production --as team-a-admin@example.com
# yes
kubectl auth can-i get pods -n team-b-production --as team-a-admin@example.com
# no

# 审计员: 可以查看所有资源 (包括 Secrets)
kubectl auth can-i get secrets --all-namespaces --as auditor@example.com
# yes
kubectl auth can-i delete pods --all-namespaces --as auditor@example.com
# no
```

---

### 5.3 案例 3: CI/CD 系统跨 Namespace 部署

**场景**: GitLab CI/CD 需要部署到多个 namespace,使用 ClusterRole + RoleBinding。

```yaml
---
# ServiceAccount: GitLab CI/CD Runner
apiVersion: v1
kind: ServiceAccount
metadata:
  name: gitlab-runner
  namespace: ci-cd
  labels:
    app: gitlab-runner
automountServiceAccountToken: false

---
# ClusterRole: CI/CD 部署权限模板
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cicd-deployer-template
  labels:
    app: gitlab-runner
  annotations:
    description: "CI/CD 部署权限模板: 可通过 RoleBinding 应用到任何 namespace"

rules:
# Deployments
- apiGroups: ["apps"]
  resources:
    - "deployments"
    - "deployments/status"
    - "deployments/scale"
    - "replicasets"
    - "replicasets/status"
  verbs:
    - "get"
    - "list"
    - "watch"
    - "create"
    - "update"
    - "patch"
    - "delete"

# Services
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

# ConfigMaps
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

# Secrets
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

# Ingresses
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

# Pods (只读,用于验证部署)
- apiGroups: [""]
  resources:
    - "pods"
    - "pods/log"
    - "pods/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# Events
- apiGroups: [""]
  resources: ["events"]
  verbs:
    - "get"
    - "list"
    - "watch"

---
# RoleBinding 1: 授权部署到 dev namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: gitlab-runner-deployer
  namespace: dev
  labels:
    app: gitlab-runner
    env: dev

subjects:
- kind: ServiceAccount
  name: gitlab-runner
  namespace: ci-cd

roleRef:
  kind: ClusterRole
  name: cicd-deployer-template
  apiGroup: rbac.authorization.k8s.io

---
# RoleBinding 2: 授权部署到 staging namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: gitlab-runner-deployer
  namespace: staging
  labels:
    app: gitlab-runner
    env: staging

subjects:
- kind: ServiceAccount
  name: gitlab-runner
  namespace: ci-cd

roleRef:
  kind: ClusterRole
  name: cicd-deployer-template
  apiGroup: rbac.authorization.k8s.io

---
# RoleBinding 3: 授权部署到 production namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: gitlab-runner-deployer
  namespace: production
  labels:
    app: gitlab-runner
    env: production

subjects:
- kind: ServiceAccount
  name: gitlab-runner
  namespace: ci-cd

roleRef:
  kind: ClusterRole
  name: cicd-deployer-template
  apiGroup: rbac.authorization.k8s.io
```

**优势**:
- 定义一次 ClusterRole,在多个 namespace 重用
- 通过 RoleBinding 控制 GitLab Runner 可以访问的 namespace
- 添加新 namespace 时只需创建 RoleBinding,无需修改 ClusterRole

---

### 5.4 案例 4: 使用 aggregationRule 扩展内建角色

**场景**: 扩展内建 `view` ClusterRole,允许查看自定义资源。

```yaml
---
# 自定义资源 CRD
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: applications.app.example.com
spec:
  group: app.example.com
  names:
    kind: Application
    listKind: ApplicationList
    plural: applications
    singular: application
  scope: Namespaced
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              replicas:
                type: integer
              image:
                type: string

---
# ClusterRole: 查看自定义资源
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: custom-resource-viewer
  labels:
    # 关键标签: 聚合到内建 view ClusterRole
    rbac.authorization.k8s.io/aggregate-to-view: "true"
    # 也聚合到 edit 和 admin
    rbac.authorization.k8s.io/aggregate-to-edit: "true"
    rbac.authorization.k8s.io/aggregate-to-admin: "true"
  annotations:
    description: "查看自定义资源 Application"

rules:
- apiGroups: ["app.example.com"]
  resources:
    - "applications"
    - "applications/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 现在使用 view ClusterRole 的用户可以自动查看 Application 资源
# 无需修改现有的 RoleBinding
```

**验证**:

```bash
# 查看内建 view ClusterRole 的 rules (应该包含 Application)
kubectl get clusterrole view -o yaml

# 输出应该包含:
# rules:
# - apiGroups:
#   - app.example.com
#   resources:
#   - applications
#   - applications/status
#   verbs:
#   - get
#   - list
#   - watch

# 测试用户权限
kubectl auth can-i get applications -n default --as viewer@example.com
# yes (假设 viewer@example.com 有 view ClusterRole)
```

---

### 5.5 案例 5: 节点维护权限 (Drain/Cordon)

**场景**: SRE 团队需要维护节点 (drain, cordon, uncordon),但不需要完整的集群管理员权限。

```yaml
---
# ClusterRole: 节点维护权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: node-maintainer
  labels:
    team: sre
    purpose: maintenance
  annotations:
    description: "节点维护权限: drain, cordon, uncordon 节点"

rules:
# 规则 1: 读取 Nodes
- apiGroups: [""]
  resources:
    - "nodes"
    - "nodes/status"
  verbs:
    - "get"
    - "list"
    - "watch"

# 规则 2: 更新 Nodes (用于 cordon/uncordon)
# Cordon: 设置 node.spec.unschedulable = true
# Uncordon: 设置 node.spec.unschedulable = false
- apiGroups: [""]
  resources: ["nodes"]
  verbs:
    - "update"
    - "patch"

# 规则 3: 读取 Pods (用于 drain)
- apiGroups: [""]
  resources: ["pods"]
  verbs:
    - "get"
    - "list"

# 规则 4: 驱逐 Pods (用于 drain)
# kubectl drain 使用 Eviction API
- apiGroups: [""]
  resources: ["pods/eviction"]
  verbs:
    - "create"

# 规则 5: 读取 PodDisruptionBudgets (用于 drain)
- apiGroups: ["policy"]
  resources: ["poddisruptionbudgets"]
  verbs:
    - "get"
    - "list"

# 规则 6: 读取 DaemonSets (用于 drain)
# kubectl drain 需要知道哪些 Pods 是 DaemonSet
- apiGroups: ["apps"]
  resources: ["daemonsets"]
  verbs:
    - "get"
    - "list"

# 规则 7: 读取 ReplicaSets, StatefulSets (用于 drain)
- apiGroups: ["apps"]
  resources:
    - "replicasets"
    - "statefulsets"
  verbs:
    - "get"
    - "list"

---
# ClusterRoleBinding: 授予 SRE 团队
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: sre-node-maintainers
  labels:
    team: sre

subjects:
# SRE 组
- kind: Group
  name: sre-team
  apiGroup: rbac.authorization.k8s.io

# SRE ServiceAccount (用于自动化)
- kind: ServiceAccount
  name: sre-automation-sa
  namespace: kube-system

roleRef:
  kind: ClusterRole
  name: node-maintainer
  apiGroup: rbac.authorization.k8s.io

---
# ServiceAccount: SRE 自动化工具
apiVersion: v1
kind: ServiceAccount
metadata:
  name: sre-automation-sa
  namespace: kube-system
  labels:
    team: sre
    purpose: automation
automountServiceAccountToken: true
```

**使用示例**:

```bash
# SRE 可以 cordon 节点 (标记为不可调度)
kubectl cordon node-1 --as sre@example.com
# node/node-1 cordoned

# SRE 可以 drain 节点 (驱逐所有 Pods)
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --as sre@example.com
# node/node-1 drained

# SRE 可以 uncordon 节点 (恢复可调度)
kubectl uncordon node-1 --as sre@example.com
# node/node-1 uncordoned

# SRE 不能删除节点
kubectl delete node node-1 --as sre@example.com
# Error from server (Forbidden): nodes "node-1" is forbidden
```

---

## 6. 最佳实践

### 6.1 ClusterRole vs Role 选择

| 场景 | 推荐 | 原因 |
|------|------|------|
| 访问集群资源 (Node, PV) | ClusterRole + ClusterRoleBinding | 集群资源无 namespace |
| 访问非资源 URL (/healthz) | ClusterRole + ClusterRoleBinding | 非资源 URL 无 namespace |
| 跨 namespace 访问 | ClusterRole + ClusterRoleBinding | 一次授权,所有 namespace |
| 单个 namespace 访问 | ClusterRole + RoleBinding | 定义一次,多个 namespace 重用 |
| Namespace 专属权限 | Role + RoleBinding | 权限隔离更好 |

### 6.2 安全建议

1. **避免过度使用 cluster-admin**:
   - 只用于集群引导和紧急情况
   - 生产环境使用自定义 ClusterRole

2. **最小权限原则**:
   - 明确指定 apiGroups, resources, verbs
   - 避免使用通配符 `*` (除非确实需要)
   - 优先使用 RoleBinding (namespace 范围)

3. **定期审计**:
   ```bash
   # 列出所有 ClusterRoleBindings
   kubectl get clusterrolebindings -o wide
   
   # 查找授予 cluster-admin 的绑定
   kubectl get clusterrolebindings -o json | \
     jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .metadata.name'
   
   # 查找所有 ServiceAccount 的集群权限
   kubectl get clusterrolebindings -o json | \
     jq -r '.items[] | select(.subjects[]?.kind=="ServiceAccount") | .metadata.name'
   ```

4. **使用 aggregationRule**:
   - 扩展内建角色而非复制
   - 便于维护和升级

5. **监控和告警**:
   - 启用 Audit Logging
   - 监控 cluster-admin 使用
   - 告警异常的 RBAC 修改

### 6.3 常见错误

```yaml
# ❌ 错误 1: ClusterRoleBinding 引用 Role
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: wrong-binding
subjects:
- kind: User
  name: alice
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role  # ❌ 错误! ClusterRoleBinding 不能引用 Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io

---
# ✅ 正确: ClusterRoleBinding 引用 ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: correct-binding
subjects:
- kind: User
  name: alice
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole  # ✅ 正确
  name: cluster-pod-reader
  apiGroup: rbac.authorization.k8s.io

---
# ❌ 错误 2: Role 使用 nonResourceURLs
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: wrong-role
  namespace: default
rules:
- nonResourceURLs: ["/healthz"]  # ❌ 错误! Role 不支持 nonResourceURLs
  verbs: ["get"]

---
# ✅ 正确: ClusterRole 使用 nonResourceURLs
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: correct-role
rules:
- nonResourceURLs: ["/healthz"]  # ✅ 正确
  verbs: ["get"]

---
# ❌ 错误 3: ServiceAccount subject 缺少 namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: wrong-sa-binding
subjects:
- kind: ServiceAccount
  name: myapp-sa
  # ❌ 缺少 namespace 字段
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io

---
# ✅ 正确: ServiceAccount subject 包含 namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: correct-sa-binding
subjects:
- kind: ServiceAccount
  name: myapp-sa
  namespace: default  # ✅ 必须指定 namespace
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io
```

---

## 7. 常见问题排查

### 7.1 权限升级风险检测

**检查是否有用户可以修改 RBAC**:

```bash
# 查找可以创建 ClusterRoleBindings 的用户
kubectl get clusterrolebindings -o json | \
  jq -r '.items[] | select(.roleRef.name=="cluster-admin") | 
  {name: .metadata.name, subjects: .subjects}'

# 检查特定用户是否可以修改 RBAC
kubectl auth can-i create clusterrolebindings --as alice@example.com
kubectl auth can-i update clusterroles --as system:serviceaccount:default:myapp-sa
```

### 7.2 ClusterRole 聚合不生效

**症状**: 使用 `aggregationRule` 的 ClusterRole 的 rules 字段为空。

**排查步骤**:

```bash
# 1. 检查父 ClusterRole
kubectl get clusterrole monitoring-aggregate -o yaml

# 2. 检查 aggregationRule 的 labelSelector
# 确保子 ClusterRole 有匹配的标签

# 3. 查找匹配的子 ClusterRole
kubectl get clusterroles -l rbac.example.com/aggregate-to-monitoring=true

# 4. 检查 kube-controller-manager 日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep -i "clusterrole"

# 5. 强制刷新: 删除并重新创建父 ClusterRole
kubectl delete clusterrole monitoring-aggregate
kubectl apply -f monitoring-aggregate.yaml
```

### 7.3 非资源 URL 访问被拒绝

**症状**:
```bash
curl -k https://kubernetes.default.svc/healthz
# Forbidden
```

**排查步骤**:

```bash
# 1. 检查是否有 ClusterRole 授权 /healthz
kubectl get clusterroles -o json | \
  jq -r '.items[] | select(.rules[]?.nonResourceURLs[]? | contains("/healthz")) | .metadata.name'

# 2. 检查 ClusterRoleBinding
kubectl get clusterrolebindings -o json | \
  jq -r '.items[] | select(.roleRef.name=="<clusterrole-name>") | .metadata.name'

# 3. 验证当前用户权限
kubectl auth can-i get /healthz

# 4. 使用 SubjectAccessReview 检查
kubectl create -f - <<EOF
apiVersion: authorization.k8s.io/v1
kind: SubjectAccessReview
spec:
  nonResourceAttributes:
    path: "/healthz"
    verb: "get"
  user: "system:anonymous"
EOF
```

---

## 8. 参考资料

- [Kubernetes RBAC 官方文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [ClusterRole API Reference](https://kubernetes.io/docs/reference/kubernetes-api/authorization-resources/cluster-role-v1/)
- [ClusterRoleBinding API Reference](https://kubernetes.io/docs/reference/kubernetes-api/authorization-resources/cluster-role-binding-v1/)
- [Default ClusterRoles](https://kubernetes.io/docs/reference/access-authn-authz/rbac/#default-roles-and-role-bindings)
- [Using RBAC Authorization](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

---

**文档版本**: v1.0  
**最后更新**: 2026-02  
**维护者**: Kubernetes 中文社区  
**适用版本**: Kubernetes v1.25 - v1.32
