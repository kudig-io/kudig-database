---
title: CRD (Custom Resource Definition)
description: CRD (Custom Resource Definition) — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- crd
- extension
- custom-resource
- api
- etcd
- rbac
- operator
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRD (Custom Resource Definition) 是什么
- 如何 CRD (Custom Resource Definition)
trigger_keywords:
- CRD
- Custom
- Resource
- Definition
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# CRD (Custom Resource Definition)

## Role

CRDs allow users to define new resource types that are treated as first-class citizens by [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|the Kubernetes API]]. Once a CRD is created, you can `kubectl create/apply/get` instances of the custom resource.

## CRD Specification

Key fields:
- `group`: API group name (e.g., `example.com`)
- `names`: kind, plural, singular, shortNames
- `scope`: Namespaced or Cluster
- `versions`: One or more API versions with OpenAPI validation schema
- `subresources`: Enable `/status` and `/scale` endpoints

## Version Management

CRDs support multiple API versions simultaneously:
- `served`: Clients can use this version
- `storage`: This version is persisted to etcd
- `conversion`: Webhook-based conversion between versions enables rolling migration

## When to Use CRDs

**Good fit**: Domain-specific configuration that benefits from Kubernetes tooling (kubectl, GitOps, RBAC, audit logging).

**Not a fit**: Simple configuration (use ConfigMap), or configuration that doesn't need Kubernetes lifecycle management.

## Related
- [[synthesis/CRD × 可观测性|CRD × 可观测性]] — 综合

- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — 控制器模式 × Operator 模式
- [[references/kubernetes-api-versions-reference|kubernetes-api-versions-reference]] — Kubernetes API Versions Reference
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/develop-crd-operator|develop-crd-operator]] — Develop CRD Operator
- [[operator-pattern|Operator Pattern]]
- Admission Webhooks
- [[concepts/declarative-api|Declarative API]]
- [[skills/develop-crd-operator|Develop CRD Operator]]
- [[entities/metal3-io|Metal3]] — Cross-reference
- [[entities/clusterpedia|Clusterpedia]] — Cross-reference
