---
title: Azure Kubernetes Service (AKS)
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- aks
- prometheus
- grafana
- istio
- argocd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Azure Kubernetes Service (AKS) 是什么
- 如何 Azure Kubernetes Service (AKS)
trigger_keywords:
- Azure
- Kubernetes
- Service
- AKS
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

# Azure Kubernetes Service (AKS)

> **云厂商**: Microsoft Azure

## 产品概述

Azure Kubernetes Service (AKS) 是 Microsoft Azure 提供的企业级托管 Kubernetes 服务，简化了在 Azure 云环境中部署、管理和扩展容器化应用程序的过程。

## 核心架构

详见源文档获取完整产品架构信息。^[inferred]


## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/argocd.md|argocd]]

## Related

- [[huawei-cce-overview]] — huawei-cce-overview
- [[251-apsara-stack-sls-logging]] — [[references/251-apsara-stack-sls-logging.md|251-apsara-stack-sls-logging]]
- [[service-ack-practical-guide]] — service-ack-practical-guide
- [[244-ack-ros-iac]] — 244-ack-ros-iac
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
