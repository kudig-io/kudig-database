---
title: Flux (entities)
description: Flux — Kubernetes 生产运维知识库
summary: Flux — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- gitops
- flux
- ci-cd
- declarative
- helm
- argocd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Flux 是什么
- 如何 Flux
trigger_keywords:
- Flux
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flux

Flux is a set of Kubernetes controllers that implement GitOpsps Principles and Practice|GitOps principles]], graduated from CNCF in 2022.

## Key Facts

- **Status**: CNCF graduated (2022)
- **Architecture**: Set of specialized controllers (lightweight: 200-500MB RAM)
- **Secret Management**: Native SOPS support with age/GPG
- **Learning Curve**: Lower than [[ArgoCD|ArgoCD]]

## Core Controllers

| Controller | Function |
|-----------|----------|
| source-controller | Manages Git/OCI/Helm repositories |
| kustomize-controller | Applies Kustomize overlays |
| helm-controller | Manages Helm releases |
| notification-controller | Sends alerts to external systems |
| image-reflector-controller | Scans registries for image updates |
| image-automation-controller | Updates Git manifests with new image tags |

## Flux vs ArgoCD

Flux is lighter and simpler, with built-in SOPS decryption and image automation. ArgoCD has richer UI and more mature multi-cluster management. Flux is preferred for fully automated pipelines; ArgoCD is preferred when visual approval workflows are needed.

## 安装与配置

### Flux CLI 安装与 Bootstrap

```bash
# 安装 Flux CLI
curl -s https://fluxcd.io/install.sh | bash

# 检查集群先决条件
flux check --pre

# Bootstrap 到 GitHub（创建 flux-system 命名空间和 GitRepository/Kustomization）
flux bootstrap github \
  --owner=my-org \
  --repository=fleet-infra \
  --branch=main \
  --path=clusters/production \
  --personal

# 验证安装
flux get all -A
flux check
```

### 核心 CRD 配置

```yaml
# GitRepository - 定义 Git 源
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: app-repo
  namespace: flux-system
spec:
  interval: 5m
  url: https://github.com/my-org/app-manifests
  ref:
    branch: main
  secretRef:
    name: git-credentials
---
# Kustomization - 定义应用部署
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: app
  namespace: flux-system
spec:
  interval: 10m
  path: ./deploy/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: app-repo
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      namespace: default
---
# HelmRelease - Helm 应用管理
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: cert-manager
  namespace: cert-manager
spec:
  interval: 10m
  chart:
    spec:
      chart: cert-manager
      version: "1.x"
      sourceRef:
        kind: HelmRepository
        name: jetstack
  values:
    installCRDs: true
```

### 镜像自动化

```yaml
# ImageRepository - 扫描镜像更新
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: app-image
  namespace: flux-system
spec:
  image: myregistry.io/myapp
  interval: 5m
---
# ImagePolicy - 定义更新策略
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: app-semver
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: app-image
  policy:
    semver:
      range: ">=1.0.0 <2.0.0"
---
# ImageUpdateAutomation - 自动更新 Git 中的镜像标签
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageUpdateAutomation
metadata:
  name: app-auto
  namespace: flux-system
spec:
  interval: 5m
  sourceRef:
    kind: GitRepository
    name: app-repo
  git:
    checkout:
      ref:
        branch: main
    commit:
      author:
        name: flux-bot
        email: flux@myorg.io
      messageTemplate: "chore: update image to {{range .Updated.Images}}{{println .}}{{end}}"
    push:
      branch: main
  update:
    path: ./deploy
    strategy: Setters
```

## 运维操作

```bash
# 🟢 查看所有 Flux 资源状态
flux get all -A

# 🟢 查看 Kustomization 同步状态
flux get kustomizations -A
flux logs --kind=Kustomization --name=app -f

# 🟢 查看 HelmRelease 状态
flux get helmreleases -A

# 🟡 手动触发同步
flux reconcile kustomization app --with-source
flux reconcile helmrelease cert-manager -n cert-manager

# 🟡 暂停/恢复同步（维护窗口）
flux suspend kustomization app
flux resume kustomization app

# 🟡 导出当前配置
flux export kustomization app -n flux-system

# 🔴 删除 Flux 资源（会删除集群中对应的工作负载）
flux delete kustomization app

# 🔴 完全卸载 Flux
flux uninstall --namespace=flux-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Kustomization 不同步 | Git 认证失败 | `flux logs --kind=GitRepository --name=app-repo` | 检查 Secret 中的 token/SSH key |
| HelmRelease 安装失败 | Chart 版本不存在 | `flux describe helmrelease <name> -n <ns>` | 检查 chart version 和 repo URL |
| 镜像自动化不触发 | ImageRepository 扫描失败 | `flux get images repository -A` | 检查 Registry 认证和网络 |
| 资源被意外删除 | prune: true 配置 | `flux diff kustomization app` | 检查 Kustomization path 和 prune 设置 |
| source-controller OOM | 大仓库内存不足 | `kubectl top pod -n flux-system` | 增加 resource limits |

**排查流程：**
```
Flux 同步失败
├── 检查控制器状态 → flux check
├── 检查源状态 → flux get sources all -A
├── 查看控制器日志 → flux logs --kind=Kustomization --name=<name>
├── 检查 Git 连接 → flux reconcile source git app-repo --with-source
└── 检查资源健康 → flux get kustomizations -A --status-selector ready=false
```

## 生产案例

### 案例一：多集群 GitOps 管理

- **场景**: 管理 20+ 个 K8s 集群，需要统一的配置分发和版本管理
- **排查**: 各集群手动 kubectl apply，配置漂移严重，回滚困难
- **方案**: 使用 Flux 多集群架构，中心 Git 仓库按 cluster/ 目录组织，每个集群 bootstrap 指向自己的路径，配合 ImageUpdateAutomation 实现自动升级
- **效果**: 配置漂移归零，部署时间从 30min 降至 2min，支持一键回滚（git revert）

### 案例二：SOPS 加密密钥管理

- **场景**: Git 仓库中需要存储加密的 Secret，不能使用明文
- **排查**: Flux 原生支持 SOPS + age 解密，无需额外组件
- **方案**: 使用 sops + age 加密 Secret YAML，Flux Kustomization 配置 decryption provider: sops，自动解密后应用
- **效果**: 密钥安全存储在 Git，无需外部 Vault，简化了密钥管理架构

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[sops]] — SOPS (Secrets OPerationS)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/gitops-principles.md|gitops-principles]] — GitOps Principles and Practice
- [[概念/gitops-principles.md|GitOps Principles]]
- [[实体/argocd.md|ArgoCD]]

- 06-flux-gitops-continuous-delivery
- 99-flux-gitops-guide
- [[故障诊断/高级排障/11-gitops-devops/03-flux-image-automation-troubleshooting.md|03-flux-image-automation-troubleshooting]]
- flux
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-2.4
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.28
- RELEASE-NOTES-2.0
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-2.1
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-2.5
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-2.2
- RELEASE-NOTES-0.4
- RELEASE-NOTES-2.6
- RELEASE-NOTES-0.0
- [[归档/release-notes/cicd-gitops/flux/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- RELEASE-NOTES-0.1
- RELEASE-NOTES-2.3
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- [[归档/release-notes/cicd-gitops/flux/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.31
- [[实体/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[实体/k8s-production-operations.md|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[概念/IaC x 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[概念/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[技能/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/helm-index.md|Helm 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
