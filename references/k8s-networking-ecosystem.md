---
title: 网络体系：CNI、Service、Ingress、Gateway API 与多集群网络
description: '# 网络体系'
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
last_updated: 2026-05
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
created: "2026-05-23"
---

# 网络体系

## CNI 插件对比

| 特性 | Calico | Cilium | Flannel |
|------|--------|--------|---------|
| 数据平面 | iptables/eBPF | eBPF | VXLAN/host-gw |
| NetworkPolicy | ✅ | ✅ | ❌ |
| 加密 | WireGuard | WireGuard/IPsec | ❌ |
| 性能 | 高 | 极高 | 中 |
| 可观测性 | 中 | 极强（Hubble） | 低 |

## Service 四种类型

- **ClusterIP**（默认）：集群内部访问
- **NodePort**：通过节点端口暴露
- **LoadBalancer**：云厂商 LB 集成
- **ExternalName**：CNAME 映射

Service → Endpoints → Pod 的映射链路由 kube-proxy 维护（iptables 或 IPVS 模式）。

## Ingress → Gateway API 演进

Gateway API 是 Ingress 的下一代替代方案：
- **GatewayClass**：基础设施提供者定义
- **Gateway**：负载均衡器实例
- **HTTPRoute/TCPRoute**：路由规则

优势：角色分离（平台团队 vs 应用团队）、更丰富的路由能力、协议无关。

## NetworkPolicy

默认 Pod 间完全开放。NetworkPolicy 实现网络隔离：
- **Ingress**规则：控制入站流量
- **Egress**规则：控制出站流量
- 基于 Pod 标签、Namespace 标签、IP CIDR 选择器

---

> 来源：.zread/wiki/drafts/9-wang-luo-ti-xi-cni-service-ingress-gateway-api-yu-duo-ji-qun-wang-luo.md

## Related

- [[synthesis/IaC x 多集群管理.md|IaC x 多集群管理]] — 基础设施即代码 x 多集群管理
- [[references/k8s-networking-domain-guide.md|k8s-networking-domain-guide]] — Kubernetes Networking Domain Guide
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)

- [[synthesis/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]