---
title: GitOps Principles and Practice
description: GitOps Principles and Practice — Kubernetes 生产运维知识库
summary: GitOps Principles and Practice — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- gitops
- ci-cd
- argocd
- flux
- declarative
- grafana
- helm
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
- GitOps Principles and Practice 是什么
- 如何 GitOps Principles and Practice
trigger_keywords:
- GitOps
- Principles
- and
- Practice
prerequisites:
- kubectl-basics
- helm-basics
- monitoring-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GitOps Principles and Practice

## [[OpenGitOps|OpenGitOps]] Four Principles

| Principle | Description | Implementation | Benefit |
|-----------|-------------|----------------|---------|
| Declarative | Desired state described declaratively | K8s YAML, [[Helm|Helm]] values, Kustomize | Reviewable, comparable, rollbackable |
| Versioned + Immutable | State stored in version-controlled system | Git repository | Full audit trail, traceable changes |
| Automated Pull | Changes pulled automatically from VCS | ArgoCD Sync, Flux Reconciliation | Reduced human error, consistency |
| Continuous Reconciliation | Agent compares actual vs desired state | ArgoCD Controller, Flux Kustomization Controller | Drift detection, self-healing |

## GitOps Maturity Model

| Level | Name | Key Characteristics | Tools |
|-------|------|-------------------|-------|
| 0 | Manual Deployment | kubectl apply / SSH, no version control | kubectl, SSH |
| 1 | Scripted | Shell script automation, partial automation | Bash, Ansible |
| 2 | Basic GitOps | Git-triggered auto-sync, declarative config | ArgoCD / Flux basic |
| 3 | Full CI/CD | Image signing, automated testing, env promotion | Tekton/GHA + ArgoCD |
| 4 | Security Compliance | SLSA Level 3+, SBOM, admission control | Cosign + Kyverno |
| 5 | Full Automation | Canary releases, auto-rollback, DORA optimized | Argo Rollouts + Flagger |

## ArgoCD vs Flux Comparison

| Dimension | ArgoCD | Flux |
|-----------|--------|------|
| Architecture | Monolith + controller | Set of specialized controllers |
| Web UI | Rich visualization | None (relies on Grafana) |
| Multi-cluster | ApplicationSet (mature) | remoteCluster |
| Learning Curve | Medium | Low |
| Resource Usage | Higher (2-4GB RAM) | Light (200-500MB) |
| Secret Management | Sealed Secrets / ESO | SOPS native support |
| Image Updates | Image Updater (plugin) | ImageAutomation (built-in) |
| CNCF Status | Graduated (2024) | Graduated (2022) |

## Multi-Environment Promotion

Standard promotion flow: Development (auto-sync, no approval) -> Staging (auto-sync, full testing) -> Production (manual approval + canary deployment with Argo Rollouts). Canary steps typically progress: 5% -> 20% -> 50% -> 100%, with metric analysis at each stage triggering auto-rollback on failure.

## Key DORA Metrics

| Metric | Elite Target | GitOps Impact |
|--------|-------------|---------------|
| Deployment Frequency | On-demand (multiple/day) | Auto-sync eliminates manual bottleneck |
| Lead Time for Changes | < 1 hour | CI automation + GitOps instant sync |
| Mean Time to Recovery | < 1 hour | Git revert instant rollback |
| Change Failure Rate | < 5% | Automated testing + progressive delivery |

## Related

- [[概念/production-operations-best-practices.md|production-operations-best-practices]] — Production Operations Best Practices
- [[概念/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[flux]] — Flux
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD Pipeline Patterns
- [[概念/infrastructure-as-code.md|Infrastructure as Code]]
- [[概念/production-operations-best-practices.md|Production Operations Best Practices]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[实体/argocd.md|ArgoCD]]
- [[flux|Flux]]
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]] — synthesis


<!-- risk-assessed -->
