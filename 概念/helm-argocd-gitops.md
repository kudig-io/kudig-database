---
title: Helm 与 ArgoCD GitOps 工作流
summary: Helm 与 ArgoCD GitOps 工作流：Helm 负责模板化和打包，ArgoCD 负责声明式同步和漂移检测。两者结合形成完整的 GitOps
  工作流。
category: synthesis
tags:
- synthesis
- helm
- argocd
- gitops
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Helm 与 ArgoCD GitOps 工作流

> Helm 作为 Kubernetes 包管理器与 ArgoCD 作为声明式 GitOps 持续部署工具的协同工作模式。

## 概述

Helm 负责模板化和打包，ArgoCD 负责声明式同步和漂移检测。两者结合形成完整的 GitOps 工作流：Helm 解决"如何生成多环境配置"的问题，ArgoCD 解决"如何将配置可靠地部署到集群并保持同步"的问题。

## 工作流

```
# 🟢 低风险：信息收集，无副作用
代码变更 → Helm Chart 更新 → Git 推送 → ArgoCD 检测 → 自动/手动同步 → 集群更新
                                                         ↓
                                                  漂移检测（self-heal）
```

## ArgoCD 处理 Helm 的两种模式

### 模式 1：Helm Chart 作为 ArgoCD 应用源

ArgoCD 原生支持将 Helm chart 作为应用源，在 ArgoCD 内部执行 `helm template`，将渲染后的 manifest 视为普通 Kubernetes 资源：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: order-service
  namespace: argocd
spec:
  source:
    repoURL: https://github.com/org/charts
    path: charts/order-service
    targetRevision: main
    helm:
      valueFiles:
        - values.yaml
        - values-production.yaml     # 环境覆盖
      parameters:
        - name: image.tag
          value: v2.1.0               # 参数覆盖
        - name: replicaCount
          value: "3"
      skipCrds: false
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

**特点**：ArgoCD 接管 Helm 渲染，不依赖 Tiller/Helm release 元数据。漂移检测有效。

### 模式 2：Helm Umbrella Chart

使用一个 umbrella chart 聚合多个子 chart，实现环境级配置打包：

```yaml
# Chart.yaml (umbrella)
apiVersion: v2
name: production-platform
version: 1.0.0
dependencies:
  - name: order-service
    version: 2.1.0
    repository: file://../order-service
  - name: payment-service
    version: 1.5.0
    repository: file://../payment-service
  - name: prometheus
    version: 58.0.0
    repository: https://prometheus-community.github.io/helm-charts
```

```yaml
# values-production.yaml (统一配置)
order-service:
  replicaCount: 3
  image:
    tag: v2.1.0
  resources:
    limits:
      cpu: 1000m
      memory: 1Gi

payment-service:
  replicaCount: 2
  database:
    host: prod-db.internal
```

## 多集群部署（ApplicationSet）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-cluster-deploy
spec:
  generators:
    - list:
        elements:
          - cluster: us-east
            url: https://cluster-us-east-api:6443
          - cluster: eu-west
            url: https://cluster-eu-west-api:6443
  template:
    metadata:
      name: '{{cluster}}-order-service'
    spec:
      source:
        chart: order-service
        repoURL: https://charts.internal/org
        targetRevision: 2.1.0
        helm:
          valueFiles:
            - 'values-{{cluster}}.yaml'    # 每集群独立 values
      destination:
        server: '{{url}}'
        namespace: production
```

## Sync Windows 生产保护

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
spec:
  syncPolicy:
    syncOptions:
      - ApplyOutOfSyncOnly=true
# 集群级 Sync Window（在 argocd-cm 配置）
# 仅允许工作日 10:00-18:00 同步生产环境
```

## 最佳实践

- **使用 Helm umbrella chart 管理多组件部署**：将相关微服务打包为一个 umbrella，通过单一 Application 管理整个应用的版本一致性
- **通过 ArgoCD ApplicationSet 实现多集群部署**：避免为每个集群手动创建 Application，使用 generator 模板化批量管理
- **配置 Sync Windows 控制生产环境变更窗口**：限制非工作时间自动同步，防止意外变更影响生产
- **values 文件按环境分层**：`values.yaml`（基线）→ `values-staging.yaml`（覆盖）→ `values-production.yaml`（再覆盖），最小化重复配置
- **避免使用 `helm install` 手动部署**：所有部署通过 ArgoCD 进行，否则 ArgoCD 的漂移检测会与手动操作冲突

## 常见陷阱

- **Helm hook 与 ArgoCD Sync Wave 冲突**：Helm 的 `post-install` hook 和 ArgoCD 的 sync hooks 行为不同，混用可能导致初始化顺序混乱——建议在 ArgoCD 环境中统一使用 ArgoCD Sync Wave
- **ArgoCD selfHeal 与 Helm 价值观冲突**：开启 selfHeal 后，任何手动 `kubectl` 修改都会被回滚——需要团队充分理解这一行为
- **Chart 仓库缓存延迟**：ArgoCD 默认缓存 Helm 仓库索引，新版本 chart 可能不会立即被检测到——需要配置合理的 `helm.repos` 缓存刷新间隔

## 相关页面

- [[helm]] — Helm Chart 管理
- [[argocd]] — ArgoCD 持续部署
- [[概念/prometheus-argocd-monitoring.md|Prometheus 与 ArgoCD 监控]] — 监控栈 GitOps
- [[概念/gitops-release-gate.md|GitOps 发布门控]] — 发布安全控制
- [[deployment]] — Deployment 策略
- [[kubernetes]] — 集群架构


<!-- risk-assessed -->
