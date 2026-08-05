---
title: Workload Management
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- prometheus
- mysql
- postgresql
- kafka
- hpa
- pdb
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Workload Management 是什么
- 如何 Workload Management
trigger_keywords:
- Workload
- Management
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
- etcd-basics
- kafka-basics
- mysql-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Workload Management

## 概述
[[Kubernetes|Kubernetes]] 提供多个内置 API 用于声明式地管理工作负载及其组件。虽然应用最终运行在 Pod 中，但直接管理单个 Pod 非常繁琐。工作负载对象提供了更高层次的抽象，控制平面会根据定义自动管理 Pod 的生命周期。

## 核心概念/原理
主要的工作负载管理 API 包括：
- **Deployment（及间接的 [[ReplicaSet|ReplicaSet]]）**：管理无状态应用的最常用方式。Pod 之间可互换，任意 Pod 失败均可被替换。
- **[[StatefulSet|StatefulSet]]**：用于管理需要稳定、唯一网络标识和持久存储的有状态应用。每个 Pod 具有持久标识，并与 PersistentVolume 关联。
- **[[DaemonSet|DaemonSet]]**：确保所有（或部分）节点上运行一个 Pod 副本。常用于节点级服务，如日志收集、监控代理、网络插件等。
- **Job / CronJob**：用于运行一次性或定时批处理任务。Job 运行到完成即停止，CronJob 按时间表重复创建 Job。
- **ReplicationController**：旧版 API，用于维护指定数量的 Pod 副本，现已被 Deployment 和 ReplicaSet 取代。

## 关键机制或特性
- **声明式管理**：用户描述期望状态，控制器负责将实际状态收敛到期望状态。
- **自动恢复**：当 Pod 因节点问题、维护或配置错误而终止时，控制器会自动创建替代 Pod。
- **滚动更新**：Deployment 和 StatefulSet 支持滚动更新策略，可在不中断服务的情况下更新应用。
- **扩缩容**：大多数工作负载支持手动或自动的水平扩缩容。

## 使用场景
- **Deployment**：Web 前端、API 服务、无状态微服务。
- **StatefulSet**：数据库（MySQL、PostgreSQL、MongoDB）、消息队列（Kafka、RabbitMQ）、分布式存储（ZooKeeper、etcd）。
- **DaemonSet**：节点监控（Prometheus Node Exporter）、日志收集（[[fluentd|Fluentd]]/Fluent Bit）、CNI 插件、存储驱动。
- **Job/CronJob**：数据备份、报表生成、定时清理任务、批处理计算。

## 最佳实践/注意事项
- 优先使用 Deployment 管理无状态应用，而不是直接使用 ReplicaSet 或 ReplicationController。
- StatefulSet 需要配合 Headless Service 使用，以提供稳定的网络标识。
- DaemonSet Pod 通常需要较高的优先级，以确保在节点上优先调度。
- 对于预期会自行终止的任务，使用 Job 而非 ReplicaSet/Deployment。

## 工作负载选型决策指南

```
需要管理应用？
├── 无状态应用
│   ├── 长期运行服务 → Deployment
│   └── 批处理/一次性任务
│       ├── 一次执行 → Job
│       └── 定时执行 → CronJob
├── 有状态应用
│   └── 需要稳定标识/存储 → StatefulSet
└── 节点级别服务
    └── 每个节点一个副本 → DaemonSet
```

## 实战 YAML 示例

### 工作负载选型对比矩阵

| 特性 | Deployment | StatefulSet | DaemonSet | Job | CronJob |
|------|-----------|-------------|-----------|-----|---------|
| Pod 标识 | 随机，可互换 | 有序，粘性标识 | 每节点一个 | 临时 | 临时 |
| 网络标识 | 通过 Service | Headless Service | 可用 hostPort | 无 | 无 |
| 持久存储 | 共享 PVC | 每 Pod 独立 PVC | 通常用 hostPath | 按需 | 按需 |
| 滚动更新 | 支持 | 支持（有序） | 支持 | 不适用 | 不适用 |
| 扩缩容 | 手动/HPA | 手动（有序） | 随节点自动 | 并行度控制 | 不适用 |
| 完成语义 | 无（持续运行） | 无（持续运行） | 无（持续运行） | 运行到完成 | 按计划触发 |

### 典型生产架构示例

```yaml
# 1. 无状态 API 服务 → Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
  namespace: prod
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-server
  template:
    metadata:
      labels:
        app: api-server
    spec:
      containers:
      - name: api
        image: myregistry.com/api:v2.0
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
---
# 2. 有状态数据库 → StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: prod
spec:
  serviceName: postgres-headless
  replicas: 3
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: postgres:16.3
        resources:
          requests:
            cpu: "1000m"
            memory: "2Gi"
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
---
# 3. 节点监控 → DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      tolerations:
      - operator: Exists
      containers:
      - name: exporter
        image: prom/node-exporter:v1.8.0
        resources:
          requests:
            cpu: "50m"
            memory: "64Mi"
```

## 故障排查

### 不确定应该使用哪种工作负载类型
- **判断标准**:
  ```bash
  # 查看现有工作负载分布
  kubectl get deployments,statefulsets,daemonsets,jobs,cronjobs -A --no-headers | awk '{print $1}' | sort | uniq -c | sort -rn
  ```
- **指导原则**: 如果应用的所有实例完全等价 → Deployment；如果每个实例需要独立标识或存储 → StatefulSet。

### 工作负载 Pod 不断被驱逐
- **诊断命令**:
  ```bash
  # 查看所有 Evicted Pod
  kubectl get pods -A --field-selector=status.phase=Failed | grep Evicted
  # 查看节点资源压力
  kubectl top nodes
  kubectl describe node <node-name> | grep -A 5 "Conditions"
  ```

## 生产就绪检查清单

- [ ] 每个工作负载选择了正确的控制器类型
- [ ] 所有 Deployment 配置了 PDB（Pod Disruption Budget）
- [ ] StatefulSet 配合 Headless Service 使用
- [ ] DaemonSet 设置了合适的 PriorityClass 和 tolerations
- [ ] Job/CronJob 配置了 `ttlSecondsAfterFinished` 自动清理
- [ ] 所有工作负载配置了 `resources.requests/limits`
- [ ] 关键工作负载配置了 `topologySpreadConstraints` 跨可用区分布

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有工作负载
kubectl get deployments,statefulsets,daemonsets,jobs,cronjobs -n prod

# 查看工作负载事件
kubectl describe deployment <name> -n prod | tail -20

# 批量查看所有工作负载的副本状态
kubectl get deployments -n prod -o custom-columns='NAME:.metadata.name,DESIRED:.spec.replicas,READY:.status.readyReplicas,AVAILABLE:.status.availableReplicas'
```
## 交叉引用

- [Deployments](./deployments.md)
- [StatefulSets](./statefulsets.md)
- [DaemonSet](./daemonset.md)
- [Jobs](./jobs.md)
- [CronJob](./cronjob.md)
- [Managing Workloads（操作指南）](./managing-workloads.md)
- [工作负载概览与架构](../../domain-02-workloads-applications/01-workload-overview-architecture.md)
- [工作负载故障排查手册](32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/03-workload-troubleshooting-handbook.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/

## Related

- [[domain-17-system-foundation/知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[domain-17-system-foundation/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[domain-17-system-foundation/知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
