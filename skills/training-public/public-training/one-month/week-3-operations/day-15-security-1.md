---
title: 'Day 15: 安全体系 - RBAC + 认证授权'
description: 'title: Day 15: 安全体系 - RBAC + 认证授权'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- rbac
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 15: 安全体系 - RBAC + 认证授权 是什么'
- '如何 Day 15: 安全体系 - RBAC + 认证授权'
trigger_keywords:
- Day
- '15:'
- 安全体系
- RBAC
- 认证授权
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- policy-basics
created: "2026-05-23"
---

---
title: Day 15: 安全体系 - RBAC + 认证授权
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] RBAC 配置
  - K8s 认证授权体系
  - ServiceAccount 管理
  - RBAC 权限设计
trigger_keywords:
  - RBAC
  - 认证
  - 授权
  - ServiceAccount
  - Role
  - ClusterRole
  - 权限控制
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
  - security-engineer
estimated_read_time: 240min
related_domains:
  - domain-05-security-compliance
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-[[domain-02-workloads-applications/topic-functions/cluster-create/16-security.md|16-security]]-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Day 15: 安全体系 - RBAC + 认证授权

## 概述

今天进入 K8s 安全体系的学习。安全是生产环境的底线——一个配置不当的 K8s 集群可能面临权限滥用、数据泄露、服务中断等严重风险。在所有安全机制中，**RBAC（Role-Based Access Control）** 是最基础也最重要的，它控制了"谁可以在集群中做什么"。

理解 K8s 的认证授权体系，就像理解一栋大楼的门禁系统：认证（Authentication）确认你是谁（刷卡/人脸识别），授权（Authorization）确认你能去哪里（哪些楼层/房间），准入控制（Admission Control）确认你带的东西是否符合规定（是否携带违禁品）。

### 学习目标

- 理解 K8s 认证链的完整流程（认证 → 授权 → 准入控制）
- 掌握 ServiceAccount 的创建、使用和 Token 管理
- 掌握 RBAC 四种资源（Role、ClusterRole、RoleBinding、ClusterRoleBinding）的配置
- 能够设计符合最小权限原则的访问控制方案
- 能够排查 RBAC 权限问题

---

## 核心概念详解

### K8s 认证（Authentication）

K8s 支持多种认证方式，常见的包括：

**X.509 客户端证书认证**: kubeconfig 文件中嵌入的客户端证书。当使用 `kubectl` 连接集群时，客户端证书用于证明你的身份。证书中的 CN（Common Name）字段对应用户名，O（Organization）字段对应组名。这是最常用的认证方式。

**ServiceAccount Token 认证**: Pod 中自动挂载的 JWT Token。每个 Pod 默认关联一个 ServiceAccount，Token 被挂载到 `/var/run/secrets/kubernetes.io/serviceaccount/token`。从 K8s 1.24 开始，Token 不再自动创建 Secret，而是通过 TokenRequest API 动态生成短期 Token（默认有效期 1 小时）。Pod 内的应用使用这个 Token 调用 K8s API。

**OIDC（OpenID Connect）认证**: 通过外部身份提供商（如阿里云 RAM、Okta、Azure AD）进行认证。企业环境通常使用 OIDC 统一管理用户身份，实现与现有账号系统的集成。

**Bootstrap Token**: 用于节点加入集群时的引导认证。[[kubelet|kubelet]] 在首次向 API Server 注册时使用 Bootstrap Token 获取初始凭证。

理解认证的关键点：K8s 本身不管理用户（User），它只验证请求中携带的凭证（证书、Token 等）并提取用户身份。用户的创建和管理由外部系统（如 OIDC 提供商）负责。

### K8s 授权（Authorization）

认证确认"你是谁"之后，授权确认"你能做什么"。K8s 支持多种授权模式，RBAC 是最常用的。

**RBAC（Role-Based Access Control）** 的核心思想：将权限授予角色（Role），再将角色绑定到用户/组/ServiceAccount。这种方式避免了直接给用户授权，简化了权限管理。

RBAC 的四种核心资源：

**Role**: 命名空间级别的权限定义。它定义了在某个命名空间内可以对哪些资源执行哪些操作。

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: default
rules:
- apiGroups: [""]           # core API group
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
```

关键字段说明：

- `apiGroups`: 资源所属的 API 组。核心资源（如 Pod、[[Service|Service]]）的 apiGroup 为空字符串 `""`
- `resources`: 资源类型。注意 `pods/log` 表示 Pod 的子资源日志
- `verbs`: 允许的操作。get（获取单个）、list（列出多个）、watch（监听变化）、create（创建）、update（更新）、patch（部分更新）、delete（删除）

**ClusterRole**: 集群级别的权限定义。用于两种场景：1) 授权集群级资源（如 Node、PV、Namespace）的访问；2) 定义跨命名空间的权限模板。

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: node-reader
rules:
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]
```

**RoleBinding**: 将 Role（或 ClusterRole）绑定到主体（User、Group、ServiceAccount），权限仅在 RoleBinding 所在的命名空间内生效。

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-pod-reader
  namespace: default
subjects:
- kind: ServiceAccount
  name: dev-sa
  namespace: default
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
```

**ClusterRoleBinding**: 将 ClusterRole 绑定到主体，权限在整个集群范围内生效。

RBAC 设计模式：

- **命名空间隔离**: 为每个团队创建独立的命名空间，通过 RoleBinding 授予该命名空间内的权限
- **角色复用**: 创建通用的 ClusterRole（如 reader、editor），在各个命名空间中通过 RoleBinding 引用
- **最小权限**: 只授予完成任务所需的最少操作。例如，只需要查看日志的 ServiceAccount，不需要 create 和 delete 权限

### ServiceAccount 详解

ServiceAccount 是 K8s 中 Pod 的身份标识。每个 Pod 都关联一个 ServiceAccount（默认为 `default`）。

**ServiceAccount 的用途**:

- **API 访问**: Pod 内的应用通过 SA Token 调用 K8s API（如查询其他 Service、读取 ConfigMap）
- **权限控制**: 通过 RBAC 控制每个 SA 可以访问的资源
- **镜像拉取**: 关联 imagePullSecrets 用于从私有仓库拉取镜像

**ServiceAccount 最佳实践**:

- 不要使用 default ServiceAccount。为每个应用创建专属的 SA
- 为 SA 配置最小权限的 RBAC
- 定期审计 SA 的权限，清理不再需要的权限
- 使用 AutomountServiceAccountToken: false 禁止不需要 API 访问的 Pod 挂载 Token

### 准入控制（Admission Control）

准入控制是请求经过认证和授权后的最后一道关卡。它在资源被持久化到 etcd 之前进行拦截和修改。

常见的准入控制器：

- **NamespaceLifecycle**: 防止在正在删除的命名空间中创建新资源
- **LimitRanger**: 确保资源请求在 LimitRange 范围内
- **ResourceQuota**: 确保命名空间的资源配额不被超出

- **PodSecurity**: 实施 Pod 安全标准（替代已废弃的 PodSecurityPolicy）
- **ValidatingAdmissionWebhook**: 调用外部 Webhook 进行验证（如 Kyverno）
- **MutatingAdmissionWebhook**: 调用外部 Webhook 修改资源（如注入 Sidecar）

---

## 实战演练

### 任务 1: ServiceAccount 实践 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 创建测试命名空间
kubectl create namespace rbac-test

# 创建 ServiceAccount
kubectl create serviceaccount dev-sa -n rbac-test

# 查看 ServiceAccount
kubectl get sa -n rbac-test
kubectl describe sa dev-sa -n rbac-test

# 创建长期 Token（K8s 1.24+）
cat > sa-token.yaml << 'EOF'
apiVersion: v1
kind: Secret
metadata:
  name: dev-sa-token
  namespace: rbac-test
  annotations:
    kubernetes.io/service-account.name: dev-sa
type: kubernetes.io/service-account-token
EOF

kubectl apply -f sa-token.yaml

# 创建使用 SA 的 Pod
cat > sa-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: sa-test
  namespace: rbac-test
spec:
  serviceAccountName: dev-sa
  containers:
  - name: app
    image: nginx:alpine
EOF

kubectl apply -f sa-pod.yaml

# 查看 Pod 挂载的 Token 和 CA 证书
kubectl exec -n rbac-test sa-test -- ls /var/run/secrets/kubernetes.io/serviceaccount/
kubectl exec -n rbac-test sa-test -- cat /var/run/secrets/kubernetes.io/serviceaccount/token | cut -d. -f2 | base64 -d 2>/dev/null | jq '.iss, .sub, .exp'
```

### 任务 2: RBAC 配置实践 (1h)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建 Role（命名空间级别权限）
cat > role.yaml << 'EOF'
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
  resources: ["configmaps"]
  verbs: ["get", "list"]
  resourceNames: ["app-config"]
EOF

kubectl apply -f role.yaml

# 创建 RoleBinding
cat > rolebinding.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-pod-reader
  namespace: rbac-test
subjects:
- kind: ServiceAccount
  name: dev-sa
  namespace: rbac-test
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
EOF

kubectl apply -f rolebinding.yaml

# 创建 ClusterRole（集群级别权限）
cat > clusterrole.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: node-reader
rules:
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]
EOF

kubectl apply -f clusterrole.yaml

# 创建 ClusterRoleBinding
cat > clusterrolebinding.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: dev-node-reader
subjects:
- kind: ServiceAccount
  name: dev-sa
  namespace: rbac-test
roleRef:
  kind: ClusterRole
  name: node-reader
  apiGroup: rbac.authorization.k8s.io
EOF

kubectl apply -f clusterrolebinding.yaml

# 验证权限
echo "=== 权限验证 ==="
echo "Can dev-sa get pods? $(kubectl auth can-i get pods --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test)"
echo "Can dev-sa delete pods? $(kubectl auth can-i delete pods --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test)"
echo "Can dev-sa get nodes? $(kubectl auth can-i get nodes --as=system:serviceaccount:rbac-test:dev-sa)"
echo "Can dev-sa create deployments? $(kubectl auth can-i create deployments --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test)"
```

### 任务 3: 权限排查与审计 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 查看当前用户的所有权限
kubectl auth can-i --list

# 查看特定 SA 的权限
kubectl auth can-i --list --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test

# 查看特定资源操作的权限
kubectl auth can-i create pods --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test
kubectl auth can-i get secrets --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test

# 查看谁有权限执行危险操作
kubectl auth can-i --list --as=system:anonymous
kubectl auth can-i '*' '*' --as=system:anonymous

# 查看 RoleBinding 和 ClusterRoleBinding
kubectl get rolebindings -n rbac-test
kubectl get clusterrolebindings | grep dev

# 查看绑定的详细信息
kubectl describe rolebinding dev-pod-reader -n rbac-test

# 清理
kubectl delete namespace rbac-test  # ⚠️ 不可逆：永久删除命名空间及全部资源
kubectl delete clusterrole node-reader
kubectl delete clusterrolebinding dev-node-reader
```

---

## 常见问题

### Q1: RoleBinding 可以引用 ClusterRole 吗？

可以。这是 RBAC 的一个重要设计模式：定义一个 ClusterRole 作为权限模板，然后在各个命名空间中通过 RoleBinding 引用。这样可以避免在每个命名空间中重复定义相同的 Role。例如，定义一个 `read-only` ClusterRole，在每个团队命名空间中通过 RoleBinding 绑定给对应的 SA。

### Q2: Pod 使用 default ServiceAccount 有什么风险？

default ServiceAccount 默认没有任何权限（除了通过自动挂载的 Token 认证自身），但如果集群管理员给 default SA 授予了额外的权限，所有使用 default SA 的 Pod 都会继承这些权限。建议为每个应用创建专属的 SA，并禁止不需要 API 访问的 Pod 挂载 Token（`automountServiceAccountToken: false`）。

### Q3: 如何防止用户创建特权 Pod？

使用 Pod Security Standards（PSS）。在命名空间上添加标签：`pod-security.kubernetes.io/enforce: restricted`，K8s 会自动拒绝不符合安全标准的 Pod 创建请求。也可以使用 Kyverno 等策略引擎实现更细粒度的控制。

### Q4: ServiceAccount Token 过期了怎么办？

从 K8s 1.24 开始，Pod 中自动挂载的 Token 是短期 Token（默认 1 小时有效期），kubelet 会自动刷新。如果你创建了手动绑定的 Secret Token，它不会自动刷新。建议使用 TokenRequest API 动态获取 Token，而非创建长期 Secret。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| 认证 | 确认"你是谁"（证书、Token、OIDC） |
| RBAC | Role/ClusterRole 定义权限，RoleBinding/ClusterRoleBinding 绑定到主体 |
| ServiceAccount | Pod 的身份标识，通过 Token 访问 K8s API |
| 最小权限 | 只授予完成任务所需的最少权限 |
| 准入控制 | 请求持久化前的最后一道关卡 |

---

## 延伸阅读

- [认证授权系统](../../domain-05-security-compliance/01-authentication-authorization-system.md)
- [RBAC 矩阵配置](../../domain-05-security-compliance/07-rbac-matrix-configuration.md)
- [证书管理](../../domain-05-security-compliance/10-certificate-management.md)
- [Pod 安全标准](../../domain-05-security-compliance/06-pod-security-standards.md)
