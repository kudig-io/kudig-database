---
title: CRD (Custom Resource Definition)
description: CRD (Custom Resource Definition) — Kubernetes 生产运维知识库
summary: CRD (Custom Resource Definition) — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRD (Custom Resource Definition)

## Role

CRDs allow users to define new resource types that are treated as first-class citizens by [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|the Kubernetes API]]. Once a CRD is created, you can `kubectl create/apply/get` instances of the custom resource.

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
- [[concepts/CRD × 可观测性.md|CRD × 可观测性]] — 综合

- [[concepts/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — 控制器模式 × Operator 模式
- [[entities/kubernetes-api-versions-reference.md|kubernetes-api-versions-reference]] — Kubernetes API Versions Reference
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/develop-crd-operator.md|develop-crd-operator]] — Develop CRD Operator
- [[operator-pattern|Operator Pattern]]
- Admission Webhooks
- [[concepts/declarative-api.md|Declarative API]]
- [[skills/develop-crd-operator.md|Develop CRD Operator]]
- [[entities/metal3-io.md|Metal3]] — Cross-reference
- [[entities/clusterpedia.md|Clusterpedia]] — Cross-reference


<!-- risk-assessed -->
