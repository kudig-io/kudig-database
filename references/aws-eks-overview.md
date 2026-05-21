---
title: Amazon Elastic Kubernetes Service (EKS)
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- aws
- eks
- etcd
- prometheus
- grafana
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Amazon Elastic Kubernetes Service (EKS) 是什么
- 如何 Amazon Elastic Kubernetes Service (EKS)
trigger_keywords:
- Amazon
- Elastic
- Kubernetes
- Service
- EKS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

# Amazon Elastic Kubernetes Service (EKS)

> **云厂商**: Amazon Web Services (AWS)

## 产品概述

Amazon Elastic Kubernetes Service (EKS) 是 AWS 提供的托管 Kubernetes 服务，让您能够轻松地在 AWS 上运行 Kubernetes，而无需安装、运维和扩展自己的 Kubernetes 控制平面或节点。

## 核心架构

### 网络架构

### VPC 集成
- 集群部署在用户指定的 VPC 中
- 支持私有和公共子网配置
- 安全组控制网络访问
- VPC CNI 插件提供 Pod 网络



## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]

## Related

- [[service-ack-practical-guide]] — service-ack-practical-guide
- [[244-ack-ros-iac]] — 244-ack-ros-iac
- [[azure-aks-overview]] — Azure Kubernetes Service (AKS)
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
