---
title: 存储工具演进
description: '# 存储工具演进'
category: concepts
tags:
- k8s
- release-notes
- rook
- longhorn
- velero
- storage
- csi
- backup
- prometheus
- ceph
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储工具演进 是什么
- 如何 存储工具演进
trigger_keywords:
- 存储工具演进
prerequisites:
- kubectl-basics
- prometheus-basics
- backup-basics
---

# 存储工具演进

> 本文档综合了 `domain-19-landscape-references/topic-release-notes/storage/` 目录下 Rook、Longhorn 和 Velero 三大存储/备份工具的 76 个版本发布说明 ^[inferred]

## 组件概览

| 组件 | 版本范围 | 定位 |
|---|---|---|
| Rook | 29 个版本 | 编排存储服务的 Kubernetes Operator |
| Longhorn | 19 个版本 | 云原生分布式块存储 |
| Velero | 28 个版本 | Kubernetes 备份和迁移工具 |

## Rook 版本演进

Rook 将分布式存储系统（Ceph 等）编排到 Kubernetes 原生环境中。

### 核心能力

- Ceph 集群的自动化部署和管理
- 提供块存储（RBD）、文件系统（CephFS）、对象存储（RGW）
- CSI 驱动集成
- 存储池管理
- 健康监控和自愈 ^[inferred]

### 演进方向

- 更好的多集群支持
- 改进的升级流程
- 增强的监控和告警
- 性能优化 ^[inferred]

## Longhorn 版本演进

Longhorn 是 Rancher 开发的轻量级、高可用的 Kubernetes 分布式块存储系统。

### 核心特点

- 基于微服务的存储架构
- 每卷独立副本
- 增量快照
- 自动故障恢复
- CSI 兼容 ^[inferred]

## Velero 版本演进

Velero（前身为 Heptio Ark）是 Kubernetes 备份和灾难恢复工具。

### v1.0 - 里程碑版本

这是 Velero 的重要版本，从 Ark 品牌完全迁移：

**亮点功能：**
- 新增 `velero install` 命令，简化安装
- 插件框架改进：
  - 减少插件作者的导入开销
  - 所有插件包裹 panic handler
  - 传递 `--log-level` 给插件
  - 插件错误包含文件/行位置
  - RestoreItemAction 可返回相关额外项目
  - RestoreItemAction 可跳过特定项目恢复
- Azure 安装支持 .env 文件配置凭证
- 新增 `PartiallyFailed` 阶段（备份/恢复部分成功）
- 移除所有遗留 Ark 标识（API 类型、Prometheus 指标、注解等）

**破坏性变更：**
- 移除 Ark API group（ark.heptio.com）
- 移除 Ark 注解，替换为 Velero 注解
- Ark Prometheus 指标替换为 Velero 指标
- BlockStore 插件重命名为 VolumeSnapshotter
- 插件必须使用 `example.domain.com/plugin-name` 命名格式
- 基础镜像切换为 `ubuntu:bionic`
- 对 Azure/AWS/GCP 配置执行严格验证

### 后续演进

- 改进的备份策略
- CSI 快照集成
- 集群迁移支持
- 多集群备份 ^[inferred]

## Velero 核心概念

| 概念 | 说明 |
|---|---|
| Backup | 集群资源的备份 |
| Restore | 从备份恢复资源 |
| Schedule | 定时备份计划 |
| Plugin | 扩展存储后端和备份行为 |
| VolumeSnapshotter | 持久卷快照插件（原 BlockStore） |

## 存储方案选择

| 需求 | 推荐方案 |
|---|---|
| 企业级 Ceph 存储 | Rook |
| 简单块存储 | Longhorn |
| 备份与灾难恢复 | Velero |
| 完整存储方案 | Rook + Velero |

## 来源文档

- domain-19-landscape-references/topic-release-notes/storage/rook/（29 个文件）
- domain-19-landscape-references/topic-release-notes/storage/longhorn/（19 个文件）
- domain-19-landscape-references/topic-release-notes/storage/velero/（28 个文件）

## Related

- [[concepts/observability-stack-evolution.md|observability-stack-evolution]] — 可观测性栈演进
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[rook]] — Rook
- [[longhorn]] — Longhorn
