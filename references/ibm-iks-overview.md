---
title: IBM Cloud Kubernetes Service
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- ibm
- prometheus
- grafana
- networkpolicy
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- IBM Cloud Kubernetes Service 是什么
- 如何 IBM Cloud Kubernetes Service
trigger_keywords:
- IBM
- Cloud
- Kubernetes
- Service
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

# IBM Cloud Kubernetes Service

> **云厂商**: IBM IKS

## 产品概述

IBM Cloud Kubernetes Service 是 IBM 提供的企业级托管 Kubernetes 服务，结合了 IBM 在企业IT领域的深厚积累和开源 Kubernetes 的灵活性。IKS 专为需要企业级安全、合规性和多云管理能力的大型组织设计，特别适合金融、医疗、政府等对安全性要求极高的行业。

## 核心架构

详见源文档获取完整产品架构信息。^[inferred]


## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[concepts/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[252-apsara-stack-pop-operations]] — 252-apsara-stack-pop-operations
- [[245-ack-ebs-storage]] — EBS (ESSD)
- [[ecloud-cke-overview]] — ecloud-cke-overview
- [[250-apsara-stack-ess-scaling]] — 250-apsara-stack-ess-scaling
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
