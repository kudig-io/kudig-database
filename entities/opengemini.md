---
title: openGemini (entities)
description: '## 概述'
summary: 'openGemini 是一个高性能、分布式时序数据库，专为物联网 (IoT)、可观测性和工业互联网场景设计。它基于 InfluxDB 协议兼容，提供高速写入、低延迟查询和高效压缩，支持每秒千万级数据点的写入和 PB 级数据存储。openGemini 采用存算分离架构，可独立扩展计算和存储资源。'
category: entities
tags:
- k8s
- cncf
- database
- opengemini
- coredns
- flux
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- openGemini 是什么
- 如何 openGemini
trigger_keywords:
- openGemini
prerequisites:
- kubectl-basics
---



# openGemini

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

openGemini 是一个高性能、分布式时序数据库，专为物联网 (IoT)、可观测性和工业互联网场景设计。它基于 InfluxDB 协议兼容，提供高速写入、低延迟查询和高效压缩，支持每秒千万级数据点的写入和 PB 级数据存储。openGemini 采用存算分离架构，可独立扩展计算和存储资源。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **合理分片**: 根据数据量设置合适的 Shard Duration，避免单 Shard 过大
- **标签设计**: Tag 用于高基数维度，Field 用于数值，避免高基数 Tag
- **保留策略**: 为不同精度的数据设置不同保留策略，自动降采样
- **批量写入**: 使用批量写入而非单点写入，提高写入效率
- **查询优化**: 查询时指定时间范围和 Tag 过滤，避免全表扫描

## 架构定位

在 CNCF 生态中，opengemini 属于 **Database** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opengemini
- [[entities/oxia.md|Oxia]]
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
