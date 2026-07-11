---
title: OVN-Kubernetes (entities)
description: '## 概述'
summary: 'OVN-Kubernetes 是一个基于 OVN (Open Virtual Network) 的 Kubernetes CNI 网络插件，提供企业级的虚拟网络功能。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。'
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
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OVN-Kubernetes

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 网络插件，由 Red Hat 维护，2022 年加入 CNCF Sandbox。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。OVN-Kubernetes 是 OpenShift 的默认网络插件，已在大规模生产环境中验证。

## 核心特性

- **OVN 数据平面**: 基于 OVN 的高性能虚拟网络（分布式路由/交换/ACL）
- **NetworkPolicy**: 完整支持 K8s NetworkPolicy（通过 OVN ACL 实现）
- **Egress IP**: 为 Pod 出站流量分配固定源 IP
- **Egress Firewall**: 命名空间级别的出站流量控制（CRD）
- **Hybrid SDN**: 支持命名空间级的网络隔离（非全局 Pod 网络）
- **硬件卸载**: 支持 OVS 硬件卸载（SR-IOV、 Mellanox ASAP）

## 架构

OVN-Kubernetes 架构以 OVN 为核心。OVN 北向数据库（NBDB）和南向数据库（SBDB）以 Raft 集群运行（至少 3 节点 HA）。ovnkube-master（每个控制节点一个）监听 K8s API 获取 Pod、Service、NetworkPolicy 变更，将配置写入 NBDB。ovnkube-node（每个工作节点以 DaemonSet 运行）管理本地 OVS 实例，从 SBDB 获取配置更新 OVS flow table。Pod 网络通过 OVS Geneve 隧道或裸网络（hybrid）连接。NetworkPolicy 通过 OVN ACL 实现，比 iptables 性能更高。

## Kubernetes 集成

OVN-Kubernetes 作为标准 CNI 插件集成。通过 CRD（EgressIP、EgressFirewall、AdminNetworkPolicy）提供超越标准 NetworkPolicy 的高级网络控制。EgressIP CRD 为指定命名空间的 Pod 分配固定出站 IP（配合节点 IP 池）。EgressFirewall CRD 限制命名空间出站流量到特定 CIDR/端口。在 OpenShift 中作为默认 CNI，与 OpenShift SDN 无缝集成。

## 生产使用场景

1. **企业网络隔离**: 使用 Hybrid SDN 或 AdminNetworkPolicy 实现多团队网络隔离
2. **Egress IP 控制**: 为合规要求固定 Pod 出站源 IP
3. **出站防火墙**: 限制 Pod 可访问的外部网络范围
4. **高性能 NetworkPolicy**: 使用 OVN ACL 替代 iptables 实现高性能网络策略

## 安装

```bash
# Helm 安装
helm repo add ovn-kubernetes https://ovn-kubernetes.github.io/ovn-kubernetes
helm install ovn-kubernetes ovn-kubernetes/ovn-kubernetes
# 或使用部署 YAML
kubectl apply -f https://raw.githubusercontent.com/ovn-org/ovn-kubernetes/master/dist/images/yaml-kubernetes/ovn-setup.yaml
# Egress IP 配置
kubectl apply -f - <<EOF
apiVersion: k8s.ovn.org/v1
kind: EgressIP
metadata: { name: egress-prod }
spec:
  egressIPs: ["203.0.113.10"]
  namespaceSelector:
    matchLabels: { env: production }
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **OVN-Kubernetes** | OVN 高性能、OpenShift 验证 | OVN 运维复杂 |
| Calico | BGP 原生、简单 | 无 Egress IP/Firewall |
| Cilium | eBPF 高性能、可观测强 | 企业网络功能较少 |
| Kube-OVN | VPC 多租户 | 社区较小 |

## 架构定位

在 CNCF 生态中，OVN-Kubernetes 属于 **Networking** 类别，是 OVN 技术在 Kubernetes 上的官方实现。OpenShift 默认 CNI，已在大规模生产中验证。

## 参考链接

- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[43-terway-crd-operations]] — Terway CRD 资源操作
- [[sops]] — SOPS (Secrets OPerationS)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- ovn-kubernetes
- [[实体/antrea.md|Antrea]]
- [[实体/kubeslice.md|KubeSlice]]
- [[实体/kuadrant.md|Kuadrant]]
- [[实体/kube-ovn.md|Kube-OVN]]
- [[实体/easegress.md|Easegress]]
- [[实体/bpfman.md|bpfman]]
- [[实体/telepresence.md|Telepresence]]
- [[实体/spiderpool.md|Spiderpool]]
- [[实体/k8gb.md|K8GB]]
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
