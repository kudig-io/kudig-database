---
title: 存储网络索引
description: 存储网络知识 — SAN/NAS/iSCSI/FC/NFS 协议、存储网络架构、K8s 存储网络集成
summary: 存储网络子目录，涵盖 SAN/NAS 架构、iSCSI/FC/NFS 协议对比、存储网络设计、K8s CSI 与存储网络集成
category: index
tags:
- index
- storage
- networking
- san
- nas
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
---

# 存储网络

> 存储网络架构与协议 — SAN/NAS/iSCSI/FC/NFS 及与 Kubernetes 的集成。

## 文档

| 文件 | 内容 |
|------|------|
| [[存储/存储网络/01-san-nas-architecture.md\|SAN/NAS 架构]] | SAN/NAS 架构对比、iSCSI/FC 协议、K8s CSI 集成、多路径配置 |

## 核心知识

| 主题 | 说明 |
|------|------|
| SAN vs NAS | 块存储 vs 文件存储的架构差异与适用场景 |
| iSCSI | IP 网络上的 SCSI 协议，低成本块存储 |
| Fibre Channel | 高性能专用存储网络，低延迟高吞吐 |
| NFS | 网络文件系统，K8s 中最常用的文件存储协议 |
| NVMe-oF | NVMe over Fabrics，新一代高性能存储网络 |
| K8s 集成 | CSI 驱动对接存储网络的最佳实践 |

## Related

- [[存储/K8s存储/index.md|K8s 存储]] — PV/PVC/CSI 核心概念
- [[存储/分布式存储/index.md|分布式存储]] — Ceph/Longhorn/OpenEBS
- [[标签/storage|storage 标签枢纽]]
