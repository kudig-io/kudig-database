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
last_updated: 2026-07
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# openGemini

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

OpenGemini 是一个 CNCF 沙箱项目，由华为开源，是一个高性能的分布式时间序列数据库（TSDB）。它专为 IoT、监控、运维数据分析等大规模时间序列数据场景设计，提供高写入吞吐、低查询延迟和高效的数据压缩。OpenGemini 采用存算分离架构，支持水平扩展和云原生部署。与 InfluxDB 兼容的写入/查询 API，方便迁移。

## Key Features（核心能力）

- **高性能写入**：单节点支持百万级 TPS 写入
- **存算分离**：计算节点和存储节点独立扩展，灵活适应不同负载
- **InfluxDB 协议兼容**：兼容 InfluxDB v2 写入和查询 API
- **SQL+Flux 查询**：支持 SQL 和 Flux 两种查询语言
- **多维标签索引**：高效的倒排索引支持多维度标签查询
- **数据降采样**：自动数据下采样（Downsampling）管理长期数据

## 架构与工作原理

OpenGemini 采用三节点分离架构：SQL Node（ts-sql）处理查询请求和 SQL 解析；Store Node（ts-store）负责数据写入、存储和索引；Meta Node（ts-meta）管理集群元数据和分区路由。数据按时间分片（Shard）存储，通过 Raft 协议保证元数据一致性。存储引擎基于 LSM-Tree，结合列式存储和倒排索引实现高效的时间序列查询。

## K8s 集成

OpenGemini 可通过 Helm Chart 部署到 Kubernetes 集群。ts-sql 以 Deployment 部署，通过 Service 暴露查询接口；ts-store 以 StatefulSet 部署，使用 PVC 提供持久化存储；ts-meta 以 3 节点 StatefulSet 部署保证高可用。与 Prometheus Remote Read/Write 集成，作为长期监控数据存储后端。通过 HPA 根据查询负载自动扩展 ts-sql 节点。

## 生产用例

- **监控数据存储**：作为 Prometheus 的远程存储后端
- **IoT 数据平台**：海量 IoT 设备的时间序列数据采集和查询
- **运维数据分析**：大规模 IT 基础设施的性能和日志数据分析
- **金融数据分析**：股票行情、交易指标等时间序列数据管理

## 安装与快速开始

```bash
helm repo add opengemini https://opengemini.github.io/opengemini-helm
helm install opengemini opengemini/opengemini -n database --create-namespace
```

## 对比替代方案

相比 InfluxDB，OpenGemini 提供更好的水平扩展能力和存算分离架构。相比 TimescaleDB（PostgreSQL 扩展），OpenGemini 是原生 TSDB，性能更高。相比 VictoriaMetrics，OpenGemini 功能更丰富但社区较新。

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opengemini
- [[实体/oxia.md|Oxia]]
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
