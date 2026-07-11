---
title: Kube-OVN (entities)
description: '## 概述'
summary: 'Kube-OVN 是一个基于 OVN/OVS 的高级 Kubernetes 网络 CNI 插件，将 SDN（软件定义网络）的能力引入 Kubernetes。它提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能，是 Kubernetes 网络功能最丰富的 CNI 之一。'
category: entities
tags:
- k8s
- cncf
- networking
- kube-ovn
- statefulset
- networkpolicy
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-OVN 是什么
- 如何 Kube-OVN
trigger_keywords:
- Kube-OVN
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kube-OVN

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Kube-OVN 是由灵雀云（Alauda）开源的高级 Kubernetes CNI 网络插件，基于 OVN（Open Virtual Network）/OVS（Open vSwitch）构建。它将 SDN（软件定义网络）的能力引入 Kubernetes，提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能。Kube-OVN 是 Kubernetes 网络功能最丰富的 CNI 之一，特别适合需要复杂网络拓扑的企业场景。

## 核心特性

- **子网管理**: Namespace 与子网关联，支持自定义 CIDR 和网关
- **固定 IP**: 为 [[StatefulSet|StatefulSet]] Pod 和普通 Pod 提供固定 IP 分配
- **VPC 多租户**: 自定义 VPC 实现网络级隔离，支持跨子网路由
- **QoS 带宽管理**: 为 Pod 配置入站/出站带宽限制
- **网络策略**: 增强 NetworkPolicy，支持 ICMP、网段级别控制
- **EIP/SNAT/DNAT**: 外部 IP 映射和 NAT 能力

## 架构

Kube-OVN 基于 OVN（Open Virtual Network）构建。核心组件包括：kube-ovn-controller（主控制器，监听 K8s API 管理 OVN 逻辑路由器、交换机和端口）、kube-ovn-cni（节点 CNI 插件，管理 OVS 网桥和接口）、ovn-nb/ovn-sb（OVN 北向/南向数据库）。网络数据平面使用 OVS 内核模块或 DPDK 加速。OVN 提供逻辑路由器、逻辑交换机、ACL 和 LB 能力，Kube-OVN 将这些能力以 CRD（Subnet、VPC、IP、Vip 等）暴露给用户。

## Kubernetes 集成

Kube-OVN 作为标准 CNI 插件与 Kubernetes 集成。通过 CRD（Subnet、VPC、Vip、IP、QoS）声明式管理网络资源。Subnet CRD 将 Namespace 与 OVN 子网关联，Pod 创建时自动分配子网内 IP。VPC CRD 创建隔离的虚拟网络，实现多租户。支持 Kubernetes NetworkPolicy API 和自定义增强策略。通过 kube-ovn-controller 将 OVN 配置同步到每个节点的 OVS 实例。

## 生产使用场景

1. **多租户网络隔离**: 使用自定义 VPC 为不同租户创建隔离网络环境
2. **固定 IP 需求**: 为传统应用（如数据库、中间件）提供固定 Pod IP
3. **混合云网络**: 通过 VPC 互联和 EIP 实现与外部网络的灵活连接
4. **QoS 流量控制**: 对不同优先级应用实施带宽限制

## 安装

```bash
# 一键安装
kubectl apply -f https://raw.githubusercontent.com/kubeovn/kube-ovn/master/dist/images/install.yaml
# 或 Helm
helm repo add kubeovn https://kubeovn.github.io/kube-ovn/
helm install kube-ovn kubeovn/kube-ovn
# 创建子网
kubectl apply -f - <<EOF
apiVersion: kubeovn.io/v1
kind: Subnet
metadata: { name: prod }
spec:
  protocol: IPv4
  cidrBlock: 10.0.1.0/24
  gateway: 10.0.1.1
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kube-OVN** | 功能最全面、VPC 多租户 | OVS 运维复杂、性能开销 |
| Calico | 高性能、BGP 原生 | 无 VPC/固定 IP 功能 |
| Cilium | eBPF 高性能、可观测性强 | 企业网络功能较少 |
| Antrea | OVS 基础、策略丰富 | 无 VPC 多租户 |

## 架构定位

在 CNCF 生态中，Kube-OVN 属于 **Networking** 类别，是将 SDN 能力引入 Kubernetes 的代表性项目。适合需要复杂网络功能的企业场景。

## 参考链接

- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-ovn
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
