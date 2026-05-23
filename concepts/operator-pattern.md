---
title: Operator Pattern (CRD + Controller)
description: Operator Pattern (CRD + Controller) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- operator
- crd
- webhook
- extension
- controller
- etcd
- apiserver
- prometheus
- istio
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator Pattern (CRD + Controller) 是什么
- 如何 Operator Pattern (CRD + Controller)
trigger_keywords:
- Operator
- Pattern
- CRD
- Controller
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- etcd-basics
- mysql-basics
- policy-basics
created: "2026-05-23"
---

# Operator Pattern (CRD + Controller)

## Custom Resource Definition (CRD)

CRDs extend [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|the Kubernetes API]] with custom resource types without modifying API Server code:

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
spec:
  group: example.com
  names:
    kind: Database
    plural: databases
  scope: Namespaced
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:  # Validation schema
```

CRD features:
- **Schema validation**: OpenAPI v3 JSON schema validation
- **Subresources**: `/status` and `/scale` endpoints
- **Additional printer columns**: Custom `kubectl get` columns
- **Multiple versions**: With conversion webhooks for cross-version migration

## Operator Controller

An Operator is a custom controller that manages CRD instances:

1. **Watch** CRD changes via Informer
2. **Reconcile**: Compare desired spec vs actual cluster state
3. **Create/Update** dependent Kubernetes resources ([[Deployments|Deployments]], Services, PVCs, etc.)
4. **Update Status** on the CRD instance

Popular operators: [[Prometheus|Prometheus]] Operator, Elasticsearch Operator, MySQL Operator, [[ArgoCD|ArgoCD]].

## Admission Webhooks

Webhooks intercept API requests in two phases:

| Type | Phase | Purpose | Example |
|------|-------|---------|---------|
| **Mutating** | Before validation | Modify requests | Istio sidecar injection, default values |
| **Validating** | After validation | Reject non-compliant requests | OPA/Gatekeeper policies, Kyverno |

Webhooks run as external HTTPS services registered with API Server. They must respond within the configured timeout or requests are rejected (or ignored for `failurePolicy: Ignore`).

## API Aggregation

The API aggregation layer allows running independent API Servers alongside the main kube-apiserver. Examples include metrics-server and custom metrics adapter. Requests are proxied through the main API Server.

## Related
- [[synthesis/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[synthesis/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合
- [[synthesis/CRD × 可观测性.md|CRD × 可观测性]] — 综合

- [[synthesis/Operator 模式 × 可观测性]]

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[skills/develop-crd-operator.md|develop-crd-operator]] — Develop CRD Operator
- [[entities/crd-custom-resources.md|crd-custom-resources]] — CRD (Custom Resource Definition)
- [[concepts/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[concepts/controller-pattern.md|Controller Pattern]]
- [[concepts/declarative-api.md|Declarative API]]
- [[entities/crd-custom-resources.md|CRD Custom Resources]]
- Admission Webhooks
- [[skills/develop-crd-operator.md|Develop CRD Operator]]
- [[journal/digest-2026-05-21|Wiki Digest — Daily (2026-05-21)]] — Cross-reference
- [[references/KUDIG Tag Dictionary|KUDIG Tag Dictionary]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[entities/kube-apiserver|kube-apiserver]] — Cross-reference
- [[entities/metal3-io|Metal3]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
