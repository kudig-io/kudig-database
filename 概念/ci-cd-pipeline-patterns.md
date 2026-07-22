---
title: CI/CD Pipeline Patterns
description: '- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合'
summary: '- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合'
category: concepts
tags:
- k8s
- ci-cd
- tekton
- jenkins
- github-actions
- pipeline
- argocd
- flux
- docker
- harbor
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CI/CD Pipeline Patterns 是什么
- 如何 CI/CD Pipeline Patterns
trigger_keywords:
- CI
- CD
- Pipeline
- Patterns
prerequisites:
- kubectl-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CI/CD Pipeline Patterns

## Standard CI/CD Pipeline Flow

```
Code Push -> Build Image -> Run Tests -> Security Scan -> Push to Registry -> Update GitOps Manifest -> GitOps Sync -> Deploy
```

| Stage | Action | Tools |
|-------|--------|-------|
| Build | Compile code, build container image | Docker BuildKit, Kaniko |
| Test | Unit, integration, E2E tests | pytest, Jest, Cypress |
| Scan | Vulnerability scan, SBOM generation | [[Trivy|Trivy]], Syft |
| Sign | Image signature verification | Cosign/Sigstore |
| Push | Push to container registry | [[Harbor|Harbor]], ECR, GCR |
| Update Manifest | Update K8s manifests with new image tag | kustomize edit set image |
| Deploy | GitOps controller syncs to cluster | ArgoCD, Flux |

## CI Platform Comparison

| Dimension | Tekton | GitHub Actions | GitLab CI | Jenkins |
|-----------|--------|---------------|-----------|---------|
| Architecture | K8s-native Pods | SaaS (Actions Runner) | Self-hosted/SaaS | Self-hosted Controller+Agent |
| Portability | High (K8s standard) | Low (GitHub locked) | Medium | Medium |
| Supply Chain Security | Chains + SLSA | OIDC + SLSA | Built-in scanning | Plugins |
| Learning Curve | High (verbose YAML) | Low (simple syntax) | Medium | Medium (Groovy) |
| Best For | Cloud-native enterprises | GitHub teams | GitLab teams | Traditional enterprises |

## Progressive Delivery Patterns

| Pattern | Description | When to Use |
|---------|-------------|-------------|
| Rolling Update | Replace pods incrementally | Default, low risk |
| Blue-Green | Two identical environments, switch traffic | Zero-downtime deployments |
| Canary | Gradual traffic shift (5% -> 20% -> 50% -> 100%) | High-risk changes, production |
| A/B Testing | Traffic split based on user attributes | Feature experimentation |
| Shadow Traffic | Copy production traffic to new version | Validation without user impact |

## Supply Chain Security Integration

Modern CI/CD pipelines integrate:
- SLSA-compliant builds (Tekton Chains, GitHub Actions)
- SBOM generation (Syft -> CycloneDX/SPDX format)
- Image signing (Cosign with Sigstore keyless signing)
- Admission verification (Kyverno validates signatures before deployment)

## 源码实现分析

### GitOps 调谐循环（ArgoCD）

```go
// argocd/controller/appcontroller.go 简化逻辑
func (ctrl *ApplicationController) processAppRefresh(app *appv1.Application) {
    // 1. 从 Git 拉取目标状态（manifests）
    targetState := ctrl.gitOpsEngine.GetTargetState(app.Spec.Source)
    
    // 2. 获取集群当前状态（live state）
    liveState := ctrl.kubeClient.GetLiveState(app.Spec.Destination)
    
    // 3. Diff 计算差异
    diffs := ctrl.gitOpsEngine.Diff(targetState, liveState)
    
    // 4. 自动/手动同步
    if app.Spec.SyncPolicy.Automated != nil {
        ctrl.gitOpsEngine.Sync(diffs)  // kubectl apply 等效操作
    } else {
        ctrl.setOutOfSyncStatus(app, diffs)  // 等待手动触发
    }
}
```

### Tekton Pipeline 执行模型

```
PipelineRun 创建
    │
    ▼
Tekton Controller Watch → 解析 Task 依赖图 (DAG)
    │
    ▼
TaskRun 创建 → Pod 创建（每个 Step = initContainer/container）
    │
    ▼
Step 1: git-clone (initContainer)
Step 2: build (container)
Step 3: test  (container)
Step 4: push  (container)
    │
    ▼
TaskRun Complete → 触发下游 TaskRun (DAG 依赖)
```

## 使用场景

### 场景一：完整 GitOps 流水线（GitHub Actions + ArgoCD）

```yaml
# .github/workflows/deploy.yaml
name: Build and Deploy
on:
  push:
    branches: [main]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Build & Push Image
      run: |
        docker build -t registry.example.com/app:${{ github.sha }} .
        docker push registry.example.com/app:${{ github.sha }}
    - name: Sign Image
      run: cosign sign registry.example.com/app:${{ github.sha }}
    - name: Update Kustomize
      run: |
        cd gitops-repo/overlays/production
        kustomize edit set image app=registry.example.com/app:${{ github.sha }}
        git commit -am "deploy: ${{ github.sha }}"
        git push  # ArgoCD 自动检测并同步
```

### 场景二：金丝雀发布（Argo Rollouts）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: web-app
spec:
  replicas: 5
  strategy:
    canary:
      steps:
      - setWeight: 10       # 10% 流量到新版本
      - pause: {duration: 5m}
      - analysis:            # 自动指标分析
          templates:
          - templateName: success-rate
      - setWeight: 50
      - pause: {duration: 10m}
      - setWeight: 100
  selector:
    matchLabels:
      app: web
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| CI/CD 就是自动化部署 | CI 是持续集成（构建+测试），CD 是持续交付/部署，两者职责不同 |
| GitOps 不需要 CI | GitOps 是 CD 部分，仍需 CI 构建镜像、运行测试、更新 manifest |
| 金丝雀发布等于滚动更新 | 滚动更新无流量控制，金丝雀通过流量比例+指标分析控制风险 |
| 镜像 latest 标签可用于生产 | 生产必须用不可变标签（SHA/digest），否则无法回滚和审计 |
| Tekton 可以替代 ArgoCD | Tekton 是 CI（构建/测试），ArgoCD 是 CD（同步部署），互补而非替代 |
| 流水线成功即部署成功 | 需配合健康检查、指标监控、自动回滚机制确认部署真正成功 |

## 面试要点

1. **GitOps 的核心原则？** — Git 为唯一事实源（Single Source of Truth）；声明式描述期望状态；自动调谐实际状态向期望收敛；偏差检测与自动修复。代表工具：ArgoCD（Pull 模式）、Flux（Pull 模式）。

2. **Push vs Pull 部署模式的区别？** — Push（kubectl apply / Helm upgrade）：CI 直接操作集群，需集群凭证，安全风险高；Pull（ArgoCD/Flux）：集群内 Agent 从 Git 拉取，无需外部凭证，更安全且支持多集群。

3. **如何保证供应链安全？** — SLSA 框架：可重现构建（provenance）→ SBOM 生成（Syft）→ 镜像签名（Cosign/Sigstore）→ 准入验证（Kyverno verifyImages）→ 运行时监控。确保从源码到部署的每个环节可验证。

4. **生产环境发布策略选择？** — 无状态服务用 RollingUpdate（默认）；高风险变更用 Canary（流量比例+指标分析）；数据库迁移用 Blue-Green（瞬时切换）；功能实验用 A/B Testing（用户属性分流）。

## Related
- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合

- [[实体/trivy.md|trivy]] — Trivy
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[supply-chain-security]] — Software Supply Chain Security
- [[flux]] — Flux
- [[概念/gitops-principles.md|GitOps Principles]]
- [[概念/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[supply-chain-security|Supply Chain Security]]


<!-- risk-assessed -->
