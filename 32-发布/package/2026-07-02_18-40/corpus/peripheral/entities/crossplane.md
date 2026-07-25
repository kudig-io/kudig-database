---
title: Crossplane (entities)
description: Crossplane — Kubernetes 生产运维知识库
summary: Crossplane — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- iac
- crossplane
- infrastructure
- composition
- etcd
- helm
tier: peripheral
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

## [[concepts/platform-engineering-sre.md|Platform Engineering]] Role

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

- 07-crossplane-platform-composition
- 05-crossplane-enterprise-orchestration
- 99-crossplane-platform-guide
- crossplane
- [[concepts/IaC x 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[concepts/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
