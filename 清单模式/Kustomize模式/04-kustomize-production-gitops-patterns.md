---
title: Kustomize Production Patterns — Multi-Environment, Patches, and GitOps Integration
description: Kustomize 生产模式 — 多环境管理、Strategic Merge Patch、JSON Patch、组件复用、ArgoCD/Flux 集成、安全实践
summary: Kustomize 在 GitOps 工作流中的生产级使用模式与最佳实践
category: practice
tags:
- kustomize
- gitops
- multi-environment
- patches
- configuration
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: manifest
---
# Kustomize 生产模式 — 多环境与 GitOps 集成

> 面向生产的 Kustomize 配置管理模式与 GitOps 工作流集成。

## 目录结构（生产推荐）

```
gitops-repo/
├── base/                          # 基础配置（环境无关）
│   ├── kustomization.yaml
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── hpa.yaml
│   └── configmap.yaml
├── components/                    # 可复用组件
│   ├── monitoring/
│   │   ├── kustomization.yaml
│   │   └── servicemonitor.yaml
│   ├── network-policy/
│   │   ├── kustomization.yaml
│   │   └── netpol.yaml
│   └── tls/
│       ├── kustomization.yaml
│       └── certificate.yaml
├── overlays/
│   ├── development/
│   │   ├── kustomization.yaml
│   │   ├── patches/
│   │   │   ├── replicas.yaml
│   │   │   └── resources.yaml
│   │   └── configmap-patch.yaml
│   ├── staging/
│   │   ├── kustomization.yaml
│   │   └── patches/
│   └── production/
│       ├── kustomization.yaml
│       ├── patches/
│       │   ├── replicas.yaml
│       │   ├── resources.yaml
│       │   ├── pdb.yaml
│       │   └── security.yaml
│       └── namespace.yaml
└── clusters/                      # 多集群
    ├── cn-east/
    │   └── kustomization.yaml
    └── us-west/
        └── kustomization.yaml
```

## Base 配置

```yaml
# base/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
metadata:
  name: api-server-base
resources:
  - deployment.yaml
  - service.yaml
  - hpa.yaml
  - configmap.yaml
commonLabels:
  app.kubernetes.io/name: api-server
  app.kubernetes.io/part-of: platform
commonAnnotations:
  app.kubernetes.io/managed-by: kustomize
```

```yaml
# base/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  replicas: 2
  selector:
    matchLabels:
      app: api-server
  template:
    metadata:
      labels:
        app: api-server
    spec:
      containers:
        - name: api
          image: registry.example.com/api-server
          ports:
            - containerPort: 8080
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8080
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8080
```

## Overlay 配置

### 生产环境

```yaml
# overlays/production/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: production
resources:
  - ../../base
  - namespace.yaml
components:
  - ../../components/monitoring
  - ../../components/network-policy
patches:
  - path: patches/replicas.yaml
  - path: patches/resources.yaml
  - path: patches/pdb.yaml
    target:
      kind: Deployment
  - path: patches/security.yaml
    target:
      kind: Deployment
images:
  - name: registry.example.com/api-server
    newTag: "v2.1.0"  # 生产固定版本
replicas:
  - name: api-server
    count: 5
configMapGenerator:
  - name: api-config
    behavior: merge
    literals:
      - LOG_LEVEL=info
      - METRICS_ENABLED=true
      - TRACING_SAMPLE_RATE=0.1
```

### Strategic Merge Patch

```yaml
# overlays/production/patches/resources.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  template:
    spec:
      containers:
        - name: api
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: api-server
                topologyKey: kubernetes.io/hostname
```

### JSON Patch（精细操作）

```yaml
# overlays/production/patches/security.yaml
- op: add
  path: /spec/template/spec/securityContext
  value:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
- op: add
  path: /spec/template/spec/containers/0/securityContext
  value:
    allowPrivilegeEscalation: false
    readOnlyRootFilesystem: true
    capabilities:
      drop: ["ALL"]
- op: add
  path: /spec/template/spec/automountServiceAccountToken
  value: false
```

## Components（可复用组件）

```yaml
# components/monitoring/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1alpha1
kind: Component
resources:
  - servicemonitor.yaml
patches:
  - target:
      kind: Deployment
    patch: |
      - op: add
        path: /spec/template/metadata/annotations/prometheus.io~1scrape
        value: "true"
      - op: add
        path: /spec/template/metadata/annotations/prometheus.io~1port
        value: "9090"
```

```yaml
# components/monitoring/servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: api-server
spec:
  selector:
    matchLabels:
      app: api-server
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics
```

## GitOps 集成

### ArgoCD Application

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: api-server-production
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/myorg/gitops-repo.git
    targetRevision: main
    path: overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
    retry:
      limit: 3
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
```

### Flux Kustomization

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: api-server
  namespace: flux-system
spec:
  interval: 5m
  path: ./overlays/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: gitops-repo
  targetNamespace: production
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: api-server
      namespace: production
  timeout: 5m
  patches:
    - target:
        kind: Deployment
      patch: |
        - op: replace
          path: /spec/template/spec/containers/0/image
          value: registry.example.com/api-server:v2.1.0
```

## 多集群管理

```yaml
# clusters/cn-east/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - ../../overlays/production
patches:
  - target:
      kind: Deployment
    patch: |
      - op: add
        path: /spec/template/metadata/annotations/cluster
        value: cn-east-1
  - target:
      kind: Service
    patch: |
      - op: add
        path: /metadata/annotations/external-dns.alpha.kubernetes.io~1hostname
        value: api.cn-east.example.com
```

## 验证与调试

```bash
# 构建并查看最终输出
kubectl kustomize overlays/production

# 对比不同环境
diff <(kubectl kustomize overlays/staging) <(kubectl kustomize overlays/production)

# 验证（不实际部署）
kubectl apply --dry-run=server -k overlays/production

# 查看 patch 后的资源
kubectl kustomize overlays/production | yq '.spec.template.spec.containers[0].resources'

# CI 中验证
kubectl kustomize overlays/production | kubeconform -strict -summary
kubectl kustomize overlays/production | kubeval --strict
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| Base 保持通用 | 不含环境特定配置 |
| 镜像标签在 Overlay | Base 不带 tag |
| 使用 Components | 复用跨环境功能 |
| JSON Patch 做安全加固 | 精确添加字段 |
| CI 验证 | kubeconform + kubeval |
| 固定镜像 digest | 生产用 sha256 |
| 避免 commonLabels 变更 | 会导致资源重建 |
| namespace 在 Overlay | Base 不指定 |

## Related

- [[清单模式/Kustomize模式/index.md|Kustomize 模式]]
- [[清单模式/Kustomize模式/01-kustomize-base-overlay-structure.md|Base/Overlay 结构]]
- [[发布变更/GitOps/index.md|GitOps]]
