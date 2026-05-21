---
title: OVN-Kubernetes
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- ovn-kubernetes
- cilium
- argocd
- networkpolicy
- crd
- operator
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OVN-Kubernetes 是什么
- 如何 OVN-Kubernetes
trigger_keywords:
- OVN-Kubernetes
prerequisites:
- kubectl-basics
- gitops-basics
- ebpf-basics
- cilium-basics
---

# OVN-Kubernetes

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

OVN-Kubernetes 是一个基于 OVN (Open Virtual Network) 的 Kubernetes CNI 网络插件，提供企业级的虚拟网络功能。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。OVN-Kubernetes 是 OpenShift 的默认网络插件，已在大规模生产环...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **高可用部署**: OVN 数据库 (NBDB/SBDB) 使用 Raft 集群，至少 3 节点
- **Network Policy**: 使用 OVN ACL 实现高性能策略，避免 iptables 规则膨胀
- **Egress 管理**: 使用 Egress IP 和 EgressFirewall 控制出向流量
- **监控**: 监控 OVN 数据库大小和 OVS 流表规模
- **硬件卸载**: 高吞吐场景启用 SR-IOV 或 OVS 硬件卸载

## 架构定位

在 CNCF 生态中，ovn-kubernetes 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[cilium]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[43-terway-crd-operations]] — Terway CRD 资源操作
- [[sops]] — SOPS (Secrets OPerationS)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/ovn-kubernetes/ovn-kubernetes.md|ovn-kubernetes]]
- [[entities/antrea.md|Antrea]]
- [[entities/kubeslice.md|KubeSlice]]
- [[entities/kuadrant.md|Kuadrant]]
- [[entities/kube-ovn.md|Kube-OVN]]
- [[entities/easegress.md|Easegress]]
- [[entities/bpfman.md|bpfman]]
- [[entities/telepresence.md|Telepresence]]
- [[entities/spiderpool.md|Spiderpool]]
- [[entities/k8gb.md|K8GB]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
