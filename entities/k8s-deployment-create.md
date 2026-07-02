---
title: Kubernetes Deployment 创建操作指南
description: '# Kubernetes Deployment 创建操作指南'
summary: 'dInformer appsinformers.DeploymentInformer,'
category: references
tags:
- k8s
- operations
- deployment-create
- etcd
- controller-manager
- prometheus
- argocd
- hpa
- statefulset
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes Deployment 创建操作指南 是什么
- 如何 Kubernetes Deployment 创建操作指南
trigger_keywords:
- Kubernetes
- Deployment
- 创建操作指南
prerequisites:
- kubectl-basics
- prometheus-basics
- gitops-basics
- etcd-basics
- logging-basics
---



# Kubernetes Deployment 创建操作指南

### 01 Overview

#### 函数签名

```go
func NewDeploymentController(
    dInformer appsinformers.DeploymentInformer,
    rsInformer appsinformers.ReplicaSetInformer,
    podInformer coreinformers.PodInformer,
    client clientset.Interface,
) (*DeploymentController, error)

func (dc *DeploymentController) Run(workers int, stopCh <-chan struct{})

func (dc *DeploymentController) addDeployment(obj interface{})
func (dc *DeploymentController) updateDeployment(oldObj, newObj interface{})
func (dc *DeploymentController) deleteDeployment(obj interface{})
func (dc *DeploymentController) enqueueDeployment(deployment *apps.Deployment)

func startDeploymentController(ctx ControllerContext) (http.Handler, bool, error)
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 控制器入口 | `pkg/controller/deployment/deployment_controller.go` | NewDeploymentController、Run、事件处理 |
| 同步逻辑 | `pkg/controller/deployment/sync.go` | syncDeployment 主协调函数 |
| ReplicaSet 控制器 | `pkg/controller/replicaset/replica_set.go` | RS 核心逻辑、Pod 副本管理 |
| 工具函数 | `pkg/controller/deployment/util/` | RS 查找、hash 计算、状态比较 |
| 启动注册 | `cmd/kube-controller-manager/app/apps.go` | startDeploymentController |
| 滚动更新 | `pkg/controller/deployment/rolling.go` | RollingUpdate 策略 |
| Recreate | `pkg/controller/deployment/recreate.go` | Recreate 策略 |
| 进度追踪 | `pkg/controller/deployment/progress.go` | Status 计算 |

#### NewDeploymentController 参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `dInformer` | `appsinformers.DeploymentInformer` | Deployment Informer，提供 Lister 和事件注册 |
| `rsInformer` | `appsinformers.ReplicaSetInformer` | ReplicaSet Informer，监听 RS 变更 |
| `podInformer` | `coreinformers.PodInformer` | Pod Informer，监听 Pod 删除事件 |
| `client` | `clientset.Interface` | Kubernetes API 客户端 |

---

### 02 Deployment Controller

#### 函数签名

```go
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error

func (dc *DeploymentController) getReplicaSetsForDeployment(ctx context.Context, d *apps.Deployment) ([]*apps.ReplicaSet, error)

func (dc *DeploymentController) getNewReplicaSet(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet, createIfNotExists bool) (*apps.ReplicaSet, error)

func (dc *DeploymentController) getAllReplicaSetsAndSyncRevision(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet, createIfNotExists bool) (*apps.ReplicaSet, []*apps.ReplicaSet, error)

func (dc *DeploymentController) cleanupDeployment(ctx context.Context, key string) error

func GetPodTemplateSpecHash(deployment *apps.Deployment) (string, error)

func FindNewReplicaSet(deployment *apps.Deployment, rsList []*apps.ReplicaSet) *apps.ReplicaSet
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 控制器主控 | `pkg/controller/deployment/deployment_controller.go` | NewDeploymentController、Run、事件处理 |
| 同步逻辑 | `pkg/controller/deployment/sync.go` | syncDeployment、getNewReplicaSet、cleanupOldReplicaSets |
| 工具函数 | `pkg/controller/deployment/util/deployment_util.go` | FindNewReplicaSet、GetPodTemplateSpecHash |
| 回滚逻辑 | `pkg/controller/deployment/rollback.go` | rollback、rollbackToRevision |
| 滚动更新 | `pkg/controller/deployment/rolling.go` | rolloutRolling |

#### syncDeployment 参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `ctx` | `context.Context` | 上下文，用于取消和超时控制 |
| `key` | `string` | 对象 key，格式为 `namespace/name` |

---

### 03 Replicaset Controller

#### 概述

ReplicaSet（RS）是 Deployment 的底层执行器，负责维护指定数量的 Pod 副本。Deployment 控制器决定"要哪个版本、要多少个"，ReplicaSet 控制器则负责"实际创建和删除 Pod"。

---

#### 源码路径

- **ReplicaSet 控制器主控**: `pkg/controller/replicaset/replica_set.go`
- **Pod 管理工具**: `pkg/controller/replicaset/replica_set_utils.go`
- **期望状态计算**: `pkg/controller/controller_utils.go`

---

#### ReplicaSet 控制器架构

```
Deployment Controller
         │
         ▼  创建/更新 ReplicaSet 对象
    ┌─────────────┐
    │  ReplicaSet  │  (存储在 etcd)
    │   Object     │
    └─────────────┘
         │
         ▼  Watch 事件
    ┌─────────────────────────────┐
    │    ReplicaSet Controller     │
    │                              │
    │  1. 获取当前 Pod 数量        │
    │  2. 计算差值 = Replicas - 实际 │
    │  3. 差值 > 0 → 创建 Pod      │
    │  4. 差值 < 0 → 删除 Pod      │
    └─────────────────────────────┘
```

---

---

### 04 Rolling Update

#### 概述

RollingUpdate 是 Deployment 最常用的更新策略。它通过**逐步替换** Pod 来实现零停机更新，核心参数 `maxSurge` 和 `maxUnavailable` 控制替换的速度和可用性保障。本文档基于 `pkg/controller/deployment/rolling.go` 源码，分析滚动更新的完整算法。

---

#### 源码路径

- **滚动更新逻辑**: `pkg/controller/deployment/rolling.go`
- **同步入口**: `pkg/controller/deployment/sync.go`
- **比例缩放工具**: `pkg/controller/deployment/proportion.go`

---

#### 滚动更新策略配置

```yaml
apiVersion: apps/v1
kind: Deployment
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 更新期间可超出期望副本数的比例/数量
      maxUnavailable: 25%  # 更新期间允许不可用的最大比例/数量
```

**参数解析**：

| 参数 | 类型 | 默认值 | 说明 |
|-----|------|-------|------|
| `maxSurge` | int / string | `25%` | 更新期间允许创建的**额外** Pod 数量。可以是绝对数（如 1）或百分比（如 25%） |
| `maxUnavailable` | int / string | `25%` | 更新期间允许**不可用**的 Pod 数量。可以是绝对数或百分比 |

**约束**：
- `maxSurge` 和 `maxUnavailable` 不能同时为 0
- 如果 `replicas = 1`，建议 `maxUnavailable = 0` 且 `maxSurge = 1`，确保始终有 Pod 可用

---

---

### 05 Deployment Status

#### 概述

Deployment 的 `Status` 字段反映了控制器当前执行的真实状态，包括副本数、可用性、进度和条件。控制器每次同步完成后都会更新 Status，这是用户和外部系统（如 HPA、ArgoCD）判断 Deployment 健康度的核心依据。Status 计算逻辑分布在多个源文件中，涉及 ReplicaSet 状态聚合、Condition 推导、超时检测等多个子系统。本文档从源码层面全面分析 Deployment Status 的计算过程、各字段的语义含义以及与外部系统的集成方式。

---

#### 函数签名

```go
func (dc *DeploymentController) syncRolloutStatus(
    ctx context.Context,
    allRSs []*apps.ReplicaSet,
    newRS *apps.ReplicaSet,
    deployment *apps.Deployment,
) error

func (dc *DeploymentController) updateAvailableCondition(
    status *apps.DeploymentStatus,
    d *apps.Deployment,
)

func (dc *DeploymentController) updateProgressingCondition(
    status *apps.DeploymentStatus,
    d *apps.Deployment,
    allRSs []*apps.ReplicaSet,
    newRS *apps.ReplicaSet,
)

func deploymentutil.DeploymentComplete(
    deployment *apps.Deployment,
    newStatus *apps.DeploymentStatus,
) bool

func deploymentutil.GetActualReplicaCountForReplicaSets(
    replicaSets []*apps.ReplicaSet,
) int32

func deploymentutil.GetReadyReplicaCountForReplicaSets(
    replicaSets []*apps.ReplicaSet,
) int32

func deploymentutil.GetAvailableReplicaCountForReplicaSets(
    replicaSets []*apps.ReplicaSet,
    minReadySeconds int32,
) int32
```

---

#### 源码位置

| 功能 | 文件路径 |
|------|---------|
| Status 计算 | `pkg/controller/deployment/progress.go` |
| Status 同步 | `pkg/controller/deployment/sync.go` |
| 工具函数 | `pkg/controller/deployment/util/deployment_util.go` |
| 副本计数 | `pkg/controller/deployment/util/replicaset_util.go` |
| Condition 更新 | `pkg/controller/deployment/condition.go` |

---

---

### 06 Revision History

#### 函数签名

```go
func SetNewReplicaSetAnnotations(deployment *apps.Deployment, newRS *apps.ReplicaSet, newRevision int64) error
func GetRevision(obj metav1.Object) int64
func FindNewReplicaSet(deployment *apps.Deployment, rsList []*apps.ReplicaSet) *apps.ReplicaSet
func (dc *DeploymentController) rollbackToRevision(ctx context.Context, deployment *apps.Deployment, rsList []*apps.ReplicaSet, toRevision int64) (*apps.Deployment, error)
func (dc *DeploymentController) cleanupOldReplicaSets(ctx context.Context, oldRSs []*apps.ReplicaSet, deployment *apps.Deployment) ([]*apps.ReplicaSet, error)
func SortReplicaSetsByRevision(rsList []*apps.ReplicaSet)

```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 回滚逻辑 | `pkg/controller/deployment/rollback.go` | rollbackToRevision |
| 版本管理 | `pkg/controller/deployment/util/deployment_util.go` | Revision/Hash/Annotation |
| 清理逻辑 | `pkg/controller/deployment/sync.go` | cleanupOldReplicaSets |
| kubectl undo | `pkg/kubectl/cmd/rollout/rollout_undo.go` | kubectl rollout undo |
| 滚动更新 | `pkg/controller/deployment/rolling.go` | rolloutRolling |
| 工具函数 | `pkg/controller/deployment/util/revision.go` | Revision 工具 |

#### Revision Annotation

| Annotation Key | 对象 | 说明 |
|---------------|------|------|
| `deployment.kubernetes.io/revision` | Deployment/RS | 版本号 |
| `deployment.kubernetes.io/desired-replicas` | RS | 创建时期望副本数 |
| `deployment.kubernetes.io/max-replicas` | RS | 创建时最大副本数 |
| `kubernetes.io/change-cause` | Deployment | 变更原因 |

---

### Recreate 策略源码分析

#### 函数签名

```go
func (dc *DeploymentController) rolloutRecreate(
    ctx context.Context,
    d *apps.Deployment,
    rsList []*apps.ReplicaSet,
    podMap map[types.UID][]*v1.Pod,
) error
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Recreate 入口 | `pkg/controller/deployment/recreate.go` | rolloutRecreate 主函数 |
| 缩容工具 | `pkg/controller/deployment/util/deployment_util.go` | ScaleDownOldReplicaSets |
| 同步逻辑 | `pkg/controller/deployment/sync.go` | getAllReplicaSetsAndSyncRevision |
| 进度追踪 | `pkg/controller/deployment/progress.go` | syncRolloutStatus |

#### 参数说明

| 参数 | 类型 | 说明 |
|------|------|------|
| `ctx` | `context.Context` | 控制超时与取消 |
| `d` | `*apps.Deployment` | Deployment 对象 |
| `rsList` | `[]*apps.ReplicaSet` | 关联的所有 ReplicaSet 列表 |
| `podMap` | `map[types.UID][]*v1.Pod` | 各 RS 对应的 Pod 列表 |

---

### Deployment 与 HPA 集成源码分析

#### 函数签名

```go
// HPA 控制器核心函数
func (a *HorizontalController) reconcileAutoscaler(
    ctx context.Context,
    hpaSharedInformerFactory informers.SharedInformerFactory,
    hpa *autoscalingv2.HorizontalPodAutoscaler,
    key string,
) error

// Scale 子资源接口 — Deployment 实现
func (r *ScaleREST) Update(
    ctx context.Context,
    name string,
    objInfo rest.UpdatedObjectInfo,
    createValidation rest.ValidateObjectFunc,
    updateValidation rest.ValidateObjectUpdateFunc,
    forceAllowCreate bool,
    options *metav1.UpdateOptions,
) (runtime.Object, bool, error)
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| HPA 控制器 | `pkg/controller/util/horizontal/horizontal.go` | reconcileAutoscaler 主逻辑 |
| Scale 子资源 | `pkg/registry/apps/deployment/storage/storage.go` | Deployment Scale 接口 |
| 副本数计算 | `pkg/controller/util/horizontal/` | normalizeDesiredReplicas |
| 指标适配器 | `pkg/controller/util/horizontal/metrics/` | 多种指标源 |
| HPA 类型定义 | `staging/src/k8s.io/api/autoscaling/v2/types.go` | HPA API 结构 |

#### 架构概述

```
┌────────────────────────────────────────────────────────────────────┐
│                     HPA ↔ Deployment 集成架构                       │
├────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐         ┌─────────────────────┐                  │
│  │ metrics-    │ ◄────── │  HPA Controller      │                  │
│  │ server/     │         │  - 采集当前指标       │                  │
│  │ prometheus  │         │  - 计算 desiredReps  │                  │
│  └─────────────┘         │  - 调用 Scale API    │                  │
│                           └──────────┬──────────┘                  │
│                                      │ /scale subresource           │
│                                      ▼                              │
│  ┌─────────────────────────────────────────────────┐               │
│  │              Deployment                          │               │
│  │  spec.replicas ← 被 HPA 写入                    │               │
│  │                                                  │               │
│  │  注意：Deployment Controller 独立协调 RS 副本    │               │
│  └─────────────────────────────────────────────────┘               │
│                                                                     │
│  关键约束：不要在 HPA 管理的 Deployment 中手动设置 replicas！        │
└────────────────────────────────────────────────────────────────────┘
```

---

### Deployment 金丝雀与蓝绿发布模式

#### 函数签名

```go
// pause/resume 机制 — Deployment Controller 内部处理
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error

// 判断是否暂停
func isPaused(deployment *apps.Deployment) bool {
    return deployment.Spec.Paused
}

// syncRolloutStatus 在 Paused 时仅同步状态，不执行扩缩
func (dc *DeploymentController) sync(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet) error
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| pause 处理 | `pkg/controller/deployment/sync.go` | syncDeployment 中 Paused 分支 |
| 状态条件 | `pkg/controller/deployment/progress.go` | Paused condition 写入 |
| Recreate | `pkg/controller/deployment/recreate.go` | 蓝绿实现基础 |

#### 架构概述

```
┌─────────────────────────────────────────────────────────────────────┐
│              发布策略矩阵                                              │
├───────────────┬────────────────┬────────────────┬───────────────────┤
│ 策略          │ 停机时间       │ 流量控制粒度   │ 实现复杂度        │
├───────────────┼────────────────┼────────────────┼───────────────────┤
│ 原地替换      │ 有             │ 无             │ 低                │
│ (Recreate)    │                │                │                   │
├───────────────┼────────────────┼────────────────┼───────────────────┤
│ 滚动更新      │ 零             │ 副本数比例     │ 低                │
│ (RollingUpdate)│               │                │                   │
├───────────────┼────────────────┼────────────────┼───────────────────┤
│ 金丝雀发布    │ 零             │ 副本数/权重    │ 中                │
│ (Canary)      │                │                │                   │
├───────────────┼────────────────┼────────────────┼───────────────────┤
│ 蓝绿发布      │ 秒级切换       │ Service Selector│ 中               │
│ (Blue-Green)  │                │                │                   │
├───────────────┼────────────────┼────────────────┼───────────────────┤
│ 进阶金丝雀    │ 零             │ HTTP权重/Header│ 高                │
│ (Argo/Flagger)│                │                │                   │
└───────────────┴────────────────┴────────────────┴───────────────────┘
```

---

### Deployment vs StatefulSet vs DaemonSet 选型指南

#### 三大控制器核心对比

| 维度 | Deployment | StatefulSet | DaemonSet |
|------|-----------|-------------|-----------|
| **Pod 身份** | 随机 hash 后缀 | 稳定有序编号（-0, -1, -2） | 节点绑定（每节点一个） |
| **Pod 名称** | `web-7d9f6c-xk9wl` | `db-0`, `db-1`, `db-2` | `fluentd-node1`, `fluentd-node2` |
| **存储** | 共享 PVC 或无状态 | 每 Pod 独立 PVC（VolumeClaimTemplate） | 通常挂载节点本地路径 |
| **网络** | 通过 Service 统一入口 | 每 Pod 独立 DNS（Headless Service） | 每节点独立访问 |
| **启动顺序** | 随机并行 | 严格顺序（0→1→2） | 随节点就绪 |
| **滚动更新** | 自由并行 | 逆序更新（2→1→0） | 节点逐个更新 |
| **扩缩容** | 任意副本数 | 有序扩容/逆序缩容 | 随节点数自动调整 |
| **典型场景** | 无状态微服务 | 数据库、消息队列 | 日志采集、监控代理 |

#### Deployment 控制器架构要点

```go
// 核心：通过 ReplicaSet 管理 Pod，不保证 Pod 身份
// Pod 名称格式：<deployment-name>-<pod-template-hash>-<random-suffix>
// 所有 Pod 可互换，共享同一 PVC（若挂载）

// 源码位置
// pkg/controller/deployment/deployment_controller.go
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error
```

#### Deployment 特征

```yaml
# Pod 名称示例（随机后缀）
web-7d9f6c8f5-xk9wl
web-7d9f6c8f5-pq2rt
web-7d9f6c8f5-mn4vz

# DNS 访问：通过 Service 统一入口
web.production.svc.cluster.local

# 存储：所有 Pod 挂载同一 PVC，或各自独立创建（但 PVC 名称随机）
```

---

### Deployment Create — Kuber

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]
- [[skills/ts-workloads.md|工作负载故障排查]]
- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[argo]] — Argo Workflows

```