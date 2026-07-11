---
title: GitOps 目录结构设计
description: 单仓多环境、多仓分离与混合模式目录设计
summary: GitOps 仓库目录结构最佳实践，包括单仓多环境、按团队/按应用划分及 Kustomize 集成
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- directory-structure
- kustomize
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
- 开发工程师
estimated_read_time: 10min
intent_queries:
- GitOps 目录如何组织
- 单仓 vs 多仓
- Kustomize 多环境目录
trigger_keywords:
- gitops
- directory
- monorepo
- structure
- kustomize
prerequisites:
- gitops-basics
- kustomize-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GitOps 目录结构设计

## 1. 三种仓库模式对比

| 模式 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| **单仓多环境** | 统一管理、跨环境 diff | 仓库膨胀、权限粗 | 中小团队 |
| **多仓分离** | 权限精细、独立 CI | 跨仓一致性难 | 大型组织 |
| **混合模式** | 基础设施集中、应用分散 | 需要协调 | 大中型团队 |

## 2. 模式一：单仓多环境

```
k8s-manifests/
├── .github/
│   └── workflows/
│       └── validate.yaml         ← PR 验证（kubeval/yamllint）
├── apps/
│   ├── base/                     ← 共享基础配置
│   │   ├── frontend/
│   │   │   ├── kustomization.yaml
│   │   │   ├── deployment.yaml
│   │   │   └── service.yaml
│   │   └── backend/
│   │       ├── kustomization.yaml
│   │       └── deployment.yaml
│   ├── overlays/
│   │   ├── dev/
│   │   │   ├── frontend/
│   │   │   │   ├── kustomization.yaml
│   │   │   │   └── patch-replicas.yaml
│   │   │   └── backend/
│   │   ├── staging/
│   │   │   ├── frontend/
│   │   │   └── backend/
│   │   └── production/
│   │       ├── frontend/
│   │       └── backend/
│   └── components/               ← 可复用组件
│       ├── monitoring/
│       └── network-policies/
├── infrastructure/
│   ├── base/
│   │   ├── ingress-nginx/
│   │   ├── cert-manager/
│   │   └── monitoring-stack/
│   └── overlays/
│       ├── dev/
│       └── production/
├── argocd/
│   ├── app-of-apps.yaml
│   └── projects/
└── policies/                     ← Kyverno/OPA 策略
    └── baseline/
```

## 3. 模式二：按团队/应用分仓

```
组织结构:
├── platform-team/
│   ├── infra-manifests/          ← 集群基础设施
│   └── shared-services/
├── frontend-team/
│   └── frontend-manifests/
├── backend-team/
│   └── backend-manifests/
└── security-team/
    └── policy-manifests/
```

## 4. Kustomize 集成示例

### 4.1 Base 配置

```yaml
# apps/base/frontend/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - deployment.yaml
  - service.yaml
  - configmap.yaml

commonLabels:
  app.kubernetes.io/name: frontend
  app.kubernetes.io/part-of: platform
```

### 4.2 环境 Overlay

```yaml
# apps/overlays/production/frontend/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: frontend-prod

resources:
  - ../../../base/frontend
  - ../../../components/monitoring

patches:
  - path: patch-replicas.yaml
  - path: patch-resources.yaml

images:
  - name: frontend
    newName: registry.example.com/frontend
    newTag: v1.2.3

configMapGenerator:
  - name: frontend-config
    behavior: merge
    literals:
      - LOG_LEVEL=warn
      - CACHE_TTL=300
```

### 4.3 生产 Patch

```yaml
# apps/overlays/production/frontend/patch-replicas.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
spec:
  replicas: 5
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
```

## 5. GitOps 工具配置文件

### 5.1 ArgoCD ApplicationSet

```yaml
# argocd/applicationset.yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: all-apps
  namespace: argocd
spec:
  generators:
    - git:
        repoURL: https://github.com/example/k8s-manifests
        revision: main
        directories:
          - path: apps/overlays/*/*
  template:
    metadata:
      name: '{{path.basename}}-{{path[2]}}'
    spec:
      project: '{{path[2]}}'
      source:
        repoURL: https://github.com/example/k8s-manifests
        path: '{{path}}'
      destination:
        server: https://kubernetes.default.svc
        namespace: '{{path.basename}}-{{path[2]}}'
```

### 5.2 Flux Kustomization

```yaml
# flux/apps.yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: apps
  namespace: flux-system
spec:
  interval: 1m
  path: ./apps/overlays/production
  sourceRef:
    kind: GitRepository
    name: flux-system
  prune: true
  wait: true
```

## 6. CI 验证流水线

```yaml
# .github/workflows/validate.yaml
name: Validate Manifests
on: [pull_request]
jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Kustomize Build & Kubeval
        run: |
          for overlay in apps/overlays/*/; do
            kustomize build "$overlay" | kubeval --strict
          done
      - name: Check for sensitive data
        run: |
          ! grep -r "password:" apps/ --include="*.yaml"
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| Base 只放通用配置 | 环境差异放 Overlay |
| 使用 Components 复用 | 监控、网络策略等抽为 Component |
| CI 中验证所有 Overlay | 确保每个 overlay 可成功 build |
| 标签统一管理 | 使用 `commonLabels` 避免遗漏 |
| 镜像标签用 variables | 不要在 Git 中硬编码 latest |

## Related

- [[清单模式/Kustomize模式/01-kustomize-base-overlay-structure|Kustomize Base/Overlay 分层]]
- [[清单模式/04-gitops-patterns/01-argocd-app-of-apps|App-of-Apps 模式]]

## See Also

- [GitOps 目录结构最佳实践](https://akuity.io/blog/the-state-of-gitops/)
- [Kustomize 组件文档](https://kubectl.docs.kubernetes.io/guides/config_management/components/)

<!-- risk-assessed -->
