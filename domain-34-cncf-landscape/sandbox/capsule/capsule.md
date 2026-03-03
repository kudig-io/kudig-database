# Capsule

> **成熟度**: Sandbox | **加入时间**: 2022-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://capsule.clastix.io |
| **GitHub** | https://github.com/projectcapsule/capsule |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Security & Compliance |
| **适用场景** | Kubernetes 多租户管理 |

---

## 项目概述

Capsule 是一个 Kubernetes 多租户框架，允许在单个集群中实现多租户隔离。它通过 Tenant CRD 将多个命名空间组织为逻辑单元，为每个租户提供隔离的资源配额、网络策略和 RBAC 控制。与传统的每租户一集群方案相比，Capsule 显著降低了运维复杂度和成本。

---

## 核心特性

- **多租户隔离**: 单集群内实现强隔离的多租户
- **命名空间聚合**: 将多个命名空间归属到单个租户
- **资源配额**: 租户级别的资源限制和配额
- **网络隔离**: 自动应用 NetworkPolicy 实现租户隔离
- **RBAC 管理**: 租户所有者自助管理命名空间
- **自定义策略**: 限制 NodePort、Ingress、存储类等
- **服务目录**: 控制租户可使用的服务

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Capsule Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Cluster Admin                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Capsule Controller                      │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │  Tenant     │  │  Namespace  │  │   Policy   │  │ │   │
│  │  │  │ Controller  │  │ Controller  │  │  Enforcer  │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Quota     │  │  Network    │  │   RBAC     │  │ │   │
│  │  │  │  Manager    │  │  Policy Mgr │  │  Manager   │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                       Tenant CRDs                               │
│                              │                                   │
│  ┌───────────────────────────▼───────────────────────────────┐  │
│  │                    Tenant: team-a                          │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │  Owner: alice@example.com                            │  │  │
│  │  │  Quota: CPU 10, Memory 20Gi, Pods 100               │  │  │
│  │  │  Namespaces: team-a-dev, team-a-staging, team-a-prod│  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                                                            │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │  │
│  │  │ team-a-dev  │  │team-a-staging│  │  team-a-prod   │   │  │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────────┐│   │  │
│  │  │ │  Pods   │ │  │ │  Pods   │ │  │ │    Pods     ││   │  │
│  │  │ │ Services│ │  │ │ Services│ │  │ │   Services  ││   │  │
│  │  │ └─────────┘ │  │ └─────────┘ │  │ └─────────────┘│   │  │
│  │  │ ResourceQuota│  │ ResourceQuota│  │  ResourceQuota │   │  │
│  │  │ NetworkPolicy│  │ NetworkPolicy│  │  NetworkPolicy │   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │  │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Tenant: team-b                          │ │
│  │  Owner: bob@example.com                                    │ │
│  │  Namespaces: team-b-dev, team-b-prod                      │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Capsule Controller** | 核心控制器，管理租户生命周期 |
| **Tenant CRD** | 租户自定义资源，定义租户配置 |
| **Policy Enforcer** | 策略执行器，确保租户遵守策略 |
| **Quota Manager** | 配额管理器，聚合租户资源使用 |

---

## 快速开始

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add projectcapsule https://projectcapsule.github.io/charts
helm repo update

# 安装 Capsule
helm install capsule projectcapsule/capsule \
  --namespace capsule-system \
  --create-namespace

# 验证安装
kubectl get pods -n capsule-system
```

### Manifest 安装

```bash
kubectl apply -f https://raw.githubusercontent.com/projectcapsule/capsule/main/config/install.yaml
```

---

## Tenant 配置

### 基本 Tenant

```yaml
apiVersion: capsule.clastix.io/v1beta2
kind: Tenant
metadata:
  name: team-alpha
spec:
  owners:
    - name: alice
      kind: User
    - name: dev-team
      kind: Group
  
  # 命名空间配额
  namespaceOptions:
    quota: 5  # 最多 5 个命名空间

---
# 创建命名空间（需要以 owner 身份）
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha-dev
  labels:
    capsule.clastix.io/tenant: team-alpha
```

### 完整 Tenant 配置

```yaml
apiVersion: capsule.clastix.io/v1beta2
kind: Tenant
metadata:
  name: production-team
spec:
  owners:
    - name: platform-admin
      kind: User
      clusterRoles:
        - admin
        - capsule-namespace-deleter
    - name: dev-leads
      kind: Group
      clusterRoles:
        - edit

  # 命名空间选项
  namespaceOptions:
    quota: 10
    additionalMetadata:
      labels:
        team: production
        cost-center: "12345"
      annotations:
        scheduler.alpha.kubernetes.io/defaultTolerations: '[{"operator": "Exists"}]'

  # 资源配额
  resourceQuotas:
    scope: Tenant  # 租户级别聚合
    items:
      - hard:
          requests.cpu: "20"
          requests.memory: "40Gi"
          limits.cpu: "40"
          limits.memory: "80Gi"
          pods: "100"
          services: "50"
          persistentvolumeclaims: "20"

  # LimitRange
  limitRanges:
    items:
      - limits:
          - type: Pod
            min:
              cpu: "50m"
              memory: "64Mi"
            max:
              cpu: "4"
              memory: "8Gi"
          - type: Container
            default:
              cpu: "200m"
              memory: "256Mi"
            defaultRequest:
              cpu: "100m"
              memory: "128Mi"

  # 网络策略
  networkPolicies:
    items:
      - policyTypes:
          - Ingress
          - Egress
        ingress:
          - from:
              - namespaceSelector:
                  matchLabels:
                    capsule.clastix.io/tenant: production-team
        egress:
          - to:
              - namespaceSelector:
                  matchLabels:
                    capsule.clastix.io/tenant: production-team
          - to:
              - ipBlock:
                  cidr: 0.0.0.0/0
            ports:
              - protocol: TCP
                port: 443

  # 服务选项
  serviceOptions:
    allowedServices:
      nodePort: false
      loadBalancer: false
      externalName: false
    externalServiceIPs:
      allowed: []

  # 存储类限制
  storageClasses:
    allowed:
      - standard
      - fast-ssd
    allowedRegex: "^gp.*"

  # Ingress 选项
  ingressOptions:
    allowedClasses:
      allowed:
        - nginx
    allowedHostnames:
      allowedRegex: "^.*\\.team-alpha\\.example\\.com$"
    hostnameCollisionScope: Tenant

  # 容器镜像仓库限制
  containerRegistries:
    allowed:
      - docker.io
      - gcr.io
      - ghcr.io
    allowedRegex: "^.*\\.company\\.com$"

  # 节点选择
  nodeSelector:
    kubernetes.io/os: linux
    node-type: worker

  # 优先级类
  priorityClasses:
    allowed:
      - standard
      - high-priority
```

---

## 租户所有者操作

### 创建命名空间

```bash
# 使用租户所有者身份
kubectl create namespace team-alpha-dev \
  --as alice \
  --as-group capsule.clastix.io

# 或使用 YAML
kubectl apply -f - --as alice <<EOF
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha-staging
EOF
```

### 部署应用

```bash
# 租户所有者在自己的命名空间中部署
kubectl create deployment nginx --image=nginx \
  -n team-alpha-dev \
  --as alice
```

---

## GlobalTenantResource

### 共享资源配置

```yaml
apiVersion: capsule.clastix.io/v1beta2
kind: GlobalTenantResource
metadata:
  name: shared-config
spec:
  tenantSelector:
    matchLabels:
      tier: premium
  
  resources:
    - namespaceSelector:
        matchLabels:
          shared: "true"
      additionalMetadata:
        labels:
          managed-by: capsule
      rawItems:
        - apiVersion: v1
          kind: ConfigMap
          metadata:
            name: shared-settings
          data:
            log-level: info
            
        - apiVersion: v1
          kind: Secret
          metadata:
            name: registry-credentials
          type: kubernetes.io/dockerconfigjson
          data:
            .dockerconfigjson: "base64-encoded-config"
```

---

## TenantResource

### 租户专属资源

```yaml
apiVersion: capsule.clastix.io/v1beta2
kind: TenantResource
metadata:
  name: team-alpha-resources
  namespace: capsule-system
spec:
  tenantRef:
    name: team-alpha
    
  resources:
    - namespaceSelector:
        matchLabels: {}  # 所有租户命名空间
      rawItems:
        - apiVersion: v1
          kind: ConfigMap
          metadata:
            name: team-config
          data:
            team: alpha
            environment: production
```

---

## Capsule Proxy

### 部署 Capsule Proxy

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: capsule-proxy
  namespace: capsule-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: capsule-proxy
  template:
    metadata:
      labels:
        app: capsule-proxy
    spec:
      containers:
        - name: capsule-proxy
          image: ghcr.io/projectcapsule/capsule-proxy:latest
          args:
            - --listening-port=9001
            - --capsule-user-group=capsule.clastix.io
          ports:
            - containerPort: 9001
```

### 使用 Proxy 列出命名空间

```bash
# 配置 kubeconfig 使用 capsule-proxy
kubectl --server=https://capsule-proxy:9001 get namespaces --as alice
# 只显示 alice 拥有的租户的命名空间
```

---

## RBAC 集成

### 租户所有者 ClusterRole

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: capsule-namespace-deleter
rules:
  - apiGroups: [""]
    resources: ["namespaces"]
    verbs: ["delete"]
    
---
apiVersion: capsule.clastix.io/v1beta2
kind: Tenant
metadata:
  name: team-with-delete
spec:
  owners:
    - name: admin-user
      kind: User
      clusterRoles:
        - admin
        - capsule-namespace-deleter  # 允许删除命名空间
```

---

## 最佳实践

1. **租户规划**: 按团队或项目划分租户
2. **配额设置**: 合理设置资源配额防止滥用
3. **网络隔离**: 默认启用租户间网络隔离
4. **镜像限制**: 限制容器镜像来源
5. **Proxy 使用**: 使用 Capsule Proxy 提升用户体验
6. **审计日志**: 启用 Kubernetes 审计追踪租户操作

---

## 参考资源

- [官方文档](https://capsule.clastix.io/docs/)
- [GitHub Repo](https://github.com/projectcapsule/capsule)
- [Tenant 配置](https://capsule.clastix.io/docs/tenants/)
- [Capsule Proxy](https://capsule.clastix.io/docs/proxy/)
- [示例配置](https://github.com/projectcapsule/capsule/tree/main/config/samples)

---

**维护者**: Kudig Team | **许可证**: MIT
