---
title: kube-vip (entities)
description: '## 概述'
summary: 'kube-vip 为 Kubernetes 集群提供虚拟 IP (VIP) 和负载均衡功能。它可以作为控制平面的高可用解决方案，提供浮动 VIP 确保 API Server 始终可访问。同时也可以作为 LoadBalancer 类型 [[Service|Service]] 的实现，为裸金属环境提供服务负载均衡。'
category: entities
tags:
- k8s
- cncf
- networking
- kube-vip
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-vip 是什么
- 如何 kube-vip
trigger_keywords:
- kube-vip
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-vip

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

kube-vip 是由 plunder-app 开源（现由社区维护）的 Kubernetes 虚拟 IP（VIP）和负载均衡工具，2021 年加入 CNCF Sandbox。它为 Kubernetes 集群提供虚拟 IP 管理和负载均衡功能，可作为控制平面的高可用解决方案（提供浮动 VIP 确保 API Server 始终可访问），也可作为 LoadBalancer 类型 [[Service|Service]] 的实现（为裸金属环境提供服务负载均衡）。

## 核心特性

- **控制平面 HA**: 为 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] Server 提供浮动 VIP
- **Service LoadBalancer**: 裸金属集群的 LoadBalancer 类型 Service 实现
- **ARP/BGP 双模**: Layer 2 (ARP/NDP) 和 Layer 3 (BGP) 两种模式
- **Leader 选举**: 基于 Raft 或 Kubernetes Lease 的分布式选举
- **轻量级**: 单一二进制文件，无外部依赖
- **IPv4/IPv6**: 完整双栈支持

## 架构

kube-vip 以单一 Pod/进程运行在每个节点上。控制平面 HA 模式下，kube-vip 通过 Kubernetes Lease（或 Raft）进行 Leader 选举，持有 VIP 的 Leader 节点通过 ARP（Layer 2）向局域网公告 VIP 的 MAC 地址。当 Leader 节点故障时，新 Leader 接管 VIP 并发送 ARP 广播更新 MAC 映射（ Gratuitous ARP）。BGP 模式下，多个节点同时公告 VIP 到上游路由器，由路由器进行 ECMP 负载均衡。Service LoadBalancer 模式下，kube-vip 通过 Cloud Controller Manager 接口监听 LoadBalancer Service，自动分配和公告 VIP。

## Kubernetes 集成

控制平面 HA 模式下，kube-vip 作为 Static Pod 运行在 Master 节点，为 kube-apiserver 的 6443 端口提供 VIP。配合 keepalived 替代方案。Service LoadBalancer 模式下，kube-vip 通过 `--service-provider` 标志作为 Cloud Provider 运行，监听 `type: LoadBalancer` 的 Service，从 IPAM 池分配 VIP。支持通过 CRD（KubeVIPIPSet）管理 IP 地址池。与 kube-proxy 配合实现完整的南北向流量路由。

## 生产使用场景

1. **裸金属 K8s HA**: 为 kubeadm/k0s/k3s 集群的 API Server 提供浮动 VIP
2. **裸金属 LoadBalancer**: 替代 MetalLB 为 Service 提供外部 IP
3. **BGP 负载均衡**: 大规模集群通过 BGP 实现真正的多节点负载均衡
4. **边缘集群**: 轻量级 VIP 方案适配边缘场景

## 安装

```bash
# 控制平面 HA（Static Pod）
KVVERSION=$(curl -sL https://api.github.com/repos/kube-vip/kube-vip/releases/latest | grep tag_name | cut -d '"' -f 4)
alias kube-vip="ctr image pull ghcr.io/kube-vip/kube-vip:$KVVERSION; \
  ctr run --rm --net-host ghcr.io/kube-vip/kube-vip:$KVVERSION vip /kube-vip"
kube-vip manifest pod --address 192.168.1.100 --controlplane \
  --services --arp --leaderElection | tee /etc/kubernetes/manifests/kube-vip.yaml
# Service LoadBalancer 模式
kubectl apply -f https://kube-vip.io/manifests/kube-vip-cloud-controller.yaml
kubectl apply -f https://kube-vip.io/manifests/kube-vip.yaml
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **kube-vip** | 双用途（HA + LB）、轻量 | BGP 功能不如 MetalLB 成熟 |
| MetalLB | BGP 成熟、功能丰富 | 仅 LB，无控制平面 HA |
| Keepalived | 经典 VIP 方案 | 非 K8s 原生 |
| HAProxy + Keepalived | 成熟稳定 | 运维复杂 |

## 架构定位

在 CNCF 生态中，kube-vip 属于 **Networking** 类别，是裸金属 Kubernetes 集群 VIP 和负载均衡的一体化轻量级方案。

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-vip
- [[实体/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[技能/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
