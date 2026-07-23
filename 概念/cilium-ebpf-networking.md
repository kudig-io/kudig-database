---
title: Cilium eBPF Networking
description: '- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
summary: '- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
category: concepts
tags:
- k8s
- ebpf
- cilium
- networking
- security
- hubble
- tetragon
- kubelet
- envoy
- kafka
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium eBPF Networking 是什么
- 如何 Cilium eBPF Networking
trigger_keywords:
- Cilium
- eBPF
- Networking
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cilium eBPF Networking

## What is eBPF

eBPF (Extended Berkeley Packet Filter) is a revolutionary Linux kernel technology that allows running sandboxed programs in kernel space without modifying kernel source code or loading kernel modules. A verifier ensures programs are safe before execution, and a JIT compiler optimizes them to native machine code.

## eBPF Architecture

eBPF programs attach to kernel hooks at:
- **XDP** (eXpress Data Path): Earliest packet processing point, before network driver
- **TC** (Traffic Control): After network driver, before protocol stack
- **Socket**: At socket layer for connection monitoring
- **Kprobe/Tracepoint**: At kernel function entry/exit for syscall monitoring
- **Cgroup**: At cgroup level for container-level monitoring

Data passes between kernel and user space through eBPF Maps (Hash, Array, LRU, RingBuffer, Per-CPU variants).

## Cilium CNI Architecture

Cilium replaces iptables-based networking with eBPF programs:

| Component | Role |
|-----------|------|
| Cilium Agent | [[DaemonSet|DaemonSet]] per node, programs eBPF into kernel |
| Cilium Operator | Cluster-wide operations (IPAM, node management) |
| CNI Plugin | Integrates with kubelet for Pod networking |
| eBPF dataplane | In-kernel packet processing, policy enforcement |
| Hubble Relay | Collects and serves network flow telemetry |

### L3/L4/L7 Network Policies

CiliumNetworkPolicy extends K8s NetworkPolicy to L7 (HTTP, gRPC, Kafka):
- L3: IP-based policies (source/destination CIDR)
- L4: Port/protocol-based policies (TCP/UDP/SCTP)
- L7: Application-layer policies (HTTP path, method, gRPC service, Kafka topic)

### Cilium Service Mesh

Cilium provides a sidecar-less service mesh using eBPF for L4 mTLS and optional Envoy proxy for L7 processing. Performance advantage over sidecar meshes: lower memory, lower latency, no per-Pod proxy overhead.

## Tetragon Runtime Security

Tetragon uses eBPF for real-time runtime security monitoring:
- **Process execution monitoring**: Detect unauthorized process launches in containers
- **File access monitoring**: Track sensitive file reads/writes
- **Network monitoring**: Detect anomalous network connections
- **TracingPolicy**: Declarative policy format for custom security event detection

## Hubble Network Observability

Hubble provides L3/L4/L7 flow visibility:
- **Hubble CLI**: Command-line flow analysis
- **Hubble UI**: Visual service dependency map and flow exploration
- **Hubble Relay**: Aggregates flow data from all nodes

## Kernel Requirements

| Feature | Minimum Kernel |
|---------|---------------|
| Basic eBPF | 5.10 |
| BTF (BPF Type Format) | 5.15 |
| Advanced features | 6.1+ |

## 源码实现分析

### Cilium eBPF 数据路径

```c
// cilium/bpf/bpf_overlay.c - eBPF 数据包处理入口
SEC("tc")
int handle_xgress(struct __sk_buff *skb) {
    // 1. 解析数据包头部（Ethernet/IP/TCP）
    struct ethhdr *eth = data;
    struct iphdr *ip = data + sizeof(*eth);
    // 2. 查找 endpoint（Pod）对应的策略 map
    struct endpoint_key key = {.ip = ip->saddr};
    struct endpoint_info *ep = map_lookup_elem(&cilium_lxc, &key);
    // 3. 执行 NetworkPolicy 检查（L3/L4/L7）
    if (!policy_allows(ep->policy_id, ip->daddr, tcp->dest)) {
        return TC_ACT_SHOT; // 丢弃
    }
    // 4. 连接跟踪（CT map）
    struct ct_entry *ct = ct_lookup(&cilium_ct, ip, tcp);
    if (!ct) ct_create(&cilium_ct, ip, tcp); // 新建连接
    // 5. NAT 处理（NodePort/HostPort）
    if (needs_nat(ip, tcp)) bpf_l4_csum_replace(skb, ...);
    // 6. 重定向到目标 Pod 的 veth
    return bpf_redirect(ep->ifindex, 0);
}
// 优势：完全在内核态处理，无需 iptables 链遍历
// 性能：O(1) map 查找 vs iptables O(n) 规则遍历
```

### Cilium 网络架构

```
┌──────────────────────────────────────────────────────────┐
│              Cilium eBPF 网络架构                       │
├──────────────────────────────────────────────────────────┤
│  Pod A (veth) ──┐                                      │
│                  ├─ eBPF tc hook ─ Policy Map ─ CT Map  │
│  Pod B (veth) ──┘       │                              │
│                          │                              │
│              ┌─────────┼──────────┐              │
│              │         │          │              │
│         VXLAN/    Native     eBPF Host      │
│         Geneve    Routing    Routing        │
│         (overlay) (direct)   (NodePort)     │
│              │         │          │              │
│              └─────────┼──────────┘              │
│                          │                              │
│                    Physical NIC                         │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：Cilium NetworkPolicy L7 策略

```yaml
# 🟡 中风险：创建网络策略影响流量
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-l7-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-server
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:  # L7 HTTP 策略
        - method: GET
          path: "/api/v1/.*"
        - method: POST
          path: "/api/v1/orders"
        # 拒绝其他所有 HTTP 请求
```

### 场景二：Hubble 流量可观测性

```bash
# 🟢 低风险：只读观察
# 启用 Hubble 并观察流量
hubble observe --namespace production --follow
# 按服务过滤
hubble observe --from-service frontend --to-service api-server
# 查看被拒绝的流量（策略调试）
hubble observe --verdict DROPPED --namespace production
# 查看 DNS 查询
hubble observe --type l7 --protocol dns
# 导出流量日志到 Grafana Tempo
hubble observe --output json > traffic-flow.json
```

### 场景三：Cilium 集群网格（Cluster Mesh）

```bash
# 🟠 高危：跨集群网络配置
# 启用 Cluster Mesh 连接多个集群
cilium clustermesh enable --service-type LoadBalancer
# 连接远程集群
cilium clustermesh connect --destination-context cluster2
# 验证跨集群服务发现
cilium clustermesh status
# 跨集群流量策略
kubectl apply -f - <<EOF
apiVersion: cilium.io/v2
kind: CiliumClusterwideNetworkPolicy
metadata:
  name: cross-cluster-policy
spec:
  endpointSelector:
    matchLabels:
      app: payment
  ingress:
  - fromEndpoints:
    - matchLabels:
        io.kubernetes.pod.namespace: production
        app: checkout
        io.cilium.k8s.policy.cluster: cluster1  # 跨集群标签
EOF
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | Cilium 只是 CNI 插件 | Cilium 是完整网络平台：CNI + NetworkPolicy + Service Mesh + 可观测性 + 运行时安全 |
| 2 | eBPF 需要修改内核 | eBPF 程序在内核沙箱中运行，无需修改内核源码；只需内核版本支持 |
| 3 | Cilium 不支持 iptables | Cilium 可以完全替代 iptables（kube-proxy replacement），也可共存 |
| 4 | VXLAN 模式性能很差 | 现代内核 VXLAN 硬件卸载后性能接近原生；或用 eBPF Host Routing 避免封装 |
| 5 | L7 策略不需要 sidecar | Cilium 通过 eBPF + Envoy proxy 实现 L7，无需 per-Pod sidecar（可选） |
| 6 | 所有内核版本都支持 eBPF | 基本功能需 5.10+，BTF 需 5.15+，高级功能需 6.1+；旧内核功能受限 |

## 面试要点

1. **Q: Cilium 相比传统 CNI（Calico/Flannel）的核心优势是什么？**
   A: ① 性能：eBPF 在内核态 O(1) map 查找，避免 iptables O(n) 链遍历（大规模集群差异显著）；② 可观测性：Hubble 提供 L3-L7 流量可视化、DNS 监控、HTTP 延迟分析；③ L7 策略：原生支持 HTTP/gRPC/Kafka 级别策略，无需 Service Mesh sidecar；④ 无 kube-proxy：eBPF 直接实现 NodePort/ClusterIP，减少组件；⑤ 运行时安全：Tetragon 基于 eBPF 检测异常系统调用。

2. **Q: eBPF 在 Cilium 中具体做什么？**
   A: ① 数据包转发：tc hook 中解析包头、查找 endpoint map、重定向到目标 veth；② 策略执行：policy map 存储 L3/L4/L7 规则，数据包经过时 O(1) 判断允许/拒绝；③ 连接跟踪：CT map 记录连接状态，支持有状态防火墙；④ NAT：eBPF 实现 NodePort/HostPort DNAT/SNAT；⑤ 负载均衡：eBPF 实现 ClusterIP 服务的哈希/轮询负载均衡。

3. **Q: Cilium 的三种网络模式如何选择？**
   A: ① VXLAN/Geneve（Overlay）：跨子网/跨云环境，封装开销但兼容性好；② Native Routing（Direct）：同子网/支持 BGP 环境，无封装性能最佳；③ eBPF Host Routing：绕过完整网络栈，减少延迟（需内核 5.10+）。生产建议：云环境用 VXLAN + 硬件卸载；裸金属用 BGP Native Routing。

4. **Q: 如何调试 Cilium NetworkPolicy 不生效的问题？**
   A: ① hubble observe --verdict DROPPED 查看被拒绝的流量和原因；② cilium policy trace --from <pod> --to <pod> 模拟策略匹配；③ kubectl get cnp 确认策略已应用；④ cilium endpoint list 检查 endpoint 状态；⑤ cilium monitor 查看实时事件；⑥ 检查 endpointSelector 标签是否匹配；⑦ 确认 L7 策略需要 Envoy proxy 就绪。

## Related
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[实体/tetragon.md|tetragon]] — Tetragon
- [[grpc]] — gRPC
- [[cni]] — CNI (Container Network Interface)
- [[概念/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[cilium|Cilium]]
- [[实体/tetragon.md|Tetragon]]
- Hubble
- [[概念/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis

- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference


<!-- risk-assessed -->
