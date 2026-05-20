---
title: Domain-5 网络 — 开源项目索引
description: '# Domain-5 网络 — 开源项目索引'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- istio
- envoy
- cilium
- flannel
- calico
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-5 网络 — 开源项目索引 是什么
- 如何 Domain-5 网络 — 开源项目索引
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Domain-5
- 网络
- 开源项目索引
- networking
cross_refs:
- type: domain
  path: ../domain-3-control-plane/
  label: '相关知识域: domain-3-control-plane'
- type: domain
  path: ../domain-15-network-fundamentals/
  label: '相关知识域: domain-15-network-fundamentals'
- type: domain
  path: ../domain-8-observability/
  label: '相关知识域: domain-8-observability'
- type: cheatsheet
  path: ../topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

# Domain-5 网络 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Cilium** | eBPF CNI + Service Mesh + 网络策略 | Graduated | v1.17.0 | 21k+ | Apache-2.0 |
| **Calico** | L3 CNI + 网络策略 | 非 CNCF | v3.29.0 | 6k+ | Apache-2.0 |
| **Flannel** | 简单 Overlay CNI | 非 CNCF | v0.26.0 | 9k+ | Apache-2.0 |
| **CoreDNS** | 集群 DNS | Graduated | v1.12.0 | 11k+ | Apache-2.0 |
| **Istio** | 服务网格 (流量管理) | Graduated | v1.29.0 | 36k+ | Apache-2.0 |
| **Linkerd** | 轻量级服务网格 | Graduated | v2.18.0 | 10k+ | Apache-2.0 |
| **Envoy** | L7 代理与网关 | Graduated | v1.33.0 | 25k+ | Apache-2.0 |
| **MetalLB** | 裸金属 LoadBalancer | 非 CNCF | v0.14.0 | 7k+ | Apache-2.0 |
| **Ingress NGINX** | K8s Ingress 控制器 | K8s SIG | v1.12.0 | 17k+ | Apache-2.0 |
| **Emissary-Ingress** | API 网关 | Incubating | v3.10.0 | 4.5k+ | Apache-2.0 |
| **Contour** | Envoy Ingress | Incubating | v1.30.0 | 3.5k+ | Apache-2.0 |
| **Gateway API** | 新一代 K8s 流量管理 | K8s SIG | v1.2.0 | - | Apache-2.0 |
| **Submariner** | 多集群网络互联 | 非 CNCF | v0.19.0 | 3k+ | Apache-2.0 |

---

## 参考链接

- [K8s 网络文档](https://kubernetes.io/docs/concepts/services-networking/)
- [Cilium 文档](https://docs.cilium.io/)
- [Gateway API](https://gateway-api.sigs.k8s.io/)
