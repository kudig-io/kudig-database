---
title: 服务网格演进
description: 本文档综合了 `生态参考/_archived-release-notes/networking/`
  目录下 [[istio|Istio]]、[[envoy|Envoy]]、[[cilium|Cilium]]、[[linkerd|Linkerd]] 和 Calico
  五大网络/服务网格组件的 157 个版本发布说明 ^[inferred]
summary: 本文档综合了 `生态参考/_archived-release-notes/networking/`
  目录下 [[istio|Istio]]、[[envoy|Envoy]]、[[cilium|Cilium]]、[[linkerd|Linkerd]] 和 Calico
  五大网络/服务网格组件的 157 个版本发布说明 ^[inferred]
category: concepts
tags:
- k8s
- release-notes
- istio
- envoy
- cilium
- linkerd
- service-mesh
- calico
- ingress
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务网格演进 是什么
- 如何 服务网格演进
trigger_keywords:
- 服务网格演进
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务网格演进

> 本文档综合了 `生态参考/_archived-release-notes/networking/` 目录下 [[istio|Istio]]、[[envoy|Envoy]]、[[cilium|Cilium]]、[[linkerd|Linkerd]] 和 Calico 五大网络/服务网格组件的 157 个版本发布说明 ^[inferred]

## 组件概览

| 组件 | 版本范围 | 定位 |
|---|---|---|
| Istio | v0.1 - v1.0+ | 完整服务网格控制面 |
| Envoy | 多版本 | 服务网格数据面代理 |
| Cilium | v1.0+ | eBPF 网络和服务网格 |
| Linkerd | 多版本 | 轻量级服务网格 |
| Calico | 35 个版本 | 网络策略和 CNI |

## Istio 版本演进

Istio 是最成熟的服务网格实现，由 Google、IBM 和 Lyft 联合开发。

### v0.1 - v0.8：孵化期

- Pilot（服务发现）、Mixer（策略和遥测）、Citadel（安全）架构确立
- Envoy 作为默认 Sidecar 代理
- 流量管理：路由、超时、重试、断路器
- mTLS 安全通信初版

### v1.0 - 首次 GA

- 生产就绪版本
- 核心功能稳定：流量管理、安全、遥测
- Sidecar 自动注入成熟
- 发布制品包含 istio-sidecar.deb

### v1.1 - v1.4：功能完善

- 改进的遥测和监控
- 更好的可扩展性
- 外部控制平面支持
- 改进的安装体验（istioctl）

### v1.5 - 架构重构

- **Monolithic istiod**：将 Pilot、Citadel、Galley 合并为单一 istiod 进程
- 显著降低资源消耗和运维复杂度
- 架构简化是此版本的最大变更

### v1.6 - v1.10：成熟期

- 改进的扩展性
- WebAssembly 扩展支持
- 多集群服务网格
- Ambient Mesh 概念探索

### v1.11+：现代化

- 更好的 eBPF 集成
- Ambient Mesh 引入（无 Sidecar 模式）
- 改进的多集群支持
- 持续的性能优化 ^[inferred]

## Envoy 版本演进

Envoy 是高性能服务代理，被 Istio 和其他服务网格用作数据面。

### 关键演进

- HTTP/2 和 [[gRPC|gRPC]] 支持
- xDS API 协议成熟
- 过滤器链扩展
- 负载均衡改进
- 可观测性（Metrics/Tracing/Logging）^ [inferred]

## Cilium 版本演进

Cilium 基于 eBPF 提供网络、安全和可观测性。

### v1.0

- 基础 eBPF 网络功能
- L3/L4 网络策略
- Envoy 集成（[[Ingress|Ingress]]/Egress 代理）
- BPF endpoint map 管理
- cilium-health 健康检查

### v1.1+：功能扩展

- L7 网络策略
- DNS 过滤
- 服务网格模式（Cilium [[Service|Service]]Service Mesh）|Service Mesh]]）
- Hubble 可观测性
- eBPF 加速的网络性能 ^[inferred]

## Linkerd 版本演进

Linkerd 是轻量级服务网格，由 Buoyant 开发。

- 基于 Rust 的代理（linkerd2-proxy）
- 低资源消耗
- 简单的安装和使用
- 自动 mTLS
- 服务级别的可观测性 ^[inferred]

## Calico 版本演进

Calico 提供网络策略和 CNI 实现。

- BGP 路由
- 网络策略引擎
- WireGuard 加密支持
- 服务质量（QoS）^ [inferred]

## 技术趋势

### 数据面演进

1. **Sidecar 模式**：Istio/Envoy、Linkerd 采用 Sidecar 代理
2. **eBPF 数据面**：Cilium 使用 eBPF 直接在内核层处理网络
3. **Ambient 模式**：Istio 探索无 Sidecar 的服务网格
4. **Rust 代理**：Linkerd 使用 Rust 构建高性能代理

### 控制面演进

1. **微服务架构**：Istio v1.5 前多组件架构
2. **统一控制面**：Istio v1.5+ istiod 单一进程
3. **扩展性**：WebAssembly 过滤器、自定义扩展

## 源码实现分析

### Istio Sidecar 流量劫持

```go
// istio.io/istio/pilot/pkg/networking/core/v1alpha3/listener.go
// Istio 流量劫持核心逻辑
func (configgen *ConfigGeneratorImpl) buildSidecarListeners(proxy *model.Proxy) []*listener.Listener {
    // 1. iptables 劫持所有入站流量到 15006 端口
    // 2. iptables 劫持所有出站流量到 15001 端口
    
    // 3. Envoy 根据 VirtualService/DestinationRule 路由
    for _, service := range configgen.Services(proxy) {
        // 生成 Envoy cluster 配置
        cluster := buildCluster(service)
        // 负载均衡策略: ROUND_ROBIN / LEAST_REQUEST / RING_HASH
        cluster.LbPolicy = getLbPolicy(destinationRule)
    }
    
    // 4. mTLS 自动加密服务间通信
    // PeerAuthentication 控制 STRICT/PERMISSIVE/DISABLE
}
```

```
┌─────────────────────────────────────────────────────────┐
│     Service Mesh 演进路线                              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Gen 1: Sidecar Proxy (2017-2021)                      │
│    Istio + Envoy sidecar                                │
│    └─ 问题: 资源开销大、延迟增加、升级复杂          │
│                                                         │
│  Gen 2: Sidecarless (2022-2024)                        │
│    Cilium Service Mesh / Linkerd2-proxy                 │
│    └─ eBPF 内核态处理 L3/L4，减少 sidecar 开销     │
│                                                         │
│  Gen 3: Ambient Mesh (2023+)                           │
│    Istio Ambient: ztunnel (L4) + waypoint (L7)         │
│    └─ 无 sidecar，按需部署 L7 代理                  │
│                                                         │
│  趋势: eBPF + Wasm + 无 Sidecar                     │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：Service Mesh 诊断

```bash
# 🟢 Istio 状态检查
istioctl proxy-status
istioctl analyze -A

# 🟢 检查 sidecar 注入
kubectl get pods -n <ns> -o jsonpath='{.items[*].spec.containers[*].name}' | grep istio-proxy

# 🟢 Envoy 配置调试
istioctl proxy-config clusters <pod> -n <ns>
istioctl proxy-config routes <pod> -n <ns>

# 🟡 检查 mTLS 状态
istioctl authn tls-check <pod>.<ns>.svc.cluster.local

# 🟢 查看服务间流量
kubectl exec -n <ns> <pod> -c istio-proxy -- curl -s localhost:15000/stats | grep upstream_rq
```

## 面试要点

1. **Service Mesh 的核心价值是什么？**
   - 将网络策略（重试/超时/熔断）从应用代码下沉到基础设施
   - 统一的可观测性（流量指标/分布式追踪）
   - 零信任安全（mTLS 自动加密）
   - 流量管理（金丝雀/流量镜像/故障注入）

2. **Istio Ambient Mesh 与 Sidecar 模式的区别？**
   - Sidecar：每 Pod 一个 Envoy，资源开销大（~100m CPU/Pod）
   - Ambient：ztunnel（每节点）处理 L4 + waypoint（按需）处理 L7
   - Ambient 减少资源开销 50%+，升级不影响业务 Pod
   - 但 L7 策略需要部署 waypoint proxy

3. **Cilium Service Mesh 与传统 Mesh 的区别？**
   - 无 sidecar：eBPF 在内核态处理 L3/L4 策略
   - 性能更好：无额外网络跳转，延迟增加 < 1ms
   - L7 策略通过 Envoy 代理（按需部署）
   - 与 CNI 深度集成，无需额外网络层

4. **如何评估是否需要 Service Mesh？**
   - 需要：微服务 > 20 个、多语言、需要 mTLS、复杂流量管理
   - 不需要：服务少、单语言、简单拓扑、团队规模小
   - 替代方案：Ingress + NetworkPolicy + SDK（如 Spring Cloud）
   - 评估维度：复杂度收益比、团队能力、性能影响

## 来源文档

- 生态参考/_archived-release-notes/networking/istio/（38 个文件）
- 生态参考/_archived-release-notes/networking/envoy/（38 个文件）
- 生态参考/_archived-release-notes/networking/cilium/（24 个文件）
- 生态参考/_archived-release-notes/networking/linkerd/（8 个文件）
- 生态参考/_archived-release-notes/networking/calico/（35 个文件）
- 生态参考/_archived-release-notes/networking/cni-plugins/（14 个文件）

## Related

- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]] — [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[cilium]] — Cilium
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[envoy]] — Envoy


<!-- risk-assessed -->
