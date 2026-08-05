---
title: 'Day 8: K8S 集群 RBAC'
description: '- "权限配置"'
summary: '本文深入讲解 Kubernetes 的 RBAC（Role-Based Access Control，基于角色的访问控制）权限模型。RBAC 是 K8s 安全体系的核心组件，它决定了"谁"可以在集群中"做什么"。理解 RBAC 对于实现最小权限原则、多租户隔离和审计合规至关重要。通过本文，你将掌握 RBAC 四种核心资源的创建与绑定，'
category: learning
tags:
- k8s
- training
- hands-on
- statefulset
- daemonset
- job
- cronjob
- ingress
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 8: K8S 集群 RBAC 是什么'
- '如何 Day 8: K8S 集群 RBAC'
trigger_keywords:
- Day
- '8:'
- K8S
- 集群
- RBAC
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 8: K8S 集群 RBAC

```yaml
---
title: Day 8: 集群RBAC
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "[[entities/kubernetes.md|kubernetes]] RBAC"
  - "Role ClusterRole"
  - "RoleBinding"
  - "权限配置"
  - "ServiceAccount"
trigger_keywords:
  - "RBAC"
  - "Role"
  - "ClusterRole"
  - "RoleBinding"
  - "ClusterRoleBinding"
  - "权限"
  - "ServiceAccount"
  - "最小权限"
  - "多租户"
  - "kubectl auth can-i"
reading_level: intermediate
audience:
  - sre工程师
  - 安全工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
  - domain-05-security-compliance/01-authentication-authorization-system
  - domain-05-security-compliance/07-rbac-matrix-configuration
id: WEEK2-DAY8
topic: training
type: hands-on
tags: [week-2, day-8, rbac, security, authorization, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: RBAC 权限模型与配置实践

---

## 概述

本文深入讲解 Kubernetes 的 RBAC（Role-Based Access Control，基于角色的访问控制）权限模型。RBAC 是 K8s 安全体系的核心组件，它决定了"谁"可以在集群中"做什么"。理解 RBAC 对于实现最小权限原则、多租户隔离和审计合规至关重要。通过本文，你将掌握 RBAC 四种核心资源的创建与绑定，学会使用 ServiceAccount 为应用分配权限，并了解 ACK 集群中的预置角色和最佳实践。

### 学习目标

- 理解 RBAC 四种核心资源（Role、ClusterRole、RoleBinding、ClusterRoleBinding）及其关系
- 能够创建自定义 Role/ClusterRole 并绑定到 ServiceAccount
- 掌握 `kubectl auth can-i` 命令验证权限
- 了解 ACK 预置的 ClusterRole（cluster-admin、admin、edit、view）
- 掌握生产环境 RBAC 最佳实践

---

## 核心概念详解

### RBAC 权限模型架构

Kubernetes 的授权流程发生在 API Server 的认证之后。当用户或应用通过认证后，API Server 会检查该主体是否被授权执行请求的操作。RBAC 是 K8s 默认且推荐的授权模式。

RBAC 权限模型的核心概念：

- **主体（Subject）**: 谁在发起操作。可以是 User（用户）、Group（用户组）或 ServiceAccount（服务账号）
- **动作（Verb）**: 对资源执行的操作。包括 get、list、watch、create、update、patch、delete、deletecollection 等
- **资源（Resource）**: 操作的对象。如 [[Pods|pods]]、services、[[Deployments|deployments]]、secrets 等
- **API Group**: 资源所属的 API 组。核心资源（pods、services）属于 ""（核心组），Deployment 属于 "apps" 组

权限的定义遵循"谁能对什么资源做什么操作"的模式。

### 四种 RBAC 资源

**Role** 定义了 Namespace 级别的权限规则。它只在指定的 Namespace 内生效。例如，一个 Role 可以允许用户在 `dev` Namespace 中读取 Pod 和查看日志，但不能访问 `prod` Namespace。

**ClusterRole** 定义了集群级别的权限规则。它有两种用途：一是定义集群范围的权限（如查看节点信息、管理 PV），二是作为可复用的权限模板（可以被 RoleBinding 引用，从而在特定 Namespace 中生效）。

**RoleBinding** 将 Role 或 ClusterRole 绑定到主体（用户/组/ServiceAccount），绑定只在 RoleBinding 所在的 Namespace 中生效。如果绑定的是 ClusterRole，该 ClusterRole 中定义的权限只在 RoleBinding 的 Namespace 内有效。

**ClusterRoleBinding** 将 ClusterRole 绑定到主体，权限在整个集群范围内生效。这意味着被绑定的主体可以在所有 Namespace 中行使 ClusterRole 定义的权限。

关系总结：

```
Role                → RoleBinding           → Subject (在特定 Namespace 内生效)
ClusterRole         → ClusterRoleBinding    → Subject (在整个集群范围内生效)
ClusterRole         → RoleBinding           → Subject (仅在 RoleBinding 的 Namespace 内生效)
```

### ACK 预置角色

ACK 集群（以及标准 K8s 集群）预置了以下常用 ClusterRole：

| ClusterRole | 权限范围 | 说明 |
|-------------|---------|------|
| cluster-admin | 集群完全控制 | 可以操作所有资源，包括 Node、PV、Namespace 等 |
| admin | Namespace 管理员 | 可以管理 Namespace 内的所有资源，包括 Role/RoleBinding |
| edit | Namespace 编辑 | 可以读写 Namespace 内的常见资源（Pod、Deployment、Service 等） |
| view | Namespace 只读 | 只能读取 Namespace 内的资源，不能查看 Secret |

### ServiceAccount 与 RBAC 的关联

ServiceAccount 是 K8s 为 Pod 提供的身份标识。每个 Namespace 创建时会自动生成一个 `default` ServiceAccount。Pod 可以通过挂载 ServiceAccount 的 Token 来访问 API Server。在 RBAC 体系中，ServiceAccount 作为 Subject 被绑定到 Role/ClusterRole，从而获得相应的权限。

### 权限三要素详解

一条权限规则由三个要素组成：

- **apiGroups**: 资源所属的 API 组。核心资源使用 `[""]`，apps 组使用 `["apps"]`，多组使用数组
- **resources**: 允许操作的资源类型。支持子资源，如 `["pods", "pods/log", "pods/exec"]`
- **verbs**: 允许的操作。常用动词包括 get（获取单个）、list（列出多个）、watch（监听变化）、create（创建）、update（全量更新）、patch（部分更新）、delete（删除）

---

## 实战演练

### 任务 1: 查看现有 RBAC 配置 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看预置 ClusterRole（过滤系统角色，只看用户可用的）
kubectl get clusterroles | grep -v "system:"
# 预期输出:
# NAME                                                                   CREATED AT
# admin                                                                  2024-01-01T00:00:00Z
# cluster-admin                                                          2024-01-01T00:00:00Z
# edit                                                                   2024-01-01T00:00:00Z
# view                                                                   2024-01-01T00:00:00Z
# ...

# 查看 cluster-admin 角色的权限
kubectl describe clusterrole cluster-admin
# 预期输出:
# PolicyRule:
#   Resources  Non-Resource URLs  Resource Names  Verbs
#   ---------  -----------------  --------------  -----
#   *.*        []                 []              [*]
#              [*]                []              [*]
# (通配符表示对所有资源有所有操作权限)

# 查看 admin 角色（Namespace 管理员权限）
kubectl describe clusterrole admin
# 预期输出包含对大多数 Namespace 内资源的读写权限

# 查看 edit 角色（读写权限，不能管理 RBAC）
kubectl describe clusterrole edit
# 预期输出: 可以创建/更新/删除 Pod、Deployment、Service 等

# 查看 view 角色（只读权限）
kubectl describe clusterrole view
# 预期输出: 可以 get/list/watch 大部分资源，不能查看 Secret

# 查看 ClusterRoleBinding（谁被赋予了集群级角色）
kubectl get clusterrolebindings | grep -v "system:" | head -20

# 查看当前用户的权限列表
kubectl auth can-i --list
# 预期输出: 表格形式列出当前用户的所有权限

# 检查特定权限
kubectl auth can-i create pods
# 预期输出: yes

kubectl auth can-i delete nodes
# 预期输出: yes (如果是管理员) / no (如果是普通用户)

kubectl auth can-i get secrets -n kube-system
# 预期输出: yes / no
```
### 任务 2: 创建自定义 RBAC (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试 Namespace
kubectl create namespace rbac-test
# 预期输出: namespace/rbac-test created

# 创建只读 Role（只允许查看 Pod 和 Service）
cat > readonly-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: rbac-test
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["services", "endpoints"]
  verbs: ["get", "list"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch"]
EOF
kubectl apply -f readonly-role.yaml
# 预期输出: role.rbac.authorization.k8s.io/pod-reader created

# 创建 ServiceAccount
kubectl create serviceaccount dev-user -n rbac-test
# 预期输出: serviceaccount/dev-user created

# 绑定 Role 到 ServiceAccount
cat > readonly-binding.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-user-pod-reader
  namespace: rbac-test
subjects:
- kind: ServiceAccount
  name: dev-user
  namespace: rbac-test
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
EOF
kubectl apply -f readonly-binding.yaml
# 预期输出: rolebinding.rbac.authorization.k8s.io/dev-user-pod-reader created

# 验证 Role 和 RoleBinding 已创建
kubectl get role,rolebinding -n rbac-test
# 预期输出:
# NAME                                         CREATED AT
# role.rbac.authorization.k8s.io/pod-reader    2024-01-15T00:00:00Z
# NAME                                                ROLE                AGE
# rolebinding.rbac.authorization.k8s.io/dev-user-pod-reader   Role/pod-reader   10s
```
### 任务 3: 验证 RBAC 权限 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 dev-user 身份测试权限（模拟 ServiceAccount 访问）
kubectl auth can-i get pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: yes

kubectl auth can-i list pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: yes

kubectl auth can-i create pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: no

kubectl auth can-i delete pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: no

kubectl auth can-i get secrets -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: no

# 跨 Namespace 测试（Role 只在 rbac-test 内生效）
kubectl auth can-i get pods -n default --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: no

# 查看完整的权限列表
kubectl auth can-i --list --as=system:serviceaccount:rbac-test:dev-user -n rbac-test
# 预期输出: 只列出 pod-reader Role 中定义的权限

# 创建运维角色（ClusterRole，更多权限）
cat > ops-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ops-engineer
  labels:
    rbac.example.com/aggregate-to-admin: "true"
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "events"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec"]
  verbs: ["get", "create"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["metrics.k8s.io"]
  resources: ["pods", "nodes"]
  verbs: ["get", "list"]
EOF
kubectl apply -f ops-role.yaml
# 预期输出: clusterrole.rbac.authorization.k8s.io/ops-engineer created

# 创建 ClusterRoleBinding（绑定到运维组）
cat > ops-binding.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: ops-engineer-binding
subjects:
- kind: Group
  name: ops-team
  apiGroup: rbac.authorization.k8s.io
- kind: ServiceAccount
  name: dev-user
  namespace: rbac-test
roleRef:
  kind: ClusterRole
  name: ops-engineer
  apiGroup: rbac.authorization.k8s.io
EOF
kubectl apply -f ops-binding.yaml
# 预期输出: clusterrolebinding.rbac.authorization.k8s.io/ops-engineer-binding created

# 验证 dev-user 现在有集群级权限
kubectl auth can-i get nodes --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: yes

kubectl auth can-i get pods -n default --as=system:serviceaccount:rbac-test:dev-user
# 预期输出: yes
```
### 任务 4: ACK RBAC 最佳实践 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看 ACK 集群中的自定义 ClusterRole
kubectl get clusterroles | grep -v "system:" | grep -v "kubernetes"

# 查看 ACK 预置的权限绑定
kubectl get clusterrolebindings | grep -E "ack|aliyun"

# 最佳实践一：使用预置角色而非自定义
# 将用户绑定到 view 角色（只读）
cat > view-binding.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-view
  namespace: production
subjects:
- kind: User
  name: developer@company.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: view
  apiGroup: rbac.authorization.k8s.io
EOF

# 最佳实践二：为每个应用创建专用 ServiceAccount
cat > app-sa.yaml << 'EOF'
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-controller
  namespace: rbac-test
automountServiceAccountToken: true
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: config-reader
  namespace: rbac-test
rules:
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list"]
  resourceNames: ["app-config"]
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get"]
  resourceNames: ["app-secret"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-controller-config-reader
  namespace: rbac-test
subjects:
- kind: ServiceAccount
  name: app-controller
  namespace: rbac-test
roleRef:
  kind: Role
  name: config-reader
  apiGroup: rbac.authorization.k8s.io
EOF
kubectl apply -f app-sa.yaml

# 最佳实践三：定期审查权限
# 列出所有 ClusterRoleBinding
kubectl get clusterrolebindings -o json | \
  jq -r '.items[] | select(.roleRef.name=="cluster-admin") | "\(.metadata.name): \(.subjects)"'

# 列出所有有 cluster-admin 权限的账号
kubectl get clusterrolebinding -o json | \
  jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .subjects[]? | "\(.kind)/\(.name)"'

# 清理测试资源
kubectl delete namespace rbac-test  # ⚠️ 不可逆：永久删除命名空间及全部资源
kubectl delete clusterrole ops-engineer
kubectl delete clusterrolebinding ops-engineer-binding
```
---

## 配置示例

### 开发环境权限模板

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: dev
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "pods/exec", "services", "endpoints", "configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list"]
  resourceNames: ["app-config-secret"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-binding
  namespace: dev
subjects:
- kind: Group
  name: developers
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: developer
  apiGroup: rbac.authorization.k8s.io
```

### 生产环境只读审计角色

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: auditor
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "events", "namespaces", "nodes"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies", "ingresses"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["policy"]
  resources: ["podsecuritypolicies"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["audit.k8s.io"]
  resources: ["events"]
  verbs: ["get", "list", "watch"]
```

---

## 常见问题

### Q1: Role 和 ClusterRole 的区别是什么？

Role 是 Namespace 级别的权限，只在定义它的 Namespace 中生效。ClusterRole 是集群级别的权限，可以授权集群范围的资源（如 nodes、PV）或作为可复用模板被 RoleBinding 引用。建议：能用 Role 解决的就不使用 ClusterRole。

### Q2: 如何实现"某用户只能查看特定 Namespace 的 Pod"？

创建一个 Role（在目标 Namespace 中），定义 pods 的 get/list/watch 权限，然后创建 RoleBinding 将该用户绑定到这个 Role。如果用户已经有一个 ClusterRole（如 view），可以直接用 RoleBinding 引用它，权限只在 RoleBinding 的 Namespace 内生效。

### Q3: 为什么生产环境要避免使用 cluster-admin？

cluster-admin 拥有集群的完全控制权，包括删除 Namespace、修改 RBAC、查看所有 Secret 等。如果 cluster-admin 的凭证泄露，攻击者可以完全控制集群。生产环境应遵循最小权限原则——为每个用户/应用分配完成任务所需的最少权限。

### Q4: ServiceAccount 的 Token 是怎么被 Pod 使用的？

当 Pod 指定 ServiceAccount 后，K8s 会自动将 ServiceAccount 的 Token 挂载到 Pod 的 `/var/run/secrets/kubernetes.io/serviceaccount/` 目录。应用可以通过读取该目录下的 `token` 文件来获取认证信息，然后使用它访问 API Server。

### Q5: 如何调试 RBAC 权限问题？

使用 `kubectl auth can-i` 命令是最高效的方法：`kubectl auth can-i <verb> <resource> --as=<subject>`。如果权限被拒绝，检查 API Server 日志中的 RBAC DENY 信息（需要开启 `--v=5` 或更高日志级别）。也可以使用 `kubectl auth can-i --list` 列出某个主体的所有权限。

### Q6: RoleBinding 引用 ClusterRole 时权限范围是什么？

当 RoleBinding 引用 ClusterRole 时，ClusterRole 中定义的权限只在 RoleBinding 所在的 Namespace 内生效。这意味着即使 ClusterRole 定义了 nodes 的读取权限，通过 RoleBinding 引用后也不能读取 nodes（因为 nodes 是集群级资源，不在任何 Namespace 中）。

---

## 要点总结

| 资源 | 作用域 | 用途 | 关键点 |
|------|--------|------|--------|
| Role | Namespace | 定义 Namespace 级权限 | 只在特定 Namespace 生效 |
| ClusterRole | 集群 | 定义集群级权限或可复用模板 | 可被 RoleBinding 引用 |
| RoleBinding | Namespace | 将 Role/ClusterRole 绑定到主体 | 权限在 Namespace 内生效 |
| ClusterRoleBinding | 集群 | 将 ClusterRole 绑定到主体 | 权限在集群范围生效 |

---

## 延伸阅读

- [认证授权体系](../../domain-05-security-compliance/01-authentication-authorization-system.md)
- [RBAC 矩阵配置](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/01-identity-access/06-rbac-matrix-configuration.md)
- [安全架构总览](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-01-cluster-fundamentals/01-architecture-overview/08-security-architecture.md)
- [ACK 安全管理](../../domain-12-cloud-providers/04-alicloud-ack/270-ack-security.md)


<!-- risk-assessed -->
