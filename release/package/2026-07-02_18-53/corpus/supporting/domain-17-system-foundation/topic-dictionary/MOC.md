---
title: topic-dictionary MOC
description: topic-dictionary 专题导航页，覆盖 207 篇文档
summary: topic-dictionary 专题导航页，覆盖 207 篇文档
category: moc
tags:
- k8s
- moc
- dictionary
- controller-manager
- cilium
- ingress
- gateway
- ebpf
- rag
- gpu
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- topic-dictionary MOC 是什么
- 如何 topic-dictionary MOC
- Kubernetes 17 system foundation 最佳实践
trigger_keywords:
- topic-dictionary
- MOC
- system
- foundation
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-dictionary MOC

> **MOC 版本**: 1.0
> **专题**: topic-dictionary
> **文档数量**: 207 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

运维术语词典 — K8s 运维专业术语解释

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-dictionary |
| **文档数量** | 207 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[domain-17-system-foundation/知识字典/GAP-ANALYSIS.md|[[Topic Dictionary 内容缺口分析（2026 行业最佳实践视角）|Topic Dictionary 内容缺口分析（2026 行业最佳实践视角）]]]] |  | dictionary, reference |  |
| 2 | [[domain-17-system-foundation/知识字典/configuration/configmaps.md|[[ConfigMaps|ConfigMaps]]]] |  | dictionary, reference, configuration |  |
| 3 | [[domain-17-system-foundation/知识字典/configuration/liveness-readiness-and-startup-probes.md|[[Liveness, Readiness, and Startup Probes|Liveness, Readiness, and Startup Probes]]]] |  | dictionary, reference |  |
| 4 | [[domain-17-system-foundation/知识字典/configuration/organizing-cluster-access-using-kubeconfig-files.md|Organizing Cluster Access Using kubeconfig Files]] |  | dictionary, reference, configuration |  |
| 5 | [[domain-17-system-foundation/知识字典/configuration/resource-management-for-pods-and-containers.md|Resource Management for Pods and Containers]] |  | dictionary, reference |  |
| 6 | [[domain-17-system-foundation/知识字典/configuration/resource-management-for-windows-nodes.md|Resource Management for Windows nodes]] |  | dictionary, reference |  |
| 7 | [[domain-17-system-foundation/知识字典/configuration/secrets.md|Secrets]] |  | dictionary, reference |  |
| 8 | [[domain-17-system-foundation/知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]] |  | dictionary, reference |  |
| 9 | [[domain-17-system-foundation/知识字典/fundamentals/annotations.md|注解]] |  | dictionary, reference |  |
| 10 | [[domain-17-system-foundation/知识字典/fundamentals/cloud-controller-manager.md|Cloud Controller Manager（云控制器管理器）]] |  | dictionary, reference |  |
| 11 | [[domain-17-system-foundation/知识字典/fundamentals/communication-between-nodes-and-the-control-plane.md|Communication between Nodes and the Control Plane（节点与控制平面之间的通信）]] |  | dictionary, reference |  |
| 12 | [[domain-17-system-foundation/知识字典/fundamentals/controllers.md|Controllers（控制器）]] |  | dictionary, reference |  |
| 13 | [[domain-17-system-foundation/知识字典/fundamentals/field-selectors.md|字段选择器]] |  | dictionary, reference |  |
| 14 | [[domain-17-system-foundation/知识字典/fundamentals/finalizers.md|Finalizers]] |  | dictionary, reference |  |
| 15 | [[domain-17-system-foundation/知识字典/fundamentals/garbage-collection.md|Garbage Collection（垃圾回收）]] |  | dictionary, reference |  |
| 16 | [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-components.md|Kubernetes 组件]] |  | dictionary, reference |  |
| 17 | [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-concepts-reference.md|知识地图]] |  | dictionary, reference |  |
| 18 | [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-object-management.md|Kubernetes 对象管理]] |  | dictionary, reference |  |
| 19 | [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-self-healing.md|Kubernetes Self-Healing（Kubernetes 自愈能力）]] |  | dictionary, reference |  |
| 20 | [[domain-17-system-foundation/知识字典/fundamentals/labels-and-selectors.md|标签和选择器]] |  | dictionary, reference |  |
| 21 | [[domain-17-system-foundation/知识字典/fundamentals/leases.md|Leases（租约）]] |  | dictionary, reference |  |
| 22 | [[domain-17-system-foundation/知识字典/fundamentals/mixed-version-proxy.md|Mixed Version Proxy（混合版本代理）]] |  | dictionary, reference |  |
| 23 | [[domain-17-system-foundation/知识字典/fundamentals/namespaces.md|命名空间]] |  | dictionary, reference |  |
| 24 | [[domain-17-system-foundation/知识字典/fundamentals/nodes.md|Nodes（节点）]] |  | dictionary, reference |  |
| 25 | [[domain-17-system-foundation/知识字典/fundamentals/object-names-and-ids.md|对象名称和 ID]] |  | dictionary, reference |  |
| 26 | [[domain-17-system-foundation/知识字典/fundamentals/objects-in-kubernetes.md|Kubernetes 中的对象]] |  | dictionary, reference |  |
| 27 | [[domain-17-system-foundation/知识字典/fundamentals/owners-and-dependents.md|所有者和依赖者]] |  | dictionary, reference |  |
| 28 | [[domain-17-system-foundation/知识字典/fundamentals/recommended-labels.md|推荐标签]] |  | dictionary, reference |  |
| 29 | [[domain-17-system-foundation/知识字典/fundamentals/storage-versions.md|存储版本]] |  | dictionary, reference, storage |  |
| 30 | [[domain-17-system-foundation/知识字典/fundamentals/the-kubectl-command-line-tool.md|kubectl 命令行工具]] |  | dictionary, reference |  |
| 31 | [[domain-17-system-foundation/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] |  | dictionary, reference |  |
| 32 | [[domain-17-system-foundation/知识字典/k8s-glossary.md|K8s 中英术语表（Glossary）]] |  | dictionary, reference |  |
| 33 | [[domain-17-system-foundation/知识字典/multi-cloud/edge-computing-and-k3s.md|边缘计算与轻量级 Kubernetes]] |  | dictionary, reference |  |
| 34 | [[domain-17-system-foundation/知识字典/multi-cloud/multi-cloud-operations.md|10 - 多云混合云运维手册]] |  | dictionary, reference |  |
| 35 | [[domain-17-system-foundation/知识字典/multi-cloud/spaceborne-computing.md|太空计算（Spaceborne Computing）]] |  | dictionary, reference |  |
| 36 | [[domain-17-system-foundation/知识字典/networking/cluster-mesh.md|多集群网络互联（Cluster Mesh）]] |  | dictionary, reference |  |
| 37 | [[domain-17-system-foundation/知识字典/networking/cluster-networking.md|集群网络（Cluster Networking）]] |  | dictionary, reference, networking |  |
| 38 | [[domain-17-system-foundation/知识字典/networking/dns-for-services-and-pods.md|DNS for Services and Pods]] |  | dictionary, reference |  |
| 39 | [[domain-17-system-foundation/知识字典/networking/ebpf-and-cilium-networking.md|eBPF 与 Cilium 网络]] |  | dictionary, reference, networking |  |
| 40 | [[domain-17-system-foundation/知识字典/networking/endpointslices.md|EndpointSlices]] |  | dictionary, reference |  |
| 41 | [[domain-17-system-foundation/知识字典/networking/gateway-api.md|Gateway API]] |  | dictionary, reference |  |
| 42 | [[domain-17-system-foundation/知识字典/networking/ingress-controllers.md|Ingress Controllers]] |  | dictionary, reference |  |
| 43 | [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]] |  | dictionary, reference |  |
| 44 | [[domain-17-system-foundation/知识字典/networking/ipv4-ipv6-dual-stack.md|IPv4/IPv6 dual-stack]] |  | dictionary, reference |  |
| 45 | [[domain-17-system-foundation/知识字典/networking/network-policies.md|Network Policies]] |  | dictionary, reference, networking |  |
| 46 | [[domain-17-system-foundation/知识字典/networking/networking-on-windows.md|Networking on Windows]] |  | dictionary, reference, networking |  |
| 47 | [[domain-17-system-foundation/知识字典/networking/service-clusterip-allocation.md|Service ClusterIP allocation]] |  | dictionary, reference |  |
| 48 | [[domain-17-system-foundation/知识字典/networking/service-internal-traffic-policy.md|Service Internal Traffic Policy]] |  | dictionary, reference |  |
| 49 | [[domain-17-system-foundation/知识字典/networking/service-mesh.md|服务网格（Service Mesh）]] |  | dictionary, reference |  |
| 50 | [[domain-17-system-foundation/知识字典/networking/service.md|Service]] |  | dictionary, reference |  |
| ... | 共 207 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 207 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[entities/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[entities/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[entities/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[entities/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[entities/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[entities/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[entities/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- [[domain-03-networking-traffic/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[domain-06-observability/总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[domain-01-cluster-fundamentals/kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[domain-01-cluster-fundamentals/架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[domain-04-storage-data/K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[domain-04-storage-data/K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference
- [[domain-19-landscape-references/领域索引/MOC.md|topic-index MOC]] — Cross-reference


<!-- risk-assessed -->
