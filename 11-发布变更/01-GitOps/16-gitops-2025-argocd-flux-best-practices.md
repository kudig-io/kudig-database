---
title: "GitOps 2025 最佳实践：Argo CD ApplicationSet、Flux 2.x 与渐进式交付"
description: "2025 年 GitOps 生产最佳实践：Argo CD 2.12 ApplicationSet 高级模式、Flux 2.4 OCIRepository/Helm Controller、Argo CD + Flux 双引擎协作、大规模 Fleet 管理"
summary: "全面覆盖 GitOps 2025 最佳实践：Argo CD 2.12 ApplicationSet Generator（Matrix/SCM/Pull Request）、多集群 GitOps、Flux 2.4 OCIRepository/HelmRelease/Kustomization 链式管理、GitOps 安全模式（Image Automation/Verification）、Argo CD + Flux 协同策略"
category: gitops-ci-cd
tags:
- gitops
- argocd
- flux
- applicationset
- multi-cluster
- oci
- helm
- kustomize
- fleet-management
- progressive-delivery
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
reading_level: advanced
audience:
- DevOps 工程师
- SRE
- 平台工程师
estimated_read_time: 25min
intent_queries:
- "Argo CD ApplicationSet 如何管理多集群"
- "Flux 2.4 OCIRepository 如何使用"
- "GitOps 2025 大规模集群管理最佳实践"
- "Argo CD 和 Flux 如何配合使用"
trigger_keywords:
- ApplicationSet
- Flux 2.x
- OCIRepository
- HelmRelease
- GitOps Fleet
- 多集群 GitOps
prerequisites:
- kubectl-basics
- helm-basics
- git-basics
- argocd-basics
sources:
- https://argo-cd.readthedocs.io/
- https://fluxcd.io/docs/
- https://github.com/argoproj/argo-cd
- https://github.com/fluxcd/flux2
---

# GitOps 2025 最佳实践：Argo CD ApplicationSet、Flux 2.x 与渐进式交付

> 2025 年，GitOps 已从"单集群 CD 工具"演进为"多集群 Fleet 管理平台"，ApplicationSet 和 Flux 2.x 是这一演进的核心驱动。

## Argo CD 2.12 ApplicationSet 高级模式

### ApplicationSet 生成器矩阵

Matrix Generator 允许将多个 Generator 的输出组合，实现多维度自动化：

```yaml
# Matrix Generator：环境 × 集群 × 应用
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-cluster-apps
  namespace: argocd
spec:
  generators:
  - matrix:
      generators:
      # 维度1：应用列表（从 Git 目录结构读取）
      - git:
          repoURL: https://github.com/my-company/gitops-apps
          revision: main
          directories:
          - path: apps/*
      # 维度2：集群列表（从 Argo CD Cluster Secrets 读取）
      - clusters:
          selector:
            matchLabels:
              environment: production
  template:
    metadata:
      name: '{{path.basename}}-{{name}}'
      labels:
        app: '{{path.basename}}'
        cluster: '{{name}}'
    spec:
      project: production
      source:
        repoURL: https://github.com/my-company/gitops-apps
        targetRevision: main
        path: '{{path}}/overlays/{{metadata.labels.region}}'
      destination:
        server: '{{server}}'
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
        - CreateNamespace=true
        - ServerSideApply=true
        retry:
          limit: 3
          backoff:
            duration: 5s
            factor: 2
            maxDuration: 3m
```

### SCM Provider Generator（Pull Request 自动预览环境）

```yaml
# 每个 PR 自动创建预览环境
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: preview-environments
  namespace: argocd
spec:
  generators:
  - pullRequest:
      github:
        owner: my-company
        repo: my-app
        appSecretName: github-token
        tokenRef:
          secretName: github-app-secret
          key: token
        labels:
        - preview                   # 只处理带 preview 标签的 PR
      requeueAfterSeconds: 60
  template:
    metadata:
      name: 'preview-pr-{{number}}'
      annotations:
        notifications.argoproj.io/subscribe.on-sync-succeeded.slack: '#deployments'
    spec:
      project: preview
      source:
        repoURL: https://github.com/my-company/my-app
        targetRevision: '{{head_sha}}'
        path: k8s/preview
        helm:
          valueFiles:
          - values-preview.yaml
          parameters:
          - name: image.tag
            value: 'pr-{{number}}'
          - name: ingress.host
            value: 'pr-{{number}}.preview.company.io'
      destination:
        server: https://k8s-dev.company.io
        namespace: 'preview-pr-{{number}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: false
        syncOptions:
        - CreateNamespace=true
      info:
      - name: Preview URL
        value: 'https://pr-{{number}}.preview.company.io'
      - name: PR Link
        value: '{{url}}'
```

### Git Generator 高级用法（配置文件驱动）

```yaml
# 基于配置文件的灵活多集群部署
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: config-driven-fleet
  namespace: argocd
spec:
  generators:
  - git:
      repoURL: https://github.com/my-company/fleet-config
      revision: main
      files:
      - path: "clusters/*/config.json"   # 每个集群一个 config.json
  template:
    metadata:
      name: '{{cluster.name}}-platform'
    spec:
      project: '{{cluster.environment}}'
      source:
        repoURL: https://github.com/my-company/platform-apps
        targetRevision: '{{cluster.platformVersion}}'
        path: 'platform/{{cluster.tier}}'
        helm:
          values: |
            cluster:
              name: {{cluster.name}}
              region: {{cluster.region}}
              environment: {{cluster.environment}}
      destination:
        server: '{{cluster.apiServer}}'
        namespace: platform-system
```

```json
// clusters/prod-us-east/config.json
{
  "cluster": {
    "name": "prod-us-east",
    "environment": "production",
    "region": "us-east-1",
    "tier": "enterprise",
    "platformVersion": "v2.4.0",
    "apiServer": "https://prod-us-east.k8s.company.io"
  }
}
```

### Argo CD 2.12 新特性

| 特性 | 说明 |
|------|------|
| Server-Side Apply GA | 大型 CRD 不再因大小限制失败 |
| Hydrator（实验） | GitOps 流水线中将 Helm/Kustomize 渲染结果写回 Git |
| 多源应用增强 | 多 repoURL 混合使用 |
| ApplicationSet Conditions | 条件化生成器过滤 |
| Notification 2.0 | 更丰富的通知模板和触发器 |
| RBAC 细粒度 | Application 级别的 get/sync/delete 权限分离 |

---

## Flux 2.x 生产实践

### OCIRepository：OCI 注册表作为 GitOps 源

```yaml
# 使用 OCI 注册表存储 Helm Chart
apiVersion: source.toolkit.fluxcd.io/v1beta2
kind: OCIRepository
metadata:
  name: my-company-charts
  namespace: flux-system
spec:
  interval: 5m
  url: oci://ghcr.io/my-company/helm-charts
  ref:
    tag: latest
  verify:
    provider: cosign             # Cosign 签名验证
    secretRef:
      name: cosign-public-key
  serviceAccountName: flux-oci-sa  # 使用 Workload Identity
---
# HelmRelease 引用 OCIRepository
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: my-app
  namespace: production
spec:
  interval: 10m
  chartRef:
    kind: OCIRepository
    name: my-company-charts
    namespace: flux-system
  chart:
    spec:
      chart: my-app
      version: ">=1.0.0 <2.0.0"
  values:
    replicaCount: 3
    image:
      repository: ghcr.io/my-company/my-app
      tag: v1.5.0
  postRenderers:
  - kustomize:
      patches:
      - patch: |
          apiVersion: apps/v1
          kind: Deployment
          metadata:
            name: my-app
          spec:
            template:
              metadata:
                annotations:
                  cluster-autoscaler.kubernetes.io/safe-to-evict: "true"
        target:
          kind: Deployment
          name: my-app
```

### Kustomization 链式管理

```yaml
# 基础设施 → 中间件 → 应用的依赖链
# 1. 基础设施层
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: infrastructure
  namespace: flux-system
spec:
  interval: 10m
  path: ./infrastructure
  prune: true
  sourceRef:
    kind: GitRepository
    name: fleet-infra
  healthChecks:
  - apiVersion: apps/v1
    kind: Deployment
    name: cert-manager
    namespace: cert-manager
  - apiVersion: apps/v1
    kind: Deployment
    name: ingress-nginx
    namespace: ingress-nginx
  timeout: 5m
---
# 2. 应用层（依赖基础设施就绪）
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: applications
  namespace: flux-system
spec:
  interval: 5m
  path: ./applications/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: fleet-infra
  dependsOn:
  - name: infrastructure               # 等待基础设施就绪
  - name: monitoring-stack
  patches:
  - patch: |
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: not-used
      spec:
        template:
          spec:
            topologySpreadConstraints:
            - maxSkew: 1
              topologyKey: topology.kubernetes.io/zone
              whenUnsatisfiable: DoNotSchedule
              labelSelector:
                matchLabels:
                  app.kubernetes.io/part-of: my-company
    target:
      kind: Deployment
      labelSelector: "app.kubernetes.io/managed-by=flux"
```

### Image Automation（自动镜像更新）

```yaml
# Image Policy：追踪语义版本
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: my-app
  namespace: flux-system
spec:
  image: ghcr.io/my-company/my-app
  interval: 1m
  secretRef:
    name: ghcr-credentials
---
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: my-app
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: my-app
  policy:
    semver:
      range: ">=1.0.0 <2.0.0"    # 自动跟踪 1.x 最新版
---
# ImageUpdateAutomation：自动提交镜像更新到 Git
apiVersion: image.toolkit.fluxcd.io/v1beta1
kind: ImageUpdateAutomation
metadata:
  name: flux-system
  namespace: flux-system
spec:
  interval: 30m
  sourceRef:
    kind: GitRepository
    name: fleet-infra
  git:
    checkout:
      ref:
        branch: main
    commit:
      author:
        email: fluxbot@company.io
        name: Flux Bot
      messageTemplate: |
        chore: update images

        {{range .Updated.Images}}
        - {{.}} updated to {{.NewTag}}
        {{end}}
    push:
      branch: main
  update:
    strategy: Setters
    path: ./applications
```

### Flux 2.4 新特性

| 特性 | 版本 | 说明 |
|------|------|------|
| OCIRepository GA | 2.0 | OCI 注册表作为 GitOps 源 |
| HelmRelease v2 | 2.3 | 更简洁的 API，废弃 v2beta1 |
| Cosign 验证 GA | 2.2 | 供应链安全，验证 OCI 镜像签名 |
| ResourceSet（实验） | 2.4 | 类 ApplicationSet 的多资源模板 |
| Drift Detection 增强 | 2.3 | 更精确的配置漂移检测 |
| Multi-tenancy GA | 2.1 | 隔离的 Flux 实例，安全多租户 |

---

## Argo CD + Flux 协同架构

```
推荐协同模式：

Flux GitOps Toolkit         Argo CD
(基础设施层)                (应用层)
─────────────────────────────────────────────────
• Cert-Manager             • 业务微服务
• Ingress Controller       • Preview 环境
• GPU Operator             • ApplicationSet 多集群
• Prometheus Stack         • PR 预览环境
• Crossplane               • 基于 PR 的审批流

数据流：
Git → Flux → K8s 基础设施就绪 → Argo CD 应用部署
```

```yaml
# 混合使用：Flux 管理的命名空间标记
apiVersion: v1
kind: Namespace
metadata:
  name: argocd-apps
  labels:
    toolkit.fluxcd.io/tenant: argocd   # Flux 允许 Argo CD 管理此命名空间
```

---

## 大规模 Fleet 管理最佳实践

### 仓库结构推荐

```
fleet-gitops/
├── clusters/
│   ├── production/
│   │   ├── us-east-1/
│   │   │   ├── flux-system/     # Flux bootstrap
│   │   │   └── apps/            # Kustomize overlays
│   │   └── eu-west-1/
│   └── staging/
├── infrastructure/
│   ├── base/                    # 共享基础组件
│   ├── production/              # 生产覆盖
│   └── staging/                 # 测试覆盖
├── applications/
│   ├── base/                    # 应用 HelmRelease 基础配置
│   └── overlays/
│       ├── production/
│       └── staging/
└── policies/
    ├── network-policies/
    └── kyverno-policies/
```

### 多集群配置漂移检测

```bash
# 检查所有集群与 Git 状态的差异
flux get all --all-namespaces -A | grep -v "True"

# 对比特定 Kustomization 的实际状态
flux diff kustomization applications \
  --path ./applications/production \
  --context prod-us-east

# Argo CD 检查所有 App 同步状态
argocd app list -o wide | grep -v "Synced"

# 强制所有 App 同步
argocd app list -o name | xargs -I {} argocd app sync {}
```

### 变更审批工作流

```yaml
# Argo CD：生产环境需要手动同步
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: payment-service-prod
  namespace: argocd
  annotations:
    notifications.argoproj.io/subscribe.on-sync-status-unknown.slack: '#prod-deploys'
    notifications.argoproj.io/subscribe.on-sync-succeeded.slack: '#prod-deploys'
spec:
  syncPolicy:
    # 不配置 automated：需要手动触发 sync
    syncOptions:
    - ApplyOutOfSyncOnly=true
    - ServerSideApply=true
```

```yaml
# Flux：使用 Suspend 实现人工审批
# 1. 合并 PR 后暂停自动同步
kubectl patch kustomization payment-service-prod \
  -n flux-system \
  --type merge \
  -p '{"spec":{"suspend":true}}'

# 2. 审批通过后恢复
kubectl patch kustomization payment-service-prod \
  -n flux-system \
  --type merge \
  -p '{"spec":{"suspend":false}}'
```

---

## 参考资源

- [Argo CD 官方文档](https://argo-cd.readthedocs.io/)
- [ApplicationSet 文档](https://argo-cd.readthedocs.io/en/stable/user-guide/application-set/)
- [Flux 官方文档](https://fluxcd.io/docs/)
- [Flux 2.x 迁移指南](https://fluxcd.io/docs/migration/)
- [GitOps Working Group](https://github.com/cncf/tag-app-delivery/tree/main/gitops-wg)
