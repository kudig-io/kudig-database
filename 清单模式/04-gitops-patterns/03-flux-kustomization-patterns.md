---
title: Flux Kustomization 模式
description: Flux CD Kustomization 和 HelmRelease 资源配置模式
summary: Flux Kustomization 同步策略、依赖排序、 HelmRelease 配置及多环境 GitOps 工作流
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- flux
- kustomization
- helmrelease
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- Flux Kustomization 如何配置
- Flux HelmRelease
- Flux CD 多环境部署
trigger_keywords:
- flux
- kustomization
- helmrelease
- gitrepository
- gitops
prerequisites:
- flux-basics
- gitops-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Flux Kustomization 模式

## 1. Flux 核心概念

Flux 使用 `source-controller` 监听 Git/Helm 仓库，然后 `kustomize-controller` 将清单同步到集群。

```
Git Repository → GitRepository CR → Kustomization CR → 集群资源
                                        ↑
                              depends on (排序)
```

## 2. GitRepository 配置

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: app-manifests
  namespace: flux-system
spec:
  interval: 1m                    # 轮询间隔
  url: https://github.com/example/k8s-manifests
  ref:
    branch: main
  secretRef:
    name: github-deploy-key       # 私有仓库认证
---
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: podinfo
  namespace: flux-system
spec:
  interval: 5m
  url: https://github.com/stefanprodan/podinfo
  ref:
    semver: ">=6.0.0"             # 语义化版本范围
```

## 3. Kustomization 资源

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: frontend
  namespace: flux-system
spec:
  interval: 10m                   # 同步间隔
  path: ./overlays/production     # Git 仓库中的路径
  sourceRef:
    kind: GitRepository
    name: app-manifests
  prune: true                     # 删除 Git 中已移除的资源
  wait: true                      # 等待资源就绪
  timeout: 5m
  targetNamespace: frontend
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: frontend
      namespace: frontend
    - apiVersion: v1
      kind: Service
      name: frontend
      namespace: frontend
```

## 4. 依赖排序

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: backend
  namespace: flux-system
spec:
  interval: 10m
  path: ./overlays/production/backend
  sourceRef:
    kind: GitRepository
    name: app-manifests
  dependsOn:
    - name: database              # 等 database Kustomization 先同步
    - name: redis-cache
  prune: true
  wait: true
```

## 5. HelmRelease 配置

```yaml
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: redis
  namespace: cache
spec:
  interval: 5m
  chart:
    spec:
      chart: redis
      version: ">=18.0.0 <19.0.0"
      sourceRef:
        kind: HelmRepository
        name: bitnami
        namespace: flux-system
  values:
    architecture: replication
    auth:
      enabled: true
      existingSecret: redis-secret
    replica:
      replicaCount: 3
    metrics:
      enabled: true
      serviceMonitor:
        enabled: true
  install:
    remediation:
      retries: 3
  upgrade:
    remediation:
      retries: 3
      strategy: rollback         # 升级失败自动回滚
  rollback:
    timeout: 5m
```

## 6. HelmRepository

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: HelmRepository
metadata:
  name: bitnami
  namespace: flux-system
spec:
  interval: 1h
  url: https://charts.bitnami.com/bitnami
  type: oci                       # OCI 注册表支持
---
apiVersion: source.toolkit.fluxcd.io/v1
kind: HelmRepository
metadata:
  name: private-charts
  namespace: flux-system
spec:
  interval: 1h
  url: oci://registry.example.com/charts
  type: oci
  secretRef:
    name: registry-credentials
```

## 7. 多环境配置

```
clusters/
├── production/
│   ├── flux-system/
│   │   └── gotk-components.yaml
│   ├── apps.yaml                 # 生产环境 Kustomization
│   └── infrastructure.yaml
├── staging/
│   ├── apps.yaml
│   └── infrastructure.yaml
```

```yaml
# clusters/staging/apps.yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: apps
  namespace: flux-system
spec:
  interval: 1m
  path: ./staging                 # 指向 staging overlays
  sourceRef:
    kind: GitRepository
    name: flux-system
  prune: true
  wait: true
```

## 8. Image Automation（自动更新）

```yaml
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: frontend
  namespace: flux-system
spec:
  image: registry.example.com/frontend
  interval: 1m
---
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: frontend
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: frontend
  policy:
    semver:
      range: ">=1.0.0"            # 自动更新到最新 1.x 版本
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 `dependsOn` | 确保基础设施先于应用部署 |
| 设置 `healthChecks` | 确保资源真正就绪而非仅 Apply 成功 |
| 启用 `prune: true` | Git 是唯一真相来源 |
| 分离 Source 和 Kustomization | 解耦轮询频率与同步频率 |
| 使用 OCI registry | 比传统 HTTP Helm repo 更安全 |

## Related

- [[清单模式/04-gitops-patterns/01-argocd-app-of-apps|App-of-Apps 模式]]
- [[清单模式/Kustomize模式/01-kustomize-base-overlay-structure|Kustomize Base/Overlay]]

## See Also

- [Flux CD 文档](https://fluxcd.io/docs/)
- [Flux HelmRelease API](https://fluxcd.io/flux/components/helm/api/v2/)

<!-- risk-assessed -->
