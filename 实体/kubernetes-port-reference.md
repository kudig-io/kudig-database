---
title: Kubernetes Port Reference
description: Kubernetes Port Reference — Kubernetes 生产运维知识库
summary: Kubernetes Port Reference — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- networking
- ports
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Port Reference 是什么
- 如何 Kubernetes Port Reference
trigger_keywords:
- Kubernetes
- Port
- Reference
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Port Reference

## Control Plane Ports

| Port | Protocol | Component | Direction | Notes |
|---|---|---|---|---|
| 6443 | TCP | kube-apiserver | Inbound | HTTPS API (default) |
| 2379-2380 | TCP | etcd | Internal | Client/peer communication |
| 10257 | TCP | kube-controller-manager | Inbound | HTTPS metrics |
| 10259 | TCP | kube-scheduler | Inbound | HTTPS metrics |
| 10250 | TCP | kubelet | Internal | Pod/exec, logs, metrics |

## Node Ports

| Port | Protocol | Component | Notes |
|---|---|---|---|
| 10256 | TCP | kube-proxy | Health check endpoint |
| 30000-32767 | TCP/UDP | NodePort Services | Default NodePort range |
| 179 | TCP | BGP (Calico) | BGP peering |
| 4789 | UDP | VXLAN (Calico/Flannel) | Overlay tunnel |
| 8472 | UDP | VXLAN (Flannel) | Default Flannel VXLAN |

## Addon Ports

| Port | Protocol | Addon | Notes |
|---|---|---|---|
| 53 | TCP/UDP | CoreDNS | DNS resolution |
| 9090 | TCP | Prometheus | Web UI |
| 3000 | TCP | Grafana | Dashboard |
| 8080 | TCP | ArgoCD | HTTP API (before redirect) |
| 443 | TCP | ArgoCD | HTTPS UI/API |
| 9093 | TCP | Alertmanager | Alert management |
| 8443 | TCP | Metrics Server | Aggregated API |

## CNI-Specific Ports

| Port | Protocol | CNI | Notes |
|---|---|---|---|
| 4194 | TCP | Cilium Hubble | Observability UI |
| 4240 | TCP | Cilium | Health server |
| 4244 | TCP | Cilium | Prometheus metrics |
| 6060 | TCP | Cilium Operator | Debug endpoint |
| 9090 | TCP | Calico | Felix metrics |
| 9099 | TCP | Calico | Typha health |

## Security Notes

- Ports 6443, 10250 must be restricted to cluster-internal access
- etcd ports should NEVER be exposed outside the control plane
- NodePort range should be planned to avoid conflicts with host services
- Consider using NetworkPolicy to restrict inter-pod traffic

## Related

- [[reference|#reference Hub]] — tag hub

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[概念/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]

- [[平台工程/代码分析/cluster-delete/11-network-cleanup.md|11-network-cleanup]]

<!-- risk-assessed -->
