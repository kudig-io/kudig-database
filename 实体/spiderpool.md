---
title: Spiderpool (entities)
description: '## 概述'
summary: 'Spiderpool 是一个 Kubernetes 的 Underlay 网络 IPAM (IP Address Management) 解决方案，专为数据中心和云原生环境设计。它支持固定 IP、多网卡、双栈网络等高级特性，能够与多种 CNI 插件无缝集成，特别适合需要 Pod 与物理网络直接通信的场景。'
category: entities
tags:
- k8s
- cncf
- networking
- spiderpool
- cilium
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spiderpool 是什么
- 如何 Spiderpool
trigger_keywords:
- Spiderpool
prerequisites:
- kubectl-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Spiderpool

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

SpiderPool 是一个 CNCF 沙箱项目，由 DaoCloud 开源，是 Kubernetes 下的 Underlay 网络和 IPAM（IP Address Management）解决方案。它为 K8s Pod 提供固定的 Underlay IP 地址分配能力，支持多网卡、多 CNI 协同工作。SpiderPool 特别适合需要固定 IP 的场景（如传统应用迁移、网络设备对接、跨子网通信），支持 VLAN、BGP、SR-IOV 等多种网络模式。它与 Calico、Cilium、Multus 等主流 CNI 配合使用。

## Key Features（核心能力）

- **固定 IP 分配**：为 StatefulSet 和 Deployment 的 Pod 分配固定的 Underlay IP
- **多网卡支持**：支持 Pod 多网卡，每张网卡独立 IPAM
- **多 CNI 协同**：与 Calico、Cilium、Macvlan、SR-IOV 等 CNI 协同
- **IP 预留和回收**：支持 IP 预留（不分配给新 Pod）和自动回收
- **Subnet CRD**：通过 SpiderSubnet CRD 声明式管理 IP 子网
- **VLAN/RDMA 支持**：支持 VLAN 网络和 RDMA 网络配置

## 架构与工作原理

SpiderPool 由 Spiderpool Controller 和 IPAM 插件组成。Controller 管理 SpiderSubnet 和 SpiderReservedIP CRD，协调 IP 池的分配和回收。IPAM CNI 插件在 Pod 创建时从对应的 SpiderSubnet 分配 IP，记录到 SpiderIPPool CRD 中。多 CNI 场景下，通过 Multus 编排多个 CNI 插件，SpiderPool 作为 IPAM 插件为每个接口分配 Underlay IP。

## K8s 集成

SpiderPool 通过丰富的 CRD 与 Kubernetes 集成。SpiderSubnet CRD 定义 IP 子网范围和网关配置。SpiderIPPool CRD 记录已分配的 IP 和关联的 Pod。SpiderMultusConfig CRD 管理 Multus 网络附件配置。通过 DaemonSet 部署 Spiderpool Agent 到每个节点执行 CNI 插件逻辑。与 K8s StatefulSet 集成时，Pod 重建后获得相同 IP。

## 生产用例

- **传统应用迁移**：需要固定 IP 的遗留应用迁移到 K8s
- **多 CNI 网络**：Underlay + Overlay 混合网络（如 Calico Overlay + Macvlan Underlay）
- **网络策略合规**：防火墙规则需要固定 IP 的安全合规场景
- **RDMA/GPU 网络**：AI 训练集群的 RDMA 网络配置

## 安装与快速开始

```bash
helm repo add spiderpool https://spidernet-io.github.io/spiderpool
helm install spiderpool spiderpool/spiderpool -n kube-system
```

## 对比替代方案

相比 Whereabouts（IPv6 IPAM），SpiderPool 支持更丰富的固定 IP 和多网卡能力。相比 Calico IPAM（基于 Block 分配），SpiderPool 提供精确的 Pod 级 IP 固定。

## Related

- [[openfunction]] — OpenFunction
- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiderpool
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
