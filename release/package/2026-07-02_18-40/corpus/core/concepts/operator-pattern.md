---
title: Operator Pattern (CRD + Controller)
description: Operator Pattern (CRD + Controller) — Kubernetes 生产运维知识库
summary: Operator Pattern (CRD + Controller) — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Operator Pattern (CRD + Controller)

## Custom Resource Definition (CRD)

CRDs extend [[domain-17-system-foundation/知识字典/fundamentals/the-kubernetes-api.md|the Kubernetes API]] with custom resource types without modifying API Server code:

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
- [[concepts/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[concepts/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合
- [[concepts/CRD × 可观测性.md|CRD × 可观测性]] — 综合

- [[concepts/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]

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
- Wiki Digest — Daily (2026-05-21) — Cross-reference
- [[entities/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]] — Cross-reference
- [[entities/platform-engineering-terms.md|K8s 平台工程术语参考]] — Cross-reference
- [[concepts/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[entities/kube-apiserver.md|kube-apiserver]] — Cross-reference
- [[entities/metal3-io.md|Metal3]] — Cross-reference
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/helm-index.md|Helm 全局索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
