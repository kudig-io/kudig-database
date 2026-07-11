---
title: 网络体系：CNI、Service、Ingress、Gateway API 与多集群网络
description: '# 网络体系'
summary: 'Service → Endpoints → Pod 的映射链路由 kube-proxy 维护（iptables 或 IPVS 模式）。'
category: reference
tags:
- k8s
- networking
- cni
- service
- ingress
- gateway-api
- multi-cluster
- cilium
- flannel
- calico
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络体系：CNI、Service、Ingress、Gateway API 与多集群网络 是什么
- 如何 网络体系：CNI、Service、Ingress、Gateway API 与多集群网络
trigger_keywords:
- 网络体系：CNI
- Service
- Ingress
- Gateway
- API
- 与多集群网络
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络体系

## 概述

Kubernetes 网络体系涵盖 CNI（容器网络接口）、Service（服务发现与负载均衡）、Ingress/Gateway API（外部流量入口）和 NetworkPolicy（网络安全隔离）四大核心领域。理解这些组件的原理和选型是构建生产级 Kubernetes 网络的基础。

## CNI 插件对比

| 特性 | Calico | Cilium | Flannel | Kube-OVN |
|------|--------|--------|---------|----------|
| 数据平面 | iptables/eBPF | eBPF | VXLAN/host-gw | OVS/OVN |
| NetworkPolicy | 完整支持 | 完整支持 | 不支持 | 增强 |
| 加密 | WireGuard | WireGuard/IPsec | 不支持 | IPsec |
| 性能 | 高 | 极高 | 中 | 高 |
| 可观测性 | 中（Calico 二进制） | 极强（Hubble） | 低 | 中 |
| 多租户 VPC | 不支持 | 部分 | 不支持 | 完整支持 |
| 固定 IP | 支持 | 支持 | 不支持 | 完整支持 |

选型建议：大规模高性能选 Cilium；企业复杂网络选 Kube-OVN；简单快速选 Flannel；传统运维习惯选 Calico。

## Service 四种类型

Kubernetes Service 提供稳定的虚拟 IP 和负载均衡：

- **ClusterIP**（默认）：集群内部访问，通过虚拟 ClusterIP 和 kube-proxy 规则路由到 Pod
- **NodePort**：在所有节点开放端口（30000-32767），外部可通过 `NodeIP:NodePort` 访问
- **LoadBalancer**：云厂商自动配置外部负载均衡器（ELB/SLB），裸金属需 MetalLB/kube-vip
- **ExternalName**：CNAME DNS 记录映射，将 Service 指向外部域名

Service → Endpoints → Pod 的映射链路由 kube-proxy 维护（iptables 模式或 IPVS 模式）。IPVS 模式在大规模 Service 场景下性能更好。

## Ingress → Gateway API 演进

Gateway API 是 Ingress 的下一代替代方案，引入了角色分离：

- **GatewayClass**：基础设施提供者定义（类似 StorageClass）
- **Gateway**：负载均衡器实例，由平台团队管理
- **HTTPRoute/TCPRoute/GRPCRoute**：路由规则，由应用团队管理

优势：角色分离（平台团队 vs 应用团队）、更丰富的路由能力（header 匹配、流量分割）、协议无关（HTTP/TCP/gRPC/UDP）、跨命名空间路由。

主流 Gateway 实现：Envoy Gateway、Cilium Gateway、Nginx Gateway Fabric、Traefik、Istio Gateway。

## NetworkPolicy 与网络安全

默认 Pod 间完全开放。NetworkPolicy 实现网络隔离：
- **Ingress 规则**：控制入站流量（哪些 Pod 可以访问我）
- **Egress 规则**：控制出站流量（我可以访问哪些 Pod/IP）
- 选择器：基于 Pod 标签、Namespace 标签、IP CIDR、命名端口

NetworkPolicy 是 Kubernetes 零信任安全的基础。生产建议：默认 Deny All，按需 Allow。

## 多集群网络

跨集群网络互通方案：
- **Submariner**：通过 IPsec/VXLAN 隧道打通多集群 Pod 网络
- **Cilium Cluster Mesh**：基于 eBPF 的多集群服务发现和路由
- **Meshery Multi-cluster**：统一管理面

---

> 来源：.zread/wiki/drafts/9-wang-luo-ti-xi-cni-service-ingress-gateway-api-yu-duo-ji-qun-wang-luo.md

## Related

- [[概念/IaC x 多集群管理.md|IaC x 多集群管理]] — 基础设施即代码 x 多集群管理
- [[实体/k8s-networking-domain-guide.md|k8s-networking-domain-guide]] — Kubernetes Networking Domain Guide
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)

- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]

<!-- risk-assessed -->
