# Day 8: K8S 集群 RBAC

> **学习时间**: 4-5 小时 | **主题**: RBAC 权限模型与配置实践

---

## 今日目标

- [ ] 理解 RBAC 四种核心资源及其关系
- [ ] 能够创建 Role/ClusterRole 和绑定
- [ ] 掌握 ServiceAccount 与 RBAC 的关联
- [ ] 了解 ACK 预置的 ClusterRole

---

## 理论学习 (2h)

### 必读文档

1. **认证授权体系**
   - 文件: `../../../domain-7-security/01-authentication-authorization-system.md`
   - 重点: K8S 认证、授权、准入控制三个阶段

2. **RBAC 矩阵配置**
   - 文件: `../../../domain-7-security/07-rbac-matrix-configuration.md`
   - 重点: RBAC 资源定义、权限矩阵设计

### 阅读要点

- RBAC 四种资源: Role (Namespace 级)、ClusterRole (集群级)、RoleBinding、ClusterRoleBinding
- 权限三要素: API Group + Resource + Verb
- Verb 包括: get, list, watch, create, update, patch, delete
- ACK 预置角色: cluster-admin, admin, edit, view

---

## 实践任务 (2.5h)

### 任务 1: 查看现有 RBAC 配置 (30min)

```bash
# 查看预置 ClusterRole
kubectl get clusterroles | head -20
kubectl describe clusterrole cluster-admin
kubectl describe clusterrole admin
kubectl describe clusterrole edit
kubectl describe clusterrole view

# 查看 ClusterRoleBinding
kubectl get clusterrolebindings | head -20

# 查看某个用户的权限
kubectl auth can-i --list
kubectl auth can-i create pods
kubectl auth can-i delete nodes
```

### 任务 2: 创建自定义 RBAC (45min)

```bash
# 创建测试 Namespace
kubectl create namespace rbac-test

# 创建只读 Role
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
EOF
kubectl apply -f readonly-role.yaml

# 创建 ServiceAccount
kubectl create serviceaccount dev-user -n rbac-test

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
```

### 任务 3: 验证 RBAC 权限 (45min)

```bash
# 使用 dev-user 的身份测试权限
kubectl auth can-i get pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 应该返回 yes

kubectl auth can-i create pods -n rbac-test --as=system:serviceaccount:rbac-test:dev-user
# 应该返回 no

kubectl auth can-i get pods -n default --as=system:serviceaccount:rbac-test:dev-user
# 应该返回 no (Role 仅限 rbac-test namespace)

# 创建运维角色 (更多权限)
cat > ops-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ops-engineer
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets", "events"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec"]
  verbs: ["get", "create"]
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch"]
EOF
kubectl apply -f ops-role.yaml
```

### 任务 4: ACK RBAC 最佳实践 (30min)

```bash
# 查看 ACK 集群中的自定义 ClusterRole
kubectl get clusterroles | grep -v system

# 查看 ACK 预置的权限绑定
kubectl get clusterrolebindings | grep ack

# 最佳实践:
# 1. 遵循最小权限原则
# 2. 使用 Role 而非 ClusterRole (除非确实需要集群级权限)
# 3. 避免使用 cluster-admin
# 4. 为每个团队/用户创建独立的 ServiceAccount
# 5. 定期审查权限配置

# 清理测试资源
kubectl delete namespace rbac-test
kubectl delete clusterrole ops-engineer
```

---

## 费曼复述 (0.5h)

1. **RBAC 中 Role 和 ClusterRole 的区别是什么？**
2. **如何实现"某用户只能查看特定 Namespace 的 Pod"？**
3. **为什么生产环境要避免使用 cluster-admin？**

---

## 今日检验

- [ ] 能创建 Role/ClusterRole 并绑定到 ServiceAccount
- [ ] 能使用 `kubectl auth can-i` 验证权限
- [ ] 理解 RBAC 四种资源的关系
- [ ] 了解 ACK 预置的 ClusterRole

---

## 核心概念总结

| 资源 | 作用域 | 用途 |
|------|--------|------|
| Role | Namespace | 定义 Namespace 级权限 |
| ClusterRole | 集群 | 定义集群级权限或可复用模板 |
| RoleBinding | Namespace | 将 Role/ClusterRole 绑定到用户 |
| ClusterRoleBinding | 集群 | 将 ClusterRole 绑定到用户 (集群范围) |

---

## 明日预告

Day 9 将学习 RAM 账号与 K8S 的集成方案，理解阿里云 RAM 权限如何映射到 ACK 集群。
