---
title: 多集群网络互联（Cluster Mesh）
description: '# 多集群网络互联（Cluster Mesh）'
summary: '# 多集群网络互联（Cluster Mesh）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- prometheus
- istio
- cilium
- coredns
- gateway
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多集群网络互联（Cluster Mesh） 是什么
- 如何 多集群网络互联（Cluster Mesh）
trigger_keywords:
- 多集群网络互联
- Cluster
- Mesh
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 多集群网络互联（Cluster Mesh）

## 概述

随着企业 [[Kubernetes|Kubernetes]] 集群数量从单个增长到数十甚至上百个，**多集群网络互联（Cluster Mesh）** 成为构建统一服务网格和跨集群负载均衡的关键技术。Cluster Mesh 允许不同地域、不同云厂商的 Kubernetes 集群中的 Pod 像在同一个网络中一样相互通信，实现真正的**全局服务发现**和**跨集群流量管理**。2026 年的主流实现包括 **[[Cilium|Cilium]] Cluster Mesh** 和 **[[Istio|Istio]] Multi-Cluster**。

## 核心概念/原理

### 1. 为什么需要 Cluster Mesh

- **高可用与灾难恢复**：服务在多个集群中同时运行，单个集群问题时流量自动切换
- **就近访问**：用户请求被路由到地理上最近的集群，降低延迟
- **数据主权**：敏感数据保留在特定区域的集群中，但服务可以跨集群调用
- **容量扩展**：将工作负载分布到多个集群，突破单个集群的节点上限
- **异构环境互联**：连接云上和云下、不同云厂商的 Kubernetes 集群

### 2. Cilium Cluster Mesh

**Cilium Cluster Mesh** 是基于 eBPF 的多集群互联方案：
- **Pod IP 路由互通**：不同集群的 Pod 可以直接使用 Pod IP 通信，无需 NAT
- **全局服务发现**：通过 `Global [[Service|Service]]` 将同一服务在多个集群中暴露为统一的 ClusterIP
- **负载均衡**：支持基于地理位置、权重和健康状态的跨集群流量调度
- **安全性**：跨集群流量通过 IPSec 或 WireGuard 加密
- **易于扩展**：支持连接数十个集群，数万个节点

```yaml
# Cilium Global Service 示例
apiVersion: v1
kind: Service
metadata:
  name: backend
  annotations:
    io.cilium/global-service: "true"
spec:
  type: ClusterIP
  ports:
    - port: 80
  selector:
    app: backend
```

### 3. Istio Multi-Cluster

**Istio** 提供两种多集群部署模式：
- **单控制平面（Single-network, shared control plane）**：一个 istiod 管理多个集群的 Sidecar
- **多控制平面（Multi-network, multi-primary）**：每个集群运行独立的 istiod，通过 Gateway 互联

**多主模式（Multi-primary）** 是 2026 年的推荐模式：
- 每个集群都有独立的控制平面，避免单点问题
- 通过 East-West Gateway 建立安全的 mTLS 隧道
- 支持跨集群的自动服务发现、流量管理、遥测和策略执行

### 4. 网络地址与 CIDR 规划

Cluster Mesh 的前提是各集群的 Pod CIDR 和 Service CIDR **不能重叠**：

| 集群 | Pod CIDR | Service CIDR |
|------|----------|--------------|
| 集群 A（北京） | 10.1.0.0/16 | 10.100.0.0/16 |
| 集群 B（上海） | 10.2.0.0/16 | 10.101.0.0/16 |
| 集群 C（新加坡） | 10.3.0.0/16 | 10.102.0.0/16 |

在设计多集群架构时，必须为未来的扩展预留足够的 CIDR 空间。

## 关键机制或特性

### 跨集群服务发现

Cluster Mesh 通过 **Cluster ID** 区分不同集群中的同名服务：
- `backend.default.svc.cluster.local` 可以解析到本地集群的 backend
- `backend.default.svc.global`（在 Cilium 中）可以解析到所有集群的 backend Endpoint
- Istio 通过 `ServiceEntry` 和 `WorkloadEntry` 实现类似的跨集群服务注册

### 跨集群负载均衡策略

| 策略 | 说明 | 适用场景 |
|------|------|----------|
| **Topology-aware** | 优先将流量路由到同一区域/可用区的后端 | 降低延迟和跨区带宽成本 |
| **Failover** | 本地集群健康时优先本地，问题时切换到远端 | 高可用保障 |
| **Weighted** | 按权重比例分配流量到不同集群 | 蓝绿迁移、A/B 测试 |
| **Locality-lb** | 基于地理位置的优先级调度 | 全球化应用 |

### 安全通信

- **Cilium**：支持通过 IPSec 或 WireGuard 对跨集群的 Pod-to-Pod 流量进行自动加密
- **Istio**：所有跨集群服务间通信默认启用 mTLS，通过 [[SPIFFE|SPIFFE]] 身份进行双向认证
- **NetworkPolicy 全局生效**：在 Cilium Cluster Mesh 中，CiliumNetworkPolicy 可以跨集群生效

## 使用场景

1. **全球化电商服务**：用户在北京访问时，订单服务调用本地的库存服务；当北京集群问题时，自动切换到上海的库存服务
2. **多云容灾架构**：核心服务同时在 AWS 和阿里云上运行，通过 Cluster Mesh 实现跨云的自动故障转移
3. **数据主权合规**：欧盟用户的数据处理服务必须运行在法兰克福集群，但前端服务可以通过 Cluster Mesh 安全地调用欧盟后端
4. **大规模容量扩展**：单一 Kubernetes 集群的管理上限约为 5000 节点，通过 Cluster Mesh 将业务分布到多个集群实现横向扩展
5. **边缘-中心协同**：数百个 K3s 边缘集群通过 Cilium Cluster Mesh 与中心云集群互联，边缘 AI 推理结果回传到中心分析

## 最佳实践/注意事项

- **CIDR 规划必须先行**：Cluster Mesh 建成后修改 Pod CIDR 极其困难，初期必须做好地址空间规划
- **控制集群规模**：单个 Cluster Mesh 中建议每个集群不超过 500 节点，总节点数不超过 10000（具体取决于 CNI 实现）
- **跨集群带宽成本**：云厂商通常对跨 Region/跨云流量收费，应使用 Topology-aware LB 减少不必要的远程调用
- **DNS 解析一致性**：确保各集群的 CoreDNS 配置支持跨集群服务发现，或部署全局 DNS（如 CoreDNS + etcd）
- **证书管理复杂化**：多集群 mTLS 需要统一的 CA 或互信的 CA 链，建议使用 [[cert-manager|cert-manager]] + Vault 集中管理
- **监控全局视图**：使用 Thanos / Cortex / VictoriaMetrics 聚合多个集群的 Prometheus 数据，形成全局 SLO 监控
- **分阶段互联**：不要一次性将所有集群互联，先连接 2–3 个核心集群，验证稳定性后再逐步扩展
- **故障隔离**：当某个集群出现网络风暴或控制平面问题时，应具备快速将其从 Cluster Mesh 中断开的能力

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 跨集群 Pod 通信失败 | Pod CIDR 重叠 | 对比各集群的 `--cluster-cidr` 确保不重叠 |
| Global Service 解析到空 | 远程集群 clustermesh-apiserver 连接中断 | `cilium clustermesh status`；检查 apiserver Pod |
| 跨集群延迟高 | 流量未使用 Topology-aware LB | 配置 `io.cilium/service-affinity: local` 优先本地 |
| mTLS 握手失败 | 集群间 CA 不互信 | 使用统一 CA（cert-manager + Vault）或配置 CA bundle |
| 部分 Service 无法跨集群 | 缺少 `io.cilium/global-service` 注解 | 检查 Service 注解是否正确 |

## 生产检查清单

- [ ] 所有集群的 Pod CIDR 和 Service CIDR 不重叠
- [ ] CIDR 规划预留未来集群扩展空间
- [ ] 跨集群流量已加密（WireGuard/IPSec/mTLS）
- [ ] Topology-aware LB 优先本地集群流量
- [ ] 统一 CA 管理跨集群证书
- [ ] 监控跨集群连接状态和延迟
- [ ] 具备快速断开问题集群的运维能力
- [ ] 分阶段互联，先验证 2-3 个集群

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Cilium Cluster Mesh 状态
cilium clustermesh status

# 查看远程集群连接
cilium clustermesh vm status

# 查看 Global Service 端点
kubectl get svc -A -o json | jq '.items[] | select(.metadata.annotations["io.cilium/global-service"]=="true") | .metadata.name'

# Istio 多集群检查
istioctl remote-clusters
istioctl proxy-config cluster <pod> | grep outbound

# 跨集群连通性测试
cilium connectivity test --multi-cluster
```
## 交叉引用

- [eBPF 与 Cilium](ebpf-and-cilium-networking.md) — Cilium 的 eBPF 基础架构
- [Service Mesh](service-mesh.md) — Istio 多集群部署模式
- [Cluster Networking](cluster-networking.md) — CIDR 规划基础
- [Topology Aware Routing](topology-aware-routing.md) — 单集群内的拓扑感知路由

## 参考链接

- [Cilium Cluster Mesh Documentation](https://docs.cilium.io/en/stable/network/clustermesh/)
- [Istio Multi-Cluster Deployment](https://istio.io/latest/docs/setup/install/multicluster/)
- [KubeFed (Deprecated) - Historical Reference](https://github.com/kubernetes-retired/kubefed)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)

## Related

- [[domain-19-landscape-references/topic-index/dns-index.md|DNS 知识图谱索引]]


<!-- risk-assessed -->
