---
title: GitOps Sync Waves 同步顺序
description: ArgoCD Sync Waves 和 Flux dependsOn 实现有序部署
summary: 使用 Sync Waves、Sync Hooks 和 Flux 依赖实现 GitOps 资源部署排序
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- argocd
- sync-waves
- ordering
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
estimated_read_time: 8min
intent_queries:
- ArgoCD Sync Waves 如何排序
- GitOps 部署顺序控制
- Flux dependsOn
trigger_keywords:
- sync-wave
- sync-hooks
- depends-on
- ordering
prerequisites:
- argocd-basics
- gitops-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GitOps Sync Waves 同步顺序

## 1. 为什么需要排序

Kubernetes Apply 是声明式的，但某些资源存在依赖：

- **CRD 必须先于 CR 创建** — 否则 CR 无法验证
- **Namespace 必须先于应用** — 否则部署失败
- **数据库必须先于应用** — 否则连接失败
- **NetworkPolicy 必须在 Pod 之前** — 避免安全窗口

## 2. ArgoCD Sync Waves

通过 `argocd.argoproj.io/sync-wave` 注解控制顺序，数字越小越先执行：

```yaml
# Wave -2: Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "-2"
---
# Wave -1: CRD
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: webapps.platform.example.com
  annotations:
    argocd.argoproj.io/sync-wave: "-1"
---
# Wave 0: 基础设施
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "0"
---
# Wave 1: 数据库
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "1"
---
# Wave 2: 应用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "2"
---
# Wave 3: Ingress（最后）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: frontend-ingress
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "3"
```

### 2.1 Wave 规划建议

| Wave | 内容 | 说明 |
|------|------|------|
| `-2` | Namespace | 最先创建 |
| `-1` | CRD | 先于 CR |
| `0` | ConfigMap、Secret | 基础配置 |
| `1` | 数据库、消息队列 | 中间件 |
| `2` | 应用 Deployment | 业务应用 |
| `3` | Ingress、NetworkPolicy | 流量入口/安全策略 |

## 3. Sync Hooks

在特定阶段执行操作（如数据库迁移）：

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration
  namespace: my-app
  annotations:
    argocd.argoproj.io/hook: PreSync         # Sync 前执行
    argocd.argoproj.io/hook-delete-policy: HookSucceeded  # 成功后删除
    argocd.argoproj.io/sync-wave: "0"
spec:
  template:
    spec:
      containers:
        - name: migrator
          image: registry.example.com/migrator:v1.0
          command: ["./migrate", "up"]
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-secret
                  key: url
      restartPolicy: OnFailure
```

### 3.1 Hook 类型

| Hook | 时机 |
|------|------|
| `PreSync` | Sync 开始前 |
| `Sync` | 正常 Sync 过程 |
| `PostSync` | 所有资源 Sync 完成后 |
| `SyncFail` | Sync 失败时 |
| `PostDelete` | 删除应用后 |

## 4. Flux dependsOn

Flux 通过 `dependsOn` 字段控制 Kustomization 之间的顺序：

```yaml
# 基础设施先部署
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: infrastructure
  namespace: flux-system
spec:
  interval: 10m
  path: ./infrastructure/overlays/production
  sourceRef:
    kind: GitRepository
    name: flux-system
  prune: true
  wait: true                # 等待健康检查通过
---
# 数据库依赖基础设施
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: database
  namespace: flux-system
spec:
  interval: 10m
  path: ./database/overlays/production
  sourceRef:
    kind: GitRepository
    name: flux-system
  dependsOn:
    - name: infrastructure   # 等 infrastructure 完成
  prune: true
  wait: true
---
# 应用依赖数据库
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: applications
  namespace: flux-system
spec:
  interval: 10m
  path: ./apps/overlays/production
  sourceRef:
    kind: GitRepository
    name: flux-system
  dependsOn:
    - name: database         # 等 database 完成
    - name: infrastructure   # 也依赖基础设施
  prune: true
  wait: true
```

## 5. 健康检查

确保资源真正就绪，而非仅 Apply 成功：

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: postgres
  namespace: flux-system
spec:
  healthChecks:
    - apiVersion: apps/v1
      kind: StatefulSet
      name: postgres
      namespace: database
  postBuild:
    substitute:
      cluster_name: production
```

## 6. 生产实践

| 实践 | 说明 |
|------|------|
| CRD 使用 `SkipDryRunOnMissingResource` | CRD 未注册时跳过验证 |
| 使用 `PostSync` Hook 运行冒烟测试 | 部署后自动验证 |
| 设置合理的 `timeout` | 避免 Job 卡死阻塞整个 Sync |
| `wait: true`（Flux） | 确保 Health Check 通过后再继续 |
| 使用 `PruneLast: true` | 先部署新资源再清理旧资源 |

## Related

- [[03-清单模式/05-GitOps模式/01-argocd-app-of-apps|App-of-Apps 模式]]
- [[03-清单模式/05-GitOps模式/03-flux-kustomization-patterns|Flux Kustomization]]

## See Also

- [ArgoCD Sync Waves](https://argo-cd.readthedocs.io/en/stable/user-guide/sync-waves/)
- [ArgoCD Resource Hooks](https://argo-cd.readthedocs.io/en/stable/user-guide/resource_hooks/)
- [Flux dependsOn](https://fluxcd.io/flux/components/kustomize/kustomizations/#dependencies)

<!-- risk-assessed -->
