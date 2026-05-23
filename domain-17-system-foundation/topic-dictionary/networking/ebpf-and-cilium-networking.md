---
title: eBPF 与 Cilium 网络
description: '# eBPF 与 Cilium 网络'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- istio
- envoy
- cilium
- calico
- helm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- eBPF 与 Cilium 网络 是什么
- 如何 eBPF 与 Cilium 网络
trigger_keywords:
- eBPF
- Cilium
- 网络
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- kafka-basics
created: "2026-05-23"
---

# eBPF 与 [[Cilium|Cilium]] 网络

## 概述

**eBPF（Extended Berkeley Packet Filter）** 是一项革命性的 Linux 内核技术，允许在不修改内核源码或加载内核模块的情况下，在内核中安全地运行沙箱程序。**Cilium** 是基于 eBPF 的 [[Kubernetes|Kubernetes]] 网络、安全和可观测性解决方案，正在逐步取代传统的 iptables 和 OVS 方案，成为 2026 年云原生网络的事实标准。

## 核心概念/原理

### 1. eBPF 技术特性

eBPF 改变了传统用户态/内核态的交互模式：
- **内核可编程性**：动态将程序挂载到内核事件点（网络、文件系统、调度、系统调用）
- **安全沙箱**：通过验证器（Verifier）确保 eBPF 程序不会崩溃内核或进入死循环
- **高性能**：程序直接在内核执行，避免频繁的用户态/内核态上下文切换
- **零侵入可观测性**：无需修改应用代码即可获取网络包、系统调用、文件访问等全链路数据

### 2. Cilium 架构

Cilium 作为 Kubernetes CNI，完全基于 eBPF 构建：
- **Cilium Agent**：运行在每个节点上，负责 eBPF 程序加载、策略计算和状态同步
- **Cilium Operator**：集群级控制器，管理 IPAM（IP 地址管理）、Endpoint 同步
- **Hubble**：基于 eBPF 的可观测性组件，提供网络流量可视化和安全审计
- **[[domain-19-landscape-references/01-cncf-landscape/graduated/envoy/envoy|[[Envoy|Envoy]]]] 扩展**：用于 Layer 7 应用层协议解析（HTTP、gRPC、Kafka 等）

### 3. eBPF 替代传统网络栈

| 功能 | 传统方案 | Cilium + eBPF |
|------|----------|---------------|
| 负载均衡 | kube-proxy + iptables | eBPF 原生 XDP/TC 负载均衡 |
| 网络策略 | iptables 规则链 | eBPF 高效过滤 |
| 服务网格 | Sidecar（Envoy/Istio）| eBPF + Envoy per-node（无 Sidecar）|
| 可观测性 | iptables LOG / tcpdump | eBPF 零开销流量追踪 |

### 4. Cilium 服务网格（Sidecar-less Service Mesh）

Cilium 1.13+ 提供服务网格能力，无需在每个 Pod 中注入 Sidecar：
- **mTLS 认证**：基于 SPIFFE/SPIRE 的身份认证和加密
- **L7 流量管理**：HTTP 路由、重试、超时、速率限制
- **金丝雀发布**：基于 Header 或权重的流量分割
- **优势**：避免了 Sidecar 带来的资源开销（CPU/Memory 增加 20%–40%）和启动延迟

## 关键机制或特性

### XDP（eXpress Data Path）

XDP 是 eBPF 在网络驱动层的挂载点，可实现：
- **超高速包处理**：在数据包进入内核网络栈之前即进行处理
- **DDoS 缓解**：在网卡层面丢弃恶意流量
- **负载均衡加速**：Cilium 的 XDP 负载均衡可将网络延迟降低 30%–50%

### Hubble 可观测性

Hubble 提供：
- **实时流量图**：可视化 Pod 间的网络连接
- **DNS 监控**：追踪所有 DNS 查询和响应
- **网络策略验证**：检测被策略拒绝的流量
- **Flow 日志导出**：支持 Prometheus 和 Grafana 集成

### 微分段（Microsegmentation）

Cilium 的网络策略支持基于**身份（Identity）**而非 IP 的细粒度隔离：
- 每个 Pod 获得唯一的 Cilium 身份标签
- 策略声明 `frontend` 可以访问 `backend`，无需关心具体 IP
- 即使 Pod IP 变化，策略依然有效

## 使用场景

1. **大规模集群网络**：替代 kube-proxy，支撑 5000+ 节点、百万级 Pod 的集群
2. **零信任网络**：通过 Cilium NetworkPolicy 实现微分段，配合 mTLS 实现零信任
3. **高性能负载均衡**：使用 Cilium LB + XDP 替代传统云 LB，降低网络延迟和成本
4. **无 Sidecar 服务网格**：对资源敏感的场景，使用 Cilium Service Mesh 替代 Istio Sidecar
5. **实时安全监控**：通过 Falco + eBPF 检测容器逃逸、异常系统调用等运行时威胁

## 最佳实践/注意事项

- **内核版本要求**：Cilium 要求 Linux 内核 5.10+，XDP 和某些高级特性需要 5.15+ 或 6.x
- **eBPF Map 大小规划**：大规模集群需要调整 eBPF Map 的大小限制，避免连接追踪表溢出
- **与 Calico 并存迁移**：从 Calico 迁移到 Cilium 时建议采用蓝绿集群方式，避免直接替换生产 CNI
- **启用 Hubble Relay**：多节点场景必须部署 Hubble Relay 才能聚合全集群流量视图
- **监控 Cilium Agent 健康**：Cilium Agent 崩溃会导致节点网络中断，必须配置高优先级和快速重启策略
- **合理利用 L7 代理**：L7 协议解析需要 per-node Envoy，虽然比 Sidecar 轻量，但仍会消耗额外资源
- **网络策略测试先行**：在生产启用严格的 Cilium NetworkPolicy 前，先在 `audit` 模式下观察流量影响

## 生产 YAML 示例

### Cilium Helm 安装关键配置

```bash
# Cilium 安装（替代 kube-proxy）
helm install cilium cilium/cilium --version 1.16 \
  --namespace kube-system \
  --set kubeProxyReplacement=true \
  --set k8sServiceHost=<api-server-ip> \
  --set k8sServicePort=6443 \
  --set hubble.enabled=true \
  --set hubble.relay.enabled=true \
  --set hubble.ui.enabled=true \
  --set encryption.enabled=true \
  --set encryption.type=wireguard \
  --set bpf.masquerade=true \
  --set loadBalancer.mode=dsr \
  --set loadBalancer.acceleration=native   # XDP 加速
```

### CiliumNetworkPolicy（L3/L4 + L7）

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-server
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: web-frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:                         # L7 HTTP 策略
        - method: GET
          path: "/api/v1/.*"
        - method: POST
          path: "/api/v1/orders"
  egress:
  - toEndpoints:
    - matchLabels:
        app: database
    toPorts:
    - ports:
      - port: "5432"
  - toFQDNs:                         # DNS-based FQDN 策略
    - matchName: "api.external.com"
    toPorts:
    - ports:
      - port: "443"
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Cilium Agent CrashLoop | 内核版本过低或 eBPF Map 溢出 | `uname -r` 确认 ≥ 5.10；检查 Agent 日志 |
| Pod 间通信中断 | Agent 崩溃导致 eBPF 规则丢失 | `cilium status`；`cilium connectivity test` |
| L7 策略不生效 | Envoy per-node 未部署 | `kubectl get pods -n kube-system -l k8s-app=cilium-envoy` |
| XDP 加速未启用 | 网卡驱动不支持 XDP native | `cilium status \| grep XDP`；回退到 generic 模式 |
| Hubble 无流量数据 | Hubble Relay 未部署 | `kubectl get pods -n kube-system -l k8s-app=hubble-relay` |

## 生产检查清单

- [ ] Linux 内核 ≥ 5.10（推荐 5.15+）
- [ ] Cilium Agent 配置 `system-node-critical` 优先级
- [ ] 启用 Hubble + Relay 用于可观测性
- [ ] 大规模集群调整 eBPF Map 大小限制
- [ ] NetworkPolicy 先在 audit 模式验证再 enforce
- [ ] 监控 Cilium Agent 健康状态和重启次数
- [ ] 从 Calico 迁移采用蓝绿集群策略

## 命令快速参考

```bash
# Cilium 状态检查
cilium status
cilium connectivity test

# Hubble 流量观测
hubble observe --namespace production
hubble observe --verdict DROPPED         # 查看被丢弃的流量
hubble observe --to-label app=database

# 查看 eBPF 程序
cilium bpf endpoint list
cilium bpf ct list global                # 连接追踪表

# 查看网络策略
cilium policy get -n production
cilium endpoint list

# 监控 Cilium Agent
kubectl logs -n kube-system -l k8s-app=cilium --tail=50
```

## 交叉引用

- [Network Policies](network-policies.md) — 标准 NetworkPolicy 与 CiliumNetworkPolicy 的区别
- [Cluster Networking](cluster-networking.md) — CNI 选型对比
- [Service Mesh](service-mesh.md) — Cilium Sidecar-less Service Mesh
- [Cluster Mesh](cluster-mesh.md) — Cilium 多集群互联

## 参考链接

- [Cilium Documentation](https://docs.cilium.io/)
- [eBPF.io - What is eBPF?](https://ebpf.io/what-is-ebpf/)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)
- [VMblog - 2026 Kubernetes and Cilium Networking Predictions](https://vmblog.com/prediction/2026-kubernetes-and-cilium-networking-predictions/)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
