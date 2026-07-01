---
title: kube-controller-manager 深度解析
description: 深入解析 kube-controller-manager 的架构设计、40+ 内置控制器、Leader 选举机制、控制器协同工作原理与生产级运维
category: domain-01-cluster-fundamentals
tags:
- k8s
- controller-manager
- controllers
- leader-election
- reconcile
- kubernetes
- etcd
- apiserver
- kubelet
- prometheus
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 25min
intent_queries:
- kube-controller-manager 深度解析 是什么
- 如何 kube-controller-manager 深度解析
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- kube-controller-manager
- 深度解析
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- etcd-basics
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
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
- type: domain
  path: ../domain-05-security-compliance/
  label: '相关知识域: domain-05-security-compliance'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/controller-manager-fta.md
  label: '故障树: controller-manager'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
related_docs:
- path: 11-etcd-deep-dive.md
  type: depth
  desc: etcd 深度解析
- path: 12-apiserver-deep-dive.md
  type: depth
  desc: API Server 深度解析
- path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/controller-manager-fta.md
  type: fta
  desc: Controller Manager 故障树
created: "2026-05-23"
---

# kube-controller-manager 深度解析 (KCM Deep Dive)

> kube-controller-manager (KCM) 是 [[Kubernetes|Kubernetes]] 控制平面的核心组件，运行所有内置控制器，确保集群状态与期望状态一致

---

<!-- chunk: 1. 架构概述 (Architecture Overview) -->
## 1. 架构概述 (Architecture Overview)

### 1.1 核心设计理念

| 概念 | 英文名 | 说明 |
|:---|:---|:---|
| **控制循环** | Control Loop | 持续监控并调整实际状态到期望状态 |
| **声明式管理** | Declarative | 用户声明期望状态，控制器实现 |
| **最终一致性** | Eventual Consistency | 系统最终会收敛到期望状态 |
| **单一职责** | Single Responsibility | 每个控制器只负责一种资源类型 |
| **水平触发** | Level-Triggered | 基于状态差异而非事件触发 |

### 1.2 整体架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        kube-controller-manager                          │
│                                                                         │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────┐  │
│  │ Leader Election  │  │  Controller      │  │   Shared Informers   │  │
│  │    (Lease)       │  │  Manager         │  │   (Watch Cache)      │  │
│  └────────┬─────────┘  └────────┬─────────┘  └──────────┬───────────┘  │
│           │                     │                        │              │
│           ▼                     ▼                        ▼              │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                        Controllers                                │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────┐ │  │
│  │  │ Deployment │ │ ReplicaSet │ │   Node     │ │ ServiceAccount │ │  │
│  │  │ Controller │ │ Controller │ │ Controller │ │   Controller   │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────┘ │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────┐ │  │
│  │  │ Endpoint   │ │ Namespace  │ │   PV/PVC   │ │     Job        │ │  │
│  │  │ Controller │ │ Controller │ │ Controller │ │   Controller   │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────┘ │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────┐ │  │
│  │  │ DaemonSet  │ │ StatefulSet│ │    HPA     │ │   CronJob      │ │  │
│  │  │ Controller │ │ Controller │ │ Controller │ │   Controller   │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────┘ │  │
│  │  ... (40+ Controllers)                                           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────┬────────────────────────────┘
                                             │
                                             ▼
                                    ┌────────────────┐
                                    │  kube-apiserver│
                                    └────────┬───────┘
                                             │
                                             ▼
                                    ┌────────────────┐
                                    │      etcd      │
                                    └────────────────┘
```

### 1.3 控制器工作模式

```
                    ┌─────────────────────────────────┐
                    │         Controller Loop         │
                    └─────────────────────────────────┘
                                   │
                    ┌──────────────┴──────────────┐
                    ▼                             ▼
        ┌─────────────────────┐      ┌─────────────────────┐
        │    Watch/Inform     │      │    Work Queue       │
        │  (Shared Informer)  │      │  (Rate Limited)     │
        └──────────┬──────────┘      └──────────┬──────────┘
                   │                            │
                   │  Event                     │  Pop Item
                   ▼                            ▼
        ┌─────────────────────┐      ┌─────────────────────┐
        │   Event Handler     │      │   Reconcile Logic   │
        │  Add/Update/Delete  │─────▶│   (Sync Handler)    │
        └─────────────────────┘      └──────────┬──────────┘
                                                │
                                  ┌─────────────┴─────────────┐
                                  ▼                           ▼
                        ┌────────────────┐         ┌──────────────────┐
                        │    Success     │         │     Failure      │
                        │   (Done)       │         │   (Requeue)      │
                        └────────────────┘         └──────────────────┘
```

---

<!-- chunk: 2. 内置控制器分类总览 (Built-in Controllers Overview) -->
## 2. 内置控制器分类总览 (Built-in Controllers Overview)

### 2.1 工作负载控制器 (Workload Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **DeploymentController** | Deployment | ReplicaSet | 滚动更新、回滚、版本管理 |
| **ReplicaSetController** | ReplicaSet | Pod | 维护Pod副本数 |
| **StatefulSetController** | StatefulSet | Pod, PVC | 有状态应用管理、有序部署 |
| **DaemonSetController** | DaemonSet | Pod | 每节点运行一个Pod |
| **JobController** | Job | Pod | 批处理任务、完成追踪 |
| **CronJobController** | CronJob | Job | 定时任务调度 |
| **[[ReplicationController|ReplicationController]]** | RC | Pod | 旧版副本控制(已弃用) |

### 2.2 服务与网络控制器 ([[Service|Service]] & Network Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **EndpointsController** | Service, Pod | Endpoints | 维护Service端点列表 (legacy) |
| **EndpointSliceController** | Service, Pod | EndpointSlice | 分片端点管理 (推荐) |
| **EndpointSliceMirroringController** | Endpoints | EndpointSlice | 将legacy Endpoints镜像到EndpointSlice |
| **ServiceController** | Service | Cloud LB | 云负载均衡器管理 |
| **RouteController** | Node | Cloud Routes | 云路由配置 |
| **NodeIPAMController** | Node | Node.spec.podCIDR | 节点Pod CIDR分配 |

### 2.3 存储控制器 (Storage Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **PersistentVolumeController** | PV, PVC | PV, PVC | PV/PVC绑定与回收 |
| **AttachDetachController** | Pod, Node | VolumeAttachment | 卷挂载/卸载 |
| **PVCProtectionController** | PVC, Pod | PVC Finalizer | 防止使用中的PVC被删除 |
| **StorageProtectionController** | PVC, PV | Finalizer | 存储对象删除保护 |
| **VolumeExpansionController** | PVC | PVC | 卷扩容 |
### 2.4 节点与生命周期控制器 (Node & Lifecycle Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **NodeLifecycleController** | Node | Node(Taints), Pod | 节点健康检测、Taint管理、Pod驱逐 |
| **LegacyNodeRoleController** | Node | Node Labels | 节点角色标签同步 (master/worker) |
| **CloudNodeLifecycleController** | Node | Node | 云节点生命周期管理 |
| **PodGCController** | Pod | Pod | 清理已完成/孤儿Pod |
| **TTLController** | Job, Pod | Job, Pod | TTL到期后自动清理 |
| **TTLAfterFinishedController** | Job | Job | Job完成后TTL清理 |
| **NamespaceController** | Namespace | 所有NS内资源 | Namespace删除时级联清理资源 |

### 2.5 安全与配置控制器 (Security & Configuration Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **ServiceAccountController** | Namespace | ServiceAccount, Secret | 创建默认SA及Token Secret |
| **TokenController** | ServiceAccount, Secret | Secret | SA Token的生成与清理 |
| **LegacyServiceAccountTokenCleanUpController** | Secret | Secret | 清理过期的legacy SA Token |
| **CertificateSigningController** | CSR | Certificate | 证书签名请求审批与签发 |
| **BootstrapsignerController** | ConfigMap | ConfigMap | 为[[entities/kubelet.md|kubelet]]引导签名Token |
| **TokencleanerController** | Secret | Secret | 清理过期的引导Token |
| **ClusterTrustBundleController** | ClusterTrustBundle | ConfigMap/Secret | 集群信任包分发与管理 |
| **ResourceQuotaController** | ResourceQuota | ResourceQuota Status | 配额使用量计算与更新 |
| **RootCACertPublisher** | ConfigMap | ConfigMap | 将根CA证书发布到所有Namespace |

### 2.6 垃圾回收与依赖管理 (Garbage Collection & Dependencies)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **GarbageCollectorController** | 所有资源 | 所有资源 | ownerReference级联删除、孤儿清理 |
| **DisruptionController** | PDB, Deployment/RS/SS/DS | PDB Status | 计算并更新PDB允许的 disruptions |

### 2.7 自动伸缩控制器 (Autoscaling Controllers)

| 控制器 | 监控资源 | 管理资源 | 核心职责 |
|:---|:---|:---|:---|
| **HPAController** | HPA, Metrics API | Deployment/RS/SS | 水平Pod自动伸缩 |

---

<!-- chunk: 3. 核心控制器详细解析 (Key Controllers Deep Dive) -->
## 3. 核心控制器详细解析 (Key Controllers Deep Dive)

> 每个控制器按统一格式说明：**作用**、**监视资源**、**输出动作**、**启动参数**、**问题影响**

---

### 3.1 工作负载控制器 (Workload Controllers)

#### 3.1.1 Deployment Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理Deployment资源的声明式更新，通过创建和管理ReplicaSet实现滚动更新、回滚、版本暂停/恢复 |
| **监视资源** | Deployment, ReplicaSet, Pod |
| **输出动作** | 1. 根据PodTemplateSpec变化创建新ReplicaSet<br>2. 按策略逐步扩缩新旧RS实现滚动更新<br>3. 更新Deployment Status (replicas/updatedReplicas/readyReplicas/conditions)<br>4. 清理历史RS (保留revisionHistoryLimit个) |
| **启动参数** | `--concurrent-deployment-syncs` (默认5), `--deployment-controller-sync-period` |
| **问题影响** | Deployment更新停滞、Pod无法扩缩容、滚动更新中断、回滚不可用；现有Pod不受影响 |

```
Deployment Controller 工作流程:

1. Watch Deployment/ReplicaSet/Pod 变化
2. 同步 Deployment 状态
   │
   ├─▶ 检查是否需要创建新 ReplicaSet
   │   └─ PodTemplateSpec 发生变化时创建新RS
   │
   ├─▶ 根据更新策略执行滚动更新
   │   ├─ RollingUpdate: 逐步替换Pod
   │   │   ├─ maxSurge: 最大超出副本数
   │   │   └─ maxUnavailable: 最大不可用数
   │   └─ Recreate: 先删后建
   │
   └─▶ 更新 Deployment Status
       ├─ replicas: 当前副本数
       ├─ updatedReplicas: 已更新副本数
       ├─ readyReplicas: 就绪副本数
       ├─ availableReplicas: 可用副本数
       └─ conditions: 状态条件
```

```yaml
# Deployment 滚动更新策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 最多可超出25%，即12个Pod
      maxUnavailable: 25%  # 最多25%不可用，即保证75%可用
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: web
        image: nginx:1.25
        readinessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

#### 3.1.2 ReplicaSet Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 确保指定数量的Pod副本始终运行，通过selector匹配Pod，多退少补 |
| **监视资源** | ReplicaSet, Pod |
| **输出动作** | 1. 创建/删除Pod以匹配replicas数量<br>2. 处理Pod被驱逐、节点问题等导致的副本缺失<br>3. 更新ReplicaSet Status |
| **启动参数** | `--concurrent-replicaset-syncs` (默认5) |
| **问题影响** | Pod副本数无法维持，过多或过少；Deployment的滚动更新依赖RS，会级联问题 |

#### 3.1.3 StatefulSet Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理有状态应用，保证Pod有序部署/扩缩容/滚动更新，维护稳定的网络标识和存储 |
| **监视资源** | StatefulSet, Pod, PVC |
| **输出动作** | 1. 按序创建/删除Pod (从0到N-1创建，从N-1到0删除)<br>2. 为每个Pod创建/管理PVC (volumeClaimTemplates)<br>3. 管理Pod的hostname和ordinal index<br>4. 执行有序滚动更新 (partition策略) |
| **启动参数** | `--concurrent-statefulset-syncs` (默认5), `--statefulset-pod-deletion-timeout` |
| **问题影响** | 有状态应用无法扩缩容、有序更新失败、PVC无法创建；对运行中的Pod无直接影响 |

#### 3.1.4 DaemonSet Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 确保集群中每个(或指定)节点上运行一个Pod副本，常用于日志收集、监控代理、网络组件 |
| **监视资源** | DaemonSet, Node, Pod |
| **输出动作** | 1. 根据nodeSelector/tolerations在每个匹配节点创建Pod<br>2. 新节点加入时自动调度Pod<br>3. 节点移除时清理对应Pod<br>4. 处理滚动更新 (maxUnavailable/maxSurge) |
| **启动参数** | `--concurrent-daemonset-syncs` (默认2) |
| **问题影响** | 新节点无法自动部署DaemonSet Pod；滚动更新停滞；节点监控/日志采集断链 |

#### 3.1.5 Job Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理批处理任务，确保指定数量的Pod成功完成(completions)，支持并行执行(parallelism) |
| **监视资源** | Job, Pod |
| **输出动作** | 1. 创建Pod执行任务<br>2. 跟踪成功/失败次数，达到completions后停止创建<br>3. 处理backoffLimit失败重试<br>4. 支持Indexed Job为每个Pod分配索引 |
| **启动参数** | `--concurrent-job-syncs` (默认5) |
| **问题影响** | 批处理任务无法创建或无法完成计数；CronJob依赖Job，会级联问题 |

#### 3.1.6 CronJob Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 按照Cron表达式定时创建Job，管理定时任务的调度和历史保留 |
| **监视资源** | CronJob, Job |
| **输出动作** | 1. 按schedule字段定时创建Job<br>2. 管理并发策略 (Allow/Forbid/Replace)<br>3. 清理历史Job (successfulJobsHistoryLimit/failedJobsHistoryLimit)<br>4. 处理时区 (timeZone字段) |
| **启动参数** | `--concurrent-cronjob-syncs` (默认5), `--cronjob-schedule-duration` |
| **问题影响** | 定时任务不再触发、历史Job堆积、并发策略失效 |

#### 3.1.7 ReplicationController (Legacy)

| 项目 | 说明 |
|:---|:---|
| **作用** | ReplicaSet的前身，同样维护Pod副本数，功能已被ReplicaSet完全取代 |
| **监视资源** | ReplicationController, Pod |
| **输出动作** | 创建/删除Pod以匹配replicas数量 |
| **启动参数** | `--concurrent-rc-syncs` (默认5) |
| **问题影响** | Pod副本数无法维持 |
| **备注** | 已弃用，仅用于兼容旧系统。新部署应使用Deployment+ReplicaSet |

---

### 3.2 服务与网络控制器 (Service & Network Controllers)

#### 3.2.1 Endpoints Controller (Legacy)

| 项目 | 说明 |
|:---|:---|
| **作用** | 监听Service和Pod变化，维护Endpoints资源，将Service的selector与后端Pod IP:Port映射。在EndpointSlice普及后，Legacy Endpoints控制器仍运行以保持向后兼容 |
| **监视资源** | Service, Pod |
| **输出动作** | 1. 根据Service selector匹配Pod，生成Endpoints.subsets<br>2. 当Pod IP/Port/Ready状态变化时更新Endpoints<br>3. headless Service也由此控制器管理 |
| **启动参数** | `--concurrent-endpoint-syncs` (默认5) |
| **问题影响** | Service无法解析到后端Pod，导致流量中断；kube-proxy依赖Endpoints，服务完全不可用 |
| **与EndpointSlice关系** | Endpoints是legacy API，单个对象最多存储1000个端点；EndpointSlice将其分片，无此限制。两者通常并行运行，EndpointSliceMirroringController会将用户创建的Endpoints同步到EndpointSlice |

#### 3.2.2 EndpointSlice Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | EndpointSlice的推荐实现方式，将Service后端端点分片存储，解决大规模集群下单个Endpoints对象过大导致的性能和etcd存储问题 |
| **监视资源** | Service, Pod, Node |
| **输出动作** | 1. 根据Service selector创建/更新/删除EndpointSlice<br>2. 每个EndpointSlice最多容纳100个端点，自动分片<br>3. 支持topology感知 (zone/region hints)<br>4. 管理端点条件 (Ready/Serving/Terminating) |
| **启动参数** | `--concurrent-endpointslice-syncs` (默认5), `--max-endpoints-per-slice` (默认100) |
| **问题影响** | Service无法发现后端端点，流量中断；EndpointSlice是kube-proxy的默认数据源 |

```
EndpointSlice Controller 工作流程:

1. Watch Service/Pod/Node 变化
   │
   ├─▶ Service创建/更新/删除
   │   └─ 重新计算所有关联EndpointSlice
   │
   ├─▶ Pod变化 (IP/Ready/Labels/DeletionTimestamp)
   │   └─ 更新对应Service的EndpointSlice
   │
   └─▶ Node变化 (Labels/Ready)
       └─ 更新topology hints

2. 端点分片策略
   ├─ 每个EndpointSlice最多100个端点
   ├─ 按端点名称哈希分片，保持稳定
   └─ 自动合并/拆分以优化数量

3. Topology 感知
   ├─ 读取Node topology labels (topology.kubernetes.io/zone)
   ├─ 计算zone hints
   └─ kube-proxy根据hints进行就近路由
```

#### 3.2.3 EndpointSliceMirroring Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 将用户手动创建或legacy系统创建的Endpoints资源自动镜像为EndpointSlice，确保所有Service发现机制都能通过EndpointSlice获取端点信息 |
| **监视资源** | Endpoints |
| **输出动作** | 1. 检测非EndpointSlice-managed的Endpoints<br>2. 创建对应的EndpointSlice副本<br>3. 保持Endpoints与EndpointSlice的实时同步<br>4. Endpoints删除时清理对应的EndpointSlice |
| **启动参数** | `--concurrent-endpointslice-mirroring-syncs` (默认5) |
| **问题影响** | 手动创建的Endpoints无法被kube-proxy(EndpointSlice模式)识别，导致Service流量黑洞 |

#### 3.2.4 Service Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理类型为LoadBalancer的Service，与底层云提供商API交互创建/更新/删除外部负载均衡器 |
| **监视资源** | Service, Node |
| **输出动作** | 1. 为LoadBalancer Service创建云LB<br>2. 更新Service Status.LoadBalancer.Ingress<br>3. 管理健康检查 (NodePort/HealthCheckNodePort)<br>4. 节点变化时更新LB后端池 |
| **启动参数** | `--concurrent-service-syncs` (默认1), `--cloud-provider` |
| **问题影响** | LoadBalancer类型Service无法分配外部IP，外部流量无法进入集群 |

#### 3.2.5 Route Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 在云环境中为Pod网络配置路由表，确保节点间Pod通信可达 |
| **监视资源** | Node |
| **输出动作** | 1. 为每个节点在云路由表中创建路由 (目标: Node CIDR, 下一跳: Node IP)<br>2. 节点删除时清理路由 |
| **启动参数** | `--configure-cloud-routes`, `--cluster-cidr`, `--allocate-node-cidrs` |
| **问题影响** | 跨节点Pod通信失败；通常只在非CNI覆盖网络模式下使用 |

#### 3.2.6 NodeIPAM Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 为每个Node分配Pod CIDR (node.spec.podCIDR)，管理集群IP地址池的分配与回收 |
| **监视资源** | Node |
| **输出动作** | 1. 新节点注册时从cluster-cidr分配子网并写入node.spec.podCIDR<br>2. 节点删除时回收CIDR<br>3. 支持双栈(IPv4/IPv6) CIDR分配 |
| **启动参数** | `--allocate-node-cidrs=true`, `--cluster-cidr=<CIDR>`, `--node-cidr-mask-size` (默认24), `--node-cidr-mask-size-ipv4`, `--node-cidr-mask-size-ipv6`, `--service-cluster-ip-range` |
| **问题影响** | 新节点无法分配Pod CIDR，kubelet无法启动Pod；IP地址池耗尽导致节点无法注册 |
| **生产建议** | 大规模集群需合理规划cluster-cIDR大小和node-cidr-mask-size；使用CIDR计算器确保地址足够。禁用CNI自带IPAM时(如Flannel host-gw模式)此控制器必须启用 |

---
### 3.3 存储控制器 (Storage Controllers)

#### 3.3.1 PersistentVolume Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理PV的生命周期和PVC的绑定/释放，实现存储的声明式供给(静态/动态) |
| **监视资源** | PV, PVC, StorageClass |
| **输出动作** | 1. 将未绑定的PVC与匹配的PV绑定 (静态供给)<br>2. 无匹配PV时为PVC触发动态供给 (创建PV)<br>3. 处理PVC删除后的PV回收 (Retain/Recycle/Delete)<br>4. 更新PV/PVC的Phase (Pending/Bound/Released/Failed) |
| **启动参数** | `--enable-dynamic-provisioning` (默认true), `--volume-host-cidr-deny-list` |
| **问题影响** | PVC一直处于Pending无法绑定，Pod无法启动；PV释放后无法回收，存储泄漏 |

#### 3.3.2 AttachDetach Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理存储卷的物理挂载/卸载，确保卷在Pod调度到的节点上attach，Pod删除后detach |
| **监视资源** | Pod, Node, VolumeAttachment |
| **输出动作** | 1. Pod调度到节点后，调用CSI/内置插件将卷attach到节点<br>2. 创建/更新VolumeAttachment对象记录attach状态<br>3. Pod删除/迁移后触发detach<br>4. 处理多Pod共享卷的attach决策 |
| **启动参数** | `--attach-detach-reconcile-sync-period` (默认1m), `--disable-attach-detach-reconcile-sync` |
| **问题影响** | Pod无法挂载卷，处于ContainerCreating状态；卷无法卸载导致节点无法迁移Pod |

#### 3.3.3 PVCProtection Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 防止正在使用中的PVC被用户误删除，通过Finalizer机制实现删除保护 |
| **监视资源** | PVC, Pod |
| **输出动作** | 1. 为活跃PVC添加`kubernetes.io/pvc-protection` Finalizer<br>2. 检测PVC是否被Pod使用<br>3. PVC被使用时阻止删除完成，直到所有Pod解除绑定<br>4. PVC不再使用时移除Finalizer，允许删除 |
| **启动参数** | (无独立参数，通过`--controllers`启用/禁用) |
| **问题影响** | PVC可能被误删导致数据丢失；或PVC的Finalizer无法移除导致Namespace删除卡住 |

#### 3.3.4 StorageProtection Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 对StorageClass和相关存储对象提供删除保护，确保有依赖关系时阻止删除。常与PVCProtection协同工作，提供存储生态系统的整体保护 |
| **监视资源** | PVC, PV, StorageClass, 相关存储资源 |
| **输出动作** | 1. 监控存储对象的删除请求<br>2. 检查是否存在依赖关系 (如PVC引用StorageClass)<br>3. 通过Finalizer阻止有依赖的存储对象被删除<br>4. 依赖解除后自动清理Finalizer |
| **启动参数** | (无独立参数) |
| **问题影响** | 存储对象被误删导致数据不可恢复；Finalizer残留导致资源泄漏 |

#### 3.3.5 VolumeExpansion Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 处理PVC的在线/离线扩容请求，协调CSI驱动完成文件系统扩容 |
| **监视资源** | PVC, PV, StorageClass |
| **输出动作** | 1. 检测PVC的`resources.requests.storage`增大<br>2. 验证StorageClass是否允许扩容 (allowVolumeExpansion)<br>3. 触发底层存储扩容<br>4. 更新PVC Status.Capacity |
| **启动参数** | (无独立参数) |
| **问题影响** | PVC扩容请求无法处理，存储空间不足导致应用问题 |

---

### 3.4 节点与生命周期控制器 (Node & Lifecycle Controllers)

#### 3.4.1 NodeLifecycle Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 监控节点健康状况，管理节点Taint，执行基于Taint的Pod驱逐 (TaintBasedEvictions)，是集群节点容错的核心控制器 |
| **监视资源** | Node, Pod, NodeLease |
| **输出动作** | 1. 检测节点心跳超时 (NodeLease/NodeStatus)<br>2. 为不健康节点添加Taint (not-ready, unreachable, memory-pressure, disk-pressure, pid-pressure, network-unavailable)<br>3. **TaintBasedEvictions**: 根据Pod的tolerations和Taint匹配决定是否驱逐<br>4. 大规模问题时执行速率限制驱逐 (zone级检测)<br>5. 节点恢复时移除Taint |
| **启动参数** | `--node-monitor-period` (默认5s), `--node-monitor-grace-period` (默认40s), `--pod-eviction-timeout` (默认5m), `--node-eviction-rate` (默认0.1), `--secondary-node-eviction-rate` (默认0.01), `--large-cluster-size-threshold` (默认50), `--unhealthy-zone-threshold` (默认0.55) |
| **问题影响** | 节点问题时Pod无法被驱逐，应用单点问题持续；或健康节点被误判导致不必要驱逐 |

```
NodeLifecycle Controller 工作流程:

1. 监控 Node 状态变化
   |
   ├─▶ 节点心跳检测
   │   ├─ NodeLease 更新 (默认10s, kubelet上报)
   │   └─ NodeStatus 更新 (默认1m)
   │
   ├─▶ 状态判定与Taint管理
   │   ├─ Ready -> NotReady: 添加 node.kubernetes.io/not-ready:NoExecute
   │   ├─ 失联 > grace-period: 添加 node.kubernetes.io/unreachable:NoExecute
   │   ├─ 内存压力: node.kubernetes.io/memory-pressure
   │   ├─ 磁盘压力: node.kubernetes.io/disk-pressure
   │   ├─ PID压力: node.kubernetes.io/pid-pressure
   │   └─ 网络不可用: node.kubernetes.io/network-unavailable
   │
   ├─▶ TaintBasedEvictions
   │   ├─ 检查Pod的tolerations是否匹配Taint
   │   ├─ 无toleration或toleration秒数到期: 驱逐Pod
   │   └─ 例如: not-ready:NoExecute 默认容忍300s后驱逐
   │
   └─▶ 大规模问题保护 (Zone检测)
       ├─ 检测不健康Zone比例
       ├─ 超过 unhealthy-zone-threshold: 降低驱逐速率
       └─ 防止雪崩式全部驱逐
```

#### 3.4.2 LegacyNodeRole Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 为节点同步legacy角色标签，确保节点角色(master/node)标签的一致性，用于兼容旧版工具和调度器逻辑 |
| **监视资源** | Node |
| **输出动作** | 1. 根据节点的control-plane组件运行状态添加/移除角色标签<br>2. 维护`node-role.kubernetes.io/master`和`node-role.kubernetes.io/control-plane`标签的一致性<br>3. 协助调度器的角色感知调度 |
| **启动参数** | (无独立参数) |
| **问题影响** | 节点角色标签不一致可能导致调度器将工作负载调度到控制平面节点，或某些依赖标签的插件行为异常 |

#### 3.4.3 CloudNodeLifecycle Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 在云环境中监控云厂商侧的节点实例状态，当云实例被删除或停止时同步更新Kubernetes节点状态 |
| **监视资源** | Node |
| **输出动作** | 1. 定期查询云API获取实例状态<br>2. 云实例已删除: 删除对应Node对象<br>3. 云实例已停止: 更新Node为NotReady<br>4. 处理云节点初始化时的providerID设置 |
| **启动参数** | `--cloud-provider`, `--cloud-config` |
| **问题影响** | 已删除的云实例对应的Node对象残留，导致调度器继续向不存在节点调度Pod |

#### 3.4.4 PodGC Controller (Pod Garbage Collector)

| 项目 | 说明 |
|:---|:---|
| **作用** | 清理集群中处于终止状态或失去Node绑定的孤儿Pod，防止etcd中Pod对象无限堆积 |
| **监视资源** | Pod, Node |
| **输出动作** | 1. 删除处于Failed/Succeeded且超过阈值的Pod<br>2. 清理Node已不存在但Pod仍残留的孤儿Pod<br>3. 清理被调度到不存在节点的Pod<br>4. 管理终止中(Terminating)但kubelet已失联的Pod强制删除 |
| **启动参数** | `--terminated-pod-gc-threshold` (默认12500) |
| **问题影响** | 已完成Pod堆积导致etcd和API Server负载升高；孤儿Pod无法自动清理，Namespace删除可能卡住 |

#### 3.4.5 TTL / TTLAfterFinished Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 为完成后的Job/Pod自动设置TTL，到期后自动清理，避免历史任务无限堆积 |
| **监视资源** | Job, Pod (带ttlSecondsAfterFinished字段) |
| **输出动作** | 1. Job/Pod完成后开始TTL倒计时<br>2. TTL到期后触发级联删除 |
| **启动参数** | `--ttl-after-finished-enabled` (默认true) |
| **问题影响** | 历史Job/Pod无法自动清理，资源堆积 |

#### 3.4.6 Namespace Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 处理Namespace的删除请求，执行Namespace内所有资源的级联清理，确保Namespace删除时不会残留孤儿资源 |
| **监视资源** | Namespace, 所有Namespace内资源 |
| **输出动作** | 1. Namespace删除时设置DeletionTimestamp和`kubernetes` Finalizer<br>2. 发现Namespace内所有资源类型并发起删除<br>3. 按GVR分组并行删除资源<br>4. 所有资源清理完成后移除Finalizer，Namespace对象被删除 |
| **启动参数** | `--concurrent-namespace-syncs` (默认10), `--namespace-sync-period` |
| **问题影响** | Namespace删除卡住(一直处于Terminating)；或Namespace被误删时资源未清理干净导致泄漏 |

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

```
Namespace Controller 终止流程:

1. 用户执行 kubectl delete ns <name>  # ⚠️ 不可逆：永久删除命名空间及全部资源
   |
   ▼
2. API Server 设置 Namespace DeletionTimestamp
   添加 Finalizer: kubernetes
   |
   ▼
3. Namespace Controller 检测 DeletionTimestamp
   |
   ├─▶ 发现该Namespace下所有资源 (通过Discovery API获取所有GVR)
   │
   ├─▶ 并行删除所有资源
   │   ├─ 优先删除有ownerReference的子资源
   │   └─ 处理每个资源的Finalizer
   │
   ├─▶ 等待所有资源被删除
   │   └─ 轮询检查资源是否仍存在
   │
   └─▶ 所有资源清理完成
       └─ 移除 Namespace Finalizer
           └─ Namespace 对象被GC删除
```

---

### 3.5 安全与配置控制器 (Security & Configuration Controllers)

#### 3.5.1 ServiceAccount Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 为每个新创建的Namespace自动创建默认ServiceAccount (default)，确保Namespace内的Pod有身份标识可运行 |
| **监视资源** | Namespace |
| **输出动作** | 1. 新Namespace创建时自动生成`default` ServiceAccount<br>2. 默认SA被误删时自动重新创建<br>3. 为default SA关联Secret或Token (取决于legacy/auto模式) |
| **启动参数** | `--service-account-private-key-file`, `--root-ca-file` |
| **问题影响** | 新Namespace无法创建默认SA，Pod无法指定ServiceAccount导致启动失败或无法访问API Server |
| **生产建议** | 确保SA私钥文件安全备份；轮转私钥时需同时更新所有已签发Token。在1.24+版本中，legacy Secret-based token自动创建已默认关闭，改用TokenRequest API |

#### 3.5.2 Token Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理ServiceAccount的Token Secret生命周期，为legacy SA token模式创建、签名、验证和清理Token Secret |
| **监视资源** | ServiceAccount, Secret |
| **输出动作** | 1. 为ServiceAccount创建包含JWT的Secret (legacy模式)<br>2. 使用`--service-account-private-key-file`签发Token<br>3. 将`--root-ca-file`注入到Secret的ca.crt字段<br>4. 清理不再被引用的SA Secret |
| **启动参数** | `--service-account-private-key-file`, `--root-ca-file`, `--service-account-max-token-expiration` |
| **问题影响** | Pod无法获取API Server访问凭证，导致in-cluster客户端认证失败；Token签名失败导致API Server拒绝认证 |

#### 3.5.3 LegacyServiceAccountTokenCleanUp Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 自动清理过期的legacy ServiceAccount Token Secret，减少因长期有效Token带来的安全风险 |
| **监视资源** | Secret (type=kubernetes.io/service-account-token) |
| **输出动作** | 1. 扫描所有legacy SA Token Secret<br>2. 检测Token是否过期 (超过有效期且未被使用)<br>3. 安全删除过期Token Secret<br>4. 配合`--service-account-max-token-expiration`使用 |
| **启动参数** | `--legacy-service-account-token-clean-up-period` |
| **问题影响** | 过期Token残留增加安全风险；或活跃Token被误删导致Pod认证失败 |

#### 3.5.4 CertificateSigning Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 处理CertificateSigningRequest (CSR)资源的审批和证书签发，用于节点加入、用户证书申请、服务证书轮换 |
| **监视资源** | CSR |
| **输出动作** | 1. 自动审批特定CSR (如kubelet bootstrap, node client/server cert)<br>2. 使用`--cluster-signing-cert-file/key-file`签发证书<br>3. 将签发后的证书写入CSR Status.Certificate<br>4. 清理已处理的CSR |
| **启动参数** | `--cluster-signing-cert-file`, `--cluster-signing-key-file`, `--cluster-signing-duration` (默认8760h=1年) |
| **问题影响** | 新节点无法加入集群 (kubelet bootstrap失败)；现有节点证书到期无法自动轮换 |

#### 3.5.5 ClusterTrustBundle Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理ClusterTrustBundle资源，将集群级信任证书包分发到各Namespace，用于服务间mTLS信任锚点管理 |
| **监视资源** | ClusterTrustBundle, ConfigMap, Secret |
| **输出动作** | 1. 监控ClusterTrustBundle变化<br>2. 将信任包内容分发到目标Namespace的ConfigMap或Secret<br>3. 确保所有目标Namespace的信任包保持最新<br>4. 处理信任包的版本更新和回滚 |
| **启动参数** | (无独立参数) |
| **问题影响** | 集群服务间mTLS认证失败，因为信任锚点未分发或过期 |

#### 3.5.6 ResourceQuota Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 计算和同步Namespace中ResourceQuota的使用量，确保配额限制的准确性和实时性，防止资源超用 |
| **监视资源** | ResourceQuota, Pod, Service, PVC, ConfigMap, Secret等所有配额涉及资源 |
| **输出动作** | 1. 统计Namespace内所有资源的实际使用量<br>2. 更新ResourceQuota Status (used字段)<br>3. 创建/更新配额时进行准入检查<br>4. 处理Pod删除后的配额释放 |
| **启动参数** | `--concurrent-resource-quota-syncs` (默认5), `--resource-quota-sync-period` |
| **问题影响** | 配额使用量不准确导致Pod被错误拒绝或超配；Namespace资源泄漏 |
| **生产建议** | 大规模集群中ResourceQuota同步可能成为瓶颈，适当增加`--concurrent-resource-quota-syncs`；监控workqueue_depth避免队列堆积 |

#### 3.5.7 RootCACertPublisher Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 将集群根CA证书发布到所有Namespace的ConfigMap中，供Pod验证API Server TLS证书 |
| **监视资源** | Namespace, ConfigMap |
| **输出动作** | 1. 在每个Namespace创建/维护`kube-root-ca.crt` ConfigMap<br>2. 包含集群根CA证书 ( PEM格式 )<br>3. 新Namespace自动创建，证书轮转时自动更新 |
| **启动参数** | `--root-ca-file` |
| **问题影响** | Pod无法验证API Server证书，导致`kubectl` in-cluster操作和客户端库TLS握手失败 |

---

### 3.6 垃圾回收与依赖管理 (Garbage Collection & Dependencies)

#### 3.6.1 GarbageCollector Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | Kubernetes资源级联删除的核心机制，通过ownerReference识别资源间的父子关系，实现资源树的自动清理 |
| **监视资源** | 所有API资源 (通过Discovery API动态发现) |
| **输出动作** | 1. 维护资源依赖图 (ownerReference关系)<br>2. 前台删除: 先删子资源再删父资源<br>3. 后台删除: 父资源删除后异步扫描并删除孤儿子资源<br>4. 孤儿模式: 移除子资源的ownerReference |
| **启动参数** | `--concurrent-gc-syncs` (默认20), `--enable-garbage-collector` (默认true) |
| **问题影响** | 资源删除后子资源残留导致泄漏；级联删除异常可能导致误删 |

```
GC Controller 级联删除流程:

                    ┌─────────────────────┐
                    │     Deployment      │
                    │  (Owner Reference)  │
                    └──────────┬──────────┘
                               │ ownerReferences
                               ▼
                    ┌─────────────────────┐
                    │     ReplicaSet      │
                    │  (Owner Reference)  │
                    └──────────┬──────────┘
                               │ ownerReferences
                               ▼
                    ┌─────────────────────┐
                    │        Pods         │
                    └─────────────────────┘

删除策略:
├─ Foreground (前台级联删除)
│   └─ 先删除所有依赖资源，最后删除Owner
│
├─ Background (后台级联删除) [默认]
│   └─ 先删除Owner，GC异步清理依赖资源
│
└─ Orphan (孤儿策略)
    └─ 只删除Owner，保留依赖资源
```

```yaml
# 删除策略示例
# Foreground 删除
kubectl delete deployment web --cascade=foreground

# Background 删除 (默认)
kubectl delete deployment web --cascade=background

# Orphan 删除
kubectl delete deployment web --cascade=orphan
```

#### 3.6.2 Disruption Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 管理PodDisruptionBudget (PDB)的状态，计算当前允许的自愿中断数量，保障应用在主动运维操作(节点升级、缩容)中的可用性 |
| **监视资源** | PDB, Deployment, ReplicaSet, StatefulSet, DaemonSet, Pod |
| **输出动作** | 1. 根据PDB selector计算匹配的Pod<br>2. 计算期望副本数和当前健康Pod数<br>3. 更新PDB Status ( disruptionsAllowed, currentHealthy, desiredHealthy, expectedPods )<br>4. evict API根据disruptionsAllowed决定是否允许驱逐 |
| **启动参数** | `--concurrent-deployment-syncs` (共享Deployment参数) |
| **问题影响** | PDB状态不准确，导致节点升级时过度驱逐Pod；或evict操作被错误拒绝，影响集群运维操作 |

---

### 3.7 自动伸缩控制器 (Autoscaling Controllers)

#### 3.7.1 HPA Controller

| 项目 | 说明 |
|:---|:---|
| **作用** | 根据CPU/内存使用率或自定义指标自动调整Deployment/ReplicaSet/StatefulSet的副本数 |
| **监视资源** | HPA, Metrics API (metrics.k8s.io/custom.metrics.k8s.io/external.metrics.k8s.io) |
| **输出动作** | 1. 定期(默认15s)查询指标<br>2. 计算期望副本数 = 当前副本数 * (当前指标 / 目标指标)<br>3. 应用伸缩策略 (scaleUp/scaleDown stabilizationWindow, policies)<br>4. 更新目标资源的replicas<br>5. 更新HPA Status |
| **启动参数** | `--concurrent-horizontal-pod-autoscaler-syncs` (默认5), `--horizontal-pod-autoscaler-sync-period` (默认15s), `--horizontal-pod-autoscaler-tolerance` (默认0.1), `--horizontal-pod-autoscaler-cpu-initialization-period`, `--horizontal-pod-autoscaler-initial-readiness-delay` |
| **问题影响** | 应用负载高峰时无法自动扩容导致服务降级；低峰时无法缩容导致资源浪费 |

---

<!-- chunk: 4. 生产环境关键控制器调优建议 (Production Tuning) -->
## 4. 生产环境关键控制器调优建议 (Production Tuning)

### 4.1 大规模集群 (>500节点) 调优参数

| 控制器 | 参数 | 默认值 | 大集群推荐 | 说明 |
|:---|:---|:---|:---|:---|
| **Deployment** | `--concurrent-deployment-syncs` | 5 | 10-20 | 高并发部署场景 |
| **ReplicaSet** | `--concurrent-replicaset-syncs` | 5 | 10-20 | 大量RS同步 |
| **EndpointSlice** | `--concurrent-endpointslice-syncs` | 5 | 10-20 | 大规模Service端点更新 |
| **GC** | `--concurrent-gc-syncs` | 20 | 30-50 | 大量资源对象清理 |
| **Namespace** | `--concurrent-namespace-syncs` | 10 | 20-30 | 多Namespace操作 |
| **ResourceQuota** | `--concurrent-resource-quota-syncs` | 5 | 10-20 | 配额计算加速 |
| **API QPS** | `--kube-api-qps` | 20 | 100-200 | API Server请求限制 |
| **API Burst** | `--kube-api-burst` | 30 | 200-400 | API Server突发限制 |
| **PodGC** | `--terminated-pod-gc-threshold` | 12500 | 5000-10000 | 根据etcd容量调整 |
| **NodeLifecycle** | `--node-eviction-rate` | 0.1 | 0.05-0.2 | 根据节点规模调整 |

### 4.2 控制器异常诊断方法

#### 4.2.1 通用诊断流程

```bash
# 1. 确认KCM Leader状态
kubectl get lease -n kube-system kube-controller-manager -o yaml

# 2. 检查控制器工作队列深度 (所有控制器通用)
curl -sk https://localhost:10257/metrics | grep workqueue_depth

# 3. 检查特定控制器的重试率
curl -sk https://localhost:10257/metrics | grep workqueue_retries_total

# 4. 查看控制器日志
journalctl -u kube-controller-manager -f --no-pager | grep -i <controller-name>

# 5. 检查API Server连接
curl -sk https://localhost:10257/healthz
```

#### 4.2.2 按类别诊断速查表

| 症状 | 涉及控制器 | 诊断命令 | 常见原因 |
|:---|:---|:---|:---|
| **Pod未创建/副本数不对** | ReplicaSet, Deployment | `kubectl describe rs`, `kubectl get events` | 控制器未Leader/队列阻塞/权限不足 |
| **Service无法访问** | EndpointSlice, Endpoints | `kubectl get endpointslices`, `kubectl get endpoints` | 端点未更新/selector不匹配 |
| **PVC一直Pending** | PersistentVolume | `kubectl describe pvc`, `kubectl get pv` | 无匹配PV/动态供给失败/SC不允许 |
| **Pod挂载卷失败** | AttachDetach | `kubectl get volumeattachment` | 卷未attach/detach卡住/CSI驱动问题 |
| **节点问题Pod不驱逐** | NodeLifecycle | `kubectl describe node`, `kubectl get node -o yaml` | grace-period过长/Zone保护/驱逐速率限制 |
| **Namespace删不掉** | Namespace | `kubectl get ns <name> -o yaml` | Finalizer阻塞/内有资源未清理 |
| **资源删除后残留** | GarbageCollector | `kubectl get <resource> --show-labels` | GC被禁用/ownerReference缺失 |
| **定时任务不触发** | CronJob | `kubectl get cronjob`, `kubectl get job` | 时区设置/schedule语法/并发策略 |
| **HPA不生效** | HPA | `kubectl describe hpa`, `kubectl top pod` | metrics-server不可用/指标类型错误 |
| **新节点无Pod CIDR** | NodeIPAM | `kubectl get node -o yaml \| grep podCIDR` | IPAM未启用/CIDR耗尽/参数错误 |
| **新Namespace无default SA** | ServiceAccount | `kubectl get sa -n <ns>` | SA控制器未运行/权限问题 |
| **证书申请未处理** | CertificateSigning | `kubectl get csr` | 自动审批条件不满足/签名证书配置错误 |
| **PDB状态异常** | Disruption | `kubectl get pdb -o yaml` | selector不匹配/目标控制器不存在 |
| **已完成Pod堆积** | PodGC | `kubectl get pods --all-namespaces --field-selector=status.phase=Succeeded` | 阈值过高/PodGC未运行 |

#### 4.2.3 日志关键模式识别

```bash
# 正常启动
I0101 00:00:00.000000   1 leaderelection.go:248] successfully acquired lease kube-system/kube-controller-manager

# 队列堆积警告
W0101 00:00:00.000000   1 reflector.go:324] watch of *v1.Pod ended with: too old resource version

# 同步超时
W0101 00:00:00.000000   1 replica_set.go:503] ReplicaSet default/nginx-rs has timed out progressing

# 权限拒绝 (RBAC问题)
E0101 00:00:00.000000   1 replica_set.go:456] Sync "default/nginx-rs" failed with pods "nginx-xxx" is forbidden

# GC错误
E0101 00:00:00.000000   1 gc_controller.go:274] garbage collector: error getting object for gvk xxx

# Node驱逐
I0101 00:00:00.000000   1 taint_manager.go:106] NoExecuteTaintManager is deleting Pod default/nginx-xxx

# EndpointSlice创建
I0101 00:00:00.000000   1 endpointslice_controller.go:306] Finished syncing Service "default/nginx" endpoint slices. (1.234ms)

# PVC保护
I0101 00:00:00.000000   1 pvc_protection_controller.go:95] PVC default/data is used by a Pod, preventing deletion
```

---

<!-- chunk: 5. 关键配置参数 (Configuration Parameters) -->
## 5. 关键配置参数 (Configuration Parameters)

### 5.1 通用参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--kubeconfig` | - | /etc/kubernetes/controller-manager.conf | API Server连接配置 |
| `--authentication-kubeconfig` | - | 同上 | 认证配置 |
| `--authorization-kubeconfig` | - | 同上 | 授权配置 |
| `--bind-address` | 0.0.0.0 | 0.0.0.0 | 监听地址 |
| `--secure-port` | 10257 | 10257 | 安全端口 |
| `--leader-elect` | true | true | 启用Leader选举 |
| `--leader-elect-lease-duration` | 15s | 15s | Lease持续时间 |
| `--leader-elect-renew-deadline` | 10s | 10s | Lease续约截止时间 |
| `--leader-elect-retry-period` | 2s | 2s | Lease重试周期 |

### 5.2 控制器通用参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--concurrent-deployment-syncs` | 5 | 5-10 | Deployment并发同步数 |
| `--concurrent-replicaset-syncs` | 5 | 5-10 | ReplicaSet并发同步数 |
| `--concurrent-endpoint-syncs` | 5 | 5-10 | Endpoints并发同步数 |
| `--concurrent-endpointslice-syncs` | 5 | 5-10 | EndpointSlice并发同步数 |
| `--concurrent-endpointslice-mirroring-syncs` | 5 | 5-10 | EndpointSliceMirroring并发数 |
| `--concurrent-service-syncs` | 1 | 1-5 | Service并发同步数 |
| `--concurrent-gc-syncs` | 20 | 20-50 | GC并发同步数 |
| `--concurrent-namespace-syncs` | 10 | 10-20 | Namespace并发同步数 |
| `--concurrent-resource-quota-syncs` | 5 | 5-10 | ResourceQuota并发同步数 |
| `--concurrent-statefulset-syncs` | 5 | 5-10 | StatefulSet并发同步数 |
| `--concurrent-job-syncs` | 5 | 5-10 | Job并发同步数 |
| `--concurrent-cronjob-syncs` | 5 | 5-10 | CronJob并发同步数 |
| `--concurrent-horizontal-pod-autoscaler-syncs` | 5 | 5-10 | HPA并发同步数 |
| `--terminated-pod-gc-threshold` | 12500 | 5000-12500 | 终止Pod GC阈值 |

### 5.3 节点控制器参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--node-monitor-period` | 5s | 5s | 节点监控周期 |
| `--node-monitor-grace-period` | 40s | 40s | 节点不响应宽限期 |
| `--pod-eviction-timeout` | 5m | 5m | Pod驱逐超时 |
| `--node-eviction-rate` | 0.1 | 0.1 | 正常情况驱逐速率(节点/秒) |
| `--secondary-node-eviction-rate` | 0.01 | 0.01 | 大规模问题驱逐速率 |
| `--large-cluster-size-threshold` | 50 | 50 | 大集群阈值 |
| `--unhealthy-zone-threshold` | 0.55 | 0.55 | 不健康Zone阈值 |

### 5.4 服务账号与证书参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--service-account-private-key-file` | - | SA Token签名私钥 |
| `--root-ca-file` | - | 根CA证书(注入到SA) |
| `--use-service-account-credentials` | false | 使用独立SA凭证 |
| `--cluster-signing-cert-file` | - | 集群签名证书 |
| `--cluster-signing-key-file` | - | 集群签名私钥 |
| `--cluster-signing-duration` | 8760h (1年) | 签名证书有效期 |

### 5.5 云控制器与网络参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--cloud-provider` | - | 云提供商(external/空) |
| `--external-cloud-volume-plugin` | - | 外部卷插件 |
| `--configure-cloud-routes` | true | 配置云路由 |
| `--allocate-node-cidrs` | false | 分配Node CIDR |
| `--cluster-cidr` | - | 集群Pod CIDR |
| `--service-cluster-ip-range` | - | Service IP范围 |
| `--node-cidr-mask-size` | 24 | 节点CIDR掩码大小 |
| `--node-cidr-mask-size-ipv4` | 24 | IPv4节点CIDR掩码 |
| `--node-cidr-mask-size-ipv6` | 64 | IPv6节点CIDR掩码 |
| `--max-endpoints-per-slice` | 100 | 每个EndpointSlice最大端点数 |

---

<!-- chunk: 6. Leader 选举机制 (Leader Election) -->
## 6. Leader 选举机制 (Leader Election)

### 6.1 选举流程

```
Leader Election 流程:

1. 创建/获取 Lease 资源
   |
   ├─▶ 检查 Lease 是否存在
   │   ├─ 不存在: 创建新Lease并成为Leader
   │   └─ 存在: 检查持有者和过期时间
   |
   ├─▶ 竞争 Leader
   │   ├─ Lease未过期且是其他节点持有: 等待
   │   ├─ Lease已过期: 尝试获取
   │   └─ 自己持有: 续约
   |
   └─▶ Leader 职责
       ├─ 周期性续约 (renew-deadline内)
       ├─ 运行所有控制器
       └─ 失去Leader时停止控制器

    ┌──────────────────────────────────────────────────┐
    │                   Lease Object                    │
    │  ┌────────────────────────────────────────────┐  │
    │  │ Namespace: kube-system                      │  │
    │  │ Name: kube-controller-manager               │  │
    │  │ HolderIdentity: kcm-master-1                │  │
    │  │ LeaseDurationSeconds: 15                    │  │
    │  │ AcquireTime: 2024-01-01T00:00:00Z          │  │
    │  │ RenewTime: 2024-01-01T00:00:10Z            │  │
    │  │ LeaseTransitions: 3                         │  │
    │  └────────────────────────────────────────────┘  │
    └──────────────────────────────────────────────────┘
```

### 6.2 查看 Leader 状态

```bash
# 查看 Leader Lease
kubectl get lease -n kube-system kube-controller-manager -o yaml

# 输出示例
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  name: kube-controller-manager
  namespace: kube-system
spec:
  holderIdentity: master-1_xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  leaseDurationSeconds: 15
  acquireTime: "2024-01-01T00:00:00.000000Z"
  renewTime: "2024-01-01T00:05:00.000000Z"
  leaseTransitions: 1
```

---

<!-- chunk: 7. 监控指标 (Monitoring Metrics) -->
## 7. 监控指标 (Monitoring Metrics)

### 7.1 关键指标表

| 指标名称 | 类型 | 说明 | 告警阈值 |
|:---|:---|:---|:---|
| `workqueue_adds_total` | Counter | 工作队列添加总数 | - |
| `workqueue_depth` | Gauge | 工作队列当前深度 | > 100 |
| `workqueue_queue_duration_seconds` | Histogram | 项在队列中等待时间 | p99 > 30s |
| `workqueue_work_duration_seconds` | Histogram | 处理项耗时 | p99 > 10s |
| `workqueue_retries_total` | Counter | 重试总数 | 异常增长 |
| `workqueue_longest_running_processor_seconds` | Gauge | 最长运行处理器时间 | > 300s |
| `leader_election_master_status` | Gauge | Leader状态(1=Leader) | - |
| `rest_client_requests_total` | Counter | API请求总数 | - |
| `rest_client_request_duration_seconds` | Histogram | API请求延迟 | p99 > 1s |
| `process_resident_memory_bytes` | Gauge | 内存使用 | > 4GB |
| `process_cpu_seconds_total` | Counter | CPU使用 | - |

### 7.2 Prometheus 告警规则

```yaml
groups:
- name: kube-controller-manager
  rules:
  - alert: KubeControllerManagerDown
    expr: absent(up{job="kube-controller-manager"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "kube-controller-manager is down"

  - alert: KubeControllerManagerNoLeader
    expr: sum(leader_election_master_status{job="kube-controller-manager"}) == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "kube-controller-manager has no leader"

  - alert: KubeControllerManagerWorkQueueDepthHigh
    expr: workqueue_depth{job="kube-controller-manager"} > 100
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Controller work queue depth is high"
      description: "Queue {{ $labels.name }} depth is {{ $value }}"

  - alert: KubeControllerManagerWorkQueueLatencyHigh
    expr: histogram_quantile(0.99, rate(workqueue_queue_duration_seconds_bucket{job="kube-controller-manager"}[5m])) > 30
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Controller work queue latency is high"

  - alert: KubeControllerManagerSyncLoopLatencyHigh
    expr: histogram_quantile(0.99, rate(workqueue_work_duration_seconds_bucket{job="kube-controller-manager"}[5m])) > 10
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Controller sync loop latency is high"

  - alert: KubeControllerManagerHighRetries
    expr: increase(workqueue_retries_total{job="kube-controller-manager"}[1h]) > 1000
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Controller has high retry rate"
```

---

<!-- chunk: 8. 故障排查 (Troubleshooting) -->
## 8. 故障排查 (Troubleshooting)

### 8.1 常见问题诊断

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| **控制器不工作** | 非Leader/未启动 | 检查Leader状态 | 检查选举配置 |
| **Pod未创建** | RS/Deployment控制器问题 | 检查控制器日志 | 排查具体错误 |
| **Pod不被驱逐** | NodeLifecycle控制器配置 | 检查超时配置 | 调整eviction-timeout |
| **PVC未绑定** | PV控制器问题 | 检查PV/PVC状态 | 检查StorageClass |
| **GC不工作** | GC控制器阻塞 | 检查GC队列深度 | 增加并发数 |
| **Endpoint未更新** | Endpoint控制器延迟 | 检查队列深度 | 检查API Server负载 |
| **HPA不生效** | Metrics不可用 | 检查metrics-server | 确保metrics可用 |
| **Namespace删不掉** | Finalizer阻塞 | kubectl get ns -o yaml | 移除stuck finalizer |
| **Service无法访问** | EndpointSlice未创建 | kubectl get endpointslices | 检查EndpointSlice控制器 |
| **孤儿Pod堆积** | PodGC阈值过高 | kubectl get pods -A --field-selector=status.phase=Failed,Succeeded | 降低gc-threshold |
| **证书到期** | CertificateSigning未处理 | kubectl get csr | 检查签名配置 |
| **配额不准** | ResourceQuota队列阻塞 | 检查workqueue_depth | 增加并发同步数 |

### 8.2 诊断命令

```bash
# 检查 KCM 状态
systemctl status kube-controller-manager
journalctl -u kube-controller-manager -f --no-pager

# 检查 Leader 状态
kubectl get lease -n kube-system kube-controller-manager -o yaml

# 检查控制器指标
curl -k https://localhost:10257/metrics | grep workqueue_depth

# 检查特定控制器日志
journalctl -u kube-controller-manager | grep -i deployment
journalctl -u kube-controller-manager | grep -i "sync error"

# 检查事件
kubectl get events --all-namespaces --sort-by='.lastTimestamp'

# 检查 Deployment 状态
kubectl describe deployment <name>
kubectl rollout status deployment <name>
kubectl rollout history deployment <name>

# 检查 ReplicaSet
kubectl get rs -o wide
kubectl describe rs <name>

# 检查控制器健康
kubectl get componentstatuses  # 已弃用但可能仍可用
curl -k https://localhost:10257/healthz

# 检查EndpointSlice
curl -k https://localhost:10257/metrics | grep endpointslice_controller

# 检查NodeLifecycle
curl -k https://localhost:10257/metrics | grep node_lifecycle_controller

# 检查存储控制器
curl -k https://localhost:10257/metrics | grep attachdetach_controller
curl -k https://localhost:10257/metrics | grep persistentvolume_controller
```

---

<!-- chunk: 9. 性能优化 (Performance Tuning) -->
## 9. 性能优化 (Performance Tuning)

### 9.1 大规模集群优化

| 优化项 | 默认值 | 大集群推荐值 | 说明 |
|:---|:---|:---|:---|
| `--concurrent-deployment-syncs` | 5 | 10-20 | 增加Deployment处理并发 |
| `--concurrent-gc-syncs` | 20 | 30-50 | 增加GC处理并发 |
| `--concurrent-endpoint-syncs` | 5 | 10-20 | 增加Endpoint处理并发 |
| `--kube-api-qps` | 20 | 100-200 | 增加API QPS限制 |
| `--kube-api-burst` | 30 | 200-400 | 增加API Burst限制 |

### 9.2 资源配置建议

| 集群规模 | CPU | 内存 | 说明 |
|:---|:---|:---|:---|
| 小型 (<100节点) | 0.5-1核 | 512MB-1GB | |
| 中型 (100-500节点) | 1-2核 | 1-2GB | |
| 大型 (500-1000节点) | 2-4核 | 2-4GB | |
| 超大型 (>1000节点) | 4-8核 | 4-8GB | |

---

<!-- chunk: 10. 高可用部署 (High Availability) -->
## 10. 高可用部署 (High Availability)

### 10.1 HA 配置要点

```yaml
# 多实例部署配置要点
# 1. 所有实例使用相同配置
# 2. 启用Leader选举
# 3. 使用相同的service-account-private-key-file

# 关键参数
--leader-elect=true
--leader-elect-lease-duration=15s
--leader-elect-renew-deadline=10s
--leader-elect-retry-period=2s
--leader-elect-resource-lock=leases  # 推荐使用leases
--leader-elect-resource-namespace=kube-system
```

### 10.2 健康检查端点

| 端点 | 用途 | 检查内容 |
|:---|:---|:---|
| `/healthz` | 整体健康检查 | 所有检查项聚合 |
| `/healthz/leaderElection` | Leader选举检查 | 选举状态 |
| `/metrics` | Prometheus指标 | 运行时指标 |

```bash
# 健康检查
curl -k https://localhost:10257/healthz
curl -k https://localhost:10257/healthz?verbose
```

---

<!-- chunk: 11. 生产环境 Checklist -->
## 11. 生产环境 Checklist

### 11.1 部署检查

| 检查项 | 状态 | 说明 |
|:---|:---|:---|
| [ ] 多实例部署 | | 高可用保证 |
| [ ] Leader选举正常 | | 选举机制工作 |
| [ ] 证书配置正确 | | API认证正常 |
| [ ] SA私钥配置 | | Token签名正常 |
| [ ] 监控告警配置 | | 运维保障 |
| [ ] 资源限制配置 | | 防止资源耗尽 |
| [ ] 日志收集配置 | | 问题排查 |

### 11.2 运维检查

| 检查项 | 频率 | 命令/方法 |
|:---|:---|:---|
| Leader状态 | 每日 | 检查Lease资源 |
| 队列深度 | 每日 | 检查workqueue_depth指标 |
| 同步延迟 | 每日 | 检查workqueue_work_duration |
| 重试率 | 每日 | 检查workqueue_retries |
| 内存使用 | 每日 | 检查process_resident_memory |
| 证书有效期 | 每月 | openssl检查 |

---

<!-- chunk: 附录: 控制器启动/禁用 -->
## 附录: 控制器启动/禁用

```bash
# 禁用特定控制器
--controllers=*,-bootstrapsigner,-tokencleaner

# 只启用特定控制器
--controllers=deployment,replicaset,namespace

# 查看所有可用控制器
kube-controller-manager --help | grep -A 100 "controllers stringSlice"

# 常见控制器名称
deployment, replicaset, statefulset, daemonset
job, cronjob
replicationcontroller
namespace, serviceaccount, endpoint, endpointslice, endpointslice-mirroring
persistentvolume-binder, persistentvolume-expander, attachdetach
pvc-protection, storage-protection
node, nodelifecycle, cloud-node-lifecycle, legacy-node-role
podgc, ttl-after-finished
garbagecollector, resourcequota
disruption
horizontalpodautoscaling
csrsigning, csrapproving, csrcleaner
serviceaccount-token, legacy-service-account-token-clean-up
root-ca-cert-publisher
cluster-trust-bundle
bootstrapsigner, tokencleaner
node-ipam, route, service
token
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## Related

- [[entities/kubelet.md|kubelet]]

- etcd 深度解析
- API Server 深度解析
- 相关知识域: domain-01-cluster-fundamentals
- 相关知识域: domain-02-workloads-applications
- 相关知识域: domain-03-networking-traffic
- 相关知识域: domain-04-storage-data
- 相关知识域: domain-05-security-compliance
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|速查卡: k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md|速查卡: kubectl-scene-cheatsheet]]

## See Also

- 11-etcd-deep-dive
- 12-apiserver-deep-dive
- 14-cloud-controller-manager-deep-dive
- 15-kubelet-deep-dive
