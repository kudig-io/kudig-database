---
title: ctyun-tke-overview
description: '## 产品概述'
category: references
tags:
- k8s
- cloud
- managed-k8s
- tke
- ctyun
- etcd
- networkpolicy
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ctyun-tke-overview 是什么
- 如何 ctyun-tke-overview
trigger_keywords:
- ctyun-tke-overview
prerequisites:
- kubectl-basics
- etcd-basics
---

# ctyun-tke-overview

> **云厂商**: 腾讯云容器服务 TKE

## 产品概述

天翼云Kubernetes引擎是天翼云提供的企业级托管容器服务，基于中国电信强大的网络基础设施和多年的电信级运维经验，为政企客户提供高性能、高安全、高可靠的容器化应用部署和管理解决方案。天翼云TKE深度融合了5G网络、边缘计算等电信特色能力，特别适合对网络性能、安全合规有严格要求的行业客户。

## 核心架构

详见源文档获取完整产品架构信息。^[inferred]


## K8s 托管服务特性

作为托管 Kubernetes 服务，该产品提供控制平面自动运维、节点自动伸缩、集成监控告警等能力，降低用户运维成本。各云厂商在控制平面管理、网络方案、存储集成等方面各有特色。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[operator-pattern]]

## Related

- [[245-ack-ebs-storage]] — EBS (ESSD)
- [[ecloud-cke-overview]] — ecloud-cke-overview
- [[250-apsara-stack-ess-scaling]] — 250-apsara-stack-ess-scaling
- [[ibm-iks-overview]] — IBM Cloud Kubernetes Service
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
