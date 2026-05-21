---
title: tencent-tke-overview
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- tke
- tencent
- prometheus
- grafana
- istio
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- tencent-tke-overview 是什么
- 如何 tencent-tke-overview
trigger_keywords:
- tencent-tke-overview
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

# tencent-tke-overview

> **云厂商**: 腾讯云容器服务 TKE

## 产品概述

腾讯云 Kubernetes 服务(TKE)是腾讯云提供的托管容器服务，基于腾讯内部海量业务实践经验，为企业提供高性能、高可靠的容器化应用部署和管理平台。TKE承载了腾讯内部包括微信、QQ、王者荣耀等核心业务的容器化部署，具备处理亿级用户并发的能力。

## 核心架构

详见源文档获取完整产品架构信息。^[inferred]


## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]

## Related

- [[ibm-iks-overview]] — IBM Cloud Kubernetes Service
- [[ctyun-tke-overview]] — ctyun-tke-overview
- [[google-cloud-gke-overview]] — Google Kubernetes Engine (GKE)
- [[volcengine-vek-overview]] — volcengine-vek-overview
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[references/k8s-cloud-provider-comparison|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
