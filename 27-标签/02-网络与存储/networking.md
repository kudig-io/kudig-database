---
title: networking
description: 网络技术标签枢纽 — 涵盖 CNI、Service Mesh、Ingress、DNS、eBPF、Gateway API、网络策略、多集群网络等全部网络领域知识
category: tag-index
tags:
- networking
- cni
- service-mesh
- ingress
- dns
- ebpf
- gateway-api
tier: core
difficulty: intermediate-to-advanced
domain: networking
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# networking Tag Hub

> 网络领域页面 — CNI、Service Mesh、Ingress、DNS、eBPF、Gateway API、网络策略等。

## 核心定义

**Kubernetes 网络**是容器编排平台中最复杂的子系统之一，负责 Pod 间通信、服务发现、外部流量入口、网络策略隔离等核心功能。它基于 CNI（Container Network Interface）插件体系，支持多种网络实现方案。

### 网络模型核心原则

1. **每个 Pod 拥有独立 IP**：Pod 内所有容器共享网络命名空间
2. **Pod 间无 NAT 通信**：任何 Pod 可直接通过 IP 访问其他 Pod
3. **节点与 Pod 无 NAT**：节点可直接通过 Pod IP 访问 Pod
4. **Service 抽象**：通过虚拟 IP 提供稳定的服务访问入口

### 网络技术栈全景

| 层级 | 技术 | 功能 |
|------|------|------|
| L2/L3 网络 | CNI (Calico/Cilium/Flannel/Terway) | Pod 网络连通性 |
| L4 负载均衡 | kube-proxy (iptables/IPVS/nftables) | Service 流量分发 |
| L7 入口 | Ingress Controller / Gateway API | HTTP/gRPC 路由 |
| 服务网格 | Istio / Linkerd / Cilium Mesh | mTLS、流量管理、可观测 |
| DNS | CoreDNS / NodeLocal DNS | 服务发现 |
| 网络策略 | NetworkPolicy / CiliumNetworkPolicy | 微分段隔离 |
| 多集群 | Submariner / Cilium Cluster Mesh | 跨集群连通 |
| 可编程网络 | eBPF / XDP | 高性能数据平面 |

### CNI 插件对比

| CNI | 数据平面 | 网络策略 | 性能 | 特色 |
|-----|----------|----------|------|------|
| Cilium | eBPF | L3/L4/L7 | 极高 | Hubble 可观测、Tetragon 安全 |
| Calico | BGP/VXLAN | L3/L4 | 高 | Felix+Typha 架构、企业版 |
| Flannel | VXLAN/host-gw | 无 | 中 | 简单稳定、无策略能力 |
| Terway | ENI/ENIIP | L3/L4 | 高 | 阿里云原生、VPC 直通 |
| Weave | VXLAN | L3/L4 | 中 | 加密、多集群 |
| Antrea | OVS | L3/L4/L7 | 高 | 企业网络特性 |

## 生产实践要点

### 网络性能基准

| 指标 | 目标 | 度量工具 |
|------|------|----------|
| Pod-to-Pod 延迟 | < 1ms (同节点), < 2ms (跨节点) | netperf, iperf3 |
| Service 转发延迟 | < 0.5ms (IPVS) | curl -w |
| DNS 解析延迟 | < 5ms (P99) | dig, nslookup |
| Ingress 吐吐量 | > 10K RPS (Nginx) | wrk, k6 |
| 网络策略延迟 | < 0.1ms (eBPF) | cilium metrics |

### 常见网络故障快速定位

| 症状 | 可能原因 | 排查命令 |
|------|----------|----------|
| Pod 无法解析 DNS | CoreDNS 异常/NodeLocal DNS 未部署 | `kubectl logs -n kube-system -l k8s-app=kube-dns` |
| Service 无法访问 | Endpoint 为空/kube-proxy 异常 | `kubectl get ep <svc>` |
| 跨节点 Pod 不通 | CNI 路由/MTU 问题 | `ip route`, `ping -s 1472` |
| Ingress 502 | 后端 Pod 未就绪/超时 | `kubectl logs -n ingress-nginx` |
| NetworkPolicy 不生效 | CNI 不支持/标签不匹配 | `kubectl describe networkpolicy` |
| 外部无法访问 LB | 云 LB 健康检查失败 | 检查 nodePort + 安全组 |

## K8s 网络核心 (K8s Networking Core)

- [[05-网络/01-K8s网络核心/00-network-in-nutshell|网络核心概要]]
- [[05-网络/01-K8s网络核心/02-network-architecture-overview|网络架构概览]]
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals|CNI 架构基础]]
- [[05-网络/01-K8s网络核心/04-cni-plugins-comparison|CNI 插件对比]]
- [[05-网络/01-K8s网络核心/05-flannel-complete-guide|Flannel 完整指南]]
- [[05-网络/01-K8s网络核心/06-terway-advanced-guide|Terway 高级指南]]
- [[05-网络/01-K8s网络核心/07-service-concepts-types|Service 概念与类型]]
- [[05-网络/01-K8s网络核心/08-service-implementation-details|Service 实现细节]]
- [[05-网络/01-K8s网络核心/10-kube-proxy-modes-performance|kube-proxy 模式与性能]]
- [[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns|DNS 服务发现 CoreDNS]]
- [[05-网络/01-K8s网络核心/14-coredns-architecture-principles|CoreDNS 架构原理]]
- [[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice|NetworkPolicy 深度实践]]
- [[05-网络/01-K8s网络核心/18-network-policy-advanced|高级网络策略]]
- [[05-网络/01-K8s网络核心/19-network-encryption-mtls|网络加密与 mTLS]]
- [[05-网络/01-K8s网络核心/20-ingress-fundamentals|Ingress 基础]]
- [[05-网络/01-K8s网络核心/21-ingress-controller-deep-dive|Ingress Controller 深度指南]]
- [[05-网络/01-K8s网络核心/22-nginx-ingress-complete-guide|Nginx Ingress 完整指南]]
- [[05-网络/01-K8s网络核心/27-ingress-production-best-practices|Ingress 生产最佳实践]]
- [[05-网络/01-K8s网络核心/32-service-mesh-deep-dive|Service Mesh 深度指南]]
- [[05-网络/01-K8s网络核心/33-multi-cluster-federation|多集群联邦]]
- [[05-网络/01-K8s网络核心/34-multi-cluster-networking|多集群网络]]
- [[05-网络/01-K8s网络核心/35-network-troubleshooting|网络故障排查]]
- [[05-网络/01-K8s网络核心/36-network-performance-tuning|网络性能调优]]
- [[05-网络/01-K8s网络核心/37-gateway-api-overview|Gateway API 概览]]
- [[05-网络/01-K8s网络核心/38-api-gateway-patterns|API 网关模式]]

## eBPF / Cilium

- [[05-网络/05-eBPF/01-ebpf-architecture-fundamentals|eBPF 架构基础]]
- [[05-网络/05-eBPF/03-cilium-cni-architecture|Cilium CNI 架构]]
- [[05-网络/05-eBPF/04-cilium-network-policy|Cilium 网络策略]]
- [[05-网络/05-eBPF/05-cilium-service-mesh|Cilium Service Mesh]]
- [[05-网络/05-eBPF/06-tetragon-runtime-security|Tetragon 运行时安全]]
- [[05-网络/05-eBPF/07-hubble-network-observability|Hubble 网络可观测性]]
- [[05-网络/05-eBPF/10-ebpf-security-applications|eBPF 安全应用]]
- [[05-网络/05-eBPF/12-ebpf-observability-tools|eBPF 可观测性工具]]

## Terway

- [[05-网络/06-Terway/01-product|Terway 产品概述]]
- [[05-网络/06-Terway/02-architecture|Terway 架构]]
- [[05-网络/06-Terway/04-operations|Terway 运维]]
- [[05-网络/06-Terway/07-troubleshooting-fta|Terway 故障树分析]]

## API 网关 (API Gateway)

- [[05-网络/04-API网关/04-higress-enterprise-gateway|Higress 企业级网关]]
- [[05-网络/04-API网关/05-apisix-enterprise-gateway|APISIX 企业级网关]]
- [[05-网络/04-API网关/06-kong-enterprise-gateway|Kong 企业级网关]]
- [[05-网络/04-API网关/08-traefik-enterprise-gateway|Traefik 企业级网关]]
- [[05-网络/04-API网关/14-api-gateway-production-operations|API 网关生产运营]]

## Service Mesh (服务网格)

- [[05-网络/03-服务网格/02-linkerd-enterprise-service-mesh|Linkerd 企业级 Service Mesh]]
- [[05-网络/03-服务网格/05-dapr-enterprise-distributed-runtime|Dapr 企业级分布式运行时]]
- [[05-网络/03-服务网格/07-service-mesh-comparison-selection|Service Mesh 对比选型]]
- [[05-网络/03-服务网格/11-istio-service-mesh-guide|Istio Service Mesh 指南]]

## 网络基础 (Networking Fundamentals)

- [[05-网络/02-网络基础/01-network-protocols-stack|网络协议栈]]
- [[05-网络/02-网络基础/02-tcp-udp-deep-dive|TCP/UDP 深度指南]]
- [[05-网络/02-网络基础/03-dns-principles-configuration|DNS 原理与配置]]
- [[05-网络/02-网络基础/04-load-balancing-technologies|负载均衡技术]]
- [[05-网络/02-网络基础/05-network-security-fundamentals|网络安全基础]]
- [[05-网络/02-网络基础/09-cilium-ebpf-network-guide|Cilium eBPF 网络指南]]

## 概念 (Concepts)

- [[22-概念/03-网络/cni-networking-model|CNI 网络模型]]
- [[22-概念/03-网络/service-networking|Service 网络]]
- [[22-概念/12-研究/k8s-networking-evolution|K8s 网络演进]]
- [[22-概念/03-网络/network-policy|NetworkPolicy]]
- [[22-概念/03-网络/cilium-ebpf-networking|Cilium eBPF 网络]]
- [[22-概念/03-网络/ingress|Ingress]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting|CNI 网络排障]]
- [[19-故障诊断/03-基础设施排障/01-network-connectivity-troubleshooting|网络连通性排障]]
- [[19-故障诊断/02-资源排障/02-service-comprehensive-troubleshooting|Service 综合排障]]
- [[19-故障诊断/02-资源排障/08-networkpolicy-troubleshooting|NetworkPolicy 排障]]
- [[19-故障诊断/04-高级排障/structural-03-networking/04-networkpolicy-troubleshooting|高级网络策略排障]]

## 云厂商网络 (Cloud Provider Networking)

- [[18-云厂商/02-AWS-EKS/02-eks-networking-vpc-cni|AWS EKS 网络 VPC CNI]]
- [[18-云厂商/04-Azure-AKS/02-aks-networking-azure-cni|Azure AKS 网络 Azure CNI]]
- [[18-云厂商/03-Google-GKE/02-gke-networking-dataplane-v2|GKE 网络 Dataplane V2]]
- [[18-云厂商/01-阿里云/03-Terway-CNI网络|阿里云 Terway CNI 网络]]
- [[18-云厂商/06-华为云CCE/01-cce-networking-vpc-router|华为云 CCE 网络 VPC Router]]
- [[18-云厂商/05-腾讯云TKE/01-tke-networking-vpc-cni|腾讯云 TKE 网络 VPC CNI]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/networking/cni|CNI]]
- [[17-系统基础/06-知识字典/networking/coredns|CoreDNS]]
- [[17-系统基础/06-知识字典/networking/dns|DNS]]
- [[17-系统基础/06-知识字典/networking/network-policy|Network Policy]]
- [[17-系统基础/06-知识字典/networking/ingress-controller|Ingress Controller]]
- [[17-系统基础/06-知识字典/networking/loadbalancer|LoadBalancer]]

## 实体 (Entities)

- [[23-实体/04-网络/cilium|Cilium]]
- [[23-实体/02-K8s核心组件/cni|CNI]]
- [[23-实体/02-K8s核心组件/coredns|CoreDNS]]
- [[23-实体/04-网络/envoy|Envoy]]
- [[23-实体/04-网络/istio|Istio]]
- [[23-实体/04-网络/metallb|MetalLB]]
- [[23-实体/15-参考与索引/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]]

## Related Tags

- [[27-标签/01-核心平台/k8s|k8s]]
- [[27-标签/03-安全与合规/security|security]]
- [[27-标签/01-核心平台/containerd|containerd]]
