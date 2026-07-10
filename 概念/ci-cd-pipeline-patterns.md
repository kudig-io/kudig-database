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
