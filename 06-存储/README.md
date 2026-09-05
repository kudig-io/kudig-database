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
| [[06-存储/01-K8s存储/index.md\|K8s存储/]] | K8s 存储原语 | PV/PVC/StorageClass/CSI/快照/加密 |
| [[06-存储/02-存储基础/index.md\|存储基础/]] | 存储技术基础 | 块/文件/对象存储、RAID、IOPS |
| [[06-存储/03-分布式存储/index.md\|分布式存储/]] | 分布式存储系统 | Ceph/Longhorn/OpenEBS/JuiceFS/NFS |
| [[06-存储/04-有状态应用存储/index.md\|有状态应用存储/]] | 有状态应用 | MySQL/PostgreSQL/Kafka/Redis StatefulSet |
| [[06-存储/07-AI存储与高级/index.md\|AI存储与高级/]] | AI 存储与高级 | MinIO/WEKA/Lustre/数据分层/混沌工程 |
| [[06-存储/05-存储网络/index.md\|存储网络/]] | 存储网络架构 | SAN/NAS/iSCSI/FC/NFS/NVMe-oF |
| [[06-存储/06-云存储对比/index.md\|云存储对比/]] | 云存储对比 | AWS/GCP/Azure/阿里云存储服务对比 |

## 跨域导航

- [[15-AI基础设施/README.md|AI基础设施]]
- [[16-专项技术/README.md|专项技术]]
- [[18-云厂商/README.md|云厂商]]
- [[11-发布变更/README.md|发布变更]]
- [[09-可观测性/README.md|可观测性]]
- [[12-可靠性/README.md|可靠性]]
- [[08-安全/README.md|安全]]
- [[14-容器运行时/README.md|容器运行时]]
- [[02-工作负载/README.md|工作负载]]
- [[10-平台工程/README.md|平台工程]]
- [[04-应用模式/README.md|应用模式]]
- [[19-故障诊断/README.md|故障诊断]]
- [[07-数据库中间件/README.md|数据库中间件]]
- [[03-清单模式/README.md|清单模式]]
- [[13-生产运维/README.md|生产运维]]
- [[21-生态参考/README.md|生态参考]]
- [[17-系统基础/README.md|系统基础]]
- [[05-网络/README.md|网络]]
- [[01-集群基础/README.md|集群基础]]

## Related

- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]
