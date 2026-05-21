---
title: alicloud-ack-overview
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- ack
- alicloud
- etcd
- prometheus
- grafana
- cilium
- flannel
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- alicloud-ack-overview 是什么
- 如何 alicloud-ack-overview
trigger_keywords:
- alicloud-ack-overview
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

# alicloud-ack-overview

> **云厂商**: 阿里云 (Alibaba Cloud)

## 产品概述

阿里云容器服务 Kubernetes 版 (ACK) 是阿里云提供的高性能容器应用管理平台，基于阿里巴巴集团十年容器技术沉淀，为企业提供安全可靠的容器化应用部署和管理服务。

## 核心架构

### 产品架构与核心组件

### 控制平面架构

ACK 采用双模式架构设计，满足不同客户的多样化需求：

**托管版 (Managed Kubernetes)**
- 完全托管的控制平面，由阿里云负责运维
- 控制平面部署在阿里云专用VPC中，与用户网络隔离
- 支持多可用区高可用部署
- 自动故障检测和恢复机制

**专有版 (Dedicated Kubernetes)**
- 用户自管理控制平面，部署在用户VPC内
- 提供更高的安全隔离和合规性
- 支持离线环境和私有化部署
- 适用于金融、政府等对数据安全要求极高的行业

### 数据平面组件

**节点管理**
- 支持多种节点类型：ECS实例、ECI弹性容器实例、自建服务器
- 节点池管理：自动扩缩容、混合实例规格、Spot实例支持
- 节点标签和污点策略，实现精细化调度

**网络架构**
- Terway网络插件：基于阿里云弹性网卡的高性能网络方案
- Flannel网络插件：兼容开源社区标准
- 支持IPv4/IPv6双栈网络
- 网络策略和安全组深度集成

**存储架构**
- 云盘CSI驱动：ESSD、SSD、普通云盘等多种性能等级
- 



## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/cni-plugins.md|cni-plugins]]

## Related

- [[ctyun-tke-overview]] — ctyun-tke-overview
- [[google-cloud-gke-overview]] — Google Kubernetes Engine (GKE)
- [[volcengine-vek-overview]] — volcengine-vek-overview
- [[tencent-tke-overview]] — tencent-tke-overview
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[references/k8s-cloud-provider-comparison|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
