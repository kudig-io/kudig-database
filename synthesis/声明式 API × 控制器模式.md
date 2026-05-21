---
title: 声明式 API × 控制器模式
description: 这个合成的关键在于理解：**最终一致性不是架构缺陷，而是声明式 API + 控制器模式的必然数学结果**。
category: synthesis
tags:
- k8s
- declarative
- controller
- reconciliation
- design-pattern
- eventual-consistency
- etcd
- hpa
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 声明式 API × 控制器模式 是什么
- 如何 声明式 API × 控制器模式
trigger_keywords:
- 声明式
- API
- 控制器模式
prerequisites:
- kubectl-basics
- etcd-basics
---

# 声明式 API × 控制器模式

## The Connection

声明式 API 和控制器模式是 Kubernetes 的一体两面——前者是**声明**，后者是**执行**。没有控制器，声明式 API 只是一堆静态的 YAML 文件；没有声明式 API，控制器就没有明确的目标状态来驱动。两者的结合产生了 K8s 最本质的特性：**系统自动趋向期望状态**。

这个合成的关键在于理解：**最终一致性不是架构缺陷，而是声明式 API + 控制器模式的必然数学结果**。

## Where They Co-occur

- **Deployment 滚动更新**：用户声明 `spec.replicas: 5`，Deployment Controller 通过协调循环逐步创建/删除 ReplicaSet，最终达到期望状态。
- **HPA 自动伸缩**：HPA Controller 持续读取 metrics-server 的指标，当 CPU 使用率超过阈值时，修改 Deployment 的 `spec.replicas`——触发 Deployment Controller 的新一轮协调。
- **自定义资源管理**：Operator 通过 CRD 扩展声明式 API 的范围，然后通过自定义控制器实现协调逻辑——将领域知识编码为自动化行为。
- **节点故障恢复**：当节点失联时，用户不需要手动迁移 Pod——只需声明期望状态，系统通过协调循环自动在新的健康节点上重新创建 Pod。

## Cross-cutting Insight

**声明式 API + 控制器 + List-Watch = 一个"永不睡觉的系统管理员"。**

这个组合的力量来自三个关键设计点的相互作用：

### 1. 状态声明的幂等性

`kubectl apply` 无论执行多少次，结果都是一样的——因为 YAML 声明的是"最终应该是什么状态"，而不是"要做什么操作"。这使得：

- GitOps 工作流成为可能——Git 仓库中的 YAML 就是系统的期望状态
- 控制器可以安全地重试——失败后重新执行不会产生副作用
- 并发操作是安全的——多个 Controller 同时修改同一资源的 spec 不会冲突（通过 resourceVersion 乐观锁保证）

### 2. List-Watch 桥接了声明与执行

Informer 模式是控制器模式的核心实现：

```
API Server (etcd)
       │
       ├─ List:  首次全量同步，建立本地缓存
       │
       └─ Watch: 持续监听变更事件，触发 Reconcile
```

关键洞察：**Watch 机制让控制器从"轮询"升级为"事件驱动"**。控制器不需要定期扫描所有资源，而是在资源发生变化时立即响应。这不仅减少了 API Server 的负载，还缩短了系统趋向期望状态的收敛时间。

### 3. 最终一致性的数学保证

控制器模式的协调循环遵循一个核心不变式：

```
while (Spec != Status) {
    Act();  // 执行修正
    UpdateStatus();
}
```

只要每次 Act() 操作都使 Status 更接近 Spec（单调收敛），系统最终会达到期望状态。这就是**最终一致性**的数学基础——不是"可能一致"，而是"必然收敛"。

## Tensions and Trade-offs

### 声明式 vs 紧急操作

- **声明式**：适合长期期望状态的管理（Deployment、ConfigMap、Service）
- **紧急场景**：当 Pod 卡在 Terminating 状态时，`kubectl delete --force --grace-period=0` 是命令式的——这种紧急操作绕过了正常协调流程
- **矛盾**：声明式系统在处理需要立即干预的紧急情况时显得笨拙

### 协调延迟 vs 资源消耗

- **快速协调**：缩短 ReSync 周期（如从 10 分钟到 1 分钟）可以加快收敛速度，但增加 API Server 和控制器 CPU 负载
- **慢速协调**：默认 ReSync 周期节省资源，但意味着系统在变更后需要更长时间才能收敛
- **解决方案**：依赖 Watch 事件驱动的即时响应，ReSync 仅作为兜底机制

### 控制器冲突

当多个控制器同时管理同一资源时会发生冲突：

- **场景**：两个 Operator 同时为同一 CRD 实例编写状态
- **后果**：状态振荡（Oscillation），系统永远无法收敛
- **预防**：使用 OwnerReference 确保每个资源只有一个主要 Controller；通过 Leader Election 避免多副本控制器冲突

## Open Questions

- **控制器协调的因果一致性**：当多个控制器存在依赖关系时（如必须先创建 PV 再创建 Pod），如何保证协调顺序？目前依赖 OwnerReference 的隐式排序，缺乏显式的依赖图机制。
- **大规模集群中的 Watch 风暴**：当 API Server 重启后，所有控制器同时执行 List 操作形成突发负载——这在 10,000+ 节点集群中的影响尚未充分研究。
- **声明式 API 的调试困难**：当系统状态偏离期望状态时，缺乏标准工具来追踪"为什么控制器还没有收敛"——是 Watch 丢失了事件，还是控制器的 Act() 逻辑有 bug？

## Related

- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/declarative-api.md|declarative-api]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/eventual-consistency.md|eventual-consistency]]
- [[concepts/watch-mechanism.md|watch-mechanism]]
- [[operator-pattern]]
- [[synthesis/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]]
