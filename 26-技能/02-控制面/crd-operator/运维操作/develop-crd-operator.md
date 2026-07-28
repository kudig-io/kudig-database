---
title: Develop CRD Operator
description: Develop CRD Operator — Kubernetes 生产运维知识库
summary: Develop CRD Operator — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- operator
- crd
- development
- controller
- kubebuilder
- etcd
- apiserver
- helm
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Develop CRD Operator 是什么
- 如何 Develop CRD Operator
trigger_keywords:
- Develop
- CRD
- Operator
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Develop CRD Operator

## Development Workflow

### Step 1: Design the Custom Resource

Define the API (spec and status) focusing on user intent:
- **spec**: What the user wants (e.g., `replicas`, `version`, `storageSize`)
- **status**: What the operator reports (e.g., `ready`, `conditions`, `currentVersion`)

### Step 2: Scaffold with Kubebuilder

```bash
kubebuilder init --domain example.com
kubebuilder create api --group apps --version v1 --kind Database --resource --controller
```

This generates:
- CRD YAML with OpenAPI validation schema
- Controller with Reconcile() method stub
- Types definitions (spec and status structs)

### Step 3: Implement Reconciliation

The Reconcile() function follows the [[22-概念/controller-pattern.md|[[22-概念/01-核心架构/controller-pattern|Controller Pattern]]]]:

1. Fetch the CR instance
2. Determine desired state from spec
3. Compare with actual cluster state
4. Create/update/delete dependent resources
5. Update CR status
6. Return (requeue on error, no-requeue on success)

### Step 4: Add [[finalizers|Finalizers]]

Finalizers prevent CR deletion before cleanup:
```go
// On creation: add finalizer
if !containsString(finalizers, "finalizer.example.com") {
    finalizers = append(finalizers, "finalizer.example.com")
}
// On deletion with finalizer: clean up resources, then remove finalizer
if !deletionTimestamp.IsZero() {
    cleanup()
    removeFinalizer()
}
```

### Step 5: Test with envtest

Use `envtest` (provided by controller-runtime) for integration testing:
- Spins up a real API Server and etcd
- Tests reconciler against real [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]
- Fast, no full cluster needed

### Step 6: Package and Deploy

Build container image, generate RBAC manifests from kubebuilder markers, and deploy with kustomize or Helm.

## Key Libraries

| Library | Purpose |
|---------|---------|
| **controller-runtime** | Core controller framework (Informer, workqueue, reconciler) |
| **kubebuilder** | CLI scaffolding tool with code generation |
| **operator-sdk** | Alternative CLI, supports Go, Ansible, and Helm operators |
| **client-go** | Low-level Kubernetes API client (used internally) |

## Best Practices

- Make reconciliation idempotent
- Use finalizers for resource cleanup
- Update status on every reconciliation
- Handle errors gracefully with requeue and backoff
- Add meaningful conditions to status
- Use ownerReferences for garbage collection

## 生产案例

### 案例 1: Operator Reconcile 死循环导致 API Server 过载

| 时间 | 事件 |
|------|------|
| 14:00 | API Server 延迟飙升，kubectl 超时 |
| 14:05 | 审计日志显示某 Operator 每秒 1000+ 次 list/watch |
| 14:08 | Reconcile 逻辑错误，每次返回 Requeue 无延迟 |
| 14:10 | 🟡 修复 Reconcile 逻辑，添加 RequeueAfter: 30s |

**根因**: Reconcile 错误处理不当，无限快速重试。

### 案例 2: CRD 删除导致 Operator 崩溃

**现象**: Operator Pod CrashLoopBackOff，日志 "no matches for kind"。

**诊断**: CRD 被误删，Operator 无法 watch 资源

**修复**: 🔴 重新应用 CRD manifest

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Operator 影响集群稳定性 | 立即停止 Operator |
| P1 | Reconcile 失败 | 检查 CR 状态和日志 |
| P2 | 性能优化 | 添加缓存和限流 |

## 面试要点

1. **Q: Reconcile 循环的最佳实践？**
   A: ① 幂等性(多次执行结果相同) ② 错误时 RequeueAfter 延迟重试 ③ 使用 finalizer 处理删除逻辑 ④ 避免在 Reconcile 中做长时间操作 ⑤ 使用 status 子资源记录状态。

2. **Q: 如何测试 Operator？**
   A: ① 单元测试(mock client) ② 集成测试(envtest 框架) ③ E2E 测试(真实集群) ④ 混沌测试(模拟 API Server 不可用)。推荐 envtest + Ginkgo/Gomega。

3. **Q: Operator 的性能优化？**
   A: ① 使用 cache 减少 API 调用 ② 合理设置 MaxConcurrentReconciles ③ 使用 predicate 过滤无关事件 ④ 批量处理而非逐个 ⑤ 避免在 Reconcile 中做外部调用。

## Related

- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]] — CRD (Custom Resource Definition)
- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[operator-pattern|Operator Pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|Controller Pattern]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|CRD Custom Resources]]
- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]]


<!-- risk-assessed -->
