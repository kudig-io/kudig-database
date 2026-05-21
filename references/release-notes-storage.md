---
title: 发布说明索引 — 存储
description: '# 发布说明索引 — 存储'
category: references
tags:
- k8s
- release-notes
- storage
- longhorn
- rook
- velero
- ceph
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 存储 是什么
- 如何 发布说明索引 — 存储
trigger_keywords:
- 发布说明索引
- 存储
prerequisites:
- kubectl-basics
- backup-basics
---

# 发布说明索引 — 存储

> 本文档汇总存储领域 3 个核心项目的发布说明索引，共覆盖 **76 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Longhorn | 19 | v1.11 | v1.7 | 云原生块存储 |
| Rook | 29 | v1.19 | v1.4 | Ceph 存储编排 |
| Velero | 28 | v1.18 | v1.18 | 备份与灾难恢复 |

---

## 项目详情

### Longhorn

- **实体页面**: [[longhorn|Longhorn]]
- **最新版本**: v1.11
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/storage/longhorn/`
- **版本覆盖**: v0.1 → v1.11（19 个版本）
- **Breaking Changes 提醒**:
  - v1.7: 存储引擎和快照格式变更
- **升级要点**: v1.x 引入数据引擎 v2（基于 SPDK）和备份增强

### Rook

- **实体页面**: [[rook|Rook]]
- **最新版本**: v1.19
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/storage/rook/`
- **版本覆盖**: v0.1 → v1.19（29 个版本）
- **Breaking Changes 提醒**:
  - v1.4: CRD API 版本升级和集群配置格式变更
- **升级要点**: Ceph 集群全生命周期管理，支持 Ceph Reef+

### Velero

- **实体页面**: Velero
- **最新版本**: v1.18
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/storage/velero/`
- **版本覆盖**: v0.1 → v1.18（28 个版本）
- **Breaking Changes 提醒**:
  - v1.18: 备份存储位置 API 和插件接口变更
- **升级要点**: v1.x 支持 CSI 快照备份和数据移动器

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v1.7 | Longhorn | 存储引擎和快照格式变更 |
| v1.4 | Rook | CRD API 版本和集群配置格式变更 |
| v1.18 | Velero | 备份存储位置 API 和插件接口变更 |

---

## 相关导航

- [[concepts/storage-tool-evolution.md|存储工具演进]]
- [[references/release-notes-reading-guide.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[references/release-notes-networking.md|release-notes-networking]] — 发布说明索引 — 网络
- [[references/k8s-storage-ecosystem.md|k8s-storage-ecosystem]] — 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
- [[references/release-notes-observability.md|release-notes-observability]] — 发布说明索引 — 可观测性
- [[rook]] — Rook
- [[longhorn]] — Longhorn
