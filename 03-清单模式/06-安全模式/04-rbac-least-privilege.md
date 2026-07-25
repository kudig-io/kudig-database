---
title: RBAC 最小权限模式
description: Role/ClusterRole 最小权限设计模式与安全最佳实践
summary: 遵循最小权限原则设计 RBAC，避免通配符权限、过度绑定 ClusterRole 及使用 audit 日志审计
category: manifests-patterns
tags:
- k8s
- manifests
- security
- rbac
- least-privilege
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- RBAC 最小权限设计
- 如何避免 RBAC 通配符
- Kubernetes 权限审计
trigger_keywords:
- rbac
- role
- clusterrole
- least-privilege
- audit
prerequisites:
- k8s-rbac-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# RBAC 最小权限模式

## 1. 最小权限原则

```
错误: verbs: ["*"] resources: ["*"]     ← 通配符，危险！
正确: verbs: ["get", "list"] resources: ["pods"]  ← 精确指定 ✅
```

## 2. 应用级 Role（Namespace 内）

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: backend-reader
  namespace: app-backend
rules:
  # 读取 ConfigMap 和 Secret
  - apiGroups: [""]
    resources: ["configmaps"]
    resourceNames: ["app-config", "feature-flags"]
    verbs: ["get", "list", "watch"]
  # 读取 Pod（用于服务发现）
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
  # 读取 Service 和 Endpoints
  - apiGroups: [""]
    resources: ["services", "endpoints"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: backend-reader-binding
  namespace: app-backend
subjects:
  - kind: ServiceAccount
    name: backend-sa
    namespace: app-backend
roleRef:
  kind: Role
  name: backend-reader
  apiGroup: rbac.authorization.k8s.io
```

## 3. 避免 ClusterRole 的场景

```yaml
# ❌ 错误：使用 ClusterRole 仅为了读 Pod（全集群）
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: pod-reader
rules:
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
# 这会允许读取所有命名空间的 Pod！

# ✅ 正确：使用 Role 限制在特定 Namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: app-backend
rules:
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
```

## 4. Operator 级 ClusterRole（必要场景）

Operator 需要管理 CRD，使用精确的 ClusterRole：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: webapp-operator-role
rules:
  # 管理 CRD 资源
  - apiGroups: ["platform.example.com"]
    resources: ["webapps", "webapps/status", "webapps/finalizers"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  # 管理 Deployment/Service
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  - apiGroups: [""]
    resources: ["services", "configmaps", "events"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  # Leader Election 需要
  - apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
```

## 5. CI/CD 部署专用 ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: deployer
  namespace: app-backend
  annotations:
    iam.gke.io/gcp-service-account: deployer@project.iam.gserviceaccount.com
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: deployer-role
  namespace: app-backend
rules:
  # 部署相关资源
  - apiGroups: ["apps"]
    resources: ["deployments", "replicasets"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  - apiGroups: [""]
    resources: ["services", "configmaps"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  # 重启/扩缩容
  - apiGroups: ["apps"]
    resources: ["deployments/scale"]
    verbs: ["get", "update", "patch"]
  # 查看 Pod 日志（调试）
  - apiGroups: [""]
    resources: ["pods", "pods/log"]
    verbs: ["get", "list"]
```

## 6. 使用 audit-log 审计权限

```yaml
# kube-apiserver 配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
    - name: kube-apiserver
      command:
        - kube-apiserver
        - --audit-policy-file=/etc/kubernetes/audit-policy.yaml
        - --audit-log-path=/var/log/kubernetes/audit/audit.log
        - --audit-log-maxage=30
        - --audit-log-maxbackup=10
        - --audit-log-maxsize=100
```

```yaml
# audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 记录 Secret 访问
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets"]
  # 记录 RBAC 变更
  - level: RequestResponse
    resources:
      - group: "rbac.authorization.k8s.io"
        resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  # 记录特权操作
  - level: RequestResponse
    verbs: ["create", "delete", "update", "patch"]
    resources:
      - group: ""
        resources: ["pods/exec", "pods/portforward"]
```

## 7. 权限审计工具

```bash
# 🟢 低风险：权限审计（只读）
# 检查 SA 是否有权限
kubectl auth can-i list pods --as=system:serviceaccount:app-backend:backend-sa -n app-backend

# 检查是否有通配符权限
kubectl get clusterrole -o json | jq '.items[] | select(.rules[] | .verbs[] | contains("*"))'

# 列出所有有 list secrets 权限的 SA
kubectl get rolebinding,clusterrolebinding --all-namespaces -o json \
  | jq '.items[] | select(.roleRef.kind | test("Role|ClusterRole"))'

# 使用 rbac-lookup 工具
rbac-lookup deployer  # 查看某用户的所有权限
```

## 8. 常见 RBAC 反模式

| 反模式 | 风险 | 修复 |
|--------|------|------|
| `verbs: ["*"]` | 可删除资源 | 精确指定 verbs |
| `resources: ["*"]` | 可访问所有资源 | 列出具体资源 |
| 绑定 `cluster-admin` | 集群管理员权限 | 使用自定义 Role |
| SA 长期 Token | Token 泄露风险 | 使用 TokenRequest 短期 Token |
| 跨 Namespace 引用 SA | 权限扩散 | 限制在同一 Namespace |

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 定期审计 RBAC | 使用 `audit2rbac` 或 `rakkess` |
| 使用 `resourceNames` 精确限制 | 只允许访问特定名称的资源 |
| 禁止 `cluster-admin` 给应用 | 使用最小权限 Role |
| 开启 audit-log | 记录敏感操作 |
| 使用 IRSA/Workload Identity | 避免长期凭证 |

## Related

- [[03-清单模式/06-安全模式/05-service-account-patterns|ServiceAccount 设计]]
- [[03-清单模式/01-YAML参考/20-rbac-role-rolebinding|RBAC Role 参考]]

## See Also

- [RBAC 最小权限指南](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [audit2rbac 工具](https://github.com/liggitt/audit2rbac)

<!-- risk-assessed -->
