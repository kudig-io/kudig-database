---
title: Kubernetes Manifest Management Strategy — From kubectl to GitOps Maturity
description: K8s 清单管理 — 从 kubectl apply 到 GitOps 的成熟度演进、模板引擎选型、多环境管理、清单治理
summary: Kubernetes 清单管理的成熟度模型与最佳实践，涵盖模板引擎、GitOps 演进与治理策略
category: practice
tags:
- manifest-management
- gitops
- kustomize
- helm
- template-engine
- governance
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: manifest-patterns
---
# Kubernetes 清单管理策略

> 从 kubectl apply 到 GitOps 的成熟度演进与治理。

## 成熟度模型

```
Level 0: kubectl apply -f（手动）
    ↓
Level 1: 模板化（Helm/Kustomize）
    ↓
Level 2: 版本控制（Git 管理清单）
    ↓
Level 3: GitOps（ArgoCD/Flux 自动同步）
    ↓
Level 4: 平台 API（Crossplane/自研抽象）
    ↓
Level 5: 智能治理（策略即代码 + 自动修复）
```

## Level 0 → 1: 模板化

### 为什么需要模板化

| 问题 | 裸 YAML | 模板化后 |
|------|---------|----------|
| 重复 | 每个环境复制粘贴 | 参数化复用 |
| 一致性 | 手动保证 | 模板强制 |
| 变更 | 逐文件修改 | 改参数即可 |
| 审计 | 无 | Git 历史 |
| 回滚 | 无 | Git revert |

### Kustomize（推荐入门）

```yaml
# base/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
  labels:
    app.kubernetes.io/name: app
spec:
  replicas: 1
  selector:
    matchLabels:
      app.kubernetes.io/name: app
  template:
    metadata:
      labels:
        app.kubernetes.io/name: app
    spec:
      containers:
        - name: app
          image: registry.example.com/app:v1.0.0
          ports:
            - containerPort: 8080
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
---
# base/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - deployment.yaml
  - service.yaml
  - hpa.yaml
commonLabels:
  app.kubernetes.io/managed-by: kustomize
---
# overlays/production/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - ../../base
namespace: production
replicas:
  - name: app
    count: 3
images:
  - name: registry.example.com/app
    newTag: v2.1.0
patches:
  - target:
      kind: Deployment
      name: app
    patch: |
      - op: add
        path: /spec/template/spec/containers/0/resources/limits
        value:
          cpu: "2"
          memory: "1Gi"
```

### Helm（复杂应用）

```yaml
# Chart.yaml
apiVersion: v2
name: my-app
version: 1.0.0
appVersion: "2.1.0"
dependencies:
  - name: postgresql
    version: "15.x"
    repository: https://charts.bitnami.com/bitnami
    condition: postgresql.enabled
  - name: redis
    version: "19.x"
    repository: https://charts.bitnami.com/bitnami
    condition: redis.enabled
---
# values.yaml（默认值）
replicaCount: 2
image:
  repository: registry.example.com/app
  tag: ""  # 默认用 appVersion
  pullPolicy: IfNotPresent
resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: "1"
    memory: 512Mi
postgresql:
  enabled: true
  auth:
    database: appdb
redis:
  enabled: true
---
# values-production.yaml（生产覆盖）
replicaCount: 5
resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: "2"
    memory: 2Gi
postgresql:
  architecture: replication
  primary:
    persistence:
      size: 100Gi
```

## Level 2 → 3: GitOps

### ArgoCD 集成

```yaml
# ArgoCD Application（Kustomize）
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app-production
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/myorg/gitops.git
    targetRevision: main
    path: apps/my-app/overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
---
# ArgoCD Application（Helm）
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app-helm
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/myorg/charts.git
    targetRevision: main
    path: charts/my-app
    helm:
      valueFiles:
        - values.yaml
        - values-production.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

### GitOps 仓库结构

```
gitops-repo/
├── apps/                          # 应用清单
│   ├── order-service/
│   │   ├── base/
│   │   └── overlays/
│   │       ├── dev/
│   │       ├── staging/
│   │       └── production/
│   └── payment-service/
│       └── ...
├── platform/                      # 平台组件
│   ├── monitoring/
│   ├── logging/
│   ├── ingress/
│   └── cert-manager/
├── clusters/                      # 集群配置
│   ├── dev/
│   │   └── kustomization.yaml    # 引用 apps + platform
│   └── production/
│       └── kustomization.yaml
└── policies/                      # 策略
    ├── kyverno/
    └── network-policies/
```

## Level 4: 平台 API

### Crossplane 抽象

```yaml
# 平台 API（XRD）— 开发者只需填写业务参数
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xwebapps.platform.example.com
spec:
  group: platform.example.com
  names:
    kind: XWebApp
    plural: xwebapps
  claimNames:
    kind: WebApp
    plural: webapps
  versions:
    - name: v1alpha1
      served: true
      referenceable: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                image:
                  type: string
                replicas:
                  type: integer
                  default: 2
                database:
                  type: boolean
                  default: true
---
# 开发者使用（极简）
apiVersion: platform.example.com/v1alpha1
kind: WebApp
metadata:
  name: my-service
  namespace: team-a
spec:
  image: registry.example.com/my-service:v1.0
  replicas: 3
  database: true
  # Crossplane 自动创建: Deployment + Service + Ingress + DB + 监控
```

## 清单治理

### 质量门禁

```yaml
# CI 中的清单检查
# .github/workflows/manifest-check.yaml
name: Manifest Quality
on:
  pull_request:
    paths: ['apps/**', 'platform/**']

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      # 语法验证
      - name: Kubeconform
        run: |
          kubeconform -strict -summary \
            -schema-location default \
            -schema-location 'https://raw.githubusercontent.com/datreeio/CRDs-catalog/main/{{.Group}}/{{.ResourceKind}}_{{.ResourceAPIVersion}}.json' \
            apps/ platform/
      # 最佳实践检查
      - name: Kube-linter
        run: kube-linter lint apps/ platform/
      # 策略检查
      - name: Conftest
        run: conftest test apps/ --policy policies/
      # Diff 预览
      - name: ArgoCD Diff
        run: |
          argocd app diff my-app-production \
            --local apps/my-app/overlays/production
```

### 清单规范

| 规范 | 要求 |
|------|------|
| 标签 | 必须有 app.kubernetes.io/* 标准标签 |
| 资源 | 必须设置 requests 和 limits |
| 探针 | 必须有 liveness + readiness |
| 安全 | 必须 runAsNonRoot + readOnlyRootFilesystem |
| 镜像 | 禁止 latest，必须固定版本 |
| PDB | 生产 Deployment 必须配置 |
| NetworkPolicy | 每个 Namespace 至少一个 |
| 注解 | 变更原因注解（commit message） |

## 工具选型决策

```
需要模板化？
├── 简单应用（< 5 资源）→ Kustomize
├── 复杂应用（依赖/条件）→ Helm
├── 平台抽象（多资源组合）→ Crossplane
└── 代码生成（类型安全）→ CDK8s / Jsonnet

需要 GitOps？
├── 多集群 + UI → ArgoCD
├── 轻量 + K8s 原生 → Flux
└── 混合（Helm + Kustomize）→ ArgoCD（都支持）
```

## Related

- [[03-清单模式/index.md|清单模式]]
- [[03-清单模式/03-Helm值模式/index.md|Helm 值模式]]
- [[03-清单模式/02-Kustomize模式/index.md|Kustomize 模式]]
- [[11-发布变更/01-GitOps/index.md|GitOps]]
- [[03-清单模式/09-平台模式/index.md|平台模式]]
