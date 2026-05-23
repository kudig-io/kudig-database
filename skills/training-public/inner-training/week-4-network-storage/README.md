---
title: 'Week 4: 网络与存储 (Day 22-28)'
description: '# Week 4: 网络与存储 (Day 22-28)'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- helm
- daemonset
- ingress
- networkpolicy
- crd
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 4: 网络与存储 (Day 22-28) 是什么'
- '如何 Week 4: 网络与存储 (Day 22-28)'
trigger_keywords:
- Week
- '4:'
- 网络与存储
- Day
- 22-28
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Week 4: 网络与存储 (Day 22-28)

```yaml
---
title: Week 4: 网络与存储
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes网络存储培训"
  - "Week4培训内容"
  - "Service Ingress学习"
  - "PV PVC StorageClass"
trigger_keywords:
  - "Week4"
  - "网络与存储"
  - "Service"
  - "Ingress"
  - "Terway"
  - "Flannel"
  - "PV"
  - "PVC"
  - "StorageClass"
  - "CNI"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 15min
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-3-node-workload
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-28-comprehensive-review
id: WEEK4-INDEX
topic: training
type: week-index
tags: [week-4, networking, storage, service, ingress, k8s, k8s-1.28-1.33]
---
```

## 概述

第四周聚焦于 K8s 中两个最核心的基础设施领域：**网络**和**存储**。在前面三周中，你已经掌握了集群管理、安全监控和节点工作负载管理。本周将深入理解 K8s 的网络模型和存储体系，这是运行生产级应用的基础。

网络是 K8s 中最复杂的子系统之一。K8s 的网络模型要求每个 Pod 都有独立的 IP 地址，且 Pod 之间可以直接通信，无需 NAT。这个看似简单的要求背后涉及 CNI 插件、Service 负载均衡、Ingress 路由等多个层次的协作。存储则是有状态应用（如数据库、消息队列）运行的基石，理解 PV/PVC/StorageClass 的工作机制对于管理有状态应用至关重要。

### 学习目标

- 理解 Service 的三种类型（ClusterIP / NodePort / LoadBalancer）并能够根据场景选择
- 掌握 Ingress 路由规则的配置，了解 ALB Ingress Controller 和 Nginx Ingress Controller 的差异
- 深入理解 Terway 和 Flannel 两种 CNI 方案的架构差异与适用场景
- 能够创建、挂载和管理存储卷（PV/PVC/StorageClass）
- 完成 4 周培训的综合复习
- **产出**: 配置 Service 和 Ingress 暴露应用、完成 PV/PVC 存储管理

---

## 核心概念详解

### Service 类型详解

Service 是 K8s 中最基础的服务暴露机制。它为一组 Pod 提供稳定的访问入口（ClusterIP）和负载均衡能力。理解 Service 的关键在于掌握 Endpoints 机制——Service 不是直接转发到 Pod，而是通过 Label Selector 选择匹配的 Pod，将其 IP 和端口记录在 Endpoints 对象中，然后由 kube-proxy 配置的 iptables/IPVS 规则将流量转发到 Endpoints 中的 Pod。

**ClusterIP** 是默认的 Service 类型。它分配一个集群内部的虚拟 IP 地址，只能在集群内部访问。适合集群内部服务间通信，如应用访问数据库。ClusterIP 的 IP 地址来自集群的 Service CIDR（创建集群时指定），不会与 Pod IP 冲突。

**NodePort** 在 ClusterIP 的基础上，在每个节点上开放一个固定的端口（默认范围 30000-32767）。外部流量可以通过 `<任意节点IP>:<NodePort>` 访问到 Service。NodePort 适合测试环境或小规模暴露服务，但不适合生产环境的大流量场景——端口范围有限且不够灵活。

**LoadBalancer** 在 NodePort 的基础上，自动创建一个外部负载均衡器（如阿里云 SLB）。LoadBalancer 会分配一个外部 IP，流量经过负载均衡器 → NodePort → kube-proxy → Pod 的路径到达目标。这是生产环境暴露 HTTP/HTTPS 服务最常用的方式（在 Ingress 不适用时）。

Service 流量路径对比：

| 类型 | 流量路径 | 延迟 | 适用场景 |
|------|---------|------|---------|
| ClusterIP | Client → kube-proxy → Pod | 1 跳 | 集群内部通信 |
| NodePort | Client → Node:Port → kube-proxy → Pod | 1-2 跳 | 测试环境 |
| LoadBalancer | Client → SLB → Node:Port → kube-proxy → Pod | 2-3 跳 | 生产环境 TCP/UDP |
| Ingress | Client → ALB/Nginx → Service → Pod | 2 跳 | 生产环境 HTTP/HTTPS |

### Ingress 深入理解

Ingress 是 K8s 中 HTTP/HTTPS 层的路由规则。相比 Service（四层 TCP/UDP 负载均衡），Ingress 工作在七层（应用层），支持基于域名和 URL 路径的路由。

Ingress 的工作原理：Ingress Controller（如 Nginx Ingress Controller、ALB Ingress Controller）持续监听集群中的 Ingress 资源变化，并根据规则配置实际的负载均衡器。当外部请求到达时，Ingress Controller 根据域名和路径将请求路由到对应的后端 Service。

**Nginx Ingress Controller** 是社区最流行的 Ingress Controller。它在集群内部以 [[DaemonSet|DaemonSet]] 或 Deployment 方式运行 Nginx 反向代理，通过 Watch Ingress 资源动态生成 Nginx 配置。优势是功能丰富、社区支持好；缺点是流量需要经过 Nginx 中转，增加了一跳延迟。

**ALB Ingress Controller** 是阿里云提供的 Ingress Controller。它直接将 Ingress 规则映射到阿里云 ALB（Application Load Balancer）实例，无需在集群内部运行代理。优势是性能好、与阿里云生态集成；缺点是依赖阿里云 ALB 服务。

Ingress Controller 对比：

| 特性 | Nginx Ingress | ALB Ingress |
|------|-------------|------------|
| 部署位置 | 集群内部 | 阿里云托管 |
| 性能 | 中（需中转） | 高（直连） |
| 功能丰富度 | 非常丰富 | 丰富 |
| 配置方式 | annotations | annotations + CRD |
| 依赖 | 无外部依赖 | 需要阿里云 ALB |
| 费用 | 集群内资源 | ALB 实例费用 |

### CNI 网络方案对比

在阿里云 ACK 中，有两种主要的 CNI 方案：**Terway** 和 **Flannel**。

**Terway** 是阿里云开发的 CNI 插件，它直接使用阿里云的 ENI（弹性网卡）为 Pod 分配 IP。Terway 支持两种模式：

- **ENI 模式**: 每个 Pod 使用独立的辅助 ENI，Pod 拥有与节点相同网络平面的 IP 地址。优势是网络性能接近原生、支持网络策略；缺点是每个节点的 Pod 数量受 ENI 配额限制
- **ENIIP 模式**: 在辅助 ENI 上分配辅助私有 IP，一个 ENI 可以为多个 Pod 提供 IP。这是 Terway 的推荐模式，在性能和密度之间取得平衡

**Flannel** 是社区最简单的 CNI 插件之一。它使用 VxLAN 覆盖网络（Overlay）在节点间建立隧道。Flannel 的优势是配置简单、不依赖特定的网络基础设施；缺点是 VxLAN 封装带来一定的性能开销、不支持 [[Kubernetes|Kubernetes]] [[NetworkPolicy|NetworkPolicy]]。

选择建议：如果使用阿里云 ACK 且需要 NetworkPolicy，选择 Terway。如果需要简单的网络方案且对网络策略没有要求，可以选择 Flannel。

### 存储体系详解

K8s 的存储体系由三个核心概念组成：

**PersistentVolume（PV）** 是集群管理员提供的存储资源。它代表了一块已经配置好的存储（如一块云盘、一个 NAS 文件系统）。PV 的生命周期独立于 Pod——PV 创建后可以被不同的 Pod 反复使用。

**PersistentVolumeClaim（PVC）** 是用户对存储的"申请"。PVC 声明了所需的存储大小、访问模式等。K8s 的 PersistentVolume Controller 会自动将 PVC 绑定到满足条件的 PV。

**StorageClass** 提供了动态供给能力。当 PVC 指定了 StorageClass 时，K8s 会自动调用 CSI 驱动创建对应的存储资源并生成 PV，无需管理员预先创建。这在生产环境中极大简化了存储管理。

阿里云 ACK 支持的存储类型：

- **云盘（Disk）**: 块存储，适合低延迟、高 IOPS 的场景（如数据库）。支持 ESSD（PL0/PL1/PL2/PL3）等多种性能级别。访问模式为 ReadWriteOnce（RWO），即只能被单个 Pod 挂载
- **NAS**: 文件存储，适合共享文件访问的场景。支持 ReadWriteMany（RWX），即可以被多个 Pod 同时挂载。NAS 不需要预配置容量，按实际使用量计费
- **OSS**: 对象存储，通过 fuse 挂载到 Pod 中。适合读取大量小文件或静态资源。性能不如云盘和 NAS，但成本最低

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 | 预计时间 |
|:---:|------|------|:-------:|
| Day 22 | Service 基础 | [day-22-service-basics.md](day-22-service-basics.md) | 4-5h |
| Day 23 | Ingress | [day-23-ingress.md](day-23-ingress.md) | 4-5h |
| Day 24 | Terway 网络 | [day-24-terway-cni.md](day-24-terway-cni.md) | 4-5h |
| Day 25 | Flannel 网络 | [day-25-flannel-cni.md](day-25-flannel-cni.md) | 4-5h |
| Day 26 | 存储卷创建 & 删除 | [day-26-storage-create-delete.md](day-26-storage-create-delete.md) | 4-5h |
| Day 27 | 存储卷挂载 | [day-27-storage-mount.md](day-27-storage-mount.md) | 4-5h |
| Day 28 | 综合复习与实践 | [day-28-comprehensive-review.md](day-28-comprehensive-review.md) | 4-5h |

### 本周自测

完成全部学习后，请进行自测: [checkpoint.md](checkpoint.md)

### 实践项目

- [P4: 网络与存储综合实践](../projects/p4-network-storage-practice.md)
- [P5: 毕业综合项目](../projects/p5-graduation-project.md)

---

## 配置示例

### Nginx Ingress Controller 安装

```bash
# 使用 Helm 安装 Nginx Ingress Controller
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update
helm install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace \
  --set controller.service.type=LoadBalancer
```

### 多路径 Ingress 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: multi-path-ingress
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/use-regex: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
      - path: /api(/|$)(.*)
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 80
      - path: /ws
        pathType: Prefix
        backend:
          service:
            name: websocket
            port:
              number: 8080
```

---

## 常见问题

### Q1: Service 的 ClusterIP 无法从集群外部访问怎么办？

ClusterIP 只能在集群内部访问，这是设计如此。如果需要从外部访问，有三种方式：1) 使用 NodePort 类型的 Service；2) 使用 LoadBalancer 类型的 Service；3) 使用 Ingress（推荐用于 HTTP/HTTPS 服务）。

### Q2: Terway 和 Flannel 可以在同一个集群中共存吗？

不建议。一个集群通常只使用一种 CNI 插件。如果确实需要不同的网络配置，可以考虑使用 Multus CNI 来管理多个网络接口。但 Multus 增加了管理复杂度，非特殊需求不建议使用。

### Q3: PVC 一直处于 Pending 状态怎么排查？

PVC Pending 通常有几个原因：1) 没有满足条件的 PV（大小不够、访问模式不匹配）；2) StorageClass 不存在或配置错误；3) 动态供给失败（CSI 驱动异常、云服务配额不足）。排查步骤：`kubectl describe pvc <name>` 查看 Events。

### Q4: 云盘和 NAS 如何选择？

选择依据：如果应用需要低延迟和高 IOPS（如数据库），使用云盘；如果需要多个 Pod 共享文件（如 Web 应用的静态资源、日志目录），使用 NAS；如果只是读写少量文件且成本敏感，使用 OSS。

### Q5: 如何排查 Service 无法访问的问题？

排查步骤：1）`kubectl get endpoints <svc-name>` 确认 Endpoints 是否有后端 Pod；2）如果没有 Endpoints，检查 Service 的 selector 是否与 Pod 的 labels 匹配；3）如果有 Endpoints 但仍无法访问，检查 kube-proxy 日志和 iptables/IPVS 规则；4）检查 NetworkPolicy 是否阻止了流量。

### Q6: Ingress 配置了但不生效怎么办？

排查步骤：1）`kubectl get ingress` 确认 Ingress 资源已创建且 ADDRESS 已分配；2）`kubectl describe ingress <name>` 查看 Events 和后端状态；3）确认 Ingress Controller 正在运行；4）检查 Ingress 的 annotations 是否正确；5）如果使用 ALB Ingress，检查 alb-config 是否配置正确。

---

## 要点总结

| 主题 | 关键知识点 | 学习日 |
|------|-----------|--------|
| Service | 三种类型、Endpoints、负载均衡 | Day 22 |
| Ingress | 路由规则、TLS、Controller 选择 | Day 23 |
| Terway | ENI 模式、ENIIP 模式、NetworkPolicy | Day 24 |
| Flannel | VxLAN 覆盖网络 | Day 25 |
| 存储创建 | PV/PVC/StorageClass、动态供给 | Day 26 |
| 存储挂载 | 云盘/NAS/OSS 挂载实践 | Day 27 |
| 综合复习 | 4 周知识回顾 | Day 28 |

---

## 延伸阅读

- [网络架构总览](../../domain-03-networking-traffic/01-network-architecture-overview.md)
- [CNI 架构基础](../../domain-03-networking-traffic/02-cni-architecture-fundamentals.md)
- [Service 概念与类型](../../domain-03-networking-traffic/06-service-concepts-types.md)
- [Ingress 基础](../../domain-03-networking-traffic/19-ingress-fundamentals.md)
- [Nginx Ingress 完整指南](../../domain-03-networking-traffic/21-nginx-ingress-complete-guide.md)
- [存储架构总览](../../domain-04-storage-data/01-storage-architecture-overview.md)
- [PV 架构基础](../../domain-04-storage-data/02-pv-architecture-fundamentals.md)
- [StorageClass 动态供给](../../domain-04-storage-data/04-storageclass-dynamic-provisioning.md)
- [ACK 服务总览](../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md)

---

## 自测题 (Self-Check)

### Q1. Kubernetes Service 的 ClusterIP 是如何实现的?

<details>
<summary>查看答案</summary>

kube-proxy 通过 iptables 或 IPVS 规则将 ClusterIP:Port 的流量 DNAT 到后端 Pod 的 PodIP:TargetPort。

</details>

### Q2. Ingress 和 Gateway API 的区别?

<details>
<summary>查看答案</summary>

Ingress 仅支持 HTTP/HTTPS, 功能有限 (需注解扩展); Gateway API 支持 HTTP/gRPC/TCP/TLS/UDP, 原生流量分割, 角色分离 (GatewayClass→Gateway→Route)。

</details>

### Q3. StatefulSet 的 Pod 为什么有稳定的网络标识?

<details>
<summary>查看答案</summary>

StatefulSet 创建的 Pod 名称格式为 <statefulset-name>-<ordinal>, 配合 Headless Service 创建 DNS 记录 <pod-name>.<service-name>.<namespace>.svc.cluster.local。

</details>

### Q4. 如何选择 CNI 插件?

<details>
<summary>查看答案</summary>

Calico (通用, 支持 BGP/VXLAN, NetworkPolicy)、Cilium (eBPF, 高性能, 丰富 NetworkPolicy)、Flannel (简单, 仅 VXLAN, 无 NetworkPolicy)。生产推荐 Cilium 或 Calico。

</details>

### Q5. PVC 的三种访问模式?

<details>
<summary>查看答案</summary>

ReadWriteOnce (单节点读写)、ReadOnlyMany (多节点只读)、ReadWriteMany (多节点读写)。并非所有存储后端都支持全部模式。

</details>


## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
