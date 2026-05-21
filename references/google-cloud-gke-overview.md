---
title: Google Kubernetes Engine (GKE)
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- gke
- etcd
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
- Google Kubernetes Engine (GKE) 是什么
- 如何 Google Kubernetes Engine (GKE)
trigger_keywords:
- Google
- Kubernetes
- Engine
- GKE
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

# Google Kubernetes Engine (GKE)

> **云厂商**: Google Cloud (GCP)

## 产品概述

Google Kubernetes Engine (GKE) 是 Google Cloud Platform 提供的托管 Kubernetes 服务，基于 Google 多年运行容器化工作负载的经验构建，为企业提供安全、可靠且高度可扩展的容器编排平台。

## 核心架构

详见源文档获取完整产品架构信息。^[inferred]


## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[entities/networkpolicy.md|networkpolicy]]

## Related

- [[ecloud-cke-overview]] — ecloud-cke-overview
- [[250-apsara-stack-ess-scaling]] — 250-apsara-stack-ess-scaling
- [[ibm-iks-overview]] — IBM Cloud Kubernetes Service
- [[ctyun-tke-overview]] — ctyun-tke-overview
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
