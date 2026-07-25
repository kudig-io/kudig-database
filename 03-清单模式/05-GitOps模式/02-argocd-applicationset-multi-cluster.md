---
title: ArgoCD ApplicationSet 多集群部署
description: ApplicationSet 实现 Git 仓库模板化多集群/多环境部署
summary: ApplicationSet Generator（Git/Cluster/List/Matrix）实现大规模多集群批量部署
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- argocd
- applicationset
- multi-cluster
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
- ApplicationSet 如何使用
- ArgoCD 多集群部署
- ApplicationSet 生成器
trigger_keywords:
- applicationset
- argocd
- multi-cluster
- generator
- gitops
prerequisites:
- argocd-basics
- gitops-basics
authors:
- name: KUDIG Team
  role: contributor
---

# ArgoCD ApplicationSet 多集群部署

## 1. ApplicationSet vs App-of-Apps

| 特性 | App-of-Apps | ApplicationSet |
|------|-------------|----------------|
| 模板化 | 手动写每个 App | 参数化模板自动生成 |
| 多集群 | 手动指定 destination | 通过 Cluster Generator 自动发现 |
| 扩展性 | 几十个 App | 数百个 App |
| 维护 | 文件多 | 单文件模板 |

## 2. Git Generator

根据 Git 目录结构自动生成 Application：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: guestbook-deployment
  namespace: argocd
spec:
  generators:
    - git:
        repoURL: https://github.com/example/guestbook-deployment
        revision: main
        directories:
          - path: apps/*
  template:
    metadata:
      name: '{{path.basename}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/example/guestbook-deployment
        targetRevision: main
        path: '{{path}}'
      destination:
        server: https://kubernetes.default.svc
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

## 3. Cluster Generator

根据已注册的 ArgoCD Cluster 自动生成 Application：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: monitoring-all-clusters
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            env: production     # 只选择生产集群
  template:
    metadata:
      name: 'monitoring-{{name}}'
    spec:
      project: monitoring
      source:
        repoURL: https://github.com/example/monitoring-stack
        targetRevision: main
        path: overlays/{{metadata.labels.environment}}
      destination:
        server: '{{server}}'
        namespace: monitoring
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

## 4. List Generator

精确指定参数组合：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-env-apps
  namespace: argocd
spec:
  generators:
    - list:
        elements:
          - cluster: prod-us
            url: https://prod-us-api.example.com:6443
            env: production
          - cluster: prod-eu
            url: https://prod-eu-api.example.com:6443
            env: production
          - cluster: staging
            url: https://staging-api.example.com:6443
            env: staging
  template:
    metadata:
      name: 'app-{{cluster}}'
    spec:
      project: '{{env}}'
      source:
        repoURL: https://github.com/example/my-app
        targetRevision: main
        path: overlays/{{env}}
      destination:
        server: '{{url}}'
        namespace: my-app
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

## 5. Matrix Generator（组合）

将 Git Generator 和 Cluster Generator 组合，实现交叉部署：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: all-apps-all-clusters
  namespace: argocd
spec:
  generators:
    - matrix:
        generators:
          - git:
              repoURL: https://github.com/example/manifests
              revision: main
              directories:
                - path: apps/*
          - clusters:
              selector:
                matchLabels:
                  argocd.argoproj.io/secret-type: cluster
  template:
    metadata:
      name: '{{path.basename}}-{{name}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/example/manifests
        targetRevision: main
        path: '{{path}}/overlays/{{metadata.labels.environment}}'
      destination:
        server: '{{server}}'
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

## 6. Merge Generator（去重/覆盖）

```yaml
generators:
  - merge:
      mergeKeys:
        - server
      generators:
        - clusters: {}  # 所有集群
        - clusters:
            selector:
              matchLabels:
                vip: true
            values:
              resourceQuota: large  # 覆盖特定集群参数
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 Cluster 标签 | `env=production`, `region=us-east` 便于筛选 |
| 模板中包含 SyncPolicy | 自动化同步，避免手动操作 |
| 使用 `RequeueAfterSeconds` | 控制 Git 仓库轮询频率 |
| 按环境分离 ApplicationSet | staging/prod 使用不同 ApplicationSet |
| 监控 Application 健康状态 | 配合 ArgoCD Notifications 告警 |

## Related

- [[03-清单模式/05-GitOps模式/01-argocd-app-of-apps|App-of-Apps 模式]]
- [[03-清单模式/05-GitOps模式/04-gitops-directory-structure|GitOps 目录结构]]

## See Also

- [ApplicationSet 文档](https://argo-cd.readthedocs.io/en/stable/operator-manual/applicationset/)
- [ApplicationSet Generators](https://argo-cd.readthedocs.io/en/stable/operator-manual/applicationset/Generators/)

<!-- risk-assessed -->
