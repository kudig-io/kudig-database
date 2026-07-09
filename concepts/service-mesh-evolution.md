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

## 来源文档

- 生态参考/_archived-release-notes/networking/istio/（38 个文件）
- 生态参考/_archived-release-notes/networking/envoy/（38 个文件）
- 生态参考/_archived-release-notes/networking/cilium/（24 个文件）
- 生态参考/_archived-release-notes/networking/linkerd/（8 个文件）
- 生态参考/_archived-release-notes/networking/calico/（35 个文件）
- 生态参考/_archived-release-notes/networking/cni-plugins/（14 个文件）

## Related

- [[entities/cni-plugins.md|cni-plugins]] — [[entities/cni-plugins.md|cni-plugins]]
- [[cilium]] — Cilium
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[envoy]] — Envoy


<!-- risk-assessed -->
