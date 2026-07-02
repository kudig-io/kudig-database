---
title: CNCF 存储与数据库项目全景
description: '# CNCF 存储与数据库项目全景'
summary: '云原生存储分为 **块/文件/对象存储**、**分布式数据库**、**数据分发与缓存** 三大类。CNCF 存储项目解决 K8s 有状态工作负载的持久化、备份和高性能数据访问需求。'
category: entities
tags:
- k8s
- cncf
- storage
- database
- distributed
- data
- containerd
- docker
- harbor
- rook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 存储与数据库项目全景 是什么
- 如何 CNCF 存储与数据库项目全景
trigger_keywords:
- CNCF
- 存储与数据库项目全景
prerequisites:
- kubectl-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF 存储与数据库项目全景

> 聚合页面 | 涵盖 19 个 CNCF 存储与数据项目

## 概述

云原生存储分为 **块/文件/对象存储**、**分布式数据库**、**数据分发与缓存** 三大类。CNCF 存储项目解决 K8s 有状态工作负载的持久化、备份和高性能数据访问需求。

---

## 云原生存储平台

### [[rook]] — 毕业项目

Rook 是 K8s 存储编排器，将 Ceph 等存储系统转化为云原生服务。

- 自动化 Ceph 集群部署和管理
- 提供块存储（RBD）、文件存储（CephFS）、对象存储（RGW）
- 存储池、快照、克隆、扩容的声明式管理

### [[longhorn]] — 孵化项目

Longhorn 是轻量级分布式块存储系统。

- 精简配置（thin provisioning）
- 增量快照和备份
- 跨节点复制，自动故障恢复
- 适合中小规模集群

### [[openebs]] — 沙箱项目

OpenEBS 提供多种存储引擎的 K8s 原生存储。

- **Jiva**: 适用于轻量级工作负载
- **cStor**: 企业级存储引擎
- **Mayastor**: 高性能 NVMe-oF 存储
- **LocalPV**: 本地存储管理

### [[hwameistor]] — 沙箱项目

HwameiStor 是高可用本地存储系统。

### [[carina]] — 沙箱项目

Carina 是基于 LVM 的本地存储管理方案。

### [[piraeus-datastore]] — 沙箱项目

Piraeus Datastore 在 K8s 上部署 LINSTOR SDS。

### [[cubefs]] — 毕业项目

CubeFS 是云原生分布式文件系统和对象存储。

- 支持 POSIX 兼容的文件接口
- S3 兼容的对象存储
- 多租户隔离

---

## 分布式数据库

### [[vitess]] — 毕业项目

Vitess 是 MySQL 水平扩展中间件。

- 数据分片（sharding）自动管理
- 在线 DDL 变更
- 连接池和查询路由
- YouTube 生产验证

### [[tikv]] — 毕业项目

TiKV 是分布式事务键值存储。

- Raft 共识协议保证一致性
- 分布式事务支持（Percolator 模型）
- 与 TiDB 搭配使用
- 水平弹性扩缩容

### [[cloudnativepg]] — 沙箱项目

CloudNativePG 是 K8s 原生的 PostgreSQL Operator。

### [[schemahero]] — 沙箱项目

SchemaHero 声明式管理数据库 Schema 变更。

### [[opengemini]] — 沙箱项目

OpenGemini 是开源时序数据库。

---

## 数据分发与加速

### [[dragonfly]] — 毕业项目

Dragonfly 是 P2P 镜像分发系统。

- 基于 P2P 的大规模镜像分发
- 减少镜像仓库带宽压力
- 与 Harbor 和 containerd 集成

### [[fluid]] — 孵化项目

Fluid 是云原生数据集编排和加速引擎。

- 缓存加速大数据/AI 训练数据
- 数据集弹性伸缩
- 与 JuiceFS、Alluxio 等集成

### [[vineyard]] — 沙箱项目

Vineyard 是内存中数据共享中间件。

---

## 容器镜像管理

### [[harbor]] — 毕业项目

Harbor 是企业级容器镜像仓库。

- 镜像漏洞扫描（[[Trivy|Trivy]] 集成）
- 镜像签名和内容信任
- RBAC 权限管理
- 跨仓库复制
- 垃圾回收和存储配额

### [[zot]] — 沙箱项目

Zot 是 OCI 原生的容器镜像仓库（纯 OCI，无 Docker 特定依赖）。

### [[distribution]] — 沙箱项目

Distribution 是 OCI 分发规范参考实现（原 Docker Registry）。

### [[ORAS]] — 沙箱项目

oras（OCI Registry As Storage）推送任意 OCI 制品到镜像仓库。

---

## 备份与恢复

### [[k8up]] — 沙箱项目

K8up 是 K8s 备份调度器，支持 Restic 后端。

### [[kanister]] — 沙箱项目

Kanister 提供应用级数据保护框架。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 通用块存储 | Rook（Ceph）或 Longhorn |
| 高性能本地存储 | OpenEBS Mayastor 或 HwameiStor |
| MySQL 水平扩展 | Vitess |
| 分布式 KV/事务 | TiKV + TiDB |
| 企业镜像仓库 | Harbor |
| P2P 镜像分发 | Dragonfly |
| AI 数据加速 | Fluid |
| PostgreSQL K8s 管理 | CloudNativePG |

---

## 相关页面

- [[entities/cncf-observability.md|cncf-observability]] — 可观测性
- [[entities/cncf-security.md|cncf-security]] — 安全与合规
- [[entities/cncf-networking.md|cncf-networking]] — 网络与服务网格
- [[concepts/block-file-object-storage.md|block-file-object-storage]] — 存储类型概念

## Related

- [[docker]] — Docker
- [[containerd]] — containerd
- [[entities/trivy.md|trivy]] — Trivy
- [[harbor]] — Harbor
- [[piraeus-datastore]] — Piraeus Datastore

- [[entities/kanister.md|Kanister]]
- [[entities/oxia.md|Oxia]]
- [[entities/opengemini.md|openGemini]]
- [[entities/schemahero.md|SchemaHero]]
- [[entities/vineyard.md|Vineyard]]

<!-- risk-assessed -->
