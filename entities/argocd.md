---
title: ArgoCD
description: '- [[synthesis/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合'
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
created: "2026-05-23"
---

# ArgoCD

## Overview

ArgoCD is a CNCF graduated project implementing the [[concepts/gitops-principles.md|[[GitOps 速查卡|GitOps]]]] pattern for Kubernetes. It continuously monitors Git repositories and automatically synchronizes cluster state to match the desired state declared in manifests.

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

```bash
# Check application status
argocd app get my-app

# View sync history
argocd app history my-app

# Force re-sync
argocd app sync my-app --force

# View diff (live vs desired)
argocd app diff my-app

# Check controller logs
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-application-controller
```

## Integration Points

- Integrates with [[supply-chain-security|Supply Chain Security]] via image updater for automated tag tracking
- Connects to [[entities/vault.md|Vault]] for secret injection via Vault Agent templates
- Works with [[kyverno|Kyverno]] for policy enforcement post-sync
- Part of broader [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]] ecosystem

## Related
- [[synthesis/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]] — 综合

- [[concepts/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[supply-chain-security]] — Software Supply Chain Security
- [[grpc]] — gRPC
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/gitops-principles.md|GitOps Principles]]
- [[flux|Flux]]
- [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[supply-chain-security|Supply Chain Security]]

- 36-ecosystem-kustomize-helm-argocd
- 09-gitops-workflow-argocd
- [[domain-10-troubleshooting-diagnostics/38-gitops-argocd-troubleshooting.md|38-gitops-argocd-troubleshooting]]
- [[domain-02-workloads-applications/06-java-cicd-tekton-argocd.md|06-java-cicd-tekton-argocd]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/gitops-argocd-fta.md|GitOps(ArgoCD) 异常故障树分析]]