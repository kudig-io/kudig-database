---
title: "K8S 网络演进"
category: concepts
tags:
  - networking
  - cilium
  - istio
  - gateway-api
  - ebpf
  - k8s
created: 2026-05-24
updated: 2026-05-24
---

# K8S 网络演进

## CNI 演进

### Cilium 1.16+

- **Netkit**：新一代 eBPF 网络设备，替代传统 veth pair，减少 30-40% 数据路径开销
- **Gateway API GAMMA**：Cilium 原生支持 Gateway API 的 GAMMA（Gateway API for Mesh Management and Administration）扩展，实现 Service Mesh 路由
- **BGPv2**：全新 BGP 控制平面，支持多集群路由、ECMP 负载均衡、精细化路由策略
- **kube-proxy 替代 GA**：Cilium kube-proxy 替代方案已 GA，基于 eBPF 的完全替代，支持 DSR（Direct Server Return）和 Maglev 一致性哈希

### Calico 3.29+

- **eBPF 数据平面 GA**：完全替代 iptables，支持 DSR、NAT 模式
- 性能对比 iptables 提升 2-3x，大规模集群（5000+ Pod）表现优异
- 支持 WireGuard 和 VXLAN 双模式加密

### kube-proxy 替代趋势

- iptables 模式在大规模集群下 O(n) 规则匹配成为瓶颈
- IPVS 模式虽有改善但仍依赖内核模块
- eBPF 替代方案（Cilium/Calico）实现 O(1) 查找，已成主流趋势

## Service Mesh

### Istio Ambient Mesh GA（v1.24+）

- **无 sidecar 架构**：移除 Envoy sidecar 注入，减少资源开销
- **ztunnel（L4）**：节点级零信任隧道代理，处理 mTLS、L4 策略，基于 Rust 实现
- **waypoint proxy（L7）**：按需部署的 L7 代理，处理 HTTP/gRPC 路由、重试、熔断
- **内存减少 90%**：对比 sidecar 模式，per-pod 内存开销从 ~50MB 降至 ~5MB
- **平滑迁移**：支持 sidecar 与 ambient 模式混合部署

### 架构对比

| 维度 | Sidecar 模式 | Ambient 模式 |
|------|-------------|-------------|
| 每 Pod 开销 | ~50MB 内存 | ~5MB 内存 |
| 启动延迟 | sidecar 注入延迟 | 即时 |
| 升级影响 | 需重启 Pod | 节点级热更新 |
| L4 能力 | 通过 sidecar | ztunnel 节点级 |
| L7 能力 | 通过 sidecar | waypoint 按需部署 |

## Gateway API v1.0 GA + GAMMA

### 核心资源

- **GatewayClass**：基础设施提供者定义的网关模板
- **Gateway**：集群运维创建的网关实例
- **HTTPRoute / GRPCRoute / TLSRoute / TCPRoute / UDPRoute**：应用开发者定义的路由规则
- **ReferenceGrant**：跨命名空间引用的授权机制

### GAMMA 扩展

- 统一南北向（入口）和东西向（服务间）流量管理
- 通过 `parentRef` 引用 Service 实现网格内路由
- 替代 VirtualService/DestinationRule 等 Istio 专有 CRD

### 生态支持

- Cilium、Istio、Envoy Gateway、Kong、Traefik 等均已 GA 支持
- Gateway API 已成为 Kubernetes 网络路由的标准抽象

## eBPF 网络

### 核心优势

- **O(1) 服务查找**：eBPF map 直接查找 Service Endpoint，跳过 iptables/IPVS 规则链
- **10x 延迟改善**：P99 延迟从 ms 级降至 μs 级
- **绕过内核协议栈**：XDP 程序在网卡驱动层处理数据包

### Katran

- Meta 开源的 L4 负载均衡器
- 基于 XDP 实现 **10+ MPPS**（百万包/秒）处理能力
- 单核即可处理 100Gbps 流量

### 技术栈

```
应用层 → Socket → eBPF sockops → 网卡
         ↓
    传统路径: sk_buff → netfilter → iptables → 路由
    eBPF 路径: XDP → eBPF program → 直接转发
```

## DNS 优化

### NodeLocal DNSCache

- **延迟降低 50-80%**：本地缓存命中时延迟从 ~1ms 降至 ~0.2ms
- 部署为 DaemonSet，每个节点一个 DNS 缓存 Pod
- 缓存未命中时直接向上游 DNS 转发，跳过 kube-dns Service
- 减少 DNS 对 CoreDNS 的压力，避免 conntrack 竞争

### CoreDNS 1.12+

- 性能优化：改进缓存策略、减少内存分配
- 新插件支持：增强 metrics、日志、转发策略
- 支持 DNS over TLS/HTTPS（DoT/DoH）

## Network Policy 演进

### L3/L4 策略（标准）

- Kubernetes NetworkPolicy：Pod/命名空间选择器、ingress/egress 规则
- 限制：仅支持 L3/L4（IP/端口）匹配

### L7 策略（Cilium）

- **HTTP**：匹配 path、method、header
- **Kafka**：匹配 topic、API key、consumer group
- **gRPC**：匹配 service、method
- **DNS**：匹配域名模式（FQDN 通配符）

```yaml
# Cilium L7 HTTP 策略示例
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: l7-policy
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: frontend
      toPorts:
        - ports:
            - port: "80"
          rules:
            http:
              - method: GET
                path: "/api/.*"
```

## 双栈 IPv4/IPv6 GA

- Kubernetes 1.23+ 双栈网络 GA
- 每个 Pod 同时分配 IPv4 和 IPv6 地址
- Service 支持 `ipFamilyPolicy: PreferDualStack` 或 `RequireDualStack`
- 支持有状态过渡：从单栈逐步迁移到双栈
- CNI 插件（Cilium、Calico）均已支持双栈

## 性能调优

### IRQ Affinity

- 将网卡中断绑定到特定 CPU 核心
- 避免跨 NUMA 节点的内存访问
- 使用 `irqbalance` 或手动配置 `/proc/irq/N/smp_affinity`

### RSS（Receive Side Scaling）

- 多队列网卡将接收流量分散到多个 CPU
- 配合 RPS/RFS（软件层面的接收流控）
- 调优 `net.core.rps_sock_flow_entries`

### Ring Buffers

- 调整网卡 ring buffer 大小：`ethtool -G eth0 rx 4096 tx 4096`
- 减少高流量场景下的丢包
- 监控 `ethtool -S eth0` 中的 `rx_dropped`/`tx_dropped`

### BIG TCP

- Linux 6.3+ 支持 BIG TCP（GRO/GSO 超大数据包）
- 减少 CPU 处理开销，提升吞吐量
- IPv6 支持 128KB+ 数据包聚合

### Netkit

- Cilium 1.16+ 引入的 eBPF 网络设备
- 替代 veth pair，减少 sk_buff 复制和上下文切换
- 单向延迟降低 30-40%

## 参考资料

- [[cilium]] - Cilium CNI 插件
- [[istio]] - Istio 服务网格
- Gateway API - Kubernetes 网络路由标准
- eBPF 技术 - 内核可编程框架
- K8S 服务网格对比

## Related

- [[concepts/k8s-security-compliance]] — K8S 安全与合规
- [[concepts/k8s-observability-stack]] — K8S 可观测性技术栈
- [[concepts/specialized-k8s-technologies]] — 特殊化 K8S 技术
