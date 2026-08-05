---
title: CNCF 网络与服务网格项目全景
description: '# CNCF 网络与服务网格项目全景'
summary: 'CNCF 网络生态围绕 **CNI（容器网络）**、**服务网格（Service Mesh）vice]] Mesh）**、**DNS/服务发现**、**负载均衡与入口（[[ingress|Ingress]]）** 四大层次构建。'
category: entities
tags:
- k8s
- cncf
- networking
- service-mesh
- cni
- dns
- load-balancer
- istio
- envoy
- cilium
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 网络与服务网格项目全景 是什么
- 如何 CNCF 网络与服务网格项目全景
trigger_keywords:
- CNCF
- 网络与服务网格项目全景
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF 网络与服务网格项目全景

> 聚合页面 | 涵盖 28 个 CNCF 网络项目

## 概述

CNCF 网络生态围绕 **CNI（容器网络）**、**服务网格（Service Mesh）vice]] Mesh）**、**DNS/服务发现**、**负载均衡与入口（[[ingress|Ingress]]）** 四大层次构建。

---

## 容器网络接口（CNI）

### [[cilium]] — 毕业项目

Cilium 是基于 eBPF 的 CNI 和服务网格。

- eBPF 内核级数据面，替代 iptables
- L3/L4/L7 网络策略
- 透明加密（WireGuard/IPsec）
- Host-level 和 Pod-level 可观测性（Hubble）

### [[antrea]] — 沙箱项目

Antrea 是基于 Open vSwitch（OVS）的 CNI。

### [[23-实体/04-网络/ovn-kubernetes.md|kubernetes]]]] — 沙箱项目

OVN-Kubernetes 基于 OVN（Open Virtual Network）的 CNI 实现。

### [[kube-ovn]] — 沙箱项目

Kube-OVN 将 OVN 与 K8s 网络深度集成。

### [[spiderpool]] — 沙箱项目

Spiderpool 为 underlay 网络提供 IP 地址管理。

### [[cni]] — 孵化项目

CNI（Container Network Interface）是 K8s 网络插件标准规范。

### [[network-service-mesh]] — 沙箱项目

Network Service Mesh 将 NFV 网络服务引入 K8s。

---

## 服务网格（Service Mesh）

### [[istio]] — 毕业项目

Istio 是最广泛使用的服务网格。

- **流量管理**: 金丝雀、蓝绿、A/B 测试、故障注入
- **安全**: mTLS、RBAC、JWT 验证
- **可观测性**: 分布式追踪、指标、访问日志
- **高级功能**: 高级流量管理、[[02-istio-security-hardening|安全加固]]
- Ambient Mesh 模式减少 sidecar 开销

### [[linkerd]] — 毕业项目

Linkerd 是轻量级服务网格。

- Rust 编写的 ultralight 数据面代理
- 自动 mTLS、流量拆分、可观测性
- 安装简单，资源占用少

### [[envoy]] — 毕业项目

Envoy 是高性能 L7 代理和通信总线。

- Istio/Linkerd 的数据面基础
- 动态配置（xDS API）
- 丰富的可观测性和追踪集成

### [[kuma]] — 沙箱项目

Kuma 是 Kong 出品的多区域服务网格（基于 Envoy）。

### [[aeraki-mesh]] — 沙箱项目

Aeraki Mesh 为 Istio 提供非 HTTP 协议支持。

### [[kmesh]] — 沙箱项目

Kmesh 基于 eBPF 的无 sidecar 服务网格数据面。

### [[meshery]] — 沙箱项目

Meshery 是服务网格管理平面，支持多网格对比和管理。

---

## DNS 与服务发现

### [[coredns]] — 毕业项目

CoreDNS 是 K8s 默认 DNS 服务。

- 插件化架构（Corefile 配置）
- K8s 服务发现
- 自定义 DNS 策略
- 与 DNS 架构和 插件生态 深度集成

### [[k8gb]] — 沙箱项目

k8gb 是 K8s 原生的全局负载均衡。

---

## 负载均衡与入口

### [[metallb]] — 沙箱项目

MetalLB 为裸机 K8s 集群提供 LoadBalancer 类型 Service。

- L2 模式和 BGP 模式
- 裸机环境必备

### [[contour]] — 孵化项目

Contour 是基于 Envoy 的 K8s Ingress Controller。

### [[emissary-ingress]] — 孵化项目

Emissary-Ingress（原 Ambassador）是基于 Envoy 的 API 网关。

### [[kgateway]] — 沙箱项目

KGateway 是基于 Envoy 的 K8s Gateway API 实现。

### [[loxilb]] — 沙箱项目

LoxiLB 是云原生负载均衡器。

### [[bfe]] — 沙箱项目

BFE 是百度开源的七层负载均衡器。

### [[kube-vip]] — 沙箱项目

Kube-VIP 提供 K8s 控制面高可用和 LoadBalancer 服务。

---

## RPC 与通信协议

### [[grpc]] — 孵化项目

gRPC 是高性能 RPC 框架。

- Protocol Buffers 序列化
- 流式通信（双向流）
- 负载均衡、健康检查、认证

### [[cloudevents]] — 毕业项目

CloudEvents 是事件数据的通用描述规范。

### [[nats]] — 孵化项目

NATS 是高性能消息系统。

- 轻量级消息发布/订阅
- JetStream 提供持久化和流处理
- 适合微服务间异步通信

---

## 多集群网络

### [[submariner]] — 沙箱项目

Submariner 连接多个 K8s 集群的网络。

### [[kubeslice]] — 沙箱项目

KubeSlice 提供跨集群网络切片。

### [[telepresence]] — 沙箱项目

Telepresence 让本地开发环境连接远程 K8s 集群。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 通用 CNI | Cilium（eBPF）或 Antrea（OVS） |
| 裸机 LoadBalancer | MetalLB |
| 服务网格（全功能） | Istio |
| 服务网格（轻量） | Linkerd |
| K8s DNS | CoreDNS |
| K8s Ingress | Contour 或 Nginx Ingress |
| K8s Gateway API | KGateway 或 Contour |
| 异步通信 | NATS 或 gRPC |

---

## 相关页面

- [[23-实体/15-参考与索引/cncf-observability.md|cncf-observability]] — 可观测性
- [[23-实体/15-参考与索引/cncf-security.md|cncf-security]] — 安全与合规
- [[22-概念/03-网络/cni-networking-model.md|cni-networking-model]] — CNI 网络模型
- concepts/service-mesh-deep-dive — 服务网格深入

## Related

- [[23-实体/04-网络/04-terway-architecture-deep-dive]] — Terway 架构深度解析
- [[23-实体/15-参考与索引/cncf-cicd.md|cncf-cicd]] — CNCF CI/CD 与发布管理项目全景
- [[aeraki-mesh]] — Aeraki Mesh
- [[network-service-mesh]] — Network Service Mesh (NSM)
- [[cni]] — CNI (Container Network Interface)

- [[23-实体/04-网络/antrea.md|Antrea]]
- [[23-实体/04-网络/kubeslice.md|KubeSlice]]
- [[23-实体/04-网络/kuadrant.md|Kuadrant]]
- [[23-实体/04-网络/kube-ovn.md|Kube-OVN]]
- [[23-实体/04-网络/easegress.md|Easegress]]
- [[23-实体/10-平台与开发工具/bpfman.md|bpfman]]
- [[23-实体/10-平台与开发工具/telepresence.md|Telepresence]]
- [[23-实体/04-网络/spiderpool.md|Spiderpool]]
- [[23-实体/04-网络/k8gb.md|K8GB]]

<!-- risk-assessed -->
