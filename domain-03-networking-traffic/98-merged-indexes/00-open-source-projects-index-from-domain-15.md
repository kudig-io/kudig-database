---
title: Domain-15 网络基础 — 开源项目索引
description: '# Domain-15 网络基础 — 开源项目索引'
summary: '# Domain-15 网络基础 — 开源项目索引'
category: network-fundamentals
tags:
- network
- tcp
- ip
- dns
- istio
- envoy
- cilium
- flannel
- calico
- coredns
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 网络工程师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-15 网络基础 — 开源项目索引 是什么
- 如何 Domain-15 网络基础 — 开源项目索引
- Kubernetes 15 network fundamentals 最佳实践
trigger_keywords:
- Domain-15
- 网络基础
- 开源项目索引
- network
- fundamentals
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-15 网络基础 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **CNI** | 容器网络接口标准 | Incubating | v1.6.0 | 5k+ | Apache-2.0 |
| **Cilium** | eBPF 网络与安全 | Graduated | v1.17.0 | 21k+ | Apache-2.0 |
| **Calico** | L3 网络与网络策略 | 非 CNCF | v3.29.0 | 6k+ | Apache-2.0 |
| **Flannel** | 简单 overlay 网络 | 非 CNCF | v0.26.0 | 9k+ | Apache-2.0 |
| **Weave Net** | 容器网络 (已归档) | 非 CNCF | v2.8.1 | 6k+ | Apache-2.0 |
| **OVN-Kubernetes** | OVN 网络方案 | CNCF Sandbox | v1.0.0 | 1k+ | Apache-2.0 |
| **Antrea** | VMware K8s 网络 | VMware | v2.2.0 | 1k+ | Apache-2.0 |
| **Kube-OVN** | 基于 OVN 的 K8s CNI | 非 CNCF | v1.13.0 | 2k+ | Apache-2.0 |
| **Submariner** | 多集群网络连接 | 非 CNCF | v0.19.0 | 3k+ | Apache-2.0 |
| **MetalLB** | 裸金属负载均衡 | 非 CNCF | v0.14.0 | 7k+ | Apache-2.0 |
| **CoreDNS** | 集群 DNS | Graduated | v1.12.0 | 11k+ | Apache-2.0 |
| **ExternalDNS** | 外部 DNS 自动同步 | 非 CNCF | v0.15.0 | 7k+ | Apache-2.0 |

---

## CNI 选型指南

| 场景 | 推荐 | 理由 |
|:---|:---|:---|
| 安全优先 + 可观测性 | Cilium | eBPF 高性能，NetworkPolicy L3-L7 |
| 简单快速上手 | Flannel | 配置最简单，overlay 模式 |
| 大规模生产 | Calico / Cilium | 成熟稳定，BGP 模式性能优 |
| Windows 容器 | Calico | 官方支持 Windows HNS |
| 裸金属 LB | MetalLB | 无云厂商时的 LoadBalancer |
| 多集群互联 | Submariner + Cilium | 跨集群 Pod IP 直通 |
| OpenStack 背景 | Kube-OVN / OVN-Kubernetes | OVN 生态一致 |

---

## Gateway API 实现

| 实现 | 特点 | 状态 |
|:---|:---|:---|
| Istio Gateway | 功能最丰富 | 成熟 |
| Cilium Gateway | eBPF 高性能 | v1.14+ GA |
| Envoy Gateway | 官方 Envoy 实现 | 快速发展 |
| NGINX Gateway Fabric | NGINX 官方 | 成熟 |
| Kong Gateway | API 管理集成 | 成熟 |
| Traefik | 云原生友好 | 成熟 |

---

## 补充: 高级网络项目

| 项目 | 作用 | 归属 | 版本 | 备注 |
|:---|:---|:---|:---|:---|
| **Multus** | 多 CNI 网络接口 | 社区 | v4.1.0 | 支持 Pod 多网卡 |
| **Whereabouts** | IPAM CNI 插件 | 社区 | v0.8.0 | 动态 IP 分配 |
| **Cilium Cluster Mesh** | 多集群 Cilium 互联 | Graduated | v1.17.0 | 跨集群服务发现 |
| **Kube-OVN** | 基于 OVN 的 K8s CNI | 非 CNCF | v1.13.0 | OpenStack 背景友好 |
| **Antrea** | VMware K8s 网络 | VMware | v2.2.0 | NSX 生态兼容 |
| **Submariner** | 多集群网络互联 | 非 CNCF | v0.19.0 | Pod IP 跨集群路由 |
| **Skupper** | 应用级安全网络 | Red Hat | v2.0.0 | 无需 CNI 改动 |

---

## 参考链接

- [CNI 规范](https://www.cni.dev/docs/)
- [Cilium 文档](https://docs.cilium.io/)
- [Calico 文档](https://docs.tigera.io/)
- [Gateway API](https://gateway-api.sigs.k8s.io/)
- [K8s 网络概念](https://kubernetes.io/docs/concepts/cluster-administration/networking/)

---

## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain-15: 网络基础]]
- 网络协议栈详解
- TCP/UDP 协议深度解析
- DNS 原理与配置
- 负载均衡技术
- 网络安全基础
- SDN 与网络虚拟化
- Cilium eBPF 网络与安全实践指南


<!-- risk-assessed -->
