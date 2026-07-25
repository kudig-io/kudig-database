---
title: 云存储对比索引
description: 各云厂商存储服务对比 — AWS EBS/S3、GCP PD/GCS、Azure Disk/Blob、阿里云 ESSD/OSS
summary: 云存储对比子目录，涵盖各云厂商块存储/对象存储/文件存储的性能、成本、CSI 驱动对比与选型
category: index
tags:
- index
- storage
- cloud-providers
- comparison
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
---

# 云存储对比

> 各云厂商存储服务的性能、成本与 K8s 集成对比。

## 文档

| 文件 | 内容 |
|------|------|
| [[06-存储/06-云存储对比/01-cloud-storage-comparison.md\|云存储全面对比]] | AWS/Azure/GCP/阿里云块存储/文件存储/对象存储对比、CSI 集成、选型决策 |

## 对比维度

| 维度 | 说明 |
|------|------|
| 性能 | IOPS、吞吐量、延迟（P99） |
| 成本 | 按量/包年、快照费用、传输费用 |
| 可靠性 | SLA、副本策略、跨 AZ/Region |
| K8s 集成 | CSI 驱动成熟度、动态 Provisioning |
| 生态 | 备份工具、监控集成、数据迁移 |
| 限制 | 最大卷大小、IOPS 上限、挂载数 |

## Related

- [[06-存储/01-K8s存储/index.md|K8s 存储]] — PV/PVC/CSI 核心概念
- [[06-存储/07-AI存储与高级/index.md|AI 存储与高级]] — 云 CSI 驱动详解
- [[18-云厂商/README.md|云厂商知识域]] — 各云 K8s 服务对比
- [[27-标签/storage|storage 标签枢纽]]
