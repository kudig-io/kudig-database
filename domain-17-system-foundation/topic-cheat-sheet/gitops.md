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



# GitOps 生产环境速查卡

> **适用版本**: [[Argo|Argo]] CD 2.10+ / [[flux|[[Flux]]]] 2.2+ | **最后更新**: 2026-05

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

```bash
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
| Sealed [[Secrets|Secrets]] | 使用 sealed-secrets 或 external-secrets 管理敏感值 |
| 通知集成 | Argo CD Notifications / Flux Alert 发送 Slack/钉钉 |
| 多集群 | Argo CD ApplicationSet / Flux Kustomization 管理多集群 |
| RBAC | Argo CD RBAC 精细化到 project/app 级别 |
| 备份 | Velero 备份 Argo CD / Flux 的 CRD 资源 |

```