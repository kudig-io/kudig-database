---
title: Kube-OVN (entities)
description: '## 概述'
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
last_updated: 2026-05
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
created: "2026-05-23"
---

# Kube-OVN

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Kube-OVN 是一个基于 OVN/OVS 的高级 Kubernetes 网络 CNI 插件，将 SDN（软件定义网络）的能力引入 Kubernetes。它提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能，是 Kubernetes 网络功能最丰富的 CNI 之一。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **子网规划**: 提前规划子网 CIDR，预留足够的 IP 空间用于扩容
- **VPC 隔离**: 不同租户使用独立 VPC 实现网络级隔离
- **固定 IP**: [[StatefulSet|StatefulSet]] 使用 IP Pool，Pod 使用固定 IP 适配传统应用
- **QoS 管理**: 对流量敏感的应用配置带宽限制，防止资源争抢
- **监控 OVS**: 关注 OVS flow table 大小和连接数，避免性能瓶颈
- **高可用**: OVN 数据库部署在多个 Master 节点实现 HA

## 架构定位

在 CNCF 生态中，kube-ovn 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-ovn
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
