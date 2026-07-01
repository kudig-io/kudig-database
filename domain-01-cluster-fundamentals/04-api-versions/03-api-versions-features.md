---
title: 03 - 功能和API表
description: '| **Pod** | core/v1 | Pod | Stable | v1.0 | v1.0 | - | - | 不直接创建，使用控制器管理；设置资源requests/limits
  |'
summary: '| **Pod** | core/v1 | Pod | Stable | v1.0 | v1.0 | - | - | 不直接创建，使用控制器管理；设置资源requests/limits
  |'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- cilium
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 10min
intent_queries:
- 功能和API表 是什么
- 如何 功能和API表
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- 功能和API表
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- observability-basics
k8s_versions:
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
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---



# 03 - 功能和API表

> **适用版本**: v1.25 - v1.33 | **最后更新**: 2026-04-24 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/reference/kubernetes-api](https://kubernetes.io/docs/reference/kubernetes-api/)

<!-- chunk: 核心工作负载API -->
## 核心工作负载API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **Pod** | core/v1 | Pod | Stable | v1.0 | v1.0 | - | - | 不直接创建，使用控制器管理；设置资源requests/limits |
| **[[ReplicaSet|ReplicaSet]]** | apps/v1 | ReplicaSet | Stable | v1.2 | v1.9 | - | - | 不直接使用，由Deployment管理 |
| **Deployment** | apps/v1 | Deployment | Stable | v1.2 | v1.9 | - | - | 无状态应用首选；配置滚动更新策略 |
| **[[StatefulSet|StatefulSet]]** | apps/v1 | StatefulSet | Stable | v1.5 | v1.9 | - | - | 有状态应用；需配合Headless [[Service|Service]] |
| **DaemonSet** | apps/v1 | DaemonSet | Stable | v1.2 | v1.9 | - | - | 节点级守护进程；使用nodeSelector精确控制 |
| **Job** | batch/v1 | Job | Stable | v1.2 | v1.9 | - | - | 批处理任务；配置backoffLimit和activeDeadlineSeconds |
| **CronJob** | batch/v1 | CronJob | Stable | v1.4 | v1.21 | - | - | 定时任务；注意时区设置(v1.25+支持) |
| **ReplicationController** | core/v1 | ReplicationController | Stable | v1.0 | v1.0 | v1.9 | - | **已弃用**，使用Deployment替代 |

<!-- chunk: 服务发现与网络API -->
## 服务发现与网络API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **Service** | core/v1 | Service | Stable | v1.0 | v1.0 | - | - | ClusterIP默认；大集群考虑Headless |
| **Endpoints** | core/v1 | Endpoints | Stable | v1.0 | v1.0 | - | - | 自动管理；大规模使用EndpointSlice |
| **EndpointSlice** | discovery.k8s.io/v1 | EndpointSlice | Stable | v1.16 | v1.21 | - | - | 大规模Service必用；自动创建 |
| **Ingress** | networking.k8s.io/v1 | Ingress | Stable | v1.1 | v1.19 | - | - | HTTP(S)路由；需安装Ingress Controller |
| **IngressClass** | networking.k8s.io/v1 | IngressClass | Stable | v1.18 | v1.19 | - | - | 多Ingress控制器时必需 |
| **NetworkPolicy** | networking.k8s.io/v1 | NetworkPolicy | Stable | v1.3 | v1.7 | - | - | 网络隔离；需CNI支持(Calico/Cilium) |
| **Gateway** | gateway.networking.k8s.io/v1 | Gateway | Stable | v1.24 | v1.31 | - | - | Ingress替代方案；更强大的路由能力 |
| **HTTPRoute** | gateway.networking.k8s.io/v1 | HTTPRoute | Stable | v1.24 | v1.31 | - | - | Gateway API的HTTP路由规则 |

<!-- chunk: 配置与存储API -->
## 配置与存储API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **ConfigMap** | core/v1 | ConfigMap | Stable | v1.2 | v1.2 | - | - | 非敏感配置；immutable字段防误改(v1.21+) |
| **Secret** | core/v1 | Secret | Stable | v1.0 | v1.0 | - | - | 敏感数据；启用etcd加密；考虑外部Secret管理 |
| **PersistentVolume** | core/v1 | PersistentVolume | Stable | v1.0 | v1.0 | - | - | 集群级存储资源；使用StorageClass动态供应 |
| **PersistentVolumeClaim** | core/v1 | PersistentVolumeClaim | Stable | v1.0 | v1.0 | - | - | Pod存储请求；注意accessModes兼容性 |
| **StorageClass** | storage.k8s.io/v1 | StorageClass | Stable | v1.4 | v1.6 | - | - | 动态供应必需；设置默认StorageClass |
| **CSIDriver** | storage.k8s.io/v1 | CSIDriver | Stable | v1.12 | v1.18 | - | - | CSI驱动注册；了解驱动能力 |
| **VolumeSnapshot** | snapshot.storage.k8s.io/v1 | VolumeSnapshot | Stable | v1.12 | v1.20 | - | - | 存储快照；需CSI驱动支持 |

<!-- chunk: 扩展与自定义API -->
## 扩展与自定义API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **CustomResourceDefinition** | apiextensions.k8s.io/v1 | CRD | Stable | v1.7 | v1.16 | - | - | 扩展K8S API；配置验证schema |
| **MutatingWebhookConfiguration** | admissionregistration.k8s.io/v1 | - | Stable | v1.9 | v1.16 | - | - | 动态修改资源；注意超时设置 |
| **ValidatingWebhookConfiguration** | admissionregistration.k8s.io/v1 | - | Stable | v1.9 | v1.16 | - | - | 验证准入控制；failurePolicy设置 |
| **ValidatingAdmissionPolicy** | admissionregistration.k8s.io/v1 | - | Stable | v1.26 | v1.30 | - | - | CEL表达式验证；替代Webhook |
| **APIService** | apiregistration.k8s.io/v1 | APIService | Stable | v1.7 | v1.10 | - | - | API聚合层；Metrics Server使用 |

<!-- chunk: 安全与访问控制API -->
## 安全与访问控制API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **ServiceAccount** | core/v1 | ServiceAccount | Stable | v1.0 | v1.0 | - | - | Pod身份；使用专用SA避免default |
| **Role** | rbac.authorization.k8s.io/v1 | Role | Stable | v1.6 | v1.8 | - | - | 命名空间级权限 |
| **ClusterRole** | rbac.authorization.k8s.io/v1 | ClusterRole | Stable | v1.6 | v1.8 | - | - | 集群级权限；聚合规则 |
| **RoleBinding** | rbac.authorization.k8s.io/v1 | RoleBinding | Stable | v1.6 | v1.8 | - | - | 绑定Role到用户/组/SA |
| **ClusterRoleBinding** | rbac.authorization.k8s.io/v1 | ClusterRoleBinding | Stable | v1.6 | v1.8 | - | - | 集群级绑定；谨慎授予 |
| **PodSecurityPolicy** | policy/v1beta1 | PSP | **已移除** | v1.3 | - | v1.21 | **v1.25** | 已移除！迁移到Pod Security Admission |
| **PodDisruptionBudget** | policy/v1 | PDB | Stable | v1.4 | v1.21 | - | - | 保护工作负载；设置minAvailable |

<!-- chunk: 自动扩缩容API -->
## 自动扩缩容API

| 功能名称 | API组/版本 | Kind | 稳定性 | 引入版本 | 稳定版本 | 弃用版本 | 移除版本 | 生产使用提示 |
|---------|-----------|------|-------|---------|---------|---------|---------|-------------|
| **HorizontalPodAutoscaler** | autoscaling/v2 | HPA | Stable | v1.1 | v1.23 | - | - | 自动扩缩Pod数；v2支持自定义指标 |
| **VerticalPodAutoscaler** | autoscaling.k8s.io/v1 | VPA | Stable | 外部项目 | - | - | - | 自动调整资源；注意与HPA冲突 |
| **PodAutoscaler** | autoscaling.k8s.io/v1alpha1 | - | Alpha | v1.27 | - | - | - | 多维度扩缩(实验性) |

<!-- chunk: API版本演进重要变更 -->
## API版本演进重要变更

| 版本 | API变更 | 影响资源 | 迁移操作 | 工具命令 |
|-----|--------|---------|---------|---------|
| **v1.16** | extensions/v1beta1弃用 | Deployment, DaemonSet, ReplicaSet | 更新apiVersion到apps/v1 | `kubectl convert` |
| **v1.16** | CRD v1稳定 | CustomResourceDefinition | 迁移schema到OpenAPI v3 | 手动更新 |
| **v1.19** | Ingress v1稳定 | Ingress | 更新apiVersion到networking.k8s.io/v1 | `kubectl convert` |
| **v1.21** | CronJob v1稳定 | CronJob | 更新apiVersion到batch/v1 | 自动 |
| **v1.22** | 移除多个beta API | Ingress, CRD等 | 必须使用v1 API | 检查并更新YAML |
| **v1.25** | 移除PodSecurityPolicy | 安全策略 | 迁移到Pod Security Admission | 重新设计安全策略 |
| **v1.26** | FlowSchema v1稳定 | API优先级 | 可选升级 | - |
| **v1.29** | 移除flowcontrol.apiserver.k8s.io/v1beta2 | FlowSchema | 升级到v1 | kubectl convert |
| **v1.29** | 弃用Node v1beta1 metrics | metrics | 升级到v1 | 更新监控查询 |
| **v1.30** | 弃用in-tree storage drivers | 存储驱动 | 迁移到CSI | 安装CSI驱动 |
| **v1.31** | 弃用kubelet --cloud-provider flag | kubelet配置 | 使用外部云控制器 | 更新kubelet配置 |
| **v1.32** | 多个Beta API升级 | 多个 | 检查变更日志 | - |
| **v1.33** | Sidecar Containers GA | Pod Spec | 无需操作(自动启用) | - |

<!-- chunk: 功能门控(Feature Gates)状态 -->
## 功能门控(Feature Gates)状态

| 功能名称 | 功能门控 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|---------|---------|-------|-------|-------|-------|-------|------|
| **Pod Security Admission** | PodSecurity | GA | GA | GA | GA | GA | PSP替代方案 |
| **Ephemeral Containers** | EphemeralContainers | GA | GA | GA | GA | GA | 调试容器 |
| **Server-side Apply** | ServerSideApply | GA | GA | GA | GA | GA | 声明式管理 |
| **IPv6 DualStack** | IPv6DualStack | GA | GA | GA | GA | GA | 双栈网络 |
| **Graceful Node Shutdown** | GracefulNodeShutdown | GA | GA | GA | GA | GA | 优雅关机 |
| **Sidecar Containers** | SidecarContainers | Beta | GA | GA | GA | **GA** | Sidecar原生生命周期管理 |
| **CEL Admission** | ValidatingAdmissionPolicy | Beta | **GA** | GA | GA | GA | CEL表达式验证策略 |
| **User Namespaces** | UserNamespacesSupport | Beta | Beta | **GA** | GA | GA | 用户命名空间隔离 |
| **Dynamic Resource Allocation** | DynamicResourceAllocation | Beta | Beta | Beta | **Beta** | **GA** | GPU/FPGA动态资源分配 |
| **In-Place Pod Resize** | InPlacePodVerticalScaling | Beta | Beta | Beta | Beta | **Alpha** | 原地调整Pod资源(注意: v1.33仍为Alpha) |
| **AppArmor Support** | AppArmor | - | - | **GA** | GA | GA | Linux AppArmor安全配置 |
| **Pod Scheduling Readiness** | PodSchedulingReadiness | Beta | **GA** | GA | GA | GA | Pod调度门控 |
| **Node Log Query** | NodeLogQuery | - | Alpha | Alpha | Alpha | Alpha | kubectl node日志查询 |
| **nftables kube-proxy** | NFTablesProxyMode | - | - | Alpha | Alpha | **Beta** | nftables后端kube-proxy |
| **PersistentVolume Last Phase** | PersistentVolumeLastPhaseTransitionTime | - | - | **GA** | GA | GA | PV最后阶段转换时间 |
| **Parallel Image Pulls** | ParallelImagePulls | - | - | **默认启用** | 默认启用 | 默认启用 | kubelet并行拉取镜像 |
| **DRA Pod Resources** | DRAControlPlaneController | - | - | - | **Beta** | **GA** | DRA控制平面 |
| **TopologyManager Per Pod** | TopologyManagerPolicyOptions | - | - | - | **Beta** | **GA** | Pod级拓扑管理策略 |
| **Scheduler Queueing Hints** | SchedulerQueueingHints | - | - | - | Alpha | **Beta** | 调度器队列提示优化 |
| **Kubelet Resource Metrics** | KubeletResourceMetrics | - | - | - | - | **Beta** | kubelet资源指标端点 |
| **Cross-Namespace References** | CrossNamespaceVolumeDataSource | - | - | - | - | **Alpha** | 跨命名空间存储引用 |

<!-- chunk: API废弃时间线 -->
## API废弃时间线

| 资源类型 | 旧API版本 | 新API版本 | 弃用版本 | 移除版本 | 迁移优先级 |
|---------|----------|----------|---------|---------|-----------|
| Deployment | extensions/v1beta1 | apps/v1 | v1.9 | v1.16 | **已移除** |
| DaemonSet | extensions/v1beta1 | apps/v1 | v1.9 | v1.16 | **已移除** |
| ReplicaSet | extensions/v1beta1 | apps/v1 | v1.9 | v1.16 | **已移除** |
| Ingress | extensions/v1beta1 | networking.k8s.io/v1 | v1.14 | v1.22 | **已移除** |
| Ingress | networking.k8s.io/v1beta1 | networking.k8s.io/v1 | v1.19 | v1.22 | **已移除** |
| CronJob | batch/v1beta1 | batch/v1 | v1.21 | v1.25 | **已移除** |
| PodSecurityPolicy | policy/v1beta1 | (无,使用PSA) | v1.21 | v1.25 | **已移除** |
| EndpointSlice | discovery.k8s.io/v1beta1 | discovery.k8s.io/v1 | v1.21 | v1.25 | **已移除** |
| FlowSchema | flowcontrol/v1beta1 | flowcontrol/v1 | v1.26 | v1.29 | **已移除** |
| CSIStorageCapacity | storage.k8s.io/v1beta1 | storage.k8s.io/v1 | v1.24 | v1.27 | **已移除** |

<!-- chunk: 生产环境API使用检查 -->
## 生产环境API使用检查

```bash
# 检查集群中使用的已弃用API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 使用kubectl检查资源API版本
kubectl api-resources -o wide

# 查找使用旧API的资源
kubectl get deploy,ds,rs -A -o yaml | grep "apiVersion: extensions"

# 转换旧API到新版本(需要kubectl-convert插件)
kubectl convert -f old-deployment.yaml --output-version apps/v1
```

---

<!-- chunk: v1.29 - v1.33 版本关键特性速查 -->
## v1.29 - v1.33 版本关键特性速查

### v1.29 (2023年12月)

| 特性 | 状态 | 影响 | 操作 |
|:---|:---|:---|:---|
| **Sidecar 容器** | Beta (默认启用) | Pod 中 init 容器支持 `restartPolicy: Always` | 无需操作，自动可用 |
| **ReadWriteOncePod** | GA | PVC 访问模式，确保单 Pod 独占读写 | 更新 StorageClass 可用 |
| **KMS v2** | GA | etcd 加密性能提升 | 更新 KMS 配置 |
| **Node Volume 健康监测** | GA | 自动检测节点存储健康 | 无需操作 |
| **弃用 Node v1beta1 metrics** | 弃用 | 监控查询需更新 | 更新 Prometheus 规则 |
| **弃用 in-tree cloud providers** | 弃用 | 需迁移外部云控制器 | 安装 CCM |

### v1.30 (2024年4月)

| 特性 | 状态 | 影响 | 操作 |
|:---|:---|:---|:---|
| **ValidatingAdmissionPolicy** | GA | CEL 表达式替代 Validating Webhook | 可迁移策略到原生 CEL |
| **BoundServiceAccountToken** | GA | ServiceAccount Token 1 小时过期 | 无需操作，自动生效 |
| **Pod Scheduling Readiness** | GA | PodScheduling Gates 稳定 | 无需操作 |
| **弃用 in-tree storage drivers** | 弃用 | CSI 迁移完成 | 确认 CSI 驱动已安装 |
| **禁止 anonymous→cluster-admin** | 安全加固 | 默认禁止匿名绑定 cluster-admin | 检查现有 RBAC |

### v1.31 (2024年8月)

| 特性 | 状态 | 影响 | 操作 |
|:---|:---|:---|:---|
| **AppArmor Support** | GA | Pod 安全上下文支持 AppArmorProfile | Linux 节点可用 |
| **Parallel Image Pulls** | 默认启用 | kubelet 默认并行拉取镜像 | 无需操作 |
| **PersistentVolume Last Phase** | GA | PV 记录最后状态转换时间 | 无需操作 |
| **nftables kube-proxy** | Alpha | 新网络后端替代 iptables/ipvs | 实验性，暂不推荐生产 |
| **OpenTelemetry Tracing** | GA | kubelet 支持 OTel 链路追踪 | 配置 OTel Collector |

### v1.32 (2024年12月)

| 特性 | 状态 | 影响 | 操作 |
|:---|:---|:---|:---|
| **DRA (Dynamic Resource Allocation)** | Beta | GPU/FPGA 动态资源分配 | 需启用 Feature Gate |
| **TopologyManager Per Pod** | Beta | Pod 级 NUMA 拓扑策略 | 需启用 Feature Gate |
| **Pod-level Resource Limits** | Alpha | Pod 级别资源限制 (非容器级) | 实验性 |
| **多个 Beta API 升级** | 升级 | 检查变更日志 | 验证兼容性 |

### v1.33 (2025年4月 - 最新)

| 特性 | 状态 | 影响 | 操作 |
|:---|:---|:---|:---|
| **Sidecar 容器** | **GA** | 原生 Sidecar 生命周期管理 | **推荐生产使用** |
| **Dynamic Resource Allocation** | **GA** | GPU/FPGA 动态资源分配稳定 | 需启用 Feature Gate |
| **TopologyManager Per Pod** | **GA** | NUMA 拓扑策略稳定 | 需启用 Feature Gate |
| **Scheduler Queueing Hints** | Beta | 调度器队列提示优化性能 | 需启用 Feature Gate |
| **Kubelet Resource Metrics** | Beta | kubelet /metrics/resource 端点 | 无需操作 |
| **In-Place Pod Vertical Scaling** | Alpha | 原地调整 Pod 资源 (无需重启) | 实验性 |
| **Cross-Namespace References** | Alpha | PVC 跨命名空间引用数据源 | 实验性 |
| **Windows HostProcess** | GA | Windows 容器 HostProcess 模式稳定 | Windows 节点可用 |
| **PodIndexLabel** | GA | StatefulSet 自动生成 pod-index 标签 | 无需操作 |

<!-- chunk: 生产环境升级检查清单 (v1.29 → v1.33) -->
## 生产环境升级检查清单 (v1.29 → v1.33)

```bash
# 1. 检查已弃用 API 使用情况
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 2. 验证 CSI 驱动就绪 (v1.30+ in-tree 存储驱动弃用)
kubectl get csidrivers

# 3. 检查云控制器管理器 (CCM) 状态 (v1.31+ kubelet --cloud-provider 弃用)
kubectl get pods -n kube-system | grep cloud-controller

# 4. 验证 Sidecar 容器兼容性 (v1.33 GA)
kubectl get pods -A -o yaml | grep -A2 "restartPolicy: Always"

# 5. 检查 Feature Gate 状态
kubectl get --raw /api/v1/nodes/NODE_NAME/proxy/configz | jq '.kubeletconfig.featureGates'

# 6. 验证 API 版本兼容性
kubectl api-versions | sort

# 7. 检查 ValidatingAdmissionPolicy (v1.30 GA)
kubectl get validatingadmissionpolicies

# 8. 验证 Pod Security Admission 配置
kubectl get ns -o json | jq '.items[].metadata.labels | keys[]' | grep pod-security
```

**兼容性提示**: 升级前务必检查API版本兼容性，使用`kubectl api-versions`确认目标版本支持的API。建议按照 v1.29 → v1.30 → v1.31 → v1.32 → v1.33 的渐进式升级路径。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)
- 10 - Windows 容器支持与集成指南

## See Also

- 01-kubernetes-architecture-overview
- 02-core-components-deep-dive
- 04-source-code-structure
- 05-kubectl-commands-reference
