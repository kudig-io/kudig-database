# Domain-33: Kubernetes Events 全域事件大全

> **适用版本**: Kubernetes v1.25 - v1.32 | **文档数量**: 15篇 | **最后更新**: 2026-02 | **质量等级**: 专家级

## 领域概述

本领域系统性收录 Kubernetes 生产环境中所有可能出现的事件 (Event)，覆盖核心组件 (kubelet、kube-scheduler、kube-controller-manager)、常见生产插件 (Node Problem Detector、Cluster Autoscaler、cert-manager) 和生态系统组件 (Istio、ArgoCD、Knative 等)。

每个事件包含:
- **事件含义与来源**: 解释事件是什么、由哪个组件在什么条件下触发
- **影响面说明**: 对用户、服务和集群稳定性的影响
- **排查建议**: 具体的 kubectl 命令和诊断步骤
- **解决建议**: 按常见原因和优先级排列的解决方案
- **版本适配范围**: 精确标注每个事件在各 Kubernetes 版本中的支持情况

---

## 文档目录

### 基础架构 (01)

| 编号 | 文档 | 关键内容 | 事件数 | 重要程度 |
|:---:|:---|:---|:---:|:---:|
| 01 | [事件系统架构与 API 参考](./01-event-system-architecture.md) | Event 数据模型、API 版本演进、事件生命周期、持久化方案 | - | 必读 |

### 核心组件事件 (02-06)

| 编号 | 文档 | 关键内容 | 事件数 | 重要程度 |
|:---:|:---|:---|:---:|:---:|
| 02 | [Pod 与容器生命周期事件](./02-pod-container-lifecycle-events.md) | Created、Started、Killing、BackOff、FailedSync、Evicted 等 | ~18 | 必读 |
| 03 | [镜像拉取事件](./03-image-pull-events.md) | Pulling、Pulled、ErrImagePull、ImagePullBackOff 等 | ~7 | 必读 |
| 04 | [探针与健康检查事件](./04-probe-health-check-events.md) | Unhealthy (Liveness/Readiness/Startup)、ProbeWarning | ~4 | 必读 |
| 05 | [调度与抢占事件](./05-scheduling-preemption-events.md) | Scheduled、FailedScheduling、Preempted、WaitingForGates | ~6 | 必读 |
| 06 | [节点生命周期与状态事件](./06-node-lifecycle-condition-events.md) | NodeReady、DiskPressure、MemoryPressure、Rebooted、Eviction 等 | ~23 | 必读 |

### 控制器事件 (07-09)

| 编号 | 文档 | 关键内容 | 事件数 | 重要程度 |
|:---:|:---|:---|:---:|:---:|
| 07 | [Deployment 与 ReplicaSet 事件](./07-deployment-replicaset-events.md) | ScalingReplicaSet、ProgressDeadlineExceeded、FailedCreate 等 | ~16 | 必读 |
| 08 | [StatefulSet 与 DaemonSet 事件](./08-statefulset-daemonset-events.md) | 有序创建/删除、FailedPlacement、UnhealthyPodEviction 等 | ~13 | 重要 |
| 09 | [Job 与 CronJob 批处理事件](./09-job-cronjob-batch-events.md) | Completed、BackoffLimitExceeded、TooManyMissedTimes 等 | ~20 | 重要 |

### 网络与存储事件 (10-11)

| 编号 | 文档 | 关键内容 | 事件数 | 重要程度 |
|:---:|:---|:---|:---:|:---:|
| 10 | [Service 与网络事件](./10-service-networking-events.md) | LoadBalancer 生命周期、EndpointSlice、HostPortConflict 等 | ~21 | 必读 |
| 11 | [存储与卷事件](./11-storage-volume-events.md) | FailedMount、ProvisioningFailed、VolumeResize 等 | ~22 | 必读 |

### 扩展与运维事件 (12-15)

| 编号 | 文档 | 关键内容 | 事件数 | 重要程度 |
|:---:|:---|:---|:---:|:---:|
| 12 | [自动扩缩容事件](./12-autoscaling-events.md) | HPA SuccessfulRescale、VPA、Cluster Autoscaler 等 | ~28 | 重要 |
| 13 | [安全、准入与 RBAC 事件](./13-security-admission-rbac-events.md) | 证书审批、Admission Webhook、Pod Security 等 | ~14 | 重要 |
| 14 | [Namespace、资源管理与 GC 事件](./14-namespace-resource-gc-events.md) | Namespace 删除、ResourceQuota、PDB 等 | ~15 | 参考 |
| 15 | [生态系统与插件事件](./15-ecosystem-addon-events.md) | NPD、Ingress、cert-manager、Istio、ArgoCD、Velero 等 | ~56 | 参考 |

---

## 学习路径建议

### 初学者路径

建议按以下顺序学习，先理解事件系统，再逐步深入:

```
01-事件系统架构 → 02-Pod容器事件 → 03-镜像事件 → 04-探针事件
       ↓
05-调度事件 → 06-节点事件 → 07-Deployment事件
```

### 运维工程师路径

关注生产环境高频故障相关事件:

```
02-Pod容器事件(BackOff/Evicted) → 03-镜像事件(ImagePullBackOff)
       ↓
05-调度事件(FailedScheduling) → 06-节点事件(NotReady/DiskPressure)
       ↓
11-存储事件(FailedMount) → 07-Deployment事件(ProgressDeadlineExceeded)
```

### SRE / 平台工程师路径

关注全局监控和自动化:

```
01-事件系统架构(持久化/监控) → 12-自动扩缩容事件 → 13-安全事件
       ↓
10-网络事件(LoadBalancer) → 14-资源管理事件(Quota/PDB)
       ↓
15-生态插件事件(NPD/Prometheus/Velero)
```

---

## 事件速查索引

按 Event Reason 字母排序，快速定位事件所在文档:

| Event Reason | Type | 来源 | 文档 |
|:---|:---|:---|:---|
| `AbleToScale` | Normal | HPA | [12](./12-autoscaling-events.md) |
| `AlreadyPresent` | Normal | kubelet | [03](./03-image-pull-events.md) |
| `BackOff` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `BackOff` (ImagePull) | Warning | kubelet | [03](./03-image-pull-events.md) |
| `BackoffLimitExceeded` | Warning | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `CalculateExpectedPodCountFailed` | Warning | disruption-controller | [14](./14-namespace-resource-gc-events.md) |
| `ClusterRoleUpdated` | Normal | clusterrole-aggregation | [13](./13-security-admission-rbac-events.md) |
| `Completed` | Normal | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `ContainerGCFailed` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `Created` | Normal | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `CreatedLoadBalancer` | Normal | service-controller | [10](./10-service-networking-events.md) |
| `DeadlineExceeded` | Warning | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `DeletedLoadBalancer` | Normal | service-controller | [10](./10-service-networking-events.md) |
| `DeletedTokenSecret` | Normal | token-controller | [13](./13-security-admission-rbac-events.md) |
| `DeletingAllPods` | Normal | node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `DeletingDependents` | Normal | gc-controller | [14](./14-namespace-resource-gc-events.md) |
| `DeletingNode` | Normal | node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `DeploymentPaused` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `DeploymentResumed` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `DeploymentRollback` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `DesiredReplicasComputed` | Normal | HPA | [12](./12-autoscaling-events.md) |
| `DisruptionAllowed` | Normal | disruption-controller | [14](./14-namespace-resource-gc-events.md) |
| `DNSConfigForming` | Warning | kubelet | [10](./10-service-networking-events.md) |
| `EnsuredLoadBalancer` | Normal | service-controller | [10](./10-service-networking-events.md) |
| `ErrImageNeverPull` | Warning | kubelet | [03](./03-image-pull-events.md) |
| `Evicted` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `EvictionThresholdMet` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `ExceededGracePeriod` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `ExternalProvisioning` | Normal | pv-controller | [11](./11-storage-volume-events.md) |
| `Failed` (image) | Warning | kubelet | [03](./03-image-pull-events.md) |
| `Failed` (container) | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FailedAdmission` | Warning | admission | [13](./13-security-admission-rbac-events.md) |
| `FailedAttachVolume` | Warning | kubelet | [11](./11-storage-volume-events.md) |
| `FailedBinding` | Warning | pv-controller | [11](./11-storage-volume-events.md) |
| `FailedCreate` (RS) | Warning | replicaset-controller | [07](./07-deployment-replicaset-events.md) |
| `FailedCreate` (STS) | Warning | statefulset-controller | [08](./08-statefulset-daemonset-events.md) |
| `FailedCreate` (DS) | Warning | daemonset-controller | [08](./08-statefulset-daemonset-events.md) |
| `FailedCreate` (Job) | Warning | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `FailedCreate` (CronJob) | Warning | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `FailedCreatePodSandBox` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FailedDaemonPod` | Warning | daemonset-controller | [08](./08-statefulset-daemonset-events.md) |
| `FailedGetResourceMetric` | Warning | HPA | [12](./12-autoscaling-events.md) |
| `FailedMount` | Warning | kubelet | [11](./11-storage-volume-events.md) |
| `FailedPlacement` | Warning | daemonset-controller | [08](./08-statefulset-daemonset-events.md) |
| `FailedPostStartHook` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FailedPreStopHook` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FailedRescale` | Warning | HPA | [12](./12-autoscaling-events.md) |
| `FailedScheduling` | Warning | default-scheduler | [05](./05-scheduling-preemption-events.md) |
| `FailedSync` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FailedToCreateEndpoint` | Warning | endpoint-controller | [10](./10-service-networking-events.md) |
| `FailedToCreateEndpointSlice` | Warning | endpointslice-controller | [10](./10-service-networking-events.md) |
| `FailedValidation` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `FileSystemResizeFailed` | Warning | kubelet | [11](./11-storage-volume-events.md) |
| `FileSystemResizeSuccessful` | Normal | kubelet | [11](./11-storage-volume-events.md) |
| `ForbidConcurrent` | Warning | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `FreeDiskSpaceFailed` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `HostPortConflict` | Warning | kubelet | [10](./10-service-networking-events.md) |
| `ImageGCFailed` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `InspectFailed` | Warning | kubelet | [03](./03-image-pull-events.md) |
| `InsufficientBudget` | Warning | disruption-controller | [14](./14-namespace-resource-gc-events.md) |
| `InvalidDiskCapacity` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `Killing` | Normal | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `MinimumReplicasAvailable` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `MinimumReplicasUnavailable` | Warning | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `MissingJob` | Normal | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `NamespaceContentRemaining` | Normal | namespace-controller | [14](./14-namespace-resource-gc-events.md) |
| `NamespaceDeletionContentFailure` | Warning | namespace-controller | [14](./14-namespace-resource-gc-events.md) |
| `NamespaceFinalizersRemaining` | Normal | namespace-controller | [14](./14-namespace-resource-gc-events.md) |
| `NetworkNotReady` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `NewReplicaSetAvailable` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `NodeAllocatableEnforced` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasDiskPressure` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasInsufficientMemory` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasInsufficientPID` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasNoDiskPressure` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasSufficientMemory` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeHasSufficientPID` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeNotReady` | Warning | kubelet/node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `NodeNotSchedulable` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeReady` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NodeSchedulable` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `NoPods` | Warning | disruption-controller | [14](./14-namespace-resource-gc-events.md) |
| `Preempted` | Normal | default-scheduler | [05](./05-scheduling-preemption-events.md) |
| `Preempting` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `ProgressDeadlineExceeded` | Warning | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `ProbeWarning` | Warning | kubelet | [04](./04-probe-health-check-events.md) |
| `ProvisioningFailed` | Warning | pv-controller | [11](./11-storage-volume-events.md) |
| `ProvisioningSucceeded` | Normal | pv-controller | [11](./11-storage-volume-events.md) |
| `Pulled` | Normal | kubelet | [03](./03-image-pull-events.md) |
| `Pulling` | Normal | kubelet | [03](./03-image-pull-events.md) |
| `Rebooted` | Warning | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `RegisteredNode` | Normal | node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `RemovingNode` | Normal | node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `Resumed` | Normal | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `SandboxChanged` | Normal | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `SawCompletedJob` | Normal | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `ScaledUpGroup` | Normal | cluster-autoscaler | [12](./12-autoscaling-events.md) |
| `ScaleDown` | Normal | cluster-autoscaler | [12](./12-autoscaling-events.md) |
| `ScaleDownFailed` | Warning | cluster-autoscaler | [12](./12-autoscaling-events.md) |
| `ScalingReplicaSet` | Normal | deployment-controller | [07](./07-deployment-replicaset-events.md) |
| `Scheduled` | Normal | default-scheduler | [05](./05-scheduling-preemption-events.md) |
| `Started` | Normal | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `Starting` | Normal | kubelet | [06](./06-node-lifecycle-condition-events.md) |
| `SuccessfulAttachVolume` | Normal | kubelet | [11](./11-storage-volume-events.md) |
| `SuccessfulCreate` (RS) | Normal | replicaset-controller | [07](./07-deployment-replicaset-events.md) |
| `SuccessfulCreate` (STS) | Normal | statefulset-controller | [08](./08-statefulset-daemonset-events.md) |
| `SuccessfulDelete` (RS) | Normal | replicaset-controller | [07](./07-deployment-replicaset-events.md) |
| `SuccessfulMountVolume` | Normal | kubelet | [11](./11-storage-volume-events.md) |
| `SuccessfulRescale` | Normal | HPA | [12](./12-autoscaling-events.md) |
| `Suspended` | Normal | job-controller | [09](./09-job-cronjob-batch-events.md) |
| `TaintManagerEviction` | Normal | node-controller | [05](./05-scheduling-preemption-events.md) |
| `TerminatingEvictedPod` | Normal | node-controller | [06](./06-node-lifecycle-condition-events.md) |
| `TooManyMissedTimes` | Warning | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `TopologyAffinityError` | Warning | kubelet | [02](./02-pod-container-lifecycle-events.md) |
| `TriggeredScaleUp` | Normal | cluster-autoscaler | [12](./12-autoscaling-events.md) |
| `Unhealthy` | Warning | kubelet | [04](./04-probe-health-check-events.md) |
| `UnexpectedJob` | Warning | cronjob-controller | [09](./09-job-cronjob-batch-events.md) |
| `UpdateLoadBalancerFailed` | Warning | service-controller | [10](./10-service-networking-events.md) |
| `VolumeDeleted` | Normal | pv-controller | [11](./11-storage-volume-events.md) |
| `VolumeFailedDelete` | Warning | pv-controller | [11](./11-storage-volume-events.md) |
| `VolumeResizeFailed` | Warning | kubelet | [11](./11-storage-volume-events.md) |
| `VolumeResizeSuccessful` | Normal | kubelet | [11](./11-storage-volume-events.md) |
| `WaitForFirstConsumer` | Normal | pv-controller | [11](./11-storage-volume-events.md) |
| `WaitingForGates` | Normal | default-scheduler | [05](./05-scheduling-preemption-events.md) |

> 生态系统插件事件 (NPD、Ingress Controller、cert-manager、Istio、ArgoCD、Knative、Prometheus Operator、Velero、External DNS、MetalLB) 的 56+ 个事件详见 [15-ecosystem-addon-events.md](./15-ecosystem-addon-events.md)

---

## 统计信息

| 分类 | 文档数 | 事件数 |
|:---|:---:|:---:|
| 事件系统架构 | 1 | - |
| 核心组件事件 (kubelet/scheduler) | 5 | ~78 |
| 控制器事件 (Deployment/STS/DS/Job) | 3 | ~49 |
| 网络与存储事件 | 2 | ~43 |
| 扩展与运维事件 | 4 | ~113 |
| **总计** | **15** | **~230+** |

---

## 相关领域

| 领域 | 关联说明 |
|:---|:---|
| [Domain-4: 工作负载与调度](../domain-4-workloads/) | Pod 生命周期、容器状态 |
| [Domain-5: 网络](../domain-5-networking/) | Service、Ingress、DNS |
| [Domain-6: 存储](../domain-6-storage/) | PV/PVC、StorageClass、CSI |
| [Domain-8: 可观测性](../domain-8-observability/) | 事件监控、审计日志 |
| [Domain-12: 故障排查](../domain-12-troubleshooting/) | 各组件故障排查指南 |
| [Domain-18: 生产运维](../domain-18-production-operations/) | 生产环境运维实践 |
| [Domain-20: 企业监控告警](../domain-20-enterprise-monitoring-alerting/) | Prometheus、告警规则 |

---

**维护者**: Allen Galler | **许可证**: MIT

*最后更新: 2026年2月*
