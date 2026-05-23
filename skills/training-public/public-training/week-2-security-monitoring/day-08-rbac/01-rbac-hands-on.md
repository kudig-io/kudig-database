---
title: 'Day 8: K8s RBAC 权限配置实操'
description: '# Day 8: K8s RBAC 权限配置实操'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- rbac
- agent
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 8: K8s RBAC 权限配置实操 是什么'
- '如何 Day 8: K8s RBAC 权限配置实操'
trigger_keywords:
- Day
- '8:'
- K8s
- RBAC
- 权限配置实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

# Day 8: K8s RBAC 权限配置实操

> **日期**: Week 2 Day 1 | **主题**: RBAC 权限模型与配置实践 | **版本**: K8s 1.28-1.33

---

## 1. RBAC 核心概念

### 1.1 四大资源对象

| 对象 | 作用域 | 说明 |
|------|--------|------|
| `Role` | Namespace | 授权特定 namespace 内的资源操作 |
| `ClusterRole` | 集群级 | 授权集群范围的资源或非资源路径（如 `/healthz`） |
| `RoleBinding` | Namespace | 将 Role/ClusterRole 绑定到用户/组/SA |
| `ClusterRoleBinding` | 集群级 | 将 ClusterRole 绑定到集群范围的主体 |

### 1.2 API 主体（Subject）类型

```yaml
subjects:
  - kind: User      # 外部用户（如 LDAP 集成）
    name: jane@example.com
    apiGroup: ""
  - kind: Group     # 用户组
    name: frontend-team
    apiGroup: ""
  - kind: ServiceAccount  # 服务账号
    name: ci-builder
    namespace: ci-system
    apiGroup: ""
```

### 1.3 规则语法（Rules）

```yaml
rules:
  - apiGroups: [""]           # "" = core API group ([[concepts/pod-lifecycle|pod]]/Service/ConfigMap)
    resources: ["pods", "services"]
    verbs: ["get", "list"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["*"]
  - nonResourceURLs: ["/healthz", "/version"]  # 非资源路径
    verbs: ["get"]
```

---

## 2. 场景一：命名空间级别只读权限

**场景**: 业务团队需要查看（但不能修改）`app-team` namespace 下的所有资源。

### 2.1 创建 Role

```bash
cat > app-team-readonly.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-team-readonly
  namespace: app-team
rules:
  - apiGroups: ["*"]
    resources: ["*"]
    verbs: ["get", "list", "watch"]
EOF
kubectl apply -f app-team-readonly.yaml
```

### 2.2 创建 RoleBinding（绑定用户）

```bash
cat > app-team-readonly-binding.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-team-readonly-binding
  namespace: app-team
subjects:
  - kind: User
    name: alice@example.com
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: app-team-readonly
  apiGroup: rbac.authorization.k8s.io
EOF
kubectl apply -f app-team-readonly-binding.yaml
```

### 2.3 验证权限

```bash
# 以 alice 身份测试（无修改权限）
kubectl auth can-i create pods -n app-team        # should be "no"
kubectl auth can-i get pods -n app-team            # should be "yes"
kubectl auth can-i delete pod -n app-team          # should be "no"

# 查看 RoleBinding 详情
kubectl describe rolebinding app-team-readonly-binding -n app-team
```

---

## 3. 场景二：跨 namespace 读取权限（ClusterRole + RoleBinding）

**场景**: `monitoring` 服务账号需要读取所有 namespace 的 Pod 和 [[Service|Service]]。

### 3.1 创建 ClusterRole（集群范围只读）

```bash
cat > cluster-readonly.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-readonly
rules:
  - apiGroups: [""]
    resources: ["pods", "services"]
    verbs: ["get", "list", "watch"]
EOF
kubectl apply -f cluster-readonly.yaml
```

### 3.2 创建 RoleBinding（跨 namespace 绑定 SA）

```bash
cat > monitoring-read-all-binding.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: monitoring-read-all
  namespace: default  # 可以在任意 namespace 创建
subjects:
  - kind: ServiceAccount
    name: monitoring-agent
    namespace: monitoring
roleRef:
  kind: ClusterRole
  name: cluster-readonly
  apiGroup: rbac.authorization.k8s.io
EOF
kubectl apply -f monitoring-read-all-binding.yaml
```

---

## 4. 场景三：Deployment 管理者权限（受限编辑）

**场景**: `dev-lead` 需要管理 `backend` namespace 的 Deployment，但不能修改其他资源。

### 4.1 创建受限编辑 Role

```bash
cat > deploy-manager.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: deploy-manager
  namespace: backend
rules:
  - apiGroups: ["apps"]
    resources: ["deployments", "replicasets"]
    verbs: ["*"]
  - apiGroups: [""]
    resources: ["configmaps", "secrets"]
    verbs: ["get", "list", "watch", "update"]
    # 限制只能操作包含 env 配置的 ConfigMap
  - apiGroups: [""]
    resources: ["pods/log"]
    verbs: ["get", "list"]
EOF
kubectl apply -f deploy-manager.yaml
```

### 4.2 绑定用户

```bash
kubectl create rolebinding dev-lead-deploy \
  --role=deploy-manager \
  --user=dev-lead@example.com \
  --namespace=backend
```

---

## 5. 场景四：节点管理员权限（NodeRestriction）

**场景**: 授予 [[kubelet|kubelet]] 正确的节点管理权限（遵循最小权限原则）。

### 5.1 使用内置 ClusterRole

```bash
# 系统已内置 node-admin ClusterRole（绑定到 Node 主体）
kubectl get clusterrole system:node-admin -o yaml

# 为节点绑定（通常由 Node 授权控制器自动完成）
kubectl create clusterrolebinding node-admin-binding \
  --clusterrole=system:node-admin \
  --user=kubelet
```

### 5.2 限制 kubelet 只可修改自己节点的 Pod

```bash
# 使用 nodeRestriction 准入控制器
# kubelet 自动获得 system:node-proxier 和 system:node-proxier 的权限
# 通过 NodeSelector 限制 Pod 调度
```

---

## 6. 场景五：审计日志查看权限

**场景**: 安全团队需要读取所有 namespace 的审计事件，但不能修改任何资源。

### 6.1 创建审计只读 ClusterRole

```bash
cat > audit-reader.yaml <<'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: audit-reader
rules:
  - apiGroups: ["audit.k8s.io"]
    resources: ["events"]
    verbs: ["get", "list", "watch"]
EOF
kubectl apply -f audit-reader.yaml

# 绑定到安全团队组
kubectl create clusterrolebinding audit-reader-group \
  --clusterrole=audit-reader \
  --group=security-team \
  --namespace=kube-system
```

---

## 7. RBAC 调试命令

```bash
# 查看当前用户权限
kubectl auth whoami
# 输出: Username: alice@example.com  Groups: [frontend-team]

# 测试具体权限
kubectl auth can-i <verb> <resource> --namespace=<ns>
kubectl auth can-i get pods -n app-team
kubectl auth can-i "*" "*" --as=alice@example.com  # 模拟他人

# 查看 Role/ClusterRole 定义
kubectl get role -n <ns>
kubectl get clusterrole

# 查看谁有权限操作某个资源
kubectl auth reconcile -f role.yaml  # 自动补充缺失权限（dry-run）
```

---

## 8. 常见错误与修复

| 错误现象 | 原因 | 修复 |
|---------|------|------|
| `Forbidden: ...` | Role 缺少对应 verb | 检查 Role rules 确认包含所需 verb |
| `User "x" groups: []` 无权限 | 用户组未同步 | 检查 LDAP/SSO 组映射配置 |
| 已绑定 Role 但仍 Forbidden | 绑定了 ClusterRole 但 RoleBinding 在不同 namespace | 使用 RoleBinding 绑定 ClusterRole 到正确 namespace |
| ServiceAccount 无权限 | 未正确指定 namespace | 确认 subjects 中的 namespace 与 SA 所在一致 |

---

## 9. 最小权限检查清单

- [ ] 每个 Role 只包含必要的 verbs（不用 `*`）
- [ ] 不使用 `system:*` 预设角色做业务授权
- [ ] ServiceAccount 权限通过 RoleBinding（不是 ClusterRoleBinding）授予
- [ ] 生产环境定期审计 `kubectl get rolebindings -A --as=system:admin`
- [ ] 使用 `kubectl auth can-i` 验证权限生效后再交付

---

```yaml
---
id: LEARN-WEEK2-DAY8
title: Day 8 - K8s RBAC 权限配置实操
topic: security-monitoring
type: hands-on-guide
tags: [rbac, authorization, role, rolebinding, clusterrole, security, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "RBAC 怎么配置只读权限"
  - "Role 和 ClusterRole 区别"
  - "ServiceAccount 权限怎么绑定"
  - "RBAC 调试命令"
  - "最小权限原则怎么实现"
trigger_keywords:
  - RBAC
  - Role
  - ClusterRole
  - RoleBinding
  - ClusterRoleBinding
  - ServiceAccount
  - Subject
  - Verb
  - API Group
  - kubectl auth can-i
  - 权限绑定
  - 最小权限
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
  - security-engineer
estimated_read_time: 45min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-05-security-compliance
related_topics:
  - security
  - rbac
  - authorization
  - serviceaccount
related:
  - domain-11-production-operations/topic-learn/public-training/week-2-security-monitoring/day-09-ram-account-management.md
  - domain-10-troubleshooting-diagnostics/07-rbac-permission-troubleshooting.md
---
```