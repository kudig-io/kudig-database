---
title: ArgoCD App-of-Apps 模式
description: 使用 App-of-Apps 模式管理大量 ArgoCD Application 资源
summary: App-of-Apps 父子应用层级设计、集群引导及多环境批量部署模式
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- argocd
- app-of-apps
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- ArgoCD App-of-Apps 是什么
- 如何批量管理 ArgoCD 应用
- ArgoCD 集群引导
trigger_keywords:
- argocd
- app-of-apps
- bootstrap
- gitops
prerequisites:
- argocd-basics
- gitops-basics
authors:
- name: KUDIG Team
  role: contributor
---

# ArgoCD App-of-Apps 模式

## 1. 核心概念

App-of-Apps 是一种用 ArgoCD Application 管理其他 Application 的模式。一个"父应用"引用包含多个"子应用"清单的 Git 目录，实现集群引导和批量部署。

```
Git Repo
└── argocd/
    ├── apps/
    │   ├── app-of-apps.yaml      ← 父应用（引用子应用目录）
    │   └── projects/
    │       ├── frontend.yaml      ← 子应用定义
    │       ├── backend.yaml
    │       └── database.yaml
    └── bootstrap/
        └── root-app.yaml          ← 根应用（手动 apply 一次）
```

## 2. 根应用（Bootstrap）

只需手动 apply 一次，之后所有应用都由 Git 管理：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: root-app
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io
spec:
  project: default
  source:
    repoURL: https://github.com/example/k8s-manifests
    targetRevision: main
    path: argocd/apps           # 引用 app-of-apps 目录
  destination:
    server: https://kubernetes.default.svc
    namespace: argocd
  syncPolicy:
    automated:
      prune: true               # 自动删除 Git 中已移除的资源
      selfHeal: true            # 自动纠正漂移
    syncOptions:
      - CreateNamespace=true
```

## 3. App-of-Apps（父应用）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: platform-app-of-apps
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/example/k8s-manifests
    targetRevision: main
    path: argocd/projects       # 包含所有子应用清单的目录
  destination:
    server: https://kubernetes.default.svc
    namespace: argocd
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

## 4. 子应用定义

```yaml
# argocd/projects/frontend.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: frontend
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io
spec:
  project: production
  source:
    repoURL: https://github.com/example/frontend-manifests
    targetRevision: main
    path: overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: frontend
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
      - PruneLast=true
      - ApplyOutOfSyncOnly=true
```

## 5. 多环境 App-of-Apps

```yaml
# 通过 Kustomize 区分环境
# argocd/apps/app-of-apps.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: environments
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/example/k8s-manifests
    targetRevision: main
    path: environments/production   # 按环境区分
  destination:
    server: https://kubernetes.default.svc
    namespace: argocd
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

目录结构：

```
environments/
├── production/
│   ├── frontend.yaml
│   ├── backend.yaml
│   └── monitoring.yaml
├── staging/
│   ├── frontend.yaml
│   └── backend.yaml
└── dev/
    └── frontend.yaml
```

## 6. ApplicationSet 替代方案

对于大规模多集群场景，ApplicationSet 更灵活（见 [[03-清单模式/05-GitOps模式/02-argocd-applicationset-multi-cluster|ApplicationSet 文档]]）。

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 `resources-finalizer` | 删除 App 时级联清理子资源 |
| 分离 App 定义与目标 manifest | App 清单与应用部署清单用不同仓库/路径 |
| 限制 `syncPolicy.automated` | 生产环境考虑手动审批 sync |
| 使用 AppProject 隔离 | 不同团队/环境使用不同 Project |
| 监控 sync 状态 | 配合 ArgoCD Notifications 告警 |

## Related

- [[03-清单模式/05-GitOps模式/02-argocd-applicationset-multi-cluster|ApplicationSet 多集群]]
- [[03-清单模式/05-GitOps模式/04-gitops-directory-structure|GitOps 目录结构]]

## See Also

- [ArgoCD App-of-Apps 文档](https://argo-cd.readthedocs.io/en/stable/operator-manual/cluster-bootstrapping/)
- [ArgoCD 最佳实践](https://argo-cd.readthedocs.io/en/stable/user-guide/best_practices/)

<!-- risk-assessed -->
