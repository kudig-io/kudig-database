---
title: Crossplane
description: Crossplane — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- iac
- crossplane
- infrastructure
- composition
- etcd
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 是什么
- 如何 Crossplane
trigger_keywords:
- Crossplane
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---

# Crossplane

Crossplane extends Kubernetes with custom resources for cloud infrastructure, enabling declarative management of AWS, GCP, Azure, and other cloud resources via kubectl.

## Key Facts

- **Status**: CNCF incubating
- **Architecture**: K8s controllers (state in etcd)
- **Providers**: AWS, GCP, Azure, Helm, and many more
- **GitOps Integration**: Native (state in K8s, reconciled by controllers)

## Core Concepts

| Concept | Description |
|---------|-------------|
| Provider | Plugin for a specific cloud platform (AWS, GCP, Azure) |
| Composition | Combine multiple resources into a higher-level abstraction |
| XRD (Composite Resource Definition) | Define a new resource type that composes multiple underlying resources |
| Claim | Namespace-scoped request for a composite resource |

## Platform Engineering Role

Crossplane enables platform teams to create self-service infrastructure abstractions. Developers request a "Database" claim, and Crossplane provisions RDS, creates K8s Secrets, and configures networking -- all declaratively.

## Related

- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[concepts/platform-engineering-idp.md|platform-engineering-idp]] — Platform Engineering and Internal Developer Platforms
- [[concepts/infrastructure-as-code.md|Infrastructure as Code]]
- [[concepts/platform-engineering-idp.md|Platform Engineering and IDP]]
- [[concepts/gitops-principles.md|GitOps Principles]]

- [[domain-07-platform-engineering/07-crossplane-platform-composition.md|07-crossplane-platform-composition]]
- [[domain-08-release-change-management/05-crossplane-enterprise-orchestration.md|05-crossplane-enterprise-orchestration]]
- [[domain-08-release-change-management/99-crossplane-platform-guide.md|99-crossplane-platform-guide]]
- [[domain-19-landscape-references/graduated/crossplane/crossplane.md|crossplane]]
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
