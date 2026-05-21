---
title: Kubernetes 版本 API 兼容矩阵 (1.28 → 1.33)
description: '## 概述'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- ceph
- redis
- hpa
- vpa
- statefulset
- daemonset
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Kubernetes 版本 API 兼容矩阵 (1.28 → 1.33) 是什么
- 如何 Kubernetes 版本 API 兼容矩阵 (1.28 → 1.33)
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- 版本
- API
- 兼容矩阵
- '1.28'
- '1.33'
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- redis-basics
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

# Kubernetes 版本 API 兼容矩阵 (1.28 → 1.33)

> **文档类型**: 版本参考手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 判断"某 YAML 能否在指定集群版本运行"、"某 API 字段是否可用"

---

<!-- chunk: 概述 -->
## 概述

Kubernetes API 资源在不同版本间的字段稳定性分为：
- **GA (Stable)**: 大版本稳定，无特殊情况不变更
- **Beta**: 可能在未来版本废弃，字段名可能变
- **Alpha**: 可能随时被移除，不应在生产环境使用

**本矩阵覆盖范围**：
- Deployment, StatefulSet, DaemonSet, Job, CronJob
- Service, Ingress, NetworkPolicy
- PersistentVolume, PersistentVolumeClaim, StorageClass
- HorizontalPodAutoscaler, VerticalPodAutoscaler
- Role, ClusterRole, RoleBinding, ClusterRoleBinding
- CustomResourceDefinition (CRD)
- Pod, ServiceAccount, ConfigMap, Secret

---

<!-- chunk: 1. Deployment -->
## 1. Deployment

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.replicas` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.selector` | GA | GA | GA | GA | GA | GA | 稳定，但创建后不可改 |
| `spec.strategy.type` | GA | GA | GA | GA | GA | GA | RollingUpdate/Recreate |
| `spec.strategy.rollingUpdate.maxSurge` | GA | GA | GA | GA | GA | GA | 整数或百分比 |
| `spec.strategy.rollingUpdate.maxUnavailable` | GA | GA | GA | GA | GA | GA | 整数或百分比 |
| `spec.progressDeadlineSeconds` | GA | GA | GA | GA | GA | GA | 默认 600s |
| `spec.minReadySeconds` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.paused` | GA | GA | GA | GA | GA | GA | 暂停滚动更新 |
| `spec.revisionHistoryLimit` | GA | GA | GA | GA | GA | GA | 保留历史版本数 |
| `spec.template` | GA | GA | GA | GA | GA | GA | PodTemplateSpec |
| `status.conditions[].type` | GA | GA | GA | GA | GA | GA | Available/Progressing |
| `spec.progressDeadlineSeconds` deprecated | - | - | - | - | - | **DEPRECATED** | 将在 1.34 移除 |

### Pod Template 字段

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.template.spec.terminationGracePeriodSeconds` | GA | GA | GA | GA | GA | GA | 默认 30s |
| `spec.template.spec.restartPolicy` | GA | GA | GA | GA | GA | GA | 始终为 Always |
| `spec.template.spec.hostIPC` | Beta | Beta | Beta | Beta | Beta | **DEPRECATED** | 将在 1.34 移除 |
| `spec.template.spec.hostNetwork` | Beta | Beta | Beta | Beta | Beta | **DEPRECATED** | 将在 1.34 移除 |
| `spec.template.spec.hostPID` | Beta | Beta | Beta | Beta | Beta | **DEPRECATED** | 将在 1.34 移除 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: `kubectl rollout restart` 使用新策略，不再创建空 ReplicaSet 直接替换
- **K8s 1.31**: 移除了 `spec.selector.matchExpressions` 中的 `exists` operator（仅 `In/NotIn/NotExists/Gt/Lt`）

---

<!-- chunk: 2. StatefulSet -->
## 2. StatefulSet

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.selector` | GA | GA | GA | GA | GA | GA | 稳定，创建后不可改 |
| `spec.serviceName` | GA | GA | GA | GA | GA | GA | 必须匹配 Headless Service |
| `spec.replicas` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.template` | GA | GA | GA | GA | GA | GA | PodTemplateSpec |
| `spec.volumeClaimTemplates` | GA | GA | GA | GA | GA | GA | 动态 PV 供给 |
| `spec.persistentVolumeClaimRetentionPolicy` | Beta | Beta | GA | GA | GA | GA | 控制 PVC 保留行为 |
| `spec.ordinals.start` | Beta | Beta | GA | GA | GA | GA | 有序 Pod 命名起始值 |
| `status.observedGeneration` | GA | GA | GA | GA | GA | GA | 控制器已处理的版本 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: StatefulSet 的 `spec.persistentVolumeClaimRetentionPolicy` 进入 GA
- **K8s 1.30**: `spec.ordinals.start` 进入 GA（之前为 Alpha）

---

<!-- chunk: 3. DaemonSet -->
## 3. DaemonSet

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.selector` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.template` | GA | GA | GA | GA | GA | GA | PodTemplateSpec |
| `spec.updateStrategy.type` | GA | GA | GA | GA | GA | GA | OnDelete/RollingUpdate |
| `spec.updateStrategy.rollingUpdate.maxUnavailable` | GA | GA | GA | GA | GA | GA | 默认 1 |
| `spec.minReadySeconds` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.revisionHistoryLimit` | GA | GA | GA | GA | GA | GA | 稳定 |
| `spec.template.spec.restartPolicy` | GA | GA | GA | GA | GA | GA | 只能为 Always |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: DaemonSet `spec.updateStrategy.rollingUpdate.maxUnavailable` 支持整数或百分比

---

<!-- chunk: 4. Job -->
## 4. Job

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.completions` | GA | GA | GA | GA | GA | GA | 默认 1（单 Job 单 Pod） |
| `spec.parallelism` | GA | GA | GA | GA | GA | GA | 默认 1 |
| `spec.backoffLimit` | GA | GA | GA | GA | GA | GA | 默认 6 |
| `spec.activeDeadlineSeconds` | GA | GA | GA | GA | GA | GA | Job 超时时间 |
| `spec.ttrlsSecondsAfterFinished` | GA | GA | GA | GA | GA | GA | 完成后保留时间 |
| `spec.failFast` | Alpha | Beta | Beta | GA | GA | GA | 快速失败 |
| `spec.podFailurePolicy` | Alpha | Beta | Beta | GA | GA | GA | 按条件处理失败 |
| `spec ttlSecondsAfterFinished` deprecated | - | - | - | - | **DEPRECATED** | **DEPRECATED** | 使用 `.spec.ttlSecondsAfterFinished` |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: `spec.failFast` 进入 GA
- **K8s 1.30**: `spec.podFailurePolicy` 进入 GA
- **K8s 1.32**: `ttlSecondsAfterFinished` 更名为 `ttlSecondsAfterFinished`（underscore）但仍然支持旧名

---

<!-- chunk: 5. CronJob -->
## 5. CronJob

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.schedule` | GA | GA | GA | GA | GA | GA | Cron 表达式 |
| `spec.timeZone` | Beta | Beta | GA | GA | GA | GA | 时区（1.27+） |
| `spec.concurrencyPolicy` | GA | GA | GA | GA | GA | GA | Allow/Forbid/Replace |
| `spec.failedJobsHistoryLimit` | GA | GA | GA | GA | GA | GA | 默认 1 |
| `spec.successfulJobsHistoryLimit` | GA | GA | GA | GA | GA | GA | 默认 3 |
| `spec.startingDeadlineSeconds` | GA | GA | GA | GA | GA | GA | 启动截止时间 |
| `spec.paused` | GA | GA | GA | GA | GA | GA | 暂停 Job 创建 |
| `spec.jobTemplate` | GA | GA | GA | GA | GA | GA | Job 模板 |
| `spec.timeZone` | GA | GA | GA | GA | GA | GA | 1.27 后 GA |

### 1.28 → 1.33 重大变化

- **K8s 1.29**: `spec.timeZone` 为 Beta，1.30 进入 GA

---

<!-- chunk: 6. Service -->
## 6. Service

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.type` | GA | GA | GA | GA | GA | GA | ClusterIP/NodePort/LoadBalancer/ExternalName |
| `spec.clusterIP` | GA | GA | GA | GA | GA | GA | None 为 Headless |
| `spec.sessionAffinity` | GA | GA | GA | GA | GA | GA | None/ClientIP |
| `spec.sessionAffinityConfig` | GA | GA | GA | GA | GA | GA | ClientIP 配置 |
| `spec.healthCheckNodePort` | GA | GA | GA | GA | GA | GA | Type=LoadBalancer 时 |
| `spec.externalTrafficPolicy` | GA | GA | GA | GA | GA | GA | Cluster/Local |
| `spec.externalName` | GA | GA | GA | GA | GA | GA | CNAME 记录 |
| `spec.ports[].name` | GA | GA | GA | GA | GA | GA | 端口名 |
| `spec.ports[].port` | GA | GA | GA | GA | GA | GA | Service 端口 |
| `spec.ports[].targetPort` | GA | GA | GA | GA | GA | GA | 容器端口（string 或 int） |
| `spec.ports[].protocol` | GA | GA | GA | GA | GA | GA | TCP/UDP/SCTP |
| `spec.ports[].appProtocol` | GA | GA | GA | GA | GA | GA | 应用协议 |
| `metadata.annotations[loadBalancer.byoip-source-ip]` | - | - | - | - | - | Alpha | BYOIP 支持 |
| `spec.loadBalancerClass` | Beta | Beta | GA | GA | GA | GA | 指定 LB 实现（1.29+） |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: `spec.loadBalancerClass` 进入 GA
- **K8s 1.31**: `spec.ipFamilies` 和 `spec.ipFamilyPolicy` GA，支持单栈/双栈 Service

---

<!-- chunk: 7. Ingress -->
## 7. Ingress

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.ingressClassName` | GA | GA | GA | GA | GA | GA | 关联 IngressClass |
| `spec.backend` | GA | GA | GA | GA | GA | GA | 默认后端 |
| `spec.rules[].host` | GA | GA | GA | GA | GA | GA | 主机名匹配 |
| `spec.rules[].http.paths[].path` | GA | GA | GA | GA | GA | GA | 路径前缀匹配 |
| `spec.rules[].http.paths[].pathType` | GA | GA | GA | GA | GA | GA | ImplementationSpecific/Exact/Prefix |
| `spec.rules[].http.paths[].backend` | GA | GA | GA | GA | GA | GA | Backend 引用 |
| `spec.tls[].hosts` | GA | GA | GA | GA | GA | GA | TLS 主机列表 |
| `spec.tls[].secretName` | GA | GA | GA | GA | GA | GA | TLS Secret |
| `status.loadBalancer.ingress[].hostname` | GA | GA | GA | GA | GA | GA | LB 主机名 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: 新增 `spec.rules[].http.paths[].backend.service.name` 和 `backend.service.port.name/number`

---

<!-- chunk: 8. NetworkPolicy -->
## 8. NetworkPolicy

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.podSelector` | GA | GA | GA | GA | GA | GA | 选择目标 Pod |
| `spec.policyTypes` | GA | GA | GA | GA | GA | GA | Ingress/Egress/Device |
| `spec.ingress[].from` | GA | GA | GA | GA | GA | GA | 允许的入站源 |
| `spec.ingress[].ports` | GA | GA | GA | GA | GA | GA | 允许的端口 |
| `spec.egress[].to` | GA | GA | GA | GA | GA | GA | 允许的目的地 |
| `spec.egress[].ports` | GA | GA | GA | GA | GA | GA | 允许的端口 |
| `spec.ingress[].from[].namespaceSelector` | GA | GA | GA | GA | GA | GA | 按命名空间选择 |
| `spec.ingress[].from[].podSelector` | GA | GA | GA | GA | GA | GA | 按 Pod 选择 |
| `spec.ingress[].from[].ipBlock` | GA | GA | GA | GA | GA | GA | CIDR 范围 |
| `spec.egress[].to[].namespaceSelector` | GA | GA | GA | GA | GA | GA | 按命名空间选择 |
| `spec.egress[].to[].podSelector` | GA | GA | GA | GA | GA | GA | 按 Pod 选择 |
| `spec.egress[].to[].ipBlock` | GA | GA | GA | GA | GA | GA | CIDR 范围 |

### 1.28 → 1.33 重大变化

- **K8s 1.31**: 新增 `devicePolicyType`（Device 插件支持时）
- **K8s 1.29**: `ipBlock` 的 `except` 字段支持更精细的排除

---

<!-- chunk: 9. PersistentVolume / PersistentVolumeClaim -->
## 9. PersistentVolume / PersistentVolumeClaim

### PV 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.persistentVolumeReclaimPolicy` | GA | GA | GA | GA | GA | GA | Retain/Recycle/Delete |
| `spec.capacity.storage` | GA | GA | GA | GA | GA | GA | 容量 |
| `spec.accessModes` | GA | GA | GA | GA | GA | GA | ReadWriteOnce/ReadOnlyMany/ReadWriteMany |
| `spec.hostPath.path` | GA | GA | GA | GA | GA | GA | 仅开发环境 |
| `spec.nfs.server/path` | GA | GA | GA | GA | GA | GA | NFS 配置 |
| `spec.claimRef` | GA | GA | GA | GA | GA | GA | 绑定的 PVC |
| `spec.nodeAffinity` | GA | GA | GA | GA | GA | GA | 节点亲和性 |
| `status.phase` | GA | GA | GA | GA | GA | GA | Pending/Bound/Released/Failed |
| `status.persistentVolumeReclaimPolicy` | GA | GA | GA | GA | GA | GA | 同 spec |

### PVC 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.selector` | Beta | Beta | GA | GA | GA | GA | 标签选择 PV |
| `spec.volumeName` | GA | GA | GA | GA | GA | GA | 绑定的 PV |
| `spec.storageClassName` | GA | GA | GA | GA | GA | GA | 存储类 |
| `spec.volumeMode` | GA | GA | GA | GA | GA | GA | Filesystem/Block |
| `spec.dataSource` | GA | GA | GA | GA | GA | GA | 数据源（VolumeSnapshot/Clone） |
| `spec.dataSourceRef` | Alpha | Alpha | Beta | Beta | GA | GA | 数据源引用 |
| `status.phase` | GA | GA | GA | GA | GA | GA | Pending/Bound/Lost |
| `status.accessModes` | GA | GA | GA | GA | GA | GA | 同 PV |
| `status.capacity.storage` | GA | GA | GA | GA | GA | GA | 实际容量 |

### 1.28 → 1.33 重大变化

- **K8s 1.31**: `spec.dataSourceRef` 进入 GA
- **K8s 1.30**: PVC `spec.selector` 进入 GA

---

<!-- chunk: 10. HPA (HorizontalPodAutoscaler) -->
## 10. HPA (HorizontalPodAutoscaler)

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.scaleTargetRef` | GA | GA | GA | GA | GA | GA | 目标资源 |
| `spec.minReplicas` | GA | GA | GA | GA | GA | GA | 最小副本数 |
| `spec.maxReplicas` | GA | GA | GA | GA | GA | GA | 最大副本数 |
| `spec.metrics[].type` | GA | GA | GA | GA | GA | GA | Resource/External/Pods/Object |
| `spec.metrics[].resource.target.averageUtilization` | GA | GA | GA | GA | GA | GA | CPU 百分比 |
| `spec.metrics[].resource.target.averageValue` | GA | GA | GA | GA | GA | GA | 内存绝对值 |
| `spec.metrics[].external.target.value` | GA | GA | GA | GA | GA | GA | 自定义指标值 |
| `spec.metrics[].external.target.averageValue` | GA | GA | GA | GA | GA | GA | 自定义指标平均值 |
| `spec.behavior.scaleDown.stabilizationWindowSeconds` | GA | GA | GA | GA | GA | GA | 缩容窗口 |
| `spec.behavior.scaleUp.stabilizationWindowSeconds` | GA | GA | GA | GA | GA | GA | 扩容窗口 |
| `spec.behavior.scaleDown.policies[].type` | GA | GA | GA | GA | GA | GA | Percent/Pods |
| `spec.behavior.scaleDown.policies[].value` | GA | GA | GA | GA | GA | GA | 缩容比例/数量 |
| `spec.behavior.scaleDown.policies[].periodSeconds` | GA | GA | GA | GA | GA | GA | 周期秒数 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: 支持 `spec.metrics[].external.target.averageValue`
- **K8s 1.32**: `spec.behavior` 支持 `scaleUp.selectPolicy` 和 `scaleDown.selectPolicy`

---

<!-- chunk: 11. VPA (VerticalPodAutoscaler) -->
## 11. VPA (VerticalPodAutoscaler)

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|
| `spec.targetRef` | GA | GA | GA | GA | GA | GA | 目标资源 |
| `spec.updatePolicy.updateMode` | GA | GA | GA | GA | GA | GA | Off/Auto/Recreate/InPlace |
| `spec.resourcePolicy.containerPolicies[].containerName` | GA | GA | GA | GA | GA | GA | 容器名 |
| `spec.resourcePolicy.containerPolicies[].minAllowed` | GA | GA | GA | GA | GA | GA | 最小资源 |
| `spec.resourcePolicy.containerPolicies[].maxAllowed` | GA | GA | GA | GA | GA | GA | 最大资源 |
| `spec.resourcePolicy.containerPolicies[].mode` | Beta | Beta | GA | GA | GA | GA | Pod/Container |
| `spec.recommendations[].containerRecommendations[].containerName` | GA | GA | GA | GA | GA | GA | 推荐容器名 |
| `spec.recommendations[].containerRecommendations[].resources` | GA | GA | GA | GA | GA | GA | 推荐资源值 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: VPA `InPlacePodVerticalScaling` Beta（Pod 垂直扩容无需重启容器）
- **K8s 1.31**: `spec.resourcePolicy.containerPolicies[].mode` 进入 GA

---

<!-- chunk: 12. StorageClass -->
## 12. StorageClass

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `provisioner` | GA | GA | GA | GA | GA | GA | 供给器名称 |
| `parameters` | GA | GA | GA | GA | GA | GA | 供给器参数 |
| `reclaimPolicy` | GA | GA | GA | GA | GA | GA | Retain/Delete |
| `volumeBindingMode` | GA | GA | GA | GA | GA | GA | Immediate/WaitForFirstConsumer |
| `allowVolumeExpansion` | GA | GA | GA | GA | GA | GA | 是否允许扩容 |
| `mountOptions` | GA | GA | GA | GA | GA | GA | 挂载选项 |
| `allowedTopologies` | GA | GA | GA | GA | GA | GA | 拓扑限制 |
| `metadata.annotations[storageclass.kubernetes.io/scenario]` | Alpha | Alpha | Beta | Beta | GA | GA | 存储场景 |

### 1.28 → 1.33 重大变化

- **K8s 1.32**: 新增 `metadata.annotations.storageclass.kubernetes.io/is-default-class` 的标准注解说明

---

<!-- chunk: 13. RBAC (Role/ClusterRole/RoleBinding/ClusterRoleBinding) -->
## 13. RBAC (Role/ClusterRole/RoleBinding/ClusterRoleBinding)

### Role/ClusterRole 核心字段

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `rules[].apiGroups` | GA | GA | GA | GA | GA | GA | "" / "apps" / "*" 等 |
| `rules[].resources` | GA | GA | GA | GA | GA | GA | pods/log 等 |
| `rules[].verbs` | GA | GA | GA | GA | GA | GA | get/list/watch/create 等 |
| `rules[].resourceNames` | GA | GA | GA | GA | GA | GA | 限制到具体资源 |
| `rules[].nonResourceURLs` | GA | GA | GA | GA | GA | GA | 非资源型 URL（如 /healthz） |

### RoleBinding/ClusterRoleBinding 核心字段

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `subjects[].kind` | GA | GA | GA | GA | GA | GA | User/Group/ServiceAccount |
| `subjects[].name` | GA | GA | GA | GA | GA | GA | 名称 |
| `subjects[].namespace` | GA | GA | GA | GA | GA | GA | 命名空间（SA/User 时） |
| `roleRef.apiGroup` | GA | GA | GA | GA | GA | GA | 固定为 rbac.authorization.k8s.io |
| `roleRef.kind` | GA | GA | GA | GA | GA | GA | Role/ClusterRole |
| `roleRef.name` | GA | GA | GA | GA | GA | GA | 引用的角色名 |

---

<!-- chunk: 14. CRD (CustomResourceDefinition) -->
## 14. CRD (CustomResourceDefinition)

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.group` | GA | GA | GA | GA | GA | GA | API 组名 |
| `spec.names.plural` | GA | GA | GA | GA | GA | GA | 资源复数名 |
| `spec.names.singular` | GA | GA | GA | GA | GA | GA | 单数名 |
| `spec.names.kind` | GA | GA | GA | GA | GA | GA | Go 类型名 |
| `spec.names.shortNames` | GA | GA | GA | GA | GA | GA | 短名称 |
| `spec.scope` | GA | GA | GA | GA | GA | GA | Namespaced/Cluster |
| `spec.versions[].name` | GA | GA | GA | GA | GA | GA | 版本名 |
| `spec.versions[].served` | GA | GA | GA | GA | GA | GA | 是否启用 |
| `spec.versions[].storage` | GA | GA | GA | GA | GA | GA | 是否存储版本 |
| `spec.versions[].schema` | GA | GA | GA | GA | GA | GA | OpenAPI v3 验证 |
| `spec.versions[].subresources` | GA | GA | GA | GA | GA | GA | /status 和 /scale |
| `spec.versions[].additionalPrinterColumns` | GA | GA | GA | GA | GA | GA | kubectl get 输出列 |
| `spec.conversion.strategy` | GA | GA | GA | GA | GA | GA | None/Webhook |
| `spec.conversion.webhook.conversionReviewVersions` | GA | GA | GA | GA | GA | GA | 支持的版本列表 |

### 1.28 → 1.33 重大变化

- **K8s 1.30**: `spec.versions[].deprecationWarning` 支持版本弃用警告
- **K8s 1.31**: CRD `spec.conversion.webhookConversionReviewVersions` 标记为必需

---

<!-- chunk: 15. Pod 核心字段稳定性 -->
## 15. Pod 核心字段稳定性

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.containers[].name` | GA | GA | GA | GA | GA | GA | 必须 |
| `spec.containers[].image` | GA | GA | GA | GA | GA | GA | 必须 |
| `spec.containers[].command` | GA | GA | GA | GA | GA | GA | 入口命令 |
| `spec.containers[].args` | GA | GA | GA | GA | GA | GA | 参数 |
| `spec.containers[].env` | GA | GA | GA | GA | GA | GA | 环境变量 |
| `spec.containers[].envFrom` | GA | GA | GA | GA | GA | GA | ConfigMap/Secret |
| `spec.containers[].imagePullPolicy` | GA | GA | GA | GA | GA | GA | Always/IfNotPresent/Never |
| `spec.containers[].ports` | GA | GA | GA | GA | GA | GA | 端口定义 |
| `spec.containers[].resources.limits` | GA | GA | GA | GA | GA | GA | 资源限制 |
| `spec.containers[].resources.requests` | GA | GA | GA | GA | GA | GA | 资源请求 |
| `spec.containers[].livenessProbe` | GA | GA | GA | GA | GA | GA | 存活探针 |
| `spec.containers[].readinessProbe` | GA | GA | GA | GA | GA | GA | 就绪探针 |
| `spec.containers[].startupProbe` | GA | GA | GA | GA | GA | GA | 启动探针 |
| `spec.containers[].securityContext` | GA | GA | GA | GA | GA | GA | 安全上下文 |
| `spec.initContainers` | GA | GA | GA | GA | GA | GA | Init 容器 |
| `spec.ephemeralContainers` | GA | GA | GA | GA | GA | GA | 临时容器（kubectl debug 用） |
| `spec.restartPolicy` | GA | GA | GA | GA | GA | GA | 始终为 Always |
| `spec.hostname` | GA | GA | GA | GA | GA | GA | 主机名 |
| `spec.subdomain` | GA | GA | GA | GA | GA | GA | Headless Service 子域名 |
| `spec.hostAliases` | GA | GA | GA | GA | GA | GA | /etc/hosts 映射 |
| `spec.imagePullSecrets` | GA | GA | GA | GA | GA | GA | 镜像仓库凭证 |
| `spec.nodeSelector` | GA | GA | GA | GA | GA | GA | 节点标签选择 |
| `spec.affinity` | GA | GA | GA | GA | GA | GA | 亲和性调度 |
| `spec.tolerations` | GA | GA | GA | GA | GA | GA | 污点容忍 |
| `spec.topologySpreadConstraints` | GA | GA | GA | GA | GA | GA | 拓扑分布 |
| `spec.schedulingGates` | Beta | Beta | GA | GA | GA | GA | 调度门控（1.30+ GA） |
| `spec.resourceClaimTracker` | Alpha | Alpha | Beta | Beta | Beta | Beta | DRA 资源追踪 |

---

<!-- chunk: 16. PodSchedulingGate（调度门控，K8s 1.30+ GA） -->
## 16. PodSchedulingGate（调度门控，K8s 1.30+ GA）

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `spec.schedulingGates[].name` | Beta | Beta | **GA** | GA | GA | GA | 调度门控名称 |
| `status.schedulingGates` | Beta | Beta | **GA** | GA | GA | GA | 当前门控状态 |

### 行为变更

| 版本 | 行为变化 |
|------|---------|
| 1.30 | `schedulingGates` 从 Beta 进入 GA，之前版本使用 Beta API |
| 1.30+ | Pod 可以通过 `schedulingGates` 延迟调度（等待外部条件满足，如资源就绪） |
| 之前版本 | 如使用 `schedulingGates` 会报错 "unknown field"（1.28 之前不支持） |

---

<!-- chunk: 17. DRA ResourceClaimStatus.devices -->
## 17. DRA ResourceClaimStatus.devices

### 核心字段变更

| 字段 | 1.28 | 1.29 | 1.30 | 1.31 | 1.32 | 1.33 | 说明 |
|------|------|------|------|------|------|------|------|
| `status.devices[].name` | Alpha | Alpha | Beta | Beta | GA | GA | 设备名称 |
| `status.devices[].type` | Alpha | Alpha | Beta | Beta | GA | GA | 设备类型（vendor/name） |
| `status.devices[].nodeName` | Alpha | Alpha | Beta | Beta | GA | GA | 分配的节点名 |
| `status.devices[].pool` | Alpha | Alpha | Beta | Beta | GA | GA | 设备池名称 |
| `status.allocationMode` | Alpha | Alpha | Beta | Beta | GA | GA | 分配模式 |

### 行为变更

| 版本 | 行为变化 |
|------|---------|
| 1.30 | `ResourceClaimStatus.devices` 从 Alpha 进入 Beta，支持设备节点拓扑信息 |
| 1.31 | `allocationMode` 进入 GA |
| 1.30+ | DRA 调度时可通过 `status.devices` 精确定位设备所在节点 |

---

<!-- chunk: 18. CSI Migration 状态变化 -->
## 18. CSI Migration 状态变化

### 树内驱动移除时间线

| 驱动 | K8s 版本 | 状态 |
|------|---------|------|
| AWS EBS (kubernetes.io/aws-ebs) | **1.26 移除** | 已完全迁移到 AWS EBS CSI |
| GCE PD (kubernetes.io/gce-pd) | **1.26 移除** | 已完全迁移到 GCE PD CSI |
| Azure Disk (kubernetes.io/azure-disk) | **1.28 废弃，1.30 移除** | 迁移进行中 |
| Azure File (kubernetes.io/azure-file) | 1.30 Beta | 迁移中 |
| vSphere (kubernetes.io/vsphere-volume) | 1.30 Beta | 迁移中 |
| Ceph RBD (kubernetes.io/rbd) | Alpha | 迁移中 |

### CSI Migration FeatureGate 对照

| FeatureGate | K8s 版本 | 说明 |
|-------------|---------|------|
| `CSIMigration` | GA (1.29+) | 启用 CSI 迁移（全局开关） |
| `CSIMigrationAWS` | GA | AWS EBS 树内迁移 |
| `CSIMigrationGCE` | GA | GCE PD 树内迁移 |
| `CSIMigrationAzureDisk` | GA (1.30+) | Azure Disk 树内迁移 |
| `CSIMigrationAzureFile` | Beta (1.30+) | Azure File 树内迁移 |
| `CSIMigrationvSphere` | Beta (1.30+) | vSphere 树内迁移 |
| `InTreePluginvSphere` | Beta (1.30+) | vSphere in-tree 插件 |

### 行为变更

| 版本 | 行为变化 |
|------|---------|
| 1.28 | Azure Disk CSI 开始强制迁移（`CSIMigrationAzureDisk=true` 默认） |
| 1.30 | Azure File CSI Migration 进入 Beta；vSphere CSI Migration 进入 Beta |
| 1.28+ | 树内卷不再支持新创建（只读已有卷），新卷必须使用 CSI |

---

<!-- chunk: 19. volumeBindingMode 行为变化 -->
## 19. volumeBindingMode 行为变化

### StorageClass volumeBindingMode

| 配置 | 1.28 行为 | 1.30+ 行为变化 |
|------|----------|--------------|
| `volumeBindingMode: Immediate` | 在 PVC 创建时即绑定 PV | **K8s 1.30+**: 调度前绑定（延迟到 Pod 调度决策时） |
| `volumeBindingMode: WaitForFirstConsumer` | Pod 调度后才绑定 | 行为不变，但增加了拓扑感知精度 |

### 关键变化说明

- **K8s 1.30 起**：`Immediate` 模式下，如果拓扑条件（如可用区）在调度时无法满足，Pod 会被标记为 `_unschedulable`（与 `WaitForFirstConsumer` 一致）
- 之前版本（1.28-1.29）：`Immediate` 在 PVC 创建时就绑定，不考虑 Pod 调度

### 影响场景

| 场景 | K8s 1.28 行为 | K8s 1.30+ 行为 |
|------|-------------|---------------|
| StorageClass 指定 `allowedTopologies`，但集群节点不在该 zone | PVC Bound 成功，Pod 调度失败 | PVC 延迟绑定，调度时检查拓扑失败才报错 |

---

<!-- chunk: 附录：版本升级检查清单 -->
## 附录：版本升级检查清单

| 检查项 | 操作 |
|--------|------|
| **YAML 中使用了废弃字段** | 检查 `kubectl explain <resource> --api-version=...` |
| **使用了 Alpha API** | 迁移到 Beta/GA 版本 |
| **使用了已移除的字段** | 对照本矩阵，检查目标版本的 `DEPRECATED` 标记 |
| **CRD 版本与 K8s 版本兼容** | 确认 `spec.versions[].served` 覆盖目标版本 |
| **kubectl 版本匹配** | 使用与集群版本一致的 kubectl 版本 |

---

```yaml
---
id: K8S-API-VERSION-MATRIX-001
domain: architecture
type: version-matrix
tags: [api-version, compatibility, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "K8s 1.30 是否支持某个字段"
  - "Deployment 的 progressDeadlineSeconds 在哪个版本移除"
  - "StorageClass 的 volumeBindingMode 是否稳定"
  - "HPA 的 failFast 在哪个版本进入 GA"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-01-cluster-fundamentals/03-api-versions-features.md
  - domain-18-manifests-patterns/01-yaml-syntax-resource-conventions.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-01-cluster-fundamentals/MOC.md|domain-01-cluster-fundamentals MOC]]
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- [[domain-01-cluster-fundamentals/00-open-source-projects-index.md|Domain-1 架构基础 — 开源项目索引]]
- [[domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md|Kubernetes 架构全景图]]
- [[domain-01-cluster-fundamentals/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]]
- [[domain-01-cluster-fundamentals/03-api-versions-features.md|03 - 功能和API表]]
- [[domain-01-cluster-fundamentals/04-source-code-structure.md|04 - Kubernetes 源码结构深度解析]]
- [[domain-01-cluster-fundamentals/05-kubectl-commands-reference.md|kubectl 命令完整参考]]
- [[domain-01-cluster-fundamentals/06-cluster-configuration-parameters.md|06 - 集群配置参数完全参考]]
- [[domain-01-cluster-fundamentals/07-upgrade-paths-strategy.md|07 - 升级路径与策略指南]]
- [[domain-01-cluster-fundamentals/08-multi-tenancy-architecture.md|08 - 多租户架构设计 (Multi-Tenancy Architecture)]]
- [[domain-01-cluster-fundamentals/09-edge-computing-kubeedge.md|09 - 边缘计算集成架构 (KubeEdge/OpenYurt)]]

## See Also

- [[domain-01-cluster-fundamentals/18-upgrade-migration-strategy.md|18-upgrade-migration-strategy]]
- [[domain-01-cluster-fundamentals/99-kubectl-v1.29-v1.33-new-commands-guide.md|99-kubectl-v1.29-v1.33-new-commands-guide]]
- [[domain-01-cluster-fundamentals/99-kubernetes-core-components-v1.29-v1.33-update.md|99-kubernetes-core-components-v1.29-v1.33-update]]
- [[domain-01-cluster-fundamentals/99-kubernetes-core-features-mermaid-diagrams.md|99-kubernetes-core-features-mermaid-diagrams]]
