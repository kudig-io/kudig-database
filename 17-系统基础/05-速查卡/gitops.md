---
title: GitOps 速查卡
description: Argo CD / Flux GitOps 工作流快速参考, 覆盖应用部署、多环境管理、回滚、安全加固
summary: Argo CD / Flux GitOps 工作流快速参考, 覆盖应用部署、多环境管理、回滚、安全加固
category: cheatsheet
tags:
- gitops
- argocd
- flux
- k8s
- deployment
- cheatsheet
- quick-reference
- opa
- rbac
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- DevOps 工程师
- SRE
- 平台工程师
estimated_read_time: 15min
intent_queries:
- GitOps 工作流怎么搭建
- Argo CD 常用命令速查
- Flux CD 快速上手
- GitOps 多环境管理最佳实践
- GitOps 回滚策略
trigger_keywords:
- GitOps
- Argo CD
- Flux
- 持续部署
- 声明式
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gitops-basics
- policy-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GitOps 生产环境速查卡

> **适用版本**: [[argo|Argo]] CD 2.10+ / [[flux|[[flux|Flux]]]] 2.2+ | **最后更新**: 2026-05

---

## 核心原则

```
Git = Single Source of Truth (唯一真实来源)
  │
  ├── 声明式: 所有环境状态用 YAML 描述
  ├── 版本化: Git 提交 = 变更审计
  ├── 自动化: Git 变更 → 自动同步到集群
  └── 可观测: Drift 检测 + 自动修复
```

---

## Argo CD 速查

### 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Argo CD
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 获取初始密码
argocd admin initial-password -n argocd

# CLI 安装
brew install argocd
```
### 核心操作

```bash
# 登录
argocd login argocd.example.com
argocd login argocd.example.com --sso  # SSO 登录

# 创建应用
argocd app create my-app \
  --repo https://github.com/org/k8s-manifests.git \
  --path overlays/production \
  --dest-server https://kubernetes.default.svc \
  --dest-namespace production \
  --sync-policy automated \
  --auto-prune \
  --self-heal

# 查看应用
argocd app list
argocd app get my-app
argocd app diff my-app

# 手动同步
argocd app sync my-app

# 回滚
argocd app history my-app
argocd app rollback my-app <REVISION>

# 删除
argocd app delete my-app
```

### Application CRD

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/org/k8s-manifests.git
    targetRevision: main
    path: overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true        # 删除 Git 中不存在的资源
      selfHeal: true     # 自动修复手动变更
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
  ignoreDifferences:     # 忽略某些字段的差异
    - group: apps
      kind: Deployment
      jsonPointers:
        - /spec/replicas
```

### 多环境管理 (Kustomize)

```
k8s-manifests/
├── base/                  # 基础配置
│   ├── kustomization.yaml
│   ├── deployment.yaml
│   └── service.yaml
└── overlays/
    ├── dev/
    │   └── kustomization.yaml    # replicas: 1
    ├── staging/
    │   └── kustomization.yaml    # replicas: 2
    └── production/
        └── kustomization.yaml    # replicas: 5, resource limits
```

### App of Apps 模式

```yaml
# 一个 Application 管理所有子 Application
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: apps
  namespace: argocd
spec:
  source:
    path: apps/           # 包含多个子 Application YAML
    repoURL: https://github.com/org/k8s-manifests.git
  syncPolicy:
    automated: {}
```

---

## Flux CD 速查

### 安装

```bash
# 安装 Flux CLI
brew install fluxcd/tap/flux

# 检查集群兼容性
flux check --pre

# Bootstrap (GitHub)
flux bootstrap github \
  --owner=my-org \
  --repository=fleet-infra \
  --branch=main \
  --path=clusters/production
```

### 核心资源

```yaml
# GitRepository - 定义 Git 源
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: my-app
  namespace: flux-system
spec:
  interval: 1m
  url: https://github.com/org/k8s-manifests.git
  ref:
    branch: main

---
# Kustomization - 定义同步策略
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: my-app
  namespace: flux-system
spec:
  interval: 5m
  path: ./overlays/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: my-app
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      namespace: production
```

### 常用命令

```bash
# 查看状态
flux get all -A
flux get kustomizations
flux get gitrepositories

# 强制同步
flux reconcile kustomization my-app --with-source

# 查看日志
flux logs --level=info

# 挂起/恢复
flux suspend kustomization my-app
flux resume kustomization my-app

# 诊断
flux trace my-app -n production

```

---

## GitOps vs 传统 CI/CD

| 维度 | 传统 CI/CD | GitOps |
|------|-----------|--------|
| 触发方式 | push (kubectl apply) | pull (Agent 拉取) |
| 状态管理 | 无 | 声明式, Git 为源 |
| 漂移检测 | 无 | 自动检测 + 修复 |
| 回滚 | 重新部署旧版本 | git revert |
| 审计 | CI 日志 | Git 历史 |
| 权限 | CI 需集群权限 | Agent 在集群内 |

---

## 生产最佳实践

| 实践 | 说明 |
|------|------|
| 分环境分仓 | dev/staging/production 使用不同分支或 overlay |
| PR 审批 | 生产变更必须 PR + Code Review |
| 渐进式发布 | Argo Rollouts / Flagger 实现金丝雀/蓝绿 |
| Sealed [[secrets\|Secrets]] | 使用 sealed-secrets 或 external-secrets 管理敏感值 |
| 通知集成 | Argo CD Notifications / Flux Alert 发送 Slack/钉钉 |
| 多集群 | Argo CD ApplicationSet / Flux Kustomization 管理多集群 |
| RBAC | Argo CD RBAC 精细化到 project/app 级别 |
| 备份 | Velero 备份 Argo CD / Flux 的 CRD 资源 |

## Argo CD 故障排查

```bash
# 检查 Application 状态
kubectl get applications -n argocd
kubectl describe application myapp -n argocd

# 查看同步状态
argocd app get myapp
argocd app get myapp --show-params
argocd app get myapp -o json | jq '.status.conditions'

# 强制同步
argocd app sync myapp --force
argocd app sync myapp --prune
argocd app sync myapp --replace

# 回滚
argocd app rollback myapp <revision>
argocd app history myapp

# 查看日志
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-repo-server --tail=100
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-application-controller --tail=100

# 常见问题
# 1. OutOfSync: 集群状态与 Git 不一致
argocd app diff myapp  # 查看差异
argocd app sync myapp  # 同步

# 2. Degraded: 资源不健康
kubectl get pods -l app.kubernetes.io/instance=myapp
kubectl describe pod <pod-name>

# 3. 同步卡住
argocd app terminate-op myapp  # 终止当前操作
argocd app sync myapp --force  # 强制重新同步
```

## Flux CD 故障排查

```bash
# 检查 Kustomization 状态
flux get kustomizations -A
flux get kustomizations myapp -n flux-system

# 查看协调状态
flux reconcile kustomization myapp --with-source
flux logs --kind=Kustomization --name=myapp

# 强制协调
flux reconcile kustomization myapp --force
flux reconcile source git myapp

# 暂停/恢复
flux suspend kustomization myapp
flux resume kustomization myapp

# 查看事件
flux events --for Kustomization/myapp
kubectl get events -n flux-system --sort-by=.metadata.creationTimestamp

# 常见问题
# 1. Git 拉取失败
flux get sources git
kubectl describe gitrepository myapp -n flux-system
# 检查: SSH key、网络连通性、分支名

# 2. 应用失败
flux logs --kind=Kustomization --name=myapp --level=error
kubectl get kustomization myapp -o yaml | grep -A5 conditions

# 3. 漂移检测
flux diff kustomization myapp
```

## ApplicationSet 多集群管理

```yaml
# Argo CD ApplicationSet 示例
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: myapp-multi-cluster
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            env: production
  template:
    metadata:
      name: 'myapp-{{name}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/gitops.git
        targetRevision: main
        path: 'apps/myapp/overlays/{{metadata.labels.region}}'
      destination:
        server: '{{server}}'
        namespace: production
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

## 渐进式交付

```yaml
# Argo Rollouts 金丝雀发布
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: myapp
spec:
  replicas: 5
  strategy:
    canary:
      steps:
        - setWeight: 20
        - pause: {duration: 5m}
        - setWeight: 40
        - pause: {duration: 5m}
        - setWeight: 60
        - pause: {duration: 5m}
        - setWeight: 80
        - pause: {duration: 5m}
      analysis:
        templates:
          - templateName: success-rate
        startingStep: 1
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
        - name: myapp
          image: registry/myapp:v2.0.0
```

```bash
# Argo Rollouts 命令
kubectl argo rollouts status myapp
kubectl argo rollouts get rollout myapp
kubectl argo rollouts promote myapp    # 推进到下一步
kubectl argo rollouts abort myapp      # 中止并回滚
kubectl argo rollouts undo myapp       # 回滚
kubectl argo rollouts restart myapp    # 重启
```

## Git 仓库结构最佳实践

```
gitops-repo/
├── apps/                    # 应用定义
│   ├── myapp/
│   │   ├── base/           # 基础配置
│   │   │   ├── deployment.yaml
│   │   │   ├── service.yaml
│   │   │   └── kustomization.yaml
│   │   └── overlays/       # 环境差异
│   │       ├── dev/
│   │       ├── staging/
│   │       └── production/
│   └── platform/
│       ├── ingress/
│       ├── monitoring/
│       └── logging/
├── infrastructure/          # 基础设施
│   ├── cluster-config/
│   ├── namespaces/
│   └── rbac/
└── clusters/                # 集群级配置
    ├── cluster-a/
    └── cluster-b/
```

## 版本兼容矩阵

| 组件 | 当前版本 | K8s 兼容 | 关键变更 |
|------|---------|----------|----------|
| Argo CD | 2.13 | 1.25+ | ApplicationSet GA |
| Flux | 2.4 | 1.25+ | OCI 仓库支持 |
| Argo Rollouts | 1.7 | 1.25+ | 插件系统 |
| Flagger | 1.38 | 1.25+ | Gateway API 支持 |
| Sealed Secrets | 0.27 | 1.25+ | 性能优化 |

## 安全检查清单

- [ ] Git 仓库访问使用 SSH key 或细粒度 Token
- [ ] 生产分支启用保护（禁止直接 push）
- [ ] Argo CD/Flux ServiceAccount 最小权限
- [ ] 敏感值使用 Sealed Secrets/External Secrets
- [ ] 生产变更必须经过 PR 审批
- [ ] 启用自动同步的 selfHeal 防止配置漂移
- [ ] 定期审计 Git 提交历史
- [ ] 备份 GitOps 工具的 CRD 资源

```

<!-- risk-assessed -->
