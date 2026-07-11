---
title: StatefulSet
summary: StatefulSet 是 Kubernetes 中用于管理有状态应用的工作负载 API 对象。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: core
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# StatefulSet

## 概述

StatefulSet 是 Kubernetes 中用于管理**有状态应用**的工作负载控制器。与 Deployment 管理的无状态副本不同，StatefulSet 为每个 Pod 提供**稳定且可预测的身份**——固定的名称（`<sts-name>-0/1/2`）、稳定的 DNS（通过 Headless Service）以及按序绑定到该 Pod 的持久化存储。这让数据库、消息队列、分布式存储等需要稳定标识和持久卷的应用得以在 Kubernetes 上原生运行。

## 架构与工作原理

```
StatefulSet (apps/v1)
   │ serviceName: db (Headless Service)
   │ volumeClaimTemplates: data-db-{0,1,2}
   ▼
Pod: db-0   DNS: db-0.db.ns.svc.cluster.local   PVC: data-db-0
Pod: db-1   DNS: db-1.db.ns.svc.cluster.local   PVC: data-db-1
Pod: db-2   DNS: db-2.db.ns.svc.cluster.local   PVC: data-db-2
```

**与 Deployment 的核心差异**：

| 维度 | Deployment | StatefulSet |
|------|------------|-------------|
| Pod 名称 | 随机 hash（webapp-7b9c-xxx） | 有序稳定（db-0, db-1, db-2） |
| DNS | Service ClusterIP 负载均衡 | Headless 给每个 Pod 独立 A 记录 |
| 存储 | 共享或重建即丢 | 每副本独立 PVC，重建自动重绑 |
| 启停顺序 | 并行，无序 | 严格顺序：0→1→2 启动，逆序停止 |
| 更新方式 | 滚动（新旧 RS） | OrderedReady / Parallel，按序 |
| 身份 | 无 | PodName + Ordinal + 持久 DNS |

**工作流**：
1. 必须先创建一个 **Headless Service**（`clusterIP: None`），StatefulSet 通过 `serviceName` 引用它。
2. 控制器按序号 0→N 创建 Pod，前一个 Ready 后才创建下一个（`podManagementPolicy: OrderedReady`，默认）。
3. 每个 Pod 通过 `volumeClaimTemplates` 自动创建独立 PVC，即使 Pod 删除重建，同名 Pod 仍绑定原 PVC。
4. 滚动更新按**逆序**（N→0）逐个更新，默认 `OnDelete` 需手动触发，推荐 `RollingUpdate` + `partition` 做金丝雀。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `serviceName` | 必填，关联 Headless Service，提供每副本 DNS |
| `replicas` | 期望副本数 |
| `podManagementPolicy` | OrderedReady（默认）/ Parallel（并行，加速大规模） |
| `updateStrategy` | RollingUpdate / OnDelete |
| `rollingUpdate.partition` | 只更新序号 ≥ partition 的副本，做金丝雀 |
| `rollingUpdate.maxUnavailable` | 1.27+ 支持并行更新数量 |
| `volumeClaimTemplates` | 每副本动态 PVC 模板 |
| `persistentVolumeClaimRetentionPolicy` | 删除 StatefulSet 时 PVC 保留/删除策略（1.27 GA） |

## 配置示例

```yaml
---
apiVersion: v1
kind: Service
metadata:
  name: postgres
  namespace: production
spec:
  clusterIP: None             # Headless
  selector:
    app: postgres
  ports:
  - port: 5432
    name: pg
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: production
spec:
  serviceName: postgres
  replicas: 3
  podManagementPolicy: OrderedReady
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0            # 金丝雀时改大，先只更新副本 N
  selector:
    matchLabels: {app: postgres}
  template:
    metadata:
      labels: {app: postgres}
    spec:
      terminationGracePeriodSeconds: 60
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels: {app: postgres}
            topologyKey: kubernetes.io/hostname
      containers:
      - name: postgres
        image: postgres:16
        ports: [{containerPort: 5432}]
        env:
        - name: POD_NAME
          valueFrom: {fieldRef: {fieldPath: metadata.name}}
        - name: POD_ORDINAL
          valueFrom: {fieldRef: {fieldPath: metadata.name}}
        envFrom:
        - secretRef: {name: pg-creds}
        volumeMounts:
        - {name: data, mountPath: /var/lib/postgresql/data}
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: fast-ssd
      resources:
        requests: {storage: 100Gi}
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain        # 删除 STS 时保留数据
    whenScaled: Delete         # 缩容时删除多余 PVC
```

## 常用操作与命令

```bash
# 查看（注意 Pod 名称有序）
kubectl get sts,pods,pvc -n production -l app=postgres

# 临时连到主节点
kubectl exec -it postgres-0 -- psql -U postgres

# 金丝雀：先只更新副本 2（partition=2），观察后再逐步推进
kubectl patch sts postgres -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'
# 推进到副本 1
kubectl patch sts postgres -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'

# 滚动状态
kubectl rollout status sts/postgres

# 临时扩容（会触发新 PVC 创建 + 新 Pod 绑定）
kubectl scale sts postgres --replicas=5

# 删除 STS 但保留 PVC（数据保留）
kubectl delete sts postgres --cascade=orphan
```

## 最佳实践

1. **必配 PodAntiAffinity**：把副本打散到不同节点/可用区，避免单节点故障丢多个副本。
2. **`terminationGracePeriodSeconds` 充足**：让数据库优雅刷盘再退出，避免数据损坏。
3. **PVC 保留策略**：生产环境 `whenDeleted: Retain`，防止误删 STS 导致数据丢失。
4. **partition 金丝雀**：先用 partition 更新最高序号副本验证，再分阶段降到 0 全量更新。
5. **podManagementPolicy: Parallel**：大批量副本（如 Elasticsearch 30 节点）可并行启停显著加速。
6. **Headless Service 必备**：没有 serviceName 的 StatefulSet 无法生成 Pod DNS，集群发现机制失效。

## 常见陷阱

- **PVC 残留**：缩容后 PVC 默认保留，再扩容会复用旧数据；若期望全新副本需手动删 PVC。
- **滚动卡住**：前一个 Pod readinessProbe 未通过，后面全部阻塞，检查 `kubectl get pods -w`。
- **节点故障 Pod 卡 Terminating**：safe 阻止 force delete；可手动 delete PVC 绑定或等待节点恢复。
- **并行扩容数据竞争**：Parallel 模式下副本同时加入集群，部分应用（如 etcd）首次 bootstrap 仍需 OrderedReady。
- **更新慢**：默认 OrderedReady 逐个串行，30 副本更新非常慢，用 partition + Parallel 优化。
- **PVC 存储类不支持动态扩容**：`allowVolumeExpansion: false` 的 StorageClass 无法 resize PVC。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]] — 无状态对照
- [[概念/daemonset.md|DaemonSet]]
- [[概念/pv.md|PersistentVolume]]
- [[概念/storageclass.md|StorageClass]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
