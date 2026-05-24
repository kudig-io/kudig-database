---
category: synthesis
tags:
  - networking
  - cilium
  - istio
  - ebpf
  - k8s
  - research
created: 2026-05-24
updated: 2026-05-24
---

# Research: Kubernetes Networking 2025-2026

## 概述

Kubernetes 网络生态在 2025-2026 年经历了三重范式转移：

1. **Cilium eBPF 革命** — eBPF 取代 iptables 成为内核级数据面标准，Cilium 已成为 CNCF 毕业项目并在生产环境中大规模部署
2. **Istio Ambient 无 Sidecar 模式** — Istio Ambient Mesh 正式 GA，通过 ztunnel + waypoint proxy 架构消除 sidecar 开销
3. **Gateway API 统一入口** — Kubernetes Gateway API 取代 Ingress 成为官方标准，实现南北向和东西向流量的统一管理

这三股力量汇聚，正在重新定义云原生网络的边界。

## 核心发现

### 1. eBPF 成为网络数据面事实标准

eBPF 程序直接在内核态处理网络包，绕过 iptables 的线性链式匹配。Cilium 利用 eBPF 实现了：
- 无 kube-proxy 的 Service 实现（替代 iptables/ipvs）
- 内核级负载均衡，延迟降低 40-60%
- 原生支持 WireGuard 加密和 Mutual Authentication

**关键指标**：在万节点规模集群中，Cilium 相比 iptables 方案将 Service 路由延迟从毫秒级降至微秒级。

### 2. Istio Ambient Mesh 重塑服务网格架构

Istio Ambient 模式采用分层架构：
- **ztunnel**（零信任隧道）：L4 层的 per-node 代理，负责 mTLS 和基本策略
- **Waypoint Proxy**：L7 层的 per-namespace 代理，按需部署

相比传统 sidecar 模式：
- 内存开销降低 90%+（无需为每个 Pod 注入 sidecar）
- 应用启动不再受 sidecar 就绪状态阻塞
- 运维复杂度显著下降

### 3. Gateway API 终结入口碎片化

Gateway API（v1.1+）已成为 Kubernetes 网络入口的标准 API：
- `GatewayClass` → `Gateway` → `HTTPRoute` 三层模型
- 原生支持流量分割、请求头修改、重定向等高级功能
- Cilium、Istio、Envoy Gateway 等均提供 Gateway API 实现

**影响**：Ingress 资源将逐步被弃用，新项目应直接采用 Gateway API。

### 4. 多集群网络走向成熟

Submariner、Cilium ClusterMesh、Skupper 等方案在 2025 年趋于稳定：
- 跨集群 Service 发现和负载均衡成为标配
- 多云/混合云场景下的网络策略一致性得到保障
- Cilium ClusterMesh 因 eBPF 原生支持在性能上领先

### 5. 网络安全左移（Shift-Left Networking）

- **NetworkPolicy v2**：SIG-Network 推进更细粒度的策略模型
- **Mutual TLS 成为默认**：Ambient Mesh 将 mTLS 下沉到基础设施层
- **身份感知网络**：基于 SPIFFE/SPIRE 的工作负载身份逐步替代 IP-based 策略

### 6. 性能优化进入内核原生时代

- io_uring 集成：减少系统调用开销
- XDP（eXpress Data Path）：在网卡驱动层实现包处理
- BIG TCP / GRO 优化：提升大包传输效率
- 用户态网络栈（DPDK/F-stack）在特定场景（低延迟交易）持续发力

## 核心概念

- [[concepts/k8s-networking-evolution]] — Kubernetes 网络模型的演进路径
- [[concepts/ebpf-networking]] — eBPF 在网络领域的应用原理
- [[concepts/service-mesh-ambient]] — Ambient Mesh 架构与设计哲学
- [[concepts/gateway-api]] — Gateway API 规范与实现
- [[concepts/multi-cluster-networking]] — 多集群网络互联方案
- [[concepts/zero-trust-networking]] — 零信任网络在 K8s 中的落地

## 矛盾与张力

| 矛盾点 | 两面 |
|---------|------|
| eBPF vs 用户态代理 | eBPF 性能优越但调试困难；用户态灵活但开销高 |
| Ambient vs Sidecar | Ambient 降低开销但牺牲了 per-pod 的精细控制 |
| Gateway API vs Ingress | Gateway API 功能强大但生态迁移需要时间 |
| 统一 CNI vs 专用方案 | Cilium 试图统一网络/安全/可观测，但可能形成厂商锁定 |
| 内核依赖 vs 可移植性 | eBPF 依赖 Linux 5.10+，Windows/macOS 支持有限 |
| 零信任 vs 性能 | mTLS 加密带来 5-15% 的额外延迟开销 |

## 参考来源

1. Cilium Documentation — https://docs.cilium.io/en/stable/
2. Istio Ambient Mesh Blog — https://istio.io/latest/blog/2024/ambient-reaches-ga/
3. Kubernetes Gateway API Spec — https://gateway-api.sigs.k8s.io/
4. CNCF Cilium Graduation Announcement — https://www.cncf.io/announcements/
5. SIG-Network KEP-1686: NetworkPolicy v2
6. eBPF Summit 2025 Proceedings
7. "eBPF for Networking" — Liz Rice, O'Reilly 2025 Edition
8. Isovalent/Cisco Cilium Enterprise Reports 2025-2026

---

> **总结**：Kubernetes 网络正在从"iptables + sidecar + Ingress"的旧范式，向"eBPF + ambient + Gateway API"的新范式全面迁移。这一转变不仅是技术栈的替换，更是架构哲学的根本变革——从应用侵入式代理转向透明基础设施层。

---

## 跨域关联

- [[concepts/k8s-security-compliance]] — 网络策略（NetworkPolicy、Cilium 审计模式）是容器安全合规的重要防线
- [[concepts/k8s-observability-stack]] — 网络可观测性（Hubble、eBPF 流量分析）为故障排查与性能优化提供实时数据
- [[concepts/specialized-k8s-technologies]] — 边缘计算与 IoT 场景对网络架构（低延迟、多集群互联）提出特殊需求
- [[concepts/platform-engineering-idp]] — 平台工程将网络能力（Gateway API、服务网格）封装为开发者自助服务
