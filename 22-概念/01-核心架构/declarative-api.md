---
title: Declarative API
description: '- 声明式 API 与面向终态设计'
summary: '- 声明式 API 与面向终态设计'
category: concepts
tags:
- k8s
- declarative
- api
- design-principle
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Declarative API 是什么
- 如何 Declarative API
trigger_keywords:
- Declarative
- API
prerequisites:
- kubectl-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Declarative API

## Core Principle

In Kubernetes, you declare **what** you want, not **how** to achieve it. A YAML manifest describes desired state (replicas, image, ports), and the system continuously works to maintain that state.

## Declarative vs Imperative

| Property | Imperative | Declarative |
|----------|-----------|-------------|
| Approach | "How to do it" | "What it should be" |
| Commands | `kubectl run`, `kubectl scale` | `kubectl apply -f` |
| Idempotency | Not guaranteed | Guaranteed |
| Order sensitivity | Order matters | Order independent |
| State tracking | Manual | System-managed |
| GitOps friendly | No | Yes |

## API Resource Model

Every Kubernetes object follows a standard structure:
- **TypeMeta**: apiVersion + kind (resource type identification)
- **ObjectMeta**: name, namespace, labels, annotations, uid, resourceVersion
- **Spec**: Desired state (user-defined, mutable)
- **Status**: Actual state (system-managed, read-only to users)

Key metadata fields:
- **resourceVersion**: etcd revision number, used for optimistic concurrency control
- **generation**: Incremented each time spec changes
- **ownerReferences**: Enables cascading deletion ([[17-系统基础/06-知识字典/fundamentals/garbage-collection.md|garbage collection]])
- **[[finalizers|finalizers]]**: Pre-delete hooks for resource cleanup

## Server-Side Apply (SSA)

Kubernetes v1.18+ supports Server-Side Apply, which enables multiple controllers to manage different fields of the same object without conflicts. Each field manager owns only the fields they declare, enabling safe collaborative editing.

## 源码实现分析

### API Server 处理流程

```
kubectl apply -f deployment.yaml
    │
    ├── 1. 认证 (Authentication): 验证身份
    ├── 2. 授权 (Authorization): RBAC 检查
    ├── 3. 准入控制 (Admission): Mutating + Validating Webhooks
    ├── 4. 验证 (Validation): Schema 校验
    ├── 5. 持久化: 写入 etcd (key: /registry/deployments/<ns>/<name>)
    └── 6. 通知: 通过 Watch 机制通知订阅者
```

### 乐观并发控制

```go
// k8s.io/apiserver/pkg/storage/etcd3/store.go
func (s *store) GuaranteedUpdate(ctx context.Context, key string, ...) error {
    // 1. 从 etcd 读取当前对象 + resourceVersion
    // 2. 应用变更
    // 3. 使用 Txn 条件写入: IF resourceVersion == expected THEN put
    // 4. 如果冲突，重试 (retry on conflict)
}
```

### Spec/Status 分离设计

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
  generation: 3          # spec 变更时递增
spec:                    # 用户声明的期望状态
  replicas: 5
  template:
    spec:
      containers:
      - name: app
        image: myapp:v2
status:                  # 系统报告的实际状态
  replicas: 5
  readyReplicas: 3       # 收敛中...
  observedGeneration: 3  # 控制器已处理的 generation
```

## 使用场景

### 场景1: GitOps 工作流

```bash
# 声明式: Git 仓库为唯一真相源
git commit -m "scale web to 10 replicas"
git push
# ArgoCD/Flux 自动同步: kubectl apply -f k8s/
```

### 场景2: 多控制器协作 (SSA)

```bash
# HPA 管理 replicas 字段
kubectl autoscale deployment web --min=2 --max=10 --cpu-percent=80

# 开发者管理 image 字段
kubectl set image deployment/web app=myapp:v3

# 两者不冲突，因为管理不同字段
```

### 场景3: 幂等性保证

```bash
# 重复执行 apply 不会产生副作用
kubectl apply -f deployment.yaml  # 第1次: 创建
kubectl apply -f deployment.yaml  # 第2次: 无变更 (no-op)
kubectl apply -f deployment.yaml  # 第3次: 无变更 (no-op)
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| apply 等于 create + update | apply 是三方合并 (last-applied + live + desired) |
| 声明式就是 YAML | 核心是“期望状态”语义，不是文件格式 |
| Status 可以随意修改 | Status 由控制器管理，用户修改会被覆盖 |
| resourceVersion 是版本号 | 它是 etcd revision，用于乐观锁 |
| SSA 可以完全替代 client-side apply | SSA 需要所有控制器都使用 SSA 才最佳 |

## 面试要点

1. **声明式 vs 命令式的核心区别是什么？**
   - 声明式描述“要什么”，系统负责“怎么做”
   - 幂等性保证，可重复执行
   - 支持 GitOps 和版本控制

2. **kubectl apply 的三方合并是如何工作的？**
   - last-applied-configuration (annotation) + 当前集群状态 + 新期望状态
   - 新增字段: 添加；删除字段: 移除；修改字段: 更新

3. **Server-Side Apply 解决了什么问题？**
   - 多控制器管理同一对象的不同字段
   - 字段所有权跟踪，冲突检测
   - 消除 client-side apply 的 annotation 大小限制

4. **resourceVersion 的作用？**
   - 乐观并发控制，防止写写冲突
   - Watch 机制的起始点
   - 每次写入 etcd 时递增

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[22-概念/01-核心架构/eventual-consistency.md|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[22-概念/01-核心架构/controller-pattern.md|Controller Pattern]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]]
- [[22-概念/01-核心架构/eventual-consistency.md|Eventual Consistency]]

- 声明式 API 与面向终态设计

<!-- risk-assessed -->
