---
title: 02 - 声明式 API 与面向终态设计 (Declarative API)
description: 'title: 声明式 API 与面向终态设计'
category: general
tags:
- k8s
- etcd
- apiserver
- opa
- hpa
- statefulset
- daemonset
- job
- cronjob
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 声明式 API 与面向终态设计 (Declarative API) 是什么
- 如何 声明式 API 与面向终态设计 (Declarative API)
- Kubernetes 01 cluster fundamentals 最佳实践
trigger_keywords:
- 声明式
- API
- 与面向终态设计
- Declarative
- API
- cluster
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
- policy-basics
---

---
title: 声明式 API 与面向终态设计
description: 深入解析 Kubernetes 声明式 API 的核心概念、Server-Side Apply (SSA)、字段管理与冲突处理的底层机制
category: domain-2-design
tags:
- k8s
- declarative
- api
- ssa
- server-side-apply
- field-management
- etcd
- opa
- hpa
- statefulset
- design-principles
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 声明式 API 与面向终态设计 是什么
- 如何 声明式 API 与面向终态设计
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- 声明式
- API
- 与面向终态设计
- design
- principles
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: 01-design-principles-foundations.md
  type: depth
  desc: 设计原则与哲学
- path: 03-controller-pattern.md
  type: depth
  desc: 控制器模式与调谐循环
- path: ../domain-01-cluster-fundamentals/12-apiserver-deep-dive.md
  type: depth
  desc: API Server 深度解析
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'

tier: peripheral---

# 02 - 声明式 API 与面向终态设计 (Declarative API)

<!-- chunk: 专家视点：为什么 SSA (Server-Side Apply) 是未来？ -->
## 专家视点：为什么 SSA (Server-Side Apply) 是未来？

在传统的 Client-Side Apply (`kubectl apply`) 中，客户端负责计算三方合并 (3-way merge)。这种方式在多管理器场景（如 HPA 修改副本数的同时，用户修改镜像）下经常导致冲突或字段丢失。

### Server-Side Apply (SSA) 的核心优势
1. **Managed Fields**: 显式记录每个字段的所有权 (Field Ownership)。
2. **解决冲突**: API Server 自动处理并发修改冲突，确保不同控制器能安全地修改同一资源。
3. **性能**: 减少了客户端的计算压力，降低了请求负载。

> **生产避坑**: 在编写现代 Operator 时，强烈建议优先使用 SSA 接口进行资源更新。

<!-- chunk: 声明式 API 核心概念 -->
## 声明式 API 核心概念

| 概念 | 英文 | 说明 | 示例 |
|-----|-----|------|-----|
| 期望状态 | Desired State | 用户声明的目标状态 | spec.replicas=3 |
| 实际状态 | Actual State | 系统当前的实际状态 | status.replicas=2 |
| 调谐 | Reconciliation | 使实际状态趋向期望状态 | 创建1个新Pod |
| 幂等性 | Idempotency | 多次操作结果相同 | apply多次无副作用 |
| 最终一致性 | Eventual Consistency | 状态最终会收敛 | 系统自动重试直到成功 |

<!-- chunk: API版本演进 -->
## API版本演进

| 版本级别 | 格式 | 稳定性 | 兼容性保证 | 典型用途 |
|---------|-----|-------|-----------|---------|
| Alpha | v1alpha1 | 不稳定 | 无保证，可能删除 | 实验功能 |
| Beta | v1beta1 | 较稳定 | 向后兼容 | 测试功能 |
| Stable | v1 | 稳定 | 长期支持 | 生产使用 |

<!-- chunk: API资源分类 -->
## API资源分类

| 类别 | 作用域 | 示例资源 | 说明 |
|-----|-------|---------|------|
| Cluster-scoped | 集群级 | Node, PV, ClusterRole | 不属于任何命名空间 |
| Namespace-scoped | 命名空间级 | Pod, Service, Deployment | 属于特定命名空间 |

<!-- chunk: API Group组织结构 -->
## API Group组织结构

| API Group | 包含资源 | 说明 |
|----------|---------|------|
| core (空) | Pod, Service, ConfigMap, Secret, PV, PVC | 核心资源 |
| apps | Deployment, StatefulSet, DaemonSet, ReplicaSet | 应用负载 |
| batch | Job, CronJob | 批处理 |
| networking.k8s.io | Ingress, NetworkPolicy | 网络 |
| storage.k8s.io | StorageClass, VolumeAttachment | 存储 |
| rbac.authorization.k8s.io | Role, ClusterRole, RoleBinding | 权限 |
| policy | PodDisruptionBudget | 策略 |
| autoscaling | HPA | 自动扩缩 |
| admissionregistration.k8s.io | ValidatingWebhookConfiguration | 准入控制 |

<!-- chunk: RESTful API路径规范 -->
## RESTful API路径规范

| 资源类型 | HTTP方法 | 路径 | 操作 |
|---------|---------|-----|------|
| 集群资源列表 | GET | /api/v1/nodes | 列出所有节点 |
| 集群资源详情 | GET | /api/v1/nodes/{name} | 获取节点详情 |
| 命名空间资源列表 | GET | /api/v1/namespaces/{ns}/pods | 列出ns下所有Pod |
| 命名空间资源详情 | GET | /api/v1/namespaces/{ns}/pods/{name} | 获取Pod详情 |
| 创建资源 | POST | /api/v1/namespaces/{ns}/pods | 创建Pod |
| 更新资源 | PUT | /api/v1/namespaces/{ns}/pods/{name} | 完整更新Pod |
| 部分更新 | PATCH | /api/v1/namespaces/{ns}/pods/{name} | 部分更新Pod |
| 删除资源 | DELETE | /api/v1/namespaces/{ns}/pods/{name} | 删除Pod |
| 更新状态 | PUT | /api/v1/namespaces/{ns}/pods/{name}/status | 更新Pod状态 |
| Watch资源 | GET | /api/v1/namespaces/{ns}/pods?watch=true | 监听变化 |

<!-- chunk: Spec vs Status设计模式 -->
## Spec vs Status设计模式

| 维度 | Spec | Status |
|-----|------|--------|
| 含义 | 期望状态 | 实际状态 |
| 写入者 | 用户 | 系统(控制器) |
| 读取者 | 控制器 | 用户、监控 |
| 存储位置 | etcd | etcd |
| 更新频率 | 用户操作时 | 控制器每次调谐 |
| 验证 | 严格验证 | 宽松验证 |

### Spec/Status示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:                          # 期望状态(用户定义)
  replicas: 3                  # 期望3个副本
  selector:
    matchLabels:
      app: nginx
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.21
status:                        # 实际状态(系统维护)
  replicas: 3                  # 当前总副本数
  readyReplicas: 3             # 就绪副本数
  updatedReplicas: 3           # 已更新副本数
  availableReplicas: 3         # 可用副本数
  observedGeneration: 5        # 控制器观察到的generation
  conditions:
  - type: Available
    status: "True"
    lastUpdateTime: "2024-01-15T10:00:00Z"
```

<!-- chunk: 乐观并发控制 (Optimistic Concurrency) -->
## 乐观并发控制 (Optimistic Concurrency)

| 概念 | 说明 |
|-----|------|
| resourceVersion | 每个资源的版本号，每次修改递增 |
| 冲突检测 | 更新时检查resourceVersion是否匹配 |
| 409 Conflict | 版本不匹配时返回冲突错误 |
| 重试策略 | 客户端需获取最新版本后重试 |

### 乐观锁工作流程

```
1. Client GET /api/v1/pods/nginx → resourceVersion: "1000"
2. Client修改spec
3. Client PUT /api/v1/pods/nginx (resourceVersion: "1000")
4. 
   情况A: 无冲突 → 成功，新resourceVersion: "1001"
   情况B: 有冲突(其他人已修改) → 409 Conflict
          → Client需重新GET最新版本后重试
```

<!-- chunk: API操作语义 -->
## API操作语义

| 操作 | 语义 | 幂等性 | 说明 |
|-----|------|-------|------|
| CREATE | 创建新资源 | 否 | 资源已存在则失败 |
| GET | 读取资源 | 是 | 不改变状态 |
| LIST | 列出资源 | 是 | 不改变状态 |
| WATCH | 监听变化 | 是 | 长连接推送事件 |
| UPDATE | 完整替换 | 是 | 需提供完整spec |
| PATCH | 部分更新 | 是 | 仅提供变更部分 |
| DELETE | 删除资源 | 是 | 已删除再删除无错误 |

<!-- chunk: Patch策略类型 -->
## Patch策略类型

| 类型 | Content-Type | 说明 | 适用场景 |
|-----|-------------|------|---------|
| Strategic Merge Patch | application/strategic-merge-patch+json | K8s特有，智能合并 | 大部分场景 |
| JSON Merge Patch | application/merge-patch+json | RFC 7386标准 | 简单覆盖 |
| JSON Patch | application/json-patch+json | RFC 6902标准 | 精确操作 |

### Strategic Merge Patch示例

```yaml
# 原始Deployment
spec:
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.20
      - name: sidecar
        image: sidecar:v1

# Patch内容(Strategic Merge)
spec:
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.21  # 只更新nginx镜像

# 结果: nginx更新，sidecar保留
```

<!-- chunk: Finalizers机制 -->
## Finalizers机制

| 概念 | 说明 |
|-----|------|
| Finalizer | 删除前必须执行的清理操作标记 |
| 删除流程 | 设置deletionTimestamp → 执行Finalizer → 移除Finalizer → 真正删除 |
| 用途 | 清理外部资源、级联删除、审计日志 |

### Finalizer示例

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  finalizers:
  - kubernetes  # 系统Finalizer，清理ns内所有资源
```

<!-- chunk: Owner References与级联删除 -->
## Owner References与级联删除

| 删除策略 | propagationPolicy | 行为 |
|---------|------------------|------|
| Orphan | Orphan | 删除Owner，保留子资源 |
| Background | Background | 删除Owner后异步删除子资源 |
| Foreground | Foreground | 先删除子资源，再删除Owner |

### OwnerReference示例

```yaml
# ReplicaSet创建的Pod自动包含OwnerReference
apiVersion: v1
kind: Pod
metadata:
  name: nginx-abc123
  ownerReferences:
  - apiVersion: apps/v1
    kind: ReplicaSet
    name: nginx-7b4f5d8c9
    uid: 12345678-1234-1234-1234-123456789012
    controller: true      # 标记为控制器
    blockOwnerDeletion: true
```

<!-- chunk: API请求流程 -->
## API请求流程

| 阶段 | 说明 |
|-----|------|
| 1. 认证(Authentication) | 验证请求者身份 |
| 2. 授权(Authorization) | 检查RBAC权限 |
| 3. 准入控制(Admission) | Mutating + Validating Webhook |
| 4. 验证(Validation) | 资源schema验证 |
| 5. 持久化(Persistence) | 写入etcd |
| 6. 通知(Notification) | 触发Watch事件 |

<!-- chunk: 最佳实践 -->
## 最佳实践

| 实践 | 说明 |
|-----|------|
| 使用kubectl apply | 声明式管理，支持三方合并 |
| 版本控制YAML | 配合GitOps工作流 |
| 使用标签而非名称 | 松耦合，支持选择器 |
| 设置resourceVersion | 避免并发冲突 |
| 使用Finalizers | 确保外部资源清理 |
| 遵循API版本 | 生产使用stable版本 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-01-cluster-fundamentals/MOC.md|domain-01-cluster-fundamentals MOC]]
- [[domain-01-cluster-fundamentals/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- [[domain-01-cluster-fundamentals/00-open-source-projects-index.md|Domain-2 设计原则 — 开源项目索引]]
- [[domain-01-cluster-fundamentals/01-design-principles-foundations.md|Kubernetes 设计原则与哲学]]
- [[domain-01-cluster-fundamentals/03-controller-pattern.md|控制器模式与调谐循环]]
- [[domain-01-cluster-fundamentals/04-watch-list-mechanism.md|04 - List-Watch 机制深度解析 (List-Watch)]]
- [[domain-01-cluster-fundamentals/05-informer-workqueue.md|05 - Informer 架构与工作队列 (Informer & Workqueue)]]
- [[domain-01-cluster-fundamentals/06-resource-version-control.md|06 - 资源版本与并发控制 (Concurrency Control)]]
- [[domain-01-cluster-fundamentals/07-distributed-consensus-etcd.md|07 - 分布式共识与 etcd 原理 (etcd & Raft)]]
- [[domain-01-cluster-fundamentals/08-high-availability-patterns.md|08 - 高可用架构模式 (HA Patterns)]]
- [[domain-01-cluster-fundamentals/09-source-code-walkthrough.md|09 - Kubernetes 源码结构与阅读指南 (Source Code)]]
- [[domain-01-cluster-fundamentals/10-cap-theorem-distributed-systems.md|10 - CAP 定理与分布式系统基础 (CAP Theorem)]]

## Related

- [[domain-01-cluster-fundamentals/01-design-principles-foundations.md|设计原则与哲学]]
- [[domain-01-cluster-fundamentals/03-controller-pattern.md|控制器模式与调谐循环]]
- [[domain-01-cluster-fundamentals/12-apiserver-deep-dive.md|API Server 深度解析]]
- [[domain-01-cluster-fundamentals/MOC.md|相关知识域: domain-01-cluster-fundamentals]]
- [[domain-01-cluster-fundamentals/MOC.md|相关知识域: domain-01-cluster-fundamentals]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]

## See Also

- [[domain-01-cluster-fundamentals/99-kubernetes-v1.33-design-principles-evolution.md|99-kubernetes-v1.33-design-principles-evolution]]
- [[domain-01-cluster-fundamentals/01-design-principles-foundations.md|01-design-principles-foundations]]
- [[domain-01-cluster-fundamentals/03-controller-pattern.md|03-controller-pattern]]
- [[domain-01-cluster-fundamentals/04-watch-list-mechanism.md|04-watch-list-mechanism]]
