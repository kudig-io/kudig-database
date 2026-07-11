---
title: Workload API
summary: Workload API 是 Kubernetes 中用于管理工作负载的核心 API，包括 Deployment、StatefulSet、DaemonSet、Job
  等。
category: concepts
tags:
- api
- workload
- deployment
- statefulset
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# Workload API

## 概述

Workload API 是 Kubernetes 中用于管理工作负载的核心 API 集合，涵盖 Deployment、StatefulSet、DaemonSet、Job、CronJob、ReplicaSet 等控制器类型。每种工作负载类型针对不同的应用模式设计，理解它们的内部机制和适用场景是正确设计云原生应用架构的基础。

## 各工作负载类型深度解析

### Deployment — 无状态应用

Deployment 是最常用的工作负载类型，通过 ReplicaSet 管理一组无状态 Pod 副本。

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%
      maxUnavailable: 25%
  selector:
    matchLabels:
      app: web-frontend
  template:
    spec:
      containers:
        - name: web
          image: nginx:1.25
          resources:
            requests: { cpu: 100m, memory: 128Mi }
            limits:   { cpu: 500m, memory: 512Mi }
          readinessProbe:
            httpGet: { path: /health, port: 8080 }
```

**关键机制**：Deployment 不直接管理 Pod，而是创建 ReplicaSet，每次模板更新会生成新的 RS。旧 RS 保留（replicas=0）用于回滚。

### StatefulSet — 有状态应用

StatefulSet 为 Pod 提供有序的部署、扩缩容和稳定的网络标识。

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: database
spec:
  serviceName: database-headless       # 必须关联 Headless Service
  replicas: 3
  podManagementPolicy: OrderedReady     # 或 Parallel
  template:
    spec:
      containers:
        - name: postgres
          image: postgres:16
          volumeMounts:
            - name: data
              mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:                 # 每个 Pod 自动创建独立 PVC
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 100Gi
```

**关键机制**：Pod 按 `database-0, database-1, database-2` 顺序创建，每个 Pod 拥有稳定 DNS 名称和持久存储。

### DaemonSet — 每节点一个

DaemonSet 确保每个（或选定的）节点运行一个 Pod 副本，典型用于日志采集、监控代理、网络插件。

```yaml
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 10%               # 控制滚动更新并发
  template:
    spec:
      tolerations:                      # 允许调度到控制平面
        - key: node-role.kubernetes.io/control-plane
          operator: Exists
      containers:
        - name: fluent-bit
          image: fluent/fluent-bit:3.0
```

### Job / CronJob — 批处理

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-migration
spec:
  completions: 1                        # 需要成功完成 1 次
  backoffLimit: 3                       # 最多重试 3 次
  activeDeadlineSeconds: 3600           # 最长运行 1 小时
  template:
    spec:
      restartPolicy: OnFailure          # 或 Never
      containers:
        - name: migration
          image: migration-tool:2.0
```

## 控制器内部机制

所有工作负载控制器都遵循**调谐循环**（Reconciliation Loop）模式：

```
for {
    desired := getDesiredState(workloadSpec)    // 从 etcd 读取期望状态
    actual := getActualState(pods, replicas)    // 从集群收集实际状态
    if desired != actual {
        act(diff(desired, actual))              // 创建/删除 Pod 使实际趋近期望
    }
    sleep(reconcileInterval)
}
```

以 Deployment 为例，其调谐路径为：Deployment Controller → 创建/更新 ReplicaSet → ReplicaSet Controller → 创建/删除 Pod。

## 最佳实践

- **选择正确的工作负载类型**：有状态用 StatefulSet，日志代理用 DaemonSet，批处理用 Job——选错类型会带来运维复杂性
- **配置合理的 PDB**：为 Deployment/StatefulSet 配置 PodDisruptionBudget，确保滚动更新和节点维护期间的最小可用副本数
- **使用 readinessProbe**：没有就绪探针的 Deployment 在滚动更新时可能将流量路由到未就绪的 Pod
- **StatefulSet 使用 Parallel 管理策略**：对不需要严格顺序的有状态应用（如 Redis Cluster），`podManagementPolicy: Parallel` 可显著加快部署速度
- **Job 设置 activeDeadlineSeconds**：防止失控的批处理任务无限运行消耗资源

## 常见陷阱

- **Deployment 不支持持久存储**：Deployment 模板中的 PVC 会被所有副本共享，需要持久存储应使用 StatefulSet + volumeClaimTemplates
- **StatefulSet 滚动更新卡住**：如果某个 Pod 不健康（readinessProbe 失败），OrderedReady 策略下后续 Pod 不会更新
- **CronJob 时区问题**：CronJob 默认使用控制平面节点时区，多区域集群需注意时区一致性

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/blue-green-deployment.md|蓝绿部署]] — 高级部署策略
- [[概念/canary-deployment.md|金丝雀发布]] — 渐进式发布

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
