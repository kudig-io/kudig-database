---
title: Flux (entities)
description: Flux — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- gitops
- flux
- ci-cd
- declarative
- helm
- argocd
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
created: "2026-05-23"
---

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

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[sops]] — SOPS (Secrets OPerationS)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[concepts/gitops-principles.md|gitops-principles]] — GitOps Principles and Practice
- [[concepts/gitops-principles.md|GitOps Principles]]
- [[entities/argocd.md|ArgoCD]]

- 06-flux-gitops-continuous-delivery
- 99-flux-gitops-guide
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting.md|03-flux-image-automation-troubleshooting]]
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
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- RELEASE-NOTES-0.1
- RELEASE-NOTES-2.3
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
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
- [[entities/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[entities/k8s-production-operations.md|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[concepts/IaC x 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[concepts/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[concepts/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[skills/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/helm-index.md|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
