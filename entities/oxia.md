---
title: Oxia
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- database
- oxia
- etcd
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Oxia 是什么
- 如何 Oxia
trigger_keywords:
- Oxia
prerequisites:
- kubectl-basics
- etcd-basics
---

# Oxia

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

Oxia 是一个可水平扩展的元数据存储和协调系统，旨在作为 ZooKeeper 和 etcd 的高可扩展性替代方案。它由 StreamNative 开发，最初用于解决 Apache Pulsar 在大规模场景下对 ZooKeeper 的扩展性瓶颈。Oxia 通过分片架构将数据分布到多个节点，支持百万级 Key 的元数据管理，同时提供与 ZooKeeper 兼容的通知和协调原语。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **分片数量**: 根据数据量和写入吞吐设置分片数，通常 8-64 个分片
- **副本因子**: 生产环境使用 3 副本保证数据安全
- **Key 设计**: 使用层级式 Key (如 `/service/config/key`) 便于范围查询
- **连接池**: 客户端复用连接，避免频繁建立 gRPC 连接
- **渐进迁移**: 先部署 ZK 代理运行已有应用，再逐步迁移到原生 Oxia SDK

## 架构定位

在 CNCF 生态中，oxia 属于 **Database** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]

## Related

- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC
- [[etcd]] — etcd

- [[domain-19-landscape-references/sandbox/oxia/oxia.md|oxia]]
- [[entities/schemahero.md|SchemaHero]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
