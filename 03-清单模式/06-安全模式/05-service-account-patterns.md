---
title: ServiceAccount 设计模式
description: ServiceAccount 隔离、权限绑定与 Workload Identity 集成
summary: ServiceAccount 粒度设计、Token 管理、云 IAM 集成及跨命名空间访问模式
category: manifests-patterns
tags:
- k8s
- manifests
- security
- serviceaccount
- workload-identity
- iam
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
- ServiceAccount 如何设计
- Workload Identity 配置
- Kubernetes SA 粒度
trigger_keywords:
- serviceaccount
- workload-identity
- irsa
- token
- iam
prerequisites:
- k8s-rbac-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# ServiceAccount 设计模式

## 1. SA 粒度设计原则

```
❌ 错误：一个 SA 用于所有应用
namespace: production
  SA: default ← 所有应用共用

✅ 正确：每个应用独立 SA
namespace: production
  SA: frontend-sa
  SA: backend-sa
  SA: database-sa
```

## 2. 应用级 ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: backend-sa
  namespace: production
  annotations:
    # AWS IRSA
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789:role/backend-role
    # GCP Workload Identity
    iam.gke.io/gcp-service-account: backend-sa@project.iam.gserviceaccount.com
    # Azure Workload Identity
    azure.workload.identity/client-id: 12345678-1234-1234-1234-123456789abc
  labels:
    app.kubernetes.io/name: backend
    app.kubernetes.io/part-of: platform
automountServiceAccountToken: false  # 不自动挂载 Token（按需使用）
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: backend-sa
      automountServiceAccountToken: true  # 该应用需要访问 API
      containers:
        - name: backend
          image: registry.example.com/backend:v1.0.0
```

## 3. Token 管理

### 3.1 短期 Token（ProjectedVolume）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: api-client
spec:
  serviceAccountName: backend-sa
  containers:
    - name: app
      image: registry.example.com/app:v1.0.0
      volumeMounts:
        - name: sa-token
          mountPath: /var/run/secrets/tokens
  volumes:
    - name: sa-token
      projected:
        sources:
          - serviceAccountToken:
              path: token            # Token 文件路径
              expirationSeconds: 3600  # 1 小时过期
              audience: api-server   # 受众限定
```

### 3.2 BoundServiceAccountTokenVolume

```yaml
# kube-apiserver 启用 BoundServiceAccountTokenVolume
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
  - name: ServiceAccount
    configuration:
      apiVersion: apiserver.config.k8s.io/v1
      kind: ServiceAccountConfiguration
      issuers:
        - issuer: https://kubernetes.default.svc
          apiAudiences: ["api-server"]
```

## 4. AWS IRSA（IAM Roles for Service Accounts）

```yaml
# 1. 创建 SA 并关联 IAM Role
apiVersion: v1
kind: ServiceAccount
metadata:
  name: backend-s3-access
  namespace: production
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789:role/s3-access-role
---
# 2. Pod 使用该 SA
apiVersion: apps/v1
kind: Deployment
metadata:
  name: file-processor
spec:
  template:
    spec:
      serviceAccountName: backend-s3-access
      containers:
        - name: processor
          image: registry.example.com/processor:v1.0.0
          # AWS SDK 会自动使用 IRSA 获取临时凭证
```

## 5. GCP Workload Identity

```yaml
# 1. Kubernetes SA
apiVersion: v1
kind: ServiceAccount
metadata:
  name: gcs-access
  namespace: production
  annotations:
    iam.gke.io/gcp-service-account: gcs-sa@my-project.iam.gserviceaccount.com
---
# 2. Pod 使用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: data-exporter
spec:
  template:
    metadata:
      labels:
        app: data-exporter
    spec:
      serviceAccountName: gcs-access
      nodeSelector:
        iam.gke.io/gke-metadata-server-enabled: "true"
      containers:
        - name: exporter
          image: registry.example.com/exporter:v1.0.0
```

## 6. 跨命名空间访问（暂时不可直接绑定）

Kubernetes 不允许 RoleBinding 直接引用其他 Namespace 的 SA。变通方案：

```yaml
# 方案：使用 ClusterRoleBinding + 条件
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: monitoring-sa-readonly
subjects:
  - kind: ServiceAccount
    name: prometheus
    namespace: monitoring
roleRef:
  kind: ClusterRole
  name: view          # 使用内置的 view 角色（只读）
  apiGroup: rbac.authorization.k8s.io
```

## 7. 禁用 default SA

```yaml
# 禁用 default SA 的 Token 自动挂载
apiVersion: v1
kind: ServiceAccount
metadata:
  name: default
  namespace: production
automountServiceAccountToken: false  # 全局禁用
---
# Pod 级别也禁用
apiVersion: v1
kind: Pod
metadata:
  name: no-api-access
spec:
  automountServiceAccountToken: false
  containers:
    - name: app
      image: nginx:1.25
```

## 8. 审计 SA 使用情况

```bash
# 🟢 低风险：安全审计
# 查找所有使用 default SA 的 Pod（安全风险）
kubectl get pods --all-namespaces -o json \
  | jq '.items[] | select(.spec.serviceAccountName == "default" or .spec.serviceAccountName == null) | .metadata.namespace + "/" + .metadata.name'

# 检查 SA 的 Token 自动挂载状态
kubectl get sa --all-namespaces -o json \
  | jq '.items[] | select(.automountServiceAccountToken != false) | .metadata.namespace + "/" + .metadata.name'

# 查看 SA 绑定的权限
kubectl get rolebinding,clusterrolebinding --all-namespaces -o json \
  | jq '.items[] | select(.subjects[]?.kind == "ServiceAccount")'
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 每个应用独立 SA | 权限隔离、审计追踪 |
| 禁用 default SA | 避免意外权限继承 |
| 使用 IRSA/Workload Identity | 避免静态云凭证 |
| 定期轮换 Token | 使用短期 Token |
| 审计 SA 使用 | 定期检查是否有未使用的 SA |

## Related

- [[03-清单模式/06-安全模式/04-rbac-least-privilege|RBAC 最小权限]]
- [[03-清单模式/01-YAML参考/19-serviceaccount-token|ServiceAccount 参考]]

## See Also

- [ServiceAccount 文档](https://kubernetes.io/docs/tasks/configure-pod-container/configure-service-account/)
- [AWS IRSA](https://docs.aws.amazon.com/eks/latest/userguide/iam-roles-for-service-accounts.html)
- [GCP Workload Identity](https://cloud.google.com/kubernetes-engine/docs/how-to/workload-identity)

<!-- risk-assessed -->
