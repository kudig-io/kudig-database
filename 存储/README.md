---
title: Storage & Data
description: Kubernetes 存储知识域 — K8s 存储原语、存储基础、分布式存储、存储网络、云存储对比、有状态应用存储
summary: 存储知识域入口，涵盖 PV/PVC/CSI 核心概念、SAN/NAS 存储网络、分布式存储、云存储对比、AI 存储与有状态应用实践
category: domain
tags:
- storage
- pv
- pvc
- csi
- distributed-storage
- san
- nas
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
---
# 存储 Storage

> K8s 存储原语、存储基础、分布式存储、存储网络与有状态应用存储。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[存储/K8s存储/index.md\|K8s存储/]] | K8s 存储原语 | PV/PVC/StorageClass/CSI/快照/加密 |
| [[存储/存储基础/index.md\|存储基础/]] | 存储技术基础 | 块/文件/对象存储、RAID、IOPS |
| [[存储/分布式存储/index.md\|分布式存储/]] | 分布式存储系统 | Ceph/Longhorn/OpenEBS/JuiceFS/NFS |
| [[存储/有状态应用存储/index.md\|有状态应用存储/]] | 有状态应用 | MySQL/PostgreSQL/Kafka/Redis StatefulSet |
| [[存储/AI存储与高级/index.md\|AI存储与高级/]] | AI 存储与高级 | MinIO/WEKA/Lustre/数据分层/混沌工程 |
| [[存储/存储网络/index.md\|存储网络/]] | 存储网络架构 | SAN/NAS/iSCSI/FC/NFS/NVMe-oF |
| [[存储/云存储对比/index.md\|云存储对比/]] | 云存储对比 | AWS/GCP/Azure/阿里云存储服务对比 |

## 跨域导航

- [[AI基础设施/README.md|AI基础设施]]
- [[专项技术/README.md|专项技术]]
- [[云厂商/README.md|云厂商]]
- [[发布变更/README.md|发布变更]]
- [[可观测性/README.md|可观测性]]
- [[可靠性/README.md|可靠性]]
- [[安全/README.md|安全]]
- [[容器运行时/README.md|容器运行时]]
- [[工作负载/README.md|工作负载]]
- [[平台工程/README.md|平台工程]]
- [[应用模式/README.md|应用模式]]
- [[故障诊断/README.md|故障诊断]]
- [[数据库中间件/README.md|数据库中间件]]
- [[清单模式/README.md|清单模式]]
- [[生产运维/README.md|生产运维]]
- [[生态参考/README.md|生态参考]]
- [[系统基础/README.md|系统基础]]
- [[网络/README.md|网络]]
- [[集群基础/README.md|集群基础]]
