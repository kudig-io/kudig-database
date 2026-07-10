---
title: OVN-Kubernetes 网络方案
description: OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red
  Hat 主导开发，是...
summary: OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red Hat
  主导开发，是...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
- ovn
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OVN-Kubernetes 网络方案 是什么
- OVN-Kubernetes 详解
trigger_keywords:
- OVN-Kubernetes 网络方案
- OVN-Kubernetes
- dictionary
prerequisites:
- kubernetes
---



# OVN-Kubernetes 网络方案（OVN-Kubernetes）

## 概述

OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red Hat 主导开发，是 OpenShift 的默认网络方案，提供完整的 L2/L3 网络、NetworkPolicy 和硬件加速能力。

## 核心概念/原理

- **OVN 数据面**：基于 OpenFlow 的虚拟网络，支持硬件卸载
- **完整 NetworkPolicy**：支持 Ingress/Egress 和 FQDN 策略
- **OpenShift 默认**：Red Hat OpenShift 的标准 CNI
- **硬件加速**：支持 SmartNIC/DPU 卸载

## 关键机制或特性

- OVN Northbound/Southbound 数据库架构
- OVS（Open vSwitch）作为节点数据面
- 支持 Hybrid Overlay（Windows + Linux 节点混合）
- EgressFirewall / EgressQoS / EgressService CRD
- AdminNetworkPolicy（K8s 增强网络策略）
- IPAM 管理和多子网支持

## 使用场景与最佳实践

- OpenShift / OCP 集群的标准网络方案
- 需要硬件加速的企业网络
- Windows + Linux 混合节点集群
- 需要 AdminNetworkPolicy 的多租户环境
- 大规模集群的高性能网络

## 参考链接

- https://github.com/ovn-kubernetes/ovn-kubernetes
- https://docs.openshift.com/container-platform/latest/networking/understanding-networking.html

## Related

- [[系统基础/topic-dictionary/networking/antrea.md|Antrea]]
- [[系统基础/topic-dictionary/networking/cilium.md|Cilium]]
- [[系统基础/topic-dictionary/networking/cni.md|CNI]]
