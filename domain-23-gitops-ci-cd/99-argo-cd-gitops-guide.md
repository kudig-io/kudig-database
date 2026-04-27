# Argo CD 企业级 GitOps 实践指南

> **适用版本**: Argo CD v3.3.8 / Helm Chart v7.8.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、架构设计](#一架构设计)
- [二、Helm 部署](#二helm-部署)
- [三、多租户与 RBAC](#三多租户与-rbac)
- [四、Application 与 AppProject](#四application-与-appproject)
- [五、ApplicationSet 多环境管理](#五applicationset-多环境管理)
- [六、密钥管理集成](#六密钥管理集成)
- [七、监控与告警](#七监控与告警)
- [八、升级与维护](#八升级与维护)

---

## 一、架构设计

### 1.1 单实例架构

```
┌─────────────────────────────────────────┐
│              Argo CD                    │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐ │
│  │ API     │  │ Repo    │  │ App     │ │
│  │ Server  │  │ Server  │  │ Control │ │
│  └─────────┘  └─────────┘  └─────────┘ │
│       │            │            │       │
│  ┌────┴────────────┴────────────┴───┐  │
│  │         Redis (缓存/锁)          │  │
│  └──────────────────────────────────┘  │
│       │                                 │
│  ┌────┴────────────────────────────┐   │
│  │  Git Repository (Source of Truth)│   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### 1.2 高可用架构

```
┌─────────────────────────────────────────┐
│            Ingress / LB                 │
│  ┌─────────┐              ┌─────────┐   │
│  │ Argo CD │◄────────────►│ Argo CD │   │
│  │ Server  │   HA Redis   │ Server  │   │
│  │ (x2)    │   (x3)       │ (x2)    │   │
│  └────┬────┘              └────┬────┘   │
│       └──────────┬─────────────┘        │
│                  Redis                 │
│              Sentinel Mode             │
└─────────────────────────────────────────┘
```

---

## 二、Helm 部署

### 2.1 生产级 values

```yaml
# values-argo-cd-production.yaml
cat << 'EOF' > values-argo-cd-production.yaml
global:
  domain: argocd.example.com

configs:
  cm:
    # 默认资源排除 (Argo CD v3.0+)
    # 自动排除高变动资源以减轻 API Server 压力
    resource.exclusions: |
      - apiGroups:
        - ""
        kinds:
        - Endpoints
        - EndpointSlice
        - Lease
        - SelfSubjectReview
        - TokenReview
        clusters:
        - "*"
      - apiGroups:
        - cilium.io
        kinds:
        - CiliumIdentity
        - CiliumEndpoint
        - CiliumEndpointSlice
        clusters:
        - "*"

  rbac:
    policy.default: role:readonly
    policy.csv: |
      p, role:org-admin, applications, *, */*, allow
      p, role:org-admin, clusters, get, *, allow
      p, role:org-admin, repositories, *, *, allow
      g, your-github-org:admin-team, role:org-admin

  secret:
    extra:
      # 用于加密 Application 中的敏感数据
      argocd.secretkey: "<base64-encoded-32-byte-key>"

dex:
  enabled: true
  env:
    - name: ARGO_WORKFLOWS_SSO_CLIENT_SECRET
      valueFrom:
        secretKeyRef:
          name: argo-workflows-sso
          key: client-secret

server:
  replicas: 2
  resources:
    requests:
      memory: 256Mi
      cpu: 100m
    limits:
      memory: 512Mi
      cpu: 500m
  ingress:
    enabled: true
    ingressClassName: nginx
    annotations:
      cert-manager.io/cluster-issuer: "letsencrypt-prod"
      nginx.ingress.kubernetes.io/ssl-redirect: "true"
    tls: true

repoServer:
  replicas: 2
  resources:
    requests:
      memory: 256Mi
      cpu: 100m
    limits:
      memory: 1Gi
      cpu: 1000m
  # 支持 Helm 插件
  volumes:
    - name: custom-tools
      emptyDir: {}
  volumeMounts:
    - name: custom-tools
      mountPath: /usr/local/bin/ksops
  initContainers:
    - name: download-tools
      image: alpine:3.19
      command: [sh, -c]
      args:
        - wget -O /custom-tools/ksops https://github.com/viaduct-ai/kustomize-sops/releases/download/v4.3.3/ksops_4.3.3_Linux_x86_64.tar.gz &&
          tar -xzf /custom-tools/ksops -C /custom-tools &&
          chmod +x /custom-tools/ksops
      volumeMounts:
        - name: custom-tools
          mountPath: /custom-tools

controller:
  replicas: 1  # 只能单实例 (状态机)
  resources:
    requests:
      memory: 512Mi
      cpu: 250m
    limits:
      memory: 2Gi
      cpu: 2000m
  # 管理大量应用时增加 workers
  args:
    - --repo-server-timeout-seconds=120
    - --status-processors=20
    - --operation-processors=10

redis:
  enabled: true
  # 生产环境建议使用外部 Redis HA
EOF

# 部署
helm repo add argo https://argoproj.github.io/argo-helm
helm install argocd argo/argo-cd \
  --namespace argocd \
  --create-namespace \
  --values values-argo-cd-production.yaml \
  --version 7.8.0
```

---

## 三、多租户与 RBAC

### 3.1 AppProject 隔离

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AppProject
metadata:
  name: team-alpha
  namespace: argocd
spec:
  description: "Team Alpha Production Environment"
  # 允许的源仓库
  sourceRepos:
    - "https://github.com/company/team-alpha-apps.git"
  # 允许部署的目标集群和命名空间
  destinations:
    - namespace: "team-alpha-*"
      server: https://kubernetes.default.svc
  # 允许的资源类型 (白名单)
  clusterResourceWhitelist:
    - group: ""
      kind: Namespace
    - group: rbac.authorization.k8s.io
      kind: ClusterRole
    - group: rbac.authorization.k8s.io
      kind: ClusterRoleBinding
  # 禁止的资源
  namespaceResourceBlacklist:
    - group: ''
      kind: ResourceQuota
  # 角色绑定
  roles:
    - name: admin
      description: "Team Alpha Admin"
      policies:
        - p, proj:team-alpha:admin, applications, *, team-alpha/*, allow
      groups:
        - "github-org:team-alpha-admin"
    - name: readonly
      description: "Team Alpha Read Only"
      policies:
        - p, proj:team-alpha:readonly, applications, get, team-alpha/*, allow
      groups:
        - "github-org:team-alpha"
```

---

## 四、Application 与 AppProject

### 4.1 标准 Application

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: team-alpha-api
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io  # 级联删除
spec:
  project: team-alpha
  source:
    repoURL: https://github.com/company/team-alpha-apps.git
    targetRevision: main
    path: apps/api/overlays/production
    helm:
      valueFiles:
        - values-production.yaml
      parameters:
        - name: replicaCount
          value: "3"
  destination:
    server: https://kubernetes.default.svc
    namespace: team-alpha-production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
      allowEmpty: false
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
      - PruneLast=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  revisionHistoryLimit: 10
```

---

## 五、ApplicationSet 多环境管理

### 5.1 Git 生成器 (多环境)

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: microservices
  namespace: argocd
spec:
  generators:
    - git:
        repoURL: https://github.com/company/gitops.git
        revision: main
        directories:
          - path: apps/*/overlays/*
  template:
    metadata:
      name: '{% raw %}{{path[1]}}-{{path[3]}}{% endraw %}'
    spec:
      project: default
      source:
        repoURL: https://github.com/company/gitops.git
        targetRevision: main
        path: '{% raw %}{{path}}{% endraw %}'
      destination:
        server: https://kubernetes.default.svc
        namespace: '{% raw %}{{path[1]}}-{{path[3]}}{% endraw %}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

### 5.2 集群生成器 (多集群)

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: cluster-addons
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            environment: production
  template:
    metadata:
      name: '{% raw %}addons-{{name}}{% endraw %}'
    spec:
      project: infrastructure
      source:
        repoURL: https://github.com/company/infrastructure.git
        targetRevision: main
        path: addons/base
      destination:
        server: '{% raw %}{{server}}{% endraw %}'
        namespace: kube-system
```

---

## 六、密钥管理集成

### 6.1 External Secrets Operator (推荐)

```yaml
# 在 Git 中只存放 ExternalSecret 引用
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: api-secrets
  namespace: team-alpha-production
spec:
  refreshInterval: 1h
  secretStoreRef:
    kind: ClusterSecretStore
    name: vault-backend
  target:
    name: api-secrets
    creationPolicy: Owner
  data:
    - secretKey: DATABASE_URL
      remoteRef:
        key: secret/data/team-alpha/api
        property: database_url
```

### 6.2 Sealed Secrets (简单场景)

```bash
# 客户端加密
kubeseal --controller-namespace=kube-system \
  --controller-name=sealed-secrets \
  < secret.yaml > sealed-secret.yaml

# sealed-secret.yaml 可安全提交到 Git
```

---

## 七、监控与告警

### 7.1 Prometheus 监控 Argo CD

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: argocd-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app.kubernetes.io/part-of: argocd
  namespaceSelector:
    matchNames:
      - argocd
  endpoints:
  - port: metrics
    interval: 30s
```

### 7.2 关键告警规则

```yaml
- alert: ArgoCDAppSyncFailed
  expr: argocd_app_info{sync_status="OutOfSync"} == 1
  for: 15m
  labels:
    severity: warning
  annotations:
    summary: "Argo CD Application 同步失败"

- alert: ArgoCDAppDegraded
  expr: argocd_app_info{health_status="Degraded"} == 1
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "Argo CD Application 处于降级状态"
```

---

## 八、升级与维护

### 8.1 Argo CD 升级路径

```bash
# 1. 备份
argocd admin export > argocd-backup.yaml

# 2. 查看升级说明
# https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/

# 3. 升级 Helm Chart
helm upgrade argocd argo/argo-cd \
  --namespace argocd \
  --values values-argo-cd-production.yaml \
  --version <new-version>

# 4. 验证
argocd version
kubectl get applications -n argocd
```

### 8.2 版本支持周期

- 每 3 个月一个 minor 版本
- 支持当前版本 + 前两个版本
- 建议保持最新补丁版本

---

## 参考链接

- [Argo CD 官方文档](https://argo-cd.readthedocs.io/)
- [Argo CD Helm Chart](https://github.com/argoproj/argo-helm/tree/main/charts/argo-cd)
- [ApplicationSet 文档](https://argo-cd.readthedocs.io/en/stable/operator-manual/applicationset/)
- [Argo CD 安全最佳实践](https://argo-cd.readthedocs.io/en/stable/operator-manual/security/)
