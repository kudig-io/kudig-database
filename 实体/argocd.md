---
title: ArgoCD
description: '- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合'
summary: '- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合'
category: entities
tags:
- argocd
- gitops
- k8s
- cncf
- cd
- helm
- flux
- redis
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ArgoCD 是什么
- 如何 ArgoCD
trigger_keywords:
- ArgoCD
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- redis-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ArgoCD

## Overview

ArgoCD is a CNCF graduated project implementing the [[概念/gitops-principles.md|[[GitOps 速查卡|GitOps]]]] pattern for Kubernetes. It continuously monitors Git repositories and automatically synchronizes cluster state to match the desired state declared in manifests.

## Architecture

ArgoCD runs as a Kubernetes Deployment with these core components:

- **API Server**: gRPC/REST API, authentication, authorization
- **Repository Server**: Clones Git repos, caches manifests, renders Helm/Kustomize
- **Application Controller**: Reconciliation loop, compares live vs desired state, triggers sync
- **Redis**: Caching layer for Git repos and cluster state

## Key Features

### Application Model

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/org/repo.git
    targetRevision: HEAD
    path: k8s/overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
```

### Sync Waves and Hooks

Sync phases execute in order: PreSync, Sync, PostSync, SyncFail. Hooks run at specific phases:

| Hook Phase | Use Case |
|---|---|
| PreSync | Database migrations, config validation |
| Sync | Main resource deployment |
| PostSync | Smoke tests, notifications |
| SyncFail | Rollback, alerting |

### ApplicationSet

Generates multiple Applications from a single template using generators:

- **List Generator**: Static list of cluster/env combinations
- **Git Generator**: Discover environments from Git directory structure
- **Cluster Generator**: Auto-generate from registered clusters
- **Matrix/Combine**: Cross-product of multiple generators

## Comparison with [[flux|Flux]]

| Dimension | ArgoCD | Flux |
|---|---|---|
| Sync Model | Pull + UI trigger | Pull only |
| UI | Rich web dashboard | Terminal-focused (Flux CLI) |
| Multi-tenancy | Project-based isolation | Namespace-based |
| ApplicationSet | Template-based generation | Kustomization composition |
| Git Providers | Broad provider support | Native Git implementation |
| Learning Curve | Moderate (GUI helps) | Steeper (CLI-first) |
| Best For | Teams needing visual oversight | Infrastructure-as-code purists |

## Key Metrics

- **Sync Status**: Synced / OutOfSync / Unknown
- **Health Status**: Healthy / Progressing / Degraded / Suspended / Missing
- **Operation State**: Running / Succeeded / Failed / Error
- **Reconciliation Duration**: Time from Git commit to cluster sync

## Debugging

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Check application status
argocd app get my-app

# View sync history
argocd app history my-app

# View diff (live vs desired)
argocd app diff my-app

# Check controller logs
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-application-controller

# 🟡 中风险：修改集群状态
# Force re-sync
argocd app sync my-app --force

# 回滚到指定版本
argocd app rollback my-app <revision>

# 暂停自动同步
argocd app set my-app --sync-policy none

# 🔴 高风险：可能造成服务中断
# 硬删除应用（保留资源）
argocd app delete my-app --cascade=false

# 硬删除应用（级联删除资源）
argocd app delete my-app --cascade
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Application OutOfSync | Git 与集群状态不一致 | `argocd app diff my-app` | 执行 sync 或修正手动变更 |
| Sync 卡住 Progressing | 资源未达 Healthy | `argocd app get my-app --show-params` | 检查 Pod 事件和资源配额 |
| repo-server 连接失败 | Git 凭据过期/网络问题 | `kubectl logs -n argocd -l app.kubernetes.io/name=argocd-repo-server` | 更新 Secret/检查网络策略 |
| Controller OOM | 管理应用过多 | `kubectl top pods -n argocd` | 增加资源/分片 Controller |
| Webhook 不触发 | Secret 配置错误 | `kubectl logs -n argocd argocd-notifications-controller-*` | 重新配置 webhook secret |

```
排查流程:
├── 同步异常
│   ├── argocd app get → Sync/Health 状态
│   ├── argocd app diff → 期望与实际差异
│   ├── kubectl get events → 资源创建失败原因
│   └── argocd app logs → 控制器日志
├── 连接问题
│   ├── kubectl get pods -n argocd → 组件状态
│   ├── 检查 repo-server Secret → Git 凭据有效性
│   └── curl Git 仓库 → 网络可达性
└── 性能问题
    ├── kubectl top pods -n argocd → 资源使用
    ├── argocd app list --selector → 应用数量
    └── Controller 分片配置 → 负载均衡
```

## 生产案例

### 案例1: 大规模 ApplicationSet 导致 Controller OOM

- **场景**: 500+ 微服务通过 ApplicationSet 生成，Controller 内存持续增长至 OOMKilled
- **排查**: `kubectl top pods -n argocd` 显示 Controller 内存 8Gi 达上限，reconcile 队列积压 2000+
- **方案**:
  1. Controller 启用分片（`--application-controller-shard-replicas=4`）
  2. 调整 `--status-processors` 和 `--operation-processors` 参数
  3. ApplicationSet 启用 progressive sync 限制并发
- **效果**: 内存稳定在 2Gi/分片，reconcile 延迟从 5min 降至 10s

### 案例2: Git Webhook 失效导致配置漂移

- **场景**: 开发团队反馈 Git push 后 ArgoCD 未自动同步，手动检查发现 3 天未更新
- **排查**: GitHub webhook 返回 502，repo-server Pod 重启后 webhook secret 丢失
- **方案**:
  1. 将 webhook secret 存入 Vault，通过 ExternalSecrets 同步
  2. 配置 `argocd app sync --retry-limit=3` 自动重试
  3. 添加 Prometheus 告警：`argocd_app_info{sync_status="OutOfSync"} > 0` 持续 10min
- **效果**: 配置漂移检测时间从 3 天缩短至 10 分钟

## Integration Points

- Integrates with [[supply-chain-security|Supply Chain Security]] via image updater for automated tag tracking
- Connects to [[实体/vault.md|Vault]] for secret injection via Vault Agent templates
- Works with [[kyverno|Kyverno]] for policy enforcement post-sync
- Part of broader [[概念/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]] ecosystem

## Related
- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合

- [[概念/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[supply-chain-security]] — Software Supply Chain Security
- [[grpc]] — gRPC
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/gitops-principles.md|GitOps Principles]]
- [[flux|Flux]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[supply-chain-security|Supply Chain Security]]

- 36-ecosystem-kustomize-helm-argocd
- 09-gitops-workflow-argocd
- [[故障诊断/高级排障/38-gitops-argocd-troubleshooting.md|38-gitops-argocd-troubleshooting]]
- [[工作负载/06-java-cicd-tekton-argocd.md|06-java-cicd-tekton-argocd]]
- [[故障诊断/FTA故障树/list/gitops-argocd-fta.md|GitOps(ArgoCD) 异常故障树分析]]

<!-- risk-assessed -->
