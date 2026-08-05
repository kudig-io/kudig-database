---
title: "多集群网络联邦"
description: "多集群网络联邦：Submariner、Liqo、Cilium ClusterMesh、跨集群 Service 与网络策略同步"
summary: "面向 SRE 与网络工程师的多集群网络联邦完整指南，覆盖 Submariner、Liqo、Cilium ClusterMesh 三大方案的选型、部署、跨集群 Service 与故障排查。"
category: 网络
tags:
- multicluster
- submariner
- liqo
- cilium
- clustermesh
- networking
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
estimated_read_time: 20min
intent_queries:
- "如何实现跨 Kubernetes 集群的 Pod 网络互通"
- "Submariner 与 Cilium ClusterMesh 如何选择"
- "跨集群 Service 如何暴露"
trigger_keywords:
- multicluster
- submariner
- liqo
- cilium clustermesh
- cross-cluster
- federation
prerequisites:
- kubectl-basics
- networking-basics
- cni-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 多集群网络联邦

> **适用版本**: Submariner 0.17+ / Cilium 1.15+ / Liqo 0.10+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

在多集群架构中，每个 Kubernetes 集群默认拥有独立的 Pod CIDR 和 Service CIDR，集群之间的 Pod 和 Service 无法直接通信。这意味着，即使你在两个集群上分别部署了前端和后端服务，它们也无法像在同一个集群内那样通过 Service 名称互相访问。多集群网络联邦要解决的核心问题就是：让分布在不同集群、不同云平台、不同地理区域的工作负载，能够像在同一集群内一样透明地互相访问。

这个问题的重要性怎么强调都不过分。没有网络联邦，多集群就只是多个孤立的孤岛，工作负载分发（参见 [[02-工作负载/04-多语言运行时/05-multicluster-workload-distribution.md|多集群工作负载分发]]）即使把 Pod 分发到了多个集群，它们之间也无法通信，多集群架构的价值就无从体现。

本文深入对比三大主流方案——Submariner、Cilium ClusterMesh、Liqo，覆盖架构原理、生产部署、跨集群 Service 暴露、网络策略同步与故障排查。已有的多集群网络基础概念可以参考 [[05-网络/01-K8s网络核心/34-multi-cluster-networking.md|多集群网络]]。

---

## 核心概念

### 1. 多集群网络的核心挑战

跨集群网络互通面临一系列单集群内不存在的挑战，理解这些挑战是正确选型和排障的基础。

| 挑战 | 说明 |
|------|------|
| CIDR 重叠 | 各集群 Pod/Service CIDR 可能冲突 |
| 跨集群路由 | Pod 流量如何跨集群转发 |
| 服务发现 | 如何发现并访问其他集群的 Service |
| 加密 | 跨公网/不可信网络的流量加密 |
| 网络策略 | NetworkPolicy 是否跨集群生效 |

CIDR 重叠是最棘手的问题。如果两个集群都使用了 10.244.0.0/16 作为 Pod CIDR（这是很多 CNI 的默认值），那么当一个集群的 Pod 试图访问另一个集群的 10.244.1.5 时，它无法确定这个地址是本集群的还是远端集群的。解决 CIDR 重叠有两种思路：一是在建集群时就规划不重叠的网段（最佳实践），二是使用 NAT 或 GlobalCIDR 技术在运行时进行地址转换。

### 2. 三大方案对比

| 维度 | Submariner | Cilium ClusterMesh | Liqo |
|------|-----------|-------------------|------|
| 实现层 | 独立组件（Gateway） | CNI 内置（eBPF） | 独立 + 网络/资源 |
| CNI 依赖 | 任意 CNI | 必须 Cilium | 任意 CNI |
| CIDR 重叠 | 支持（GlobalCIDR） | 需不重叠或别名 | 支持（NAT） |
| 服务发现 | Lighthouse（DNS） | 跨集群 Service | 虚拟 Service |
| 加密 | IPSec/WireGuard | eBPF + IPSec/WireGuard | WireGuard |
| 资源联邦 | 否（仅网络） | 否 | 是（Pod 卸载） |
| 适用场景 | 异构 CNI 多集群 | 全 Cilium 集群 | 资源弹性扩展 |

Submariner 的最大优势是 CNI 无关性——它作为独立的网络层叠加在任何 CNI 之上，不要求所有集群使用相同的 CNI。这对于异构环境（比如部分集群用 Calico、部分用 Flannel）特别有价值。它通过在每个集群部署 Gateway 节点建立跨集群隧道，通过 Lighthouse 组件实现跨集群 DNS 服务发现。

Cilium ClusterMesh 是 Cilium CNI 的内置能力，它利用 eBPF 在数据平面直接实现跨集群路由，性能开销极小。但它要求所有参与集群都使用 Cilium 作为 CNI，这在异构环境中是一个硬约束。ClusterMesh 的优势在于零额外组件、与 Cilium 的网络策略和服务网格能力深度集成。

Liqo 走了一条独特的路线——它不仅做网络互通，还做资源联邦。通过 Liqo，一个集群可以将 Pod "卸载"到另一个集群执行，实现跨集群的弹性伸缩。这使得 Liqo 特别适合需要突发容量扩展的场景，比如一个集群资源不足时，自动将部分负载溢出到另一个集群。

### 3. 连接模式

从数据平面的角度，跨集群连接主要有三种模式。Gateway 模式（Submariner）在每个集群部署专用的 Gateway 节点，所有跨集群流量都经过 Gateway 之间的加密隧道，优点是集中管控、易于监控，缺点是 Gateway 可能成为瓶颈和单点。全网格模式（Cilium ClusterMesh）在各集群的节点之间直接建立 eBPF 隧道，流量不需要经过中心节点，性能更好但连接数随集群数平方增长。Peering 模式（Liqo）建立集群间的对等关系，在此基础上实现网络互通和资源卸载。

---

## 生产部署/实现

### 1. Submariner 部署 🔴

Submariner 的部署涉及修改集群网络配置，是一个高风险操作。它使用一个中心化的 Broker 来协调各集群的连接信息。

```bash
# 🔴 高风险：修改集群网络，配置错误导致集群间不通
# 使用 subctl 部署（前提：集群已用 subctl 准备 broker）
# 1. 在集群 A 部署 broker
subctl deploy-broker --kubeconfig=cluster-a.config

# 2. 加入集群
subctl join broker-info.subm --kubeconfig=cluster-a.config --clusterid cluster-a --natt
subctl join broker-info.subm --kubeconfig=cluster-b.config --clusterid cluster-b --natt

# 3. 验证连接
subctl verify --kubeconfig=cluster-a.config --tocontext cluster-b
subctl show all --kubeconfig=cluster-a.config
```

--natt 参数启用 NAT 穿越，这对于集群位于 NAT 网络后面（大多数云环境都是如此）的情况是必需的。Broker 是一个轻量级的协调服务，它存储各集群的连接端点信息，本身不转发数据流量。部署完成后，subctl verify 会自动运行一系列连通性测试，包括 Pod 间 ping、Service 访问、DNS 解析等，是验证部署成功的关键步骤。

导出跨集群 Service：

```yaml
# 🟡 中风险：ServiceExport 使 Service 跨集群可见
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: payment-service
  namespace: production
---
# 在另一集群通过 cluster-a.payment-service.production.svc.clusterset.local 访问
```

Submariner 的服务发现基于 Lighthouse 组件，它实现了 Multi-Cluster Services API（MCS API）。创建 ServiceExport 后，该 Service 就会被注册到跨集群 DNS 中，其他集群可以通过 cluster-a.payment-service.production.svc.clusterset.local 这样的域名来访问它。

### 2. Cilium ClusterMesh 部署 🔴

```bash
# 🔴 高风险：启用 ClusterMesh 改变 Cilium 网络行为
# 前提：所有集群使用 Cilium 且 Pod CIDR 不重叠（或配置别名）
# 1. 启用 ClusterMesh
cilium clustermesh enable --context cluster-a --service-type LoadBalancer
cilium clustermesh enable --context cluster-b --service-type LoadBalancer

# 2. 等待证书就绪后建立连接
cilium clustermesh status --context cluster-a --wait

# 3. 连接集群
cilium clustermesh connect --context cluster-a --destination-context cluster-b

# 4. 验证
cilium clustermesh status --context cluster-a
```

ClusterMesh 的部署比 Submariner 更简洁，因为它不需要额外的 Broker 组件——集群间的连接信息通过 Kubernetes Secret 交换。enable 命令会在每个集群部署 clustermesh-apiserver 组件并生成 TLS 证书，connect 命令交换两个集群的证书和端点信息，建立双向连接。

跨集群 Service（Cilium 自动同步带 io.cilium/global: "true" 标签的 Service）：

```yaml
# 🟡 中风险
apiVersion: v1
kind: Service
metadata:
  name: user-service
  namespace: production
  labels:
    io.cilium/global: "true"     # 标记为全局 Service
spec:
  selector:
    app: user-service
  ports:
  - port: 80
    targetPort: 8080
```

Cilium ClusterMesh 的服务发现机制与 Submariner 不同：它不依赖额外的 DNS 组件，而是将标记为 global 的 Service 直接同步到所有连接集群的 Cilium 数据平面中。访问方式是通过 cluster-b.user-service.production.svc.clusterset.local 域名，Cilium 的 eBPF 数据平面会自动将流量路由到拥有该 Service 后端的集群。

### 3. Liqo Peering 部署 🟡

```bash
# 🟡 中风险：建立集群对等关系
# 1. 安装 Liqo
liqoctl install kubeadm --cluster-name cluster-a --kubeconfig=cluster-a.config
liqoctl install kubeadm --cluster-name cluster-b --kubeconfig=cluster-b.config

# 2. 建立 peering（生成 peer token）
liqoctl generate peer-token --kubeconfig=cluster-a.config > peer-token.yaml
liqoctl peer --kubeconfig=cluster-b.config --peer-token=peer-token.yaml

# 3. 验证
liqoctl status --kubeconfig=cluster-a.config
```

Liqo 的 peering 建立后，两个集群之间不仅实现了网络互通，还建立了资源虚拟化的通道。cluster-b 可以在 cluster-a 中创建一个"虚拟节点"，将 Pod 调度到这个虚拟节点时，Pod 实际会在 cluster-b 中运行，但对 cluster-a 来说它就像是本地 Pod 一样。

---

## 运维操作

### 1. 跨集群连通性验证 🟢

```bash
# 🟢 低风险：只读
# Submariner
subctl verify --kubeconfig=cluster-a.config --tocontext cluster-b --verbose
subctl show connections --kubeconfig=cluster-a.config

# Cilium
cilium clustermesh status --context cluster-a
kubectl --context cluster-a exec deploy/test -- curl http://cluster-b.user-service.production.svc.clusterset.local

# 通用：跨集群 Pod ping
kubectl --context cluster-a exec deploy/test -- ping <cluster-b-pod-ip>
```

连通性验证应该覆盖三个层面：Pod IP 层面的直接连通性、Service 层面的跨集群访问、DNS 层面的跨集群服务发现。subctl verify 是最全面的验证工具，它会自动测试所有这三个层面。

### 2. 网络策略同步 🟡

```yaml
# 🟡 中风险：跨集群 NetworkPolicy（需各方案支持）
# Submariner 不直接同步 NetworkPolicy，需在各集群分别部署
# Cilium ClusterMesh 中 CiliumNetworkPolicy 可跨集群生效
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: allow-cross-cluster
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: user-service
  ingress:
  - fromEndpoints:
    - matchLabels:
        io.kubernetes.pod.namespace: production
        app: payment-service
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
```

跨集群网络策略是一个容易被忽视的安全问题。在单集群中，NetworkPolicy 可以精确控制 Pod 间的访问，但在多集群环境中，标准的 NetworkPolicy 无法感知远端集群的 Pod。Cilium ClusterMesh 通过 CiliumNetworkPolicy 的 fromEndpoints 可以匹配远端集群的 Pod 标签，实现跨集群的细粒度访问控制。而 Submariner 目前不支持跨集群策略同步，需要在每个集群分别部署对应的策略。

### 3. 监控跨集群链路 🟢

```bash
# 🟢 低风险
# Submariner Gateway 状态
kubectl -n submariner-operator get gateways.submariner.io -A
# Cilium 跨集群 endpoint
cilium endpoint list --context cluster-a | grep -i remote
```

---

## 故障排查

### 症状 1：跨集群 Service 无法解析

```bash
# 🟢 低风险
kubectl --context cluster-a exec deploy/test -- nslookup cluster-b.payment-service.production.svc.clusterset.local
kubectl -n submariner-operator logs -l app=submariner-lighthouse-agent
```

根因可能是 ServiceExport 未创建（Submariner）、Lighthouse DNS 组件未正常运行、或者 ClusterMesh 连接未建立。处置方法是确认 ServiceExport 已创建、检查 DNS 组件的日志和状态、验证 clustermesh status 显示连接正常。

### 症状 2：跨集群 Pod 不通

根因可能是 CIDR 重叠导致路由冲突、Gateway 节点未就绪、防火墙阻断了 UDP 4500（IPSec NAT-T）或 UDP 4800（WireGuard）端口、或者 CNI 路由表中缺少跨集群路由。处置方法是使用 GlobalCIDR 解决重叠问题、检查 Gateway 状态、放行加密协议端口、检查 CNI 路由表。

### 症状 3：连接间歇性中断

根因可能是 Gateway 节点不稳定（资源不足或被驱逐）、NAT-T keepalive 配置不当导致隧道超时断开、或者多 Gateway 主备切换时的短暂中断。处置方法是固定 Gateway 节点（通过 label 和 nodeSelector）、调整 keepalive 间隔、部署多个 Gateway 节点实现高可用。

### 症状 4：ClusterMesh 证书错误

根因是集群间的 CA 证书未正确交换，或者节点时间不同步导致证书验证失败。处置方法是重新执行 clustermesh connect 交换证书、确保所有节点通过 NTP 同步时间。

### 排查决策树

```
跨集群异常
├── DNS 解析失败? → ServiceExport/Lighthouse/ClusterMesh
├── Pod 不通?     → CIDR/Gateway/防火墙/路由
├── 间歇中断?     → Gateway 稳定性/keepalive
└── 证书错误?     → 重连/时间同步
```

---

## 最佳实践

第一，选型上，异构 CNI 环境选 Submariner，全 Cilium 环境选 ClusterMesh，需要资源弹性扩展选 Liqo。第二，CIDR 规划是根本，建集群时就规划不重叠的 Pod 和 Service CIDR，避免后续 NAT 带来的复杂度。第三，跨公网连接必须启用 IPSec 或 WireGuard 加密，内网环境可以在评估性能影响后选择。第四，Submariner 部署多个 Gateway 节点实现主备高可用，避免单点故障。第五，统一使用 clusterset.local 域名规范进行跨集群服务发现，配合 [[05-网络/01-K8s网络核心/52-dns-advanced-external-integration.md|DNS 高级与外部集成]] 进行优化。第六，监控跨集群链路的延迟、丢包率和 Gateway 状态，建立告警。第七，跨集群安全策略需要显式设计，不要假设单集群的 NetworkPolicy 会自动在跨集群场景生效。第八，定期验证跨集群连通性并演练故障切换。

```yaml
# 🟢 低风险：Submariner 连接监控告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: submariner-alerts
spec:
  groups:
  - name: submariner
    rules:
    - alert: SubmarinerConnectionDown
      expr: submariner_connection_status == 0
      for: 3m
      labels:
        severity: critical
```

---

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|----------|
| 跨集群 DNS 解析失败 | ServiceExport 未创建 | `kubectl get serviceexport -A` | 创建 ServiceExport 资源 |
| 跨集群 Pod 不通 | CIDR 重叠/防火墙 | `subctl show connections` | 使用 GlobalCIDR/放行端口 |
| 连接间歇中断 | Gateway 不稳定 | `kubectl get gateways -A` | 固定 Gateway 节点/多副本 |
| 证书错误 | CA 未同步/时间不同步 | `cilium clustermesh status` | 重新 connect/NTP 同步 |
| 性能下降 | 隧道 MTU 不匹配 | `ping -M do -s 1400 <target>` | 调整 MTU |

## 相关工具

| 工具 | 用途 |
|------|------|
| `subctl` | Submariner 管理 |
| `cilium` | ClusterMesh 管理 |
| `liqoctl` | Liqo 管理 |

## Related

- [[05-网络/01-K8s网络核心/34-multi-cluster-networking.md|多集群网络]]
- [[05-网络/01-K8s网络核心/33-multi-cluster-federation.md|多集群联邦]]
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构基础]]
- [[05-网络/01-K8s网络核心/52-dns-advanced-external-integration.md|DNS 高级与外部集成]]
- [[02-工作负载/04-多语言运行时/05-multicluster-workload-distribution.md|多集群工作负载分发]]
- [[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice.md|NetworkPolicy 深度实践]]
