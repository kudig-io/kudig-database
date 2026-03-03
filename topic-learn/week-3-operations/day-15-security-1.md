# Day 15: 安全体系 - RBAC + 认证授权

> **学习时间**: 4-5 小时 | **主题**: K8s 认证授权机制

---

## 今日目标

- [ ] 理解 K8s 认证链和 ServiceAccount
- [ ] 掌握 RBAC 四种资源的配置
- [ ] 能够设计最小权限的访问控制

---

## 理论学习 (2h)

### 必读文档

1. **认证授权系统**
   - 文件: `../../domain-7-security/01-authentication-authorization-system.md`
   - 重点: K8s 认证链、ServiceAccount、Token

2. **RBAC 矩阵配置**
   - 文件: `../../domain-7-security/07-rbac-matrix-configuration.md`
   - 重点: Role/ClusterRole/Binding 设计模式

3. **证书管理**
   - 文件: `../../domain-7-security/10-certificate-management.md`
   - 重点: 证书轮转、kubeconfig 管理

---

## 实践任务 (2.5h)

### 任务 1: ServiceAccount 实践 (30min)

```bash
# 创建 namespace
kubectl create namespace rbac-test

# 创建 ServiceAccount
kubectl create serviceaccount dev-sa -n rbac-test

# 查看 ServiceAccount
kubectl get sa -n rbac-test
kubectl describe sa dev-sa -n rbac-test

# 创建 Token
kubectl create token dev-sa -n rbac-test

# 使用 ServiceAccount 的 Pod
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

# 查看 Pod 挂载的 Token
kubectl exec -n rbac-test sa-test -- cat /var/run/secrets/kubernetes.io/serviceaccount/token
```

### 任务 2: RBAC 配置实践 (1h)

```bash
# 创建 Role (namespace 级别)
cat > role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: rbac-test
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get"]
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

# 验证权限
kubectl auth can-i get pods --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test
kubectl auth can-i delete pods --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test

# ClusterRole (集群级别)
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
```

### 任务 3: 权限排查 (30min)

```bash
# 检查当前用户权限
kubectl auth can-i --list

# 检查特定用户权限
kubectl auth can-i --list --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test

# 模拟权限不足场景
kubectl auth can-i create deployments --as=system:serviceaccount:rbac-test:dev-sa -n rbac-test

# 清理
kubectl delete namespace rbac-test
```

---

## 费曼复述 (0.5h)

1. **Role 和 ClusterRole 的区别？RoleBinding 和 ClusterRoleBinding 的区别？**
2. **ServiceAccount 在 Pod 中如何使用？**
3. **如何设计最小权限原则的 RBAC？**

---

## 今日检验

- [ ] 能够创建 ServiceAccount 并理解其用途
- [ ] 能够配置 Role/RoleBinding 实现权限控制
- [ ] 能够使用 `kubectl auth can-i` 排查权限问题
