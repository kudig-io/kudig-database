---
title: 'Domain-32: Kubernetes YAML 配置完整参考手册'
description: '# Domain-32: Kubernetes YAML 配置完整参考手册'
summary: 'Kubernetes 生产运维终极 YAML 配置参考手册，覆盖所有原生 API 资源及常用生态工具的完整 YAML 配置规范。每个资源包含字段级完整参考、源码级原理解释、版本兼容性矩阵和生产案例。'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- kubelet
- scheduler
- helm
- argocd
- hpa
- pdb
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 'Domain-32: Kubernetes YAML 配置完整参考手册 是什么'
- '如何 Domain-32: Kubernetes YAML 配置完整参考手册'
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- 'Domain-32:'
- Kubernetes
- YAML
- 配置完整参考手册
- yaml
- manifests
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-32: Kubernetes YAML 配置完整参考手册

> **文档数量**: 36 篇 | **最后更新**: 2026-02 | **适用版本**: Kubernetes v1.25 - v1.32

---

## 概述

Kubernetes 生产运维终极 YAML 配置参考手册，覆盖所有原生 API 资源及常用生态工具的完整 YAML 配置规范。每个资源包含字段级完整参考、源码级原理解释、版本兼容性矩阵和生产案例。

**核心价值**：
- 📖 **完整参考**：覆盖 Kubernetes v1.25-v1.32 所有 60+ 种原生 API 资源
- 🎯 **双层内容**：初学者友好的最小示例 + 专家级完整字段规格
- 🔧 **即用模板**：所有 YAML 带详细中文注释，可直接用于生产
- 📊 **版本兼容**：每个字段标注引入版本和兼容范围
- 🧠 **源码级解析**：控制器内部原理、状态机转换、性能特征
- 🏭 **生产案例**：来自真实生产环境的配置模板和最佳实践

---

## 文档目录

### 基础概念 (01-02)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 01 | [YAML 语法与资源规范](./01-yaml-syntax-resource-conventions.md) | YAML 语法、资源四大字段、命名规范、标签注解 | ⭐⭐⭐⭐⭐ |
| 02 | [Namespace/ResourceQuota/LimitRange](./02-namespace-resourcequota-limitrange.md) | 命名空间隔离、资源配额、限制范围 | ⭐⭐⭐⭐⭐ |

### 核心工作负载 (03-07)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 03 | [Pod 完整规格](./03-pod-specification-complete.md) | Pod 所有字段、容器规格、卷挂载、安全上下文、调度 | ⭐⭐⭐⭐⭐ |
| 04 | [Deployment/ReplicaSet](./04-deployment-replicaset.md) | 无状态部署、滚动更新、版本管理 | ⭐⭐⭐⭐⭐ |
| 05 | [StatefulSet](./05-statefulset-reference.md) | 有状态应用、稳定标识、有序部署 | ⭐⭐⭐⭐ |
| 06 | [DaemonSet](./06-daemonset-reference.md) | 节点守护进程、滚动更新 | ⭐⭐⭐⭐ |
| 07 | [Job/CronJob](./07-job-cronjob-reference.md) | 批处理任务、定时调度、失败策略 | ⭐⭐⭐⭐ |

### 服务发现与流量管理 (08-12)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 08 | [Service 全类型](./08-service-all-types.md) | ClusterIP/NodePort/LoadBalancer/ExternalName/Headless | ⭐⭐⭐⭐⭐ |
| 09 | [Endpoints/EndpointSlice](./09-endpoints-endpointslice.md) | 端点管理、分片机制、外部服务集成 | ⭐⭐⭐ |
| 10 | [Ingress/IngressClass](./10-ingress-ingressclass.md) | HTTP 路由、TLS 终结、控制器配置 | ⭐⭐⭐⭐⭐ |
| 11 | [Gateway API 核心](./11-gateway-api-core.md) | GatewayClass/Gateway/HTTPRoute | ⭐⭐⭐⭐ |
| 12 | [Gateway API 高级路由](./12-gateway-api-advanced-routes.md) | gRPC/TCP/TLS/UDP Route、ReferenceGrant | ⭐⭐⭐ |

### 配置与存储管理 (13-18)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 13 | [ConfigMap](./13-configmap-reference.md) | 配置管理、挂载方式、热更新 | ⭐⭐⭐⭐⭐ |
| 14 | [Secret 全类型](./14-secret-all-types.md) | 8 种 Secret 类型、加密存储、安全实践 | ⭐⭐⭐⭐⭐ |
| 15 | [PersistentVolume](./15-persistentvolume-reference.md) | 持久卷、所有卷源类型、生命周期 | ⭐⭐⭐⭐ |
| 16 | [PersistentVolumeClaim](./16-persistentvolumeclaim-reference.md) | 卷声明、动态供给、扩容、克隆 | ⭐⭐⭐⭐ |
| 17 | [StorageClass/VolumeSnapshot](./17-storageclass-volumesnapshot.md) | 存储类、卷快照、快照恢复 | ⭐⭐⭐⭐ |
| 18 | [CSI 驱动资源](./18-csi-driver-resources.md) | CSIDriver/CSINode/CSIStorageCapacity | ⭐⭐⭐ |

### 安全与访问控制 (19-25)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 19 | [ServiceAccount/Token](./19-serviceaccount-token.md) | 服务账户、Token 管理、证书签发 | ⭐⭐⭐⭐ |
| 20 | [Role/RoleBinding](./20-rbac-role-rolebinding.md) | 命名空间级 RBAC | ⭐⭐⭐⭐⭐ |
| 21 | [ClusterRole/ClusterRoleBinding](./21-rbac-clusterrole-clusterrolebinding.md) | 集群级 RBAC、访问审查 | ⭐⭐⭐⭐ |
| 22 | [NetworkPolicy](./22-networkpolicy-reference.md) | 网络策略、微分段、零信任 | ⭐⭐⭐⭐⭐ |
| 23 | [Pod Security Standards](./23-pod-security-standards.md) | PSS 三级别、PSA 配置 | ⭐⭐⭐⭐ |
| 24 | [Admission Webhook](./24-admission-webhook-configuration.md) | Validating/Mutating Webhook | ⭐⭐⭐ |
| 25 | [ValidatingAdmissionPolicy](./25-validatingadmissionpolicy.md) | 原生准入策略、CEL 表达式 (v1.30+) | ⭐⭐⭐ |

### 调度与扩缩容 (26-28)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 26 | [PriorityClass/RuntimeClass](./26-priorityclass-runtimeclass.md) | 优先级抢占、运行时类、DRA | ⭐⭐⭐⭐ |
| 27 | [HPA v2](./27-hpa-autoscaling-v2.md) | 水平扩缩容、自定义指标、行为策略 | ⭐⭐⭐⭐⭐ |
| 28 | [PodDisruptionBudget](./28-poddisruptionbudget-reference.md) | Pod 中断预算、升级保护 | ⭐⭐⭐⭐ |

### 扩展与 API 管理 (29-31)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 29 | [CustomResourceDefinition](./29-customresourcedefinition.md) | CRD 开发、Schema 验证、CEL 规则 | ⭐⭐⭐⭐ |
| 30 | [APIService](./30-apiservice-aggregation.md) | API 聚合、Metrics Server | ⭐⭐⭐ |
| 31 | [FlowSchema/PriorityLevel](./31-api-priority-fairness.md) | API 优先级与公平性 | ⭐⭐⭐ |

### 辅助资源与集群配置 (32-34)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 32 | [Lease/Event/Node](./32-lease-event-node.md) | 协调资源、事件、节点管理 | ⭐⭐⭐ |
| 33 | [kubeadm 集群引导](./33-kubeadm-cluster-bootstrap.md) | 集群初始化、节点加入、高可用配置 | ⭐⭐⭐⭐ |
| 34 | [组件配置](./34-component-configuration.md) | [[entities/kubelet.md|kubelet]]/KubeProxy/Scheduler/ControllerManager | ⭐⭐⭐⭐ |

### 高级模式与生态工具 (35-36)
| # | 文档 | 关键内容 | 适用层级 |
|:---:|:---|:---|:---|
| 35 | [高级 Pod 模式](./35-advanced-pod-patterns.md) | Init/Sidecar 容器、亲和性、拓扑分布、探针 | ⭐⭐⭐⭐⭐ |
| 36 | [[entities/helm.md|helm]]/ArgoCD](./36-ecosystem-kustomize-helm-argocd.md) | 生态工具 YAML 配置参考 | ⭐⭐⭐⭐ |

---

## 学习路径建议

### 🥇 初级阶段 (入门基础)
**01 → 03 → 04 → 08 → 13 → 14**
掌握 YAML 语法和核心资源配置，能编写基本的应用部署清单

### 🥈 中级阶段 (生产实践)
**02 → 05 → 06 → 07 → 10 → 15 → 16 → 20 → 22 → 27 → 28**
深入工作负载、存储和安全配置，能管理生产环境

### 🥇 高级阶段 (专家技能)
**全部文档，重点: 11/12/25/29/31/34/35/36**
精通所有资源类型和高级模式，能设计企业级 Kubernetes 平台

---

## 快速查询索引

### 按资源类别
| 类别 | 文档编号 |
|------|---------|
| **工作负载** | 03, 04, 05, 06, 07 |
| **网络与服务** | 08, 09, 10, 11, 12, 22 |
| **配置与存储** | 13, 14, 15, 16, 17, 18 |
| **安全与 RBAC** | 19, 20, 21, 23, 24, 25 |
| **调度与扩缩容** | 26, 27, 28 |
| **扩展与 API** | 29, 30, 31 |
| **集群管理** | 02, 32, 33, 34 |
| **设计模式** | 35, 36 |

### 按使用频率
| 频率 | 文档编号 |
|------|---------|
| **高频 (日常使用)** | 03, 04, 08, 13, 14, 15, 16, 20, 22, 27 |
| **中频 (生产运维)** | 02, 05, 06, 07, 10, 17, 19, 21, 23, 26, 28 |
| **低频 (高级场景)** | 09, 11, 12, 18, 24, 25, 29, 30, 31, 32 |
| **专家 (平台工程)** | 33, 34, 35, 36 |

---

## 技术栈覆盖

✅ **工作负载**: Pod, Deployment, StatefulSet, DaemonSet, Job, CronJob, ReplicaSet
✅ **服务网络**: Service, Endpoints, EndpointSlice, Ingress, Gateway API (全系列)
✅ **配置管理**: ConfigMap, Secret (8种类型)
✅ **持久存储**: PV, PVC, StorageClass, VolumeSnapshot, CSI 驱动资源
✅ **安全控制**: RBAC (Role/ClusterRole), NetworkPolicy, PSS/PSA, Admission Webhook/Policy
✅ **调度扩缩**: PriorityClass, RuntimeClass, HPA v2, PDB, DRA
✅ **扩展机制**: CRD, APIService, FlowSchema/PriorityLevelConfiguration
✅ **集群引导**: kubeadm (ClusterConfiguration/Init/Join), 组件配置
✅ **辅助资源**: Namespace, ResourceQuota, LimitRange, Lease, Event, Node
✅ **生态工具**: Kustomize, Helm, ArgoCD

---

## 与其他域的关系

本域作为 **配置字典和快速参考手册**，与其他域形成互补：

| 相关域 | 互补关系 |
|-------|---------|
| **[Domain-4: 工作负载管理](../工作负载)** | Domain-4 讲"如何运维"，Domain-32 讲"如何配置" |
| **[Domain-5: 网络管理](../网络)** | Domain-5 讲网络原理与调优，Domain-32 讲 Service/Ingress/Gateway YAML 规格 |
| **[Domain-6: 存储管理](../存储)** | Domain-6 讲存储运维，Domain-32 讲 PV/PVC/StorageClass YAML 规格 |
| **[Domain-7: 安全合规](../安全)** | Domain-7 讲安全体系，Domain-32 讲 RBAC/NetworkPolicy/PSS YAML 规格 |
| **[Domain-10: 扩展生态](../专项技术)** | Domain-10 讲 Operator 开发流程，Domain-32 讲 CRD/APIService YAML 规格 |

---

**维护者**: Kusheet Project Team | **许可证**: MIT

## Related

- [[helm]]
- [[README]]
- [[entities/kubelet.md|kubelet]]


<!-- risk-assessed -->
