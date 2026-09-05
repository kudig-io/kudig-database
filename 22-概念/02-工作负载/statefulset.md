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

## 源码实现分析

### StatefulSet Controller 有序管理

```go
// k8s.io/kubernetes/pkg/controller/stateful/stateful_set_control.go
// StatefulSet 核心控制逻辑
func (ssc *defaultStatefulSetControl) UpdateStatefulSet(ctx context.Context, set *apps.StatefulSet, pods []*v1.Pod) (*apps.StatefulSetStatus, error) {
    // 1. 按序号排序 Pod（pod-0, pod-1, pod-2...）
    replicas := getOrdinalReplicas(set)
    
    // 2. 扩容：按序号递增创建（0→1→2）
    for i := len(pods); i < replicas; i++ {
        ssc.createPod(ctx, set, i)
        // 等待 Pod Ready 后才创建下一个
        if !isRunningAndReady(pods[i]) { return } // 阻塞等待
    }
    
    // 3. 缩容：按序号递减删除（N→N-1→...）
    for i := len(pods) - 1; i >= replicas; i-- {
        ssc.deletePod(ctx, set, pods[i])
        // 等待 Pod 完全删除后才继续
    }
    
    // 4. 更新：RollingUpdate 从最大序号开始（N→N-1→...→0）
    if updateStrategy.Type == apps.RollingUpdateStatefulSetStrategyType {
        for i := len(pods) - 1; i >= partition; i-- {
            ssc.updatePod(ctx, set, pods[i])
            // 等待新 Pod Ready 后才更新下一个
        }
    }
}
```

```
┌─────────────────────────────────────────────────────────┐
│     StatefulSet 有序管理模型                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  扩容 (replicas: 2→4):                                 │
│    pod-0 ✓ → pod-1 ✓ → pod-2 (create) → pod-3 (create)│
│    严格顺序，前一个 Ready 才创建下一个              │
│                                                         │
│  缩容 (replicas: 4→2):                                 │
│    pod-3 (delete) → pod-2 (delete) → pod-1 ✓ → pod-0 ✓│
│    严格逆序，前一个删除完才删下一个              │
│                                                         │
│  更新 (RollingUpdate, partition=0):                    │
│    pod-3 (update) → pod-2 (update) → pod-1 → pod-0    │
│    从最大序号开始，保证主节点最后更新              │
│                                                         │
│  稳定标识: pod-0 永远是 pod-0，不会变              │
│  稳定存储: PVC 与 Pod 序号绑定，重建后重新挂载    │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：StatefulSet 故障诊断

```bash
# 🟢 检查 StatefulSet 状态
kubectl get statefulset -A
kubectl describe statefulset <name> -n <ns>

# 🟢 检查 Pod 序号和状态
kubectl get pods -n <ns> -l app=<app> -o wide

# 🟡 强制删除卡住的 Pod（PVC 保留）
kubectl delete pod <name>-0 -n <ns> --force --grace-period=0
# 🔴 强制删除有状态 Pod 可能导致数据不一致

# 🟢 检查 PVC 状态
kubectl get pvc -n <ns> -l app=<app>

# 🟡 使用 partition 控制更新范围（金丝雀更新）
kubectl patch statefulset <name> -n <ns> -p \
  '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'
# 只更新 pod-2 及以上，pod-0/1 保持旧版本
```

## 面试要点

1. **StatefulSet 与 Deployment 的核心区别？**
   - 稳定网络标识：pod-0 永远是 pod-0（Headless Service + hostname）
   - 稳定存储：PVC 与 Pod 序号绑定，重建后重新挂载同一 PVC
   - 有序操作：扩容顺序、缩容逆序、更新从大到小
   - Deployment 的 Pod 是无状态可互换的

2. **StatefulSet 的更新策略有哪些？**
   - RollingUpdate：从最大序号开始逐个更新（默认）
   - OnDelete：手动删除 Pod 才触发更新
   - partition：只更新序号 ≥ partition 的 Pod（金丝雀）
   - 生产建议：数据库用 partition 先更新从节点

3. **为什么 StatefulSet 需要 Headless Service？**
   - 提供稳定的 DNS：pod-0.svc.ns.svc.cluster.local
   - 每个 Pod 有独立 DNS 记录，而非 ClusterIP 负载均衡
   - 有状态应用需要直接访问特定 Pod（如 MySQL 主从）

4. **StatefulSet 缩容时 PVC 会怎样？**
   - 默认保留！缩容删除 Pod 但不删除 PVC
   - 数据保留，扩容时重新挂载
   - 需要手动清理或设置 persistentVolumeClaimRetentionPolicy
   - 生产注意：缩容后 PVC 仍占用存储费用

## 相关概念

- [[22-概念/01-核心架构/kubernetes.md|Kubernetes]]
- [[22-概念/02-工作负载/pods.md|Pod]]
- [[22-概念/02-工作负载/deployments.md|Deployment]] — 无状态对照
- [[22-概念/02-工作负载/daemonset.md|DaemonSet]]
- [[22-概念/04-存储/pv.md|PersistentVolume]]
- [[22-概念/04-存储/storageclass.md|StorageClass]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
- [[01-集群基础/02-设计原则/03-declarative-api-pattern|02 - 声明式 API 与面向终态设计 (Declarative API)]]
- [[02-工作负载/01-核心工作负载/README-old|Domain-4: Kubernetes工作负载]]
- [[02-工作负载/01-核心工作负载/23-resource-management|16 - 资源管理表]]
- [[04-应用模式/02-行业架构/04-im-rtc-architecture|实时通信 (IM / RTC) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/03-cms-architecture|内容管理系统 (CMS) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/93-digital-twin-factory|数字孪生工厂架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/54-social-gaming-metaverse|社交游戏与元宇宙社交架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/31-instant-retail|即时零售架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/70-ecny-cbdc|数字人民币架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/42-secondhand-circular|二手交易与循环经济架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/26-aviation-travel|航空出行架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/43-enterprise-im|企业即时通讯架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/25-quantitative-trading|证券量化交易架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/48-vocational-edtech|职业教育培训架构设计 — 阿里云视角]]
- [[09-可观测性/00-总览/04-elastic-stack-enterprise-observability|Elastic Stack企业级可观测性平台深度实践]]
- [[09-可观测性/02-日志/07-splunk-enterprise-siem|Splunk企业级日志分析与安全智能平台深度实践]]
- [[11-发布变更/07-迁移方案/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[17-系统基础/04-K8s事件/09-job-cronjob-batch-events|09 - Job 与 CronJob 批处理事件]]
- [[17-系统基础/04-K8s事件/12-autoscaling-events|12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]
- [[17-系统基础/04-K8s事件/08-statefulset-daemonset-events|08 - StatefulSet 与 DaemonSet 控制器事件]]
- [[17-系统基础/04-K8s事件/14-namespace-resource-gc-events|14 - Namespace、资源管理与垃圾回收事件]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes|Kubernetes 中的对象]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning（动态卷供给）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/workloads/horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/workload-management|Workload Management]]
- [[17-系统基础/06-知识字典/workloads/pod-hostname|Pod Hostname]]
- [[19-故障诊断/02-资源排障/13-statefulset-troubleshooting|StatefulSet 故障排查]]
- [[19-故障诊断/02-资源排障/07-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]]
- [[20-最佳实践/04-migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[22-概念/11-交叉分析/etcd-×-StatefulSet|etcd × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service|StatefulSet × Service]]
- [[22-概念/11-交叉分析/apiserver-×-StatefulSet|apiserver × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Ingress|StatefulSet × Ingress]]
- [[22-概念/11-交叉分析/StatefulSet-×-NetworkPolicy|StatefulSet × NetworkPolicy]]
- [[22-概念/11-交叉分析/StatefulSet-×-PVC|StatefulSet × PVC]]
- [[22-概念/11-交叉分析/StatefulSet-×-RBAC|StatefulSet × RBAC]]
- [[26-技能/01-集群运维/migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics|第15课：调度与亲和性]]
- [[26-技能/04-工作负载/hpa-vpa/培训/learn-09-hpa-basics|第九课：HPA - 自动伸缩]]
- [[26-技能/04-工作负载/job-cronjob/培训/learn-11-job-cronjob|第九课：Job 和 CronJob - 任务调度]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/字典-horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[26-技能/04-工作负载/pod/配置与字典/pod-hostname|Pod Hostname]]
- [[26-技能/04-工作负载/statefulset/statefulset-fta|StatefulSet 异常故障树分析 (skills)]]
- [[26-技能/04-工作负载/statefulset/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation (skills)]]
- [[26-技能/04-工作负载/statefulset/培训/learn-14-statefulset-basics|第14课：StatefulSet - 有状态应用管理]]
- [[26-技能/05-网络/service/培训/kubernetes-service-presentation|Kubernetes Service 全栈进阶培训 (从入门到专家) [topic-presentations]]]
