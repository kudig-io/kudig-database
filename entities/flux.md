---
title: Flux
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
---

# Flux

Flux is a set of Kubernetes controllers that implement GitOps principles, graduated from CNCF in 2022.

## Key Facts

- **Status**: CNCF graduated (2022)
- **Architecture**: Set of specialized controllers (lightweight: 200-500MB RAM)
- **Secret Management**: Native SOPS support with age/GPG
- **Learning Curve**: Lower than ArgoCD

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

- [[domain-08-release-change-management/06-flux-gitops-continuous-delivery.md|06-flux-gitops-continuous-delivery]]
- [[domain-08-release-change-management/99-flux-gitops-guide.md|99-flux-gitops-guide]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting.md|03-flux-image-automation-troubleshooting]]
- [[domain-19-landscape-references/graduated/flux/flux.md|flux]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.36.md|RELEASE-NOTES-0.36]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.32.md|RELEASE-NOTES-0.32]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.33.md|RELEASE-NOTES-0.33]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.27.md|RELEASE-NOTES-0.27]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.37.md|RELEASE-NOTES-0.37]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.4.md|RELEASE-NOTES-2.4]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.28.md|RELEASE-NOTES-0.28]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.38.md|RELEASE-NOTES-0.38]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.29.md|RELEASE-NOTES-0.29]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.39.md|RELEASE-NOTES-0.39]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.5.md|RELEASE-NOTES-2.5]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.0.md|RELEASE-NOTES-0.0]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.30.md|RELEASE-NOTES-0.30]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.41.md|RELEASE-NOTES-0.41]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.34.md|RELEASE-NOTES-0.34]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.40.md|RELEASE-NOTES-0.40]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.35.md|RELEASE-NOTES-0.35]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.31.md|RELEASE-NOTES-0.31]]
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
