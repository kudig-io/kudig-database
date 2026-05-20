---
title: Kubernetes v1.25 - v1.33 特性对比总表
description: '## 二、网络 (Networking)'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- containerd
- hpa
- pdb
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 10min
intent_queries:
- Kubernetes v1.25 - v1.33 特性对比总表 是什么
- 如何 Kubernetes v1.25 - v1.33 特性对比总表
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.25
- v1.33
- 特性对比总表
- architecture
- fundamentals
cross_refs:
- type: domain
  path: ../domain-13-docker/
  label: '相关知识域: domain-13-docker'
- type: domain
  path: ../domain-2-design-principles/
  label: '相关知识域: domain-2-design-principles'
- type: cheatsheet
  path: ../topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---


# Kubernetes v1.25 - v1.33 特性对比总表

> **适用版本**: Kubernetes v1.25 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 全版本特性横向对比，快速定位功能引入版本

---

## 一、工作负载 (Workloads)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Sidecar 容器** | - | - | Alpha | Beta | Beta | GA | GA | GA | **GA** | init 容器 restartPolicy: Always |
| **ReadWriteOncePod** | - | - | - | - | **GA** | GA | GA | GA | GA | PVC 单 Pod 独占 |
| **Pod Scheduling Readiness** | - | - | - | - | Beta | **GA** | GA | GA | GA | SchedulingGates |
| **In-Place Pod Resize** | - | - | - | - | Beta | Beta | Beta | Beta | **Alpha** | 原地调整资源 |
| **PodIndexLabel** | - | - | - | - | - | - | - | - | **GA** | StatefulSet 自动标签 |
| **Job Mutable Scheduling Directives** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **PodDisruptionBudget (v1)** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **CronJob 时区支持** | - | - | - | - | - | - | - | - | - | v1.25+ 已稳定 |
| **Job Tracking with Finalizers** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 二、网络 (Networking)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Gateway API (v1)** | - | - | - | - | - | - | **GA** | GA | GA | Ingress 替代方案 |
| **nftables kube-proxy** | - | - | - | - | - | - | Alpha | Alpha | **Beta** | 新网络后端 |
| **IPv6 DualStack** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 双栈网络 |
| **EndpointSlice (v1)** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 大规模 Service |
| **Service Traffic Distribution** | - | - | - | - | - | - | Alpha | Alpha | Alpha | 拓扑感知路由 |
| **Network Policy Status** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 三、存储 (Storage)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **ReadWriteOncePod** | - | - | - | - | **GA** | GA | GA | GA | GA | 单 Pod 独占 |
| **CSI Migration (in-tree)** | - | - | - | - | - | 弃用 | 弃用 | 弃用 | 弃用 | 迁移到 CSI |
| **VolumeGroupSnapshot** | - | - | Beta | Beta | Beta | Beta | Beta | Beta | Beta | 卷组快照 |
| **VolumeAttributesClass** | - | - | - | - | - | - | - | - | **Alpha** | 动态存储性能 |
| **Cross-Namespace PVC** | - | - | - | - | - | - | - | - | **Alpha** | 跨命名空间引用 |
| **PV Last Phase Time** | - | - | - | - | - | - | **GA** | GA | GA | 状态转换时间 |
| **Retroactive Default SC** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 四、安全 (Security)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Pod Security Admission** | **GA** | GA | GA | GA | GA | GA | GA | GA | GA | PSP 替代 |
| **PodSecurityPolicy 移除** | **移除** | - | - | - | - | - | - | - | - | 已移除 |
| **ValidatingAdmissionPolicy** | - | Alpha | Beta | Beta | Beta | **GA** | GA | GA | GA | CEL 准入 |
| **BoundServiceAccountToken** | - | - | - | - | - | **GA** | GA | GA | GA | 1h 过期 |
| **AppArmor Support** | - | - | - | - | - | - | **GA** | GA | GA | Linux 安全 |
| **User Namespaces** | Alpha | Alpha | Beta | Beta | Beta | Beta | **GA** | GA | GA | 用户隔离 |
| **匿名用户安全加固** | - | - | - | - | - | **默认** | 默认 | 默认 | 默认 | 禁止匿名 cluster-admin |
| **KMS v2** | - | - | - | - | **GA** | GA | GA | GA | GA | etcd 加密 |
| **SELinux Mount** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 五、调度 (Scheduling)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **DRA (Dynamic Resource Allocation)** | Alpha | Alpha | Alpha | Beta | Beta | Beta | Beta | **Beta** | **GA** | GPU/FPGA 分配 |
| **TopologyManager Per Pod** | - | - | - | - | - | - | - | **Beta** | **GA** | NUMA 拓扑 |
| **Scheduler Queueing Hints** | - | - | - | - | - | - | - | Alpha | **Beta** | 队列优化 |
| **Pod Scheduling Readiness** | - | - | - | - | Beta | **GA** | GA | GA | GA | 调度门控 |
| **MatchLabelKeys in PDB** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **MinDomains in PodTopologySpread** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 六、可观测性 (Observability)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **OpenTelemetry Tracing (kubelet)** | - | - | - | - | - | - | **GA** | GA | GA | 链路追踪 |
| **Kubelet Resource Metrics** | - | - | - | - | - | - | - | - | **Beta** | 资源指标端点 |
| **Node Log Query** | - | - | - | - | - | Alpha | Alpha | Alpha | Alpha | 节点日志查询 |
| **Component SLIs** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **PodAndContainerStatsFromCRI** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 七、节点/运行时 (Node/Runtime)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Graceful Node Shutdown** | Beta | **GA** | GA | GA | GA | GA | GA | GA | GA | 优雅关机 |
| **Parallel Image Pulls** | - | - | - | - | - | - | **默认启用** | 默认 | 默认 | 并行拉取 |
| **Swap Support** | Alpha | Alpha | Alpha | Beta | Beta | Beta | Beta | Beta | Beta | 内存交换 |
| **User Namespaces** | Alpha | Alpha | Beta | Beta | Beta | Beta | **GA** | GA | GA | 用户隔离 |
| **Node Volume Health** | - | - | - | - | **GA** | GA | GA | GA | GA | 存储健康监测 |
| **Kubelet OpenTelemetry** | - | - | - | - | - | - | **GA** | GA | GA | 链路追踪 |
| **Kubelet Resource Metrics** | - | - | - | - | - | - | - | - | **Beta** | 资源指标 |
| **In-Place Pod Resize** | - | - | - | - | Beta | Beta | Beta | Beta | **Alpha** | 原地调整 |
| **containerd 1.7+** | - | - | - | - | - | - | - | - | - | 推荐运行时 |

---

## 八、控制平面 (Control Plane)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **API Priority & Fairness (v1)** | - | **GA** | GA | GA | GA | GA | GA | GA | GA | 请求优先级 |
| **Server-side Apply** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 声明式管理 |
| **ValidatingAdmissionPolicy** | - | Alpha | Beta | Beta | Beta | **GA** | GA | GA | GA | CEL 准入 |
| **API Server Tracing** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **Aggregated Discovery** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **Storage Version API** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

## 九、API 废弃与移除

| API/功能 | 废弃版本 | 移除版本 | 替代方案 |
|:---|:---|:---|:---|
| PodSecurityPolicy | v1.21 | **v1.25** | Pod Security Admission |
| CronJob v1beta1 | v1.21 | **v1.25** | batch/v1 |
| EndpointSlice v1beta1 | v1.21 | **v1.25** | discovery.k8s.io/v1 |
| Event v1beta1 | v1.19 | **v1.25** | events.k8s.io/v1 |
| HPA v2beta1 | v1.19 | **v1.25** | autoscaling/v2 |
| PDB v1beta1 | v1.21 | **v1.25** | policy/v1 |
| RuntimeClass v1beta1 | v1.22 | **v1.25** | node.k8s.io/v1 |
| FlowSchema v1beta1 | v1.26 | **v1.26** | flowcontrol/v1 |
| PriorityLevelConfiguration v1beta1 | v1.26 | **v1.26** | flowcontrol/v1 |
| CSIStorageCapacity v1beta1 | v1.24 | **v1.27** | storage.k8s.io/v1 |
| FlowSchema v1beta2 | v1.26 | **v1.29** | flowcontrol/v1 |
| Node v1beta1 metrics | v1.29 | 预计 v1.34+ | metrics/v1 |
| in-tree storage drivers | v1.30 | 预计 v1.35+ | CSI 驱动 |
| kubelet --cloud-provider | v1.31 | 预计 v1.35+ | 外部 CCM |

---

## 十、Feature Gate 状态总览

| Feature Gate | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---|
| SidecarContainers | Beta | GA | GA | GA | **GA** | 原生 Sidecar |
| DynamicResourceAllocation | Beta | Beta | Beta | Beta | **GA** | DRA |
| InPlacePodVerticalScaling | Beta | Beta | Beta | Beta | **Alpha** | 原地调整 |
| NFTablesProxyMode | - | - | Alpha | Alpha | **Beta** | nftables |
| SchedulerQueueingHints | - | - | - | Alpha | **Beta** | 队列提示 |
| KubeletResourceMetrics | - | - | - | - | **Beta** | 资源指标 |
| CrossNamespaceVolumeDataSource | - | - | - | - | **Alpha** | 跨 NS 存储 |
| NodeLogQuery | - | Alpha | Alpha | Alpha | **Alpha** | 节点日志 |
| PodLevelResources | - | - | - | - | **Alpha** | Pod 级资源 |

---

## 快速参考

```bash
# 检查当前版本
kubectl version

# 查看所有 Feature Gates
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | jq '.kubeletconfig.featureGates'

# 检查已弃用 API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 查看支持的 API 版本
kubectl api-versions

# 检查 PSP (已移除 v1.25)
kubectl get psp 2>/dev/null || echo "PSP 已移除"

# 检查 CSI 驱动
kubectl get csidrivers
```

---

## 参考链接

- [K8s Feature Gates](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [K8s 版本发布](https://kubernetes.io/releases/)
- [K8s API 变更](https://kubernetes.io/docs/reference/using-api/deprecation-guide/)
