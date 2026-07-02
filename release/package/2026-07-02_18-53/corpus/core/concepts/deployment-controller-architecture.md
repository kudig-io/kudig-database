---
title: Deployment 控制器架构
description: '# Deployment 控制器架构'
summary: 'Deployment 控制器是 Kubernetes 中管理无状态工作负载的核心组件。它通过 [[ReplicaSet|ReplicaSet]] 间接管理 Pod，实现声明式更新、滚动发布、版本回滚等能力。控制器采用典型的 Kubernetes 控制器模式：Informer + WorkQueue + Reconcile Loop。'
category: concepts
tags:
- k8s
- deployment
- controller
- replicaset
- informer
- workqueue
- reconciliation
- etcd
- kubelet
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployment 控制器架构 是什么
- 如何 Deployment 控制器架构
trigger_keywords:
- Deployment
- 控制器架构
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Deployment 控制器架构

## 概述

Deployment 控制器是 Kubernetes 中管理无状态工作负载的核心组件。它通过 [[ReplicaSet|ReplicaSet]] 间接管理 Pod，实现声明式更新、滚动发布、版本回滚等能力。控制器采用典型的 Kubernetes 控制器模式：Informer + WorkQueue + Reconcile Loop。

## 控制器分层架构

```
用户创建 Deployment
    ↓
Deployment 控制器 → 创建/管理 ReplicaSet
    ↓
ReplicaSet 控制器 → 创建/删除 Pod
    ↓
kubelet → 调度并运行容器
```

Deployment 不直接管理 Pod，而是通过 ReplicaSet 中间层。这种设计实现：
1. **版本管理**：每次更新创建新 ReplicaSet，旧版本保留支持回滚
2. **滚动发布控制**：同时管理新旧多个 ReplicaSet 的副本数
3. **关注点分离**：RS 控制器专注 Pod 副本管理，Deployment 控制器专注发布策略

## 控制器初始化

```
NewDeploymentController:
  ├── 注册 Deployment Informer（监听 Deployment 变更）
  ├── 注册 ReplicaSet Informer（监听 RS 变更）
  ├── 注册 Pod Informer（监听 Pod 变更）
  ├── 创建 WorkQueue（限速队列）
  └── 启动事件处理器（add/update/delete）
```

## syncDeployment 主协调函数

```
syncDeployment:
  1. 从 Lister 获取 Deployment 对象
  2. 获取关联的所有 ReplicaSet 和 Pod
  3. 判断 Deployment.Spec.Paused?
     ├── 是 → 仅同步状态，不执行替换
     └── 否 → 继续
  4. 判断是否回滚？
     ├── 是 → 执行 rollback
     └── 否 → 继续
  5. 判断是否扩缩容事件？
     ├── 是 → 同步所有 RS 副本数
     └── 否 → 继续
  6. 根据 Strategy.Type 选择执行路径:
     ├── RollingUpdate → rolloutRolling
     └── Recreate → rolloutRecreate
```

## 关键数据结构

```yaml
# DeploymentSpec
spec:
  replicas: 1                    # 期望副本数
  selector:
    matchLabels:
      app: nginx                 # Pod 选择器（必填）
  template:                      # Pod 模板（必填）
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
  strategy:
    type: RollingUpdate          # 默认
    rollingUpdate:
      maxSurge: 25%
      maxUnavailable: 25%
  minReadySeconds: 0             # Pod 就绪后最少等待秒数
  revisionHistoryLimit: 10       # 保留历史 ReplicaSet 数量
  progressDeadlineSeconds: 600   # 进度超时时间（秒）
  paused: false                  # 是否暂停发布
```

## 执行流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
1. 用户执行 kubectl apply -f deployment.yaml
2. API Server 接收请求，验证并写入 etcd
3. Deployment Informer 通过 Watch 机制捕获新增事件
4. addDeployment 将 namespace/name key 加入 WorkQueue
5. Worker goroutine 取出 key，调用 syncDeployment
6. syncDeployment 获取 Deployment 及关联 RS/Pod
7. 根据 Strategy.Type 选择执行路径
8. 创建/更新 ReplicaSet 对象
9. ReplicaSet Informer 捕获 RS 变更，触发 RS Controller
10. RS Controller 创建/删除 Pod
11. Deployment Controller 更新 Deployment Status
```
## ReplicaSet 版本管理

每次 Deployment 更新 Pod 模板时：
1. 计算 PodTemplate 的 hash 值
2. 创建新的 ReplicaSet（名称包含 hash）
3. 旧 ReplicaSet 保留（按 `revisionHistoryLimit` 限制数量）
4. 滚动更新：逐步扩容新 RS，缩容旧 RS

## 相关概念

- [[skills/deployment-rolling-update.md|[[Deployment 滚动更新策略|Deployment 滚动更新策略]]]]
- [[skills/deployment-canary-and-bluegreen.md|[[金丝雀与蓝绿发布|金丝雀与蓝绿发布]]]]
- [[deployment|Deployment]]
- [[concepts/controller-pattern.md|控制器模式]]
- [[concepts/watch-mechanism.md|Watch 机制]]

## Related

- [[concepts/bp-security.md|bp-security]]

- MOC]]

- README.md|bp-README]]

- [[concepts/ai-agent-openclaw-workspace.md|ai-agent-openclaw-workspace]]

- [[bp-MOC]]

- [[concepts/bp-operations.md|bp-operations]]

- [[concepts/bp-observability.md|bp-observability]]

- [[concepts/bp-infrastructure.md|bp-infrastructure]]

- [[concepts/bp-common-best-practices.md|bp-common-best-practices]]

- [[operator-pattern]] — Operator Pattern (CRD + Controller)
- [[entities/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[deployment]] — Deployment

- [[domain-07-platform-engineering/topic-code-analysis/deployment-create/README.md|Deployment Create — Kubernetes Deployment 控制器源码分析]]

<!-- risk-assessed -->
