---
title: 06 - 资源版本与并发控制 (Concurrency Control)
description: '## 专家解析：410 Gone 的终极治理'
summary: '在生产环境中，频繁出现 `410 Gone (Too old resource version)` 错误通常意味着你的 Watch 客户端跟不上 [[etcd|etcd]] 的压缩 (Compaction) 速度。'
category: design-principles
tags:
- k8s
- design
- principles
- etcd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 资源版本与并发控制 (Concurrency Control) 是什么
- 如何 资源版本与并发控制 (Concurrency Control)
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- 资源版本与并发控制
- Concurrency
- Control
- design
- principles
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 06 - 资源版本与并发控制 (Concurrency Control)

<!-- chunk: 专家解析：410 Gone 的终极治理 -->
## 专家解析：410 Gone 的终极治理

在生产环境中，频繁出现 `410 Gone (Too old resource version)` 错误通常意味着你的 Watch 客户端跟不上 [[etcd|etcd]] 的压缩 (Compaction) 速度。

### 根本原因分析
1. **etcd 压缩**: etcd 定期清理旧版本的 MVCC 数据。
2. **处理延迟**: 客户端处理事件太慢，导致其持有的 `ResourceVersion` 已经超出了 etcd 的保留窗口。

### 治理方案
* **优化 Handler 吞吐**: 使用工作队列并发处理。
* **合理设置 Resync**: 避免过短的 Resync 导致无意义的全量重新计算。
* **利用 Bookmarks**: 确保即使在无事件发生时，客户端的 RV 也能保持最新。

<!-- chunk: 核心概念 -->
## 核心概念

| 概念 | 英文 | 说明 |
|-----|-----|------|
| ResourceVersion | 资源版本 | 每个对象的版本号,每次修改递增 |
| Optimistic Concurrency | 乐观并发 | 假设冲突少,更新时检测冲突 |
| Pessimistic Concurrency | 悲观并发 | 假设冲突多,操作前加锁 |
| Conflict | 冲突 | 并发修改同一对象导致 |
| 409 Conflict | HTTP状态码 | 表示版本冲突 |

<!-- chunk: ResourceVersion来源 -->
## ResourceVersion来源

| 来源 | 说明 |
|-----|------|
| etcd revision | 全局递增的修订号 |
| 作用域 | 整个etcd集群全局唯一 |
| 递增时机 | 每次etcd事务提交 |
| 格式 | 字符串形式的数字 |

<!-- chunk: 乐观锁工作流程 -->
## 乐观锁工作流程

| 步骤 | 操作 | 说明 |
|-----|------|------|
| 1 | GET资源 | 获取当前resourceVersion |
| 2 | 修改对象 | 在内存中修改 |
| 3 | PUT/PATCH | 发送更新请求 |
| 4a | 成功 | RV匹配,更新成功,返回新RV |
| 4b | 409冲突 | RV不匹配,需要重试 |
| 5 | 重试 | 重新GET后再试 |

### 乐观锁示意图

```
Client A                    API Server                    Client B
   │                            │                            │
   │ GET pod (rv=100)           │                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │                            │         GET pod (rv=100)   │
   │                            │───────────────────────────►│
   │                            │                            │
   │                            │    PUT pod (rv=100) ─────►│
   │                            │    成功, 新 rv=101         │
   │                            │◄───────────────────────────│
   │                            │                            │
   │ PUT pod (rv=100) ─────────►│                            │
   │ 失败! 409 Conflict         │                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │ GET pod (rv=101) ─────────►│                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │ PUT pod (rv=101) ─────────►│                            │
   │ 成功, 新 rv=102            │                            │
   │◄───────────────────────────│                            │
```

<!-- chunk: ResourceVersion使用场景 -->
## ResourceVersion使用场景

| 场景 | RV值 | 含义 |
|-----|------|------|
| 更新操作 | 具体值 | 必须匹配当前RV |
| List | 空 | 获取最新数据 |
| List | "0" | 可从缓存读取 |
| Watch | 具体值 | 从该版本开始监听 |
| Watch | "0" | 从任意版本开始 |

<!-- chunk: 冲突处理策略 -->
## 冲突处理策略

| 策略 | 说明 | 适用场景 |
|-----|------|---------|
| 重试 | 获取最新版本后重试 | 大多数场景 |
| 合并 | 三方合并变更 | 复杂更新 |
| 覆盖 | 强制更新(危险) | 紧急修复 |
| 放弃 | 返回错误给用户 | 用户操作 |

### 重试模式代码

```go
func updateWithRetry(client kubernetes.Interface, pod *v1.Pod) error {
    return retry.RetryOnConflict(retry.DefaultRetry, func() error {
        // 1. 获取最新版本
        current, err := client.CoreV1().Pods(pod.Namespace).Get(
            context.TODO(), pod.Name, metav1.GetOptions{})
        if err != nil {
            return err
        }
        
        // 2. 应用修改
        current.Spec = pod.Spec
        
        // 3. 尝试更新
        _, err = client.CoreV1().Pods(pod.Namespace).Update(
            context.TODO(), current, metav1.UpdateOptions{})
        return err
    })
}
```

<!-- chunk: Generation vs ResourceVersion -->
## Generation vs ResourceVersion

| 维度 | Generation | ResourceVersion |
|-----|-----------|-----------------|
| 作用域 | 单个对象 | 全局 |
| 递增时机 | spec变更时 | 任何变更时 |
| 用途 | 判断spec是否变化 | 乐观锁/Watch |
| 控制器使用 | 判断是否需要调谐 | 冲突检测 |

### Generation使用示例

```go
// 控制器中判断是否需要处理
func needsReconcile(deploy *appsv1.Deployment) bool {
    // 如果observedGeneration < generation,说明有新的spec变更需要处理
    return deploy.Status.ObservedGeneration < deploy.Generation
}
```

<!-- chunk: SSA (Server-Side Apply) -->
## SSA (Server-Side Apply)

| 特性 | 说明 |
|-----|------|
| 字段管理器 | 每个字段记录管理者 |
| 冲突检测 | 不同管理者修改同一字段时冲突 |
| force参数 | 强制获取字段所有权 |
| 部分更新 | 只需提供要修改的字段 |

### SSA请求示例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply --server-side --field-manager=my-controller -f pod.yaml
```
### SSA API调用

```go
patchOptions := metav1.PatchOptions{
    FieldManager: "my-controller",
    Force:        pointer.Bool(true), // 强制接管
}
client.CoreV1().Pods(ns).Patch(
    ctx, name, types.ApplyPatchType, 
    patchData, patchOptions)
```

<!-- chunk: Managed Fields -->
## Managed Fields

```yaml
# kubectl get pod nginx -o yaml 中的managedFields
metadata:
  managedFields:
  - manager: kubectl-client-side-apply
    operation: Update
    apiVersion: v1
    time: "2024-01-15T10:00:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:labels:
          f:app: {}
      f:spec:
        f:containers:
          k:{"name":"nginx"}:
            .: {}
            f:image: {}
```

<!-- chunk: 最佳实践 -->
## 最佳实践

| 实践 | 说明 |
|-----|------|
| 使用retry库 | RetryOnConflict处理409 |
| 不要忽略RV | 更新时必须带RV |
| 使用SSA | 多控制器场景 |
| 检查Generation | 避免无意义的调谐 |
| 合理重试 | 设置重试上限 |

<!-- chunk: 常见问题 -->
## 常见问题

| 问题 | 原因 | 解决 |
|-----|------|------|
| 409 Conflict | 并发修改 | 重试 |
| 410 Gone | RV过旧(Watch) | 重新List |
| 频繁冲突 | 高并发更新 | 减少更新频率/分散处理 |
| 死循环更新 | 控制器互相触发 | 检查Generation |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 KUDIG Database — Global MOC
- [[集群基础/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- index.md|Domain-2 设计原则 — 开源项目索引]]
- Kubernetes 设计原则与哲学
- 声明式 API 与面向终态设计
- 控制器模式与调谐循环
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 08 - 高可用架构模式 (HA Patterns)
- 09 - Kubernetes 源码结构与阅读指南 (Source Code)
- 10 - CAP 定理与分布式系统基础 (CAP Theorem)

## See Also

- 04-watch-list-mechanism
- 05-informer-workqueue
- 07-distributed-consensus-etcd
- 08-high-availability-patterns

## Related

- [[生态参考/领域索引/etcd-index.md|[[etcd 知识图谱索引|etcd 知识图谱索引]]]]


<!-- risk-assessed -->
