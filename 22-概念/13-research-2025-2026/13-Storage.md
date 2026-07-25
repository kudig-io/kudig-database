---
title: 'Research: Kubernetes 存储深度研究 2025-2026'
summary: 3 轮深度研究覆盖 K8S 存储全栈：云原生存储系统对比（Longhorn/Rook-Ceph/OpenEBS/JuiceFS）、 CSI 高级模式与最新进展、存储性能优化、数据保护与灾难恢复。
category: synthesis
tags:
- storage
- k8s
- research
- csi
- longhorn
- rook
- ceph
- velero
- backup
tier: supporting
sources:
- https://kubernetes.io/docs/concepts/storage/persistent-volumes/
- https://kubernetes.io/docs/concepts/storage/volume-snapshots/
- https://kubernetes-csi.github.io/docs/
- https://longhorn.io/docs/
- https://rook.io/docs/
- https://velero.io/docs/
- https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/3751-volume-attributes-class
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
provenance:
  extracted: 0.7
  inferred: 0.25
  ambiguous: 0.05
base_confidence: 0.85
lifecycle: draft
lifecycle_changed: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Research: Kubernetes 存储深度研究 2025-2026

## 概述

本报告是 kudig-database 存储域（domain-04）的系统性深度研究，覆盖 5 个研究角度：
云原生存储系统、CSI 高级模式、存储性能优化、多云存储策略、数据保护与灾难恢复。
研究发现 K8S 存储生态在 2025-2026 年进入成熟期，CSI 迁移全面完成，
VolumeAttributesClass 等新特性正在补齐存储 QoS 短板。

## 核心发现

1. **CSI 迁移全面完成** — 所有主要 in-tree 驱动已迁移至 CSI 并 GA，feature gate 已移除。
   CSI 是 K8S 存储的唯一路径。^[kubernetes.io docs]

2. **VolumeAttributesClass GA（v1.34）** — 首次为 K8S 提供原生存储 QoS 能力，
   支持 per-PVC 的 IOPS/吞吐量限制，无需重建卷即可修改。^[KEP-3751]

3. **Volume Group Snapshots GA（v1.36）** — 支持对一组卷做一致性快照，
   对分布式数据库场景至关重要。^[kubernetes.io blog]

4. **云原生存储四强格局**：
   - **Longhorn**：最简单，适合中小规模通用工作负载
   - **Rook/Ceph**：最强大，企业级首选，需专业团队
   - **OpenEBS Mayastor**：NVMe-oF 最低延迟，仍在成熟
   - **JuiceFS**：AI/ML 共享数据集最佳选择^[k8s-cloud-native-storage-2025.md]

5. **Velero 仍是 K8S 备份事实标准** — v1.14-1.15 支持 CSI 快照优先、
   不可变备份（S3 Object Lock）、勒索软件防护。^[velero.io docs]

6. **存储 QoS 从无到有** — 除 VolumeAttributesClass 外，Portworx/Trident/Ceph-CSI
   已在 CSI 层面提供 IOPS 限制能力，Linux blkio cgroup 可做容器级强制。^[研究综合]

## 核心概念

- [[22-概念/04-存储/csi-drivers.md|CSI 驱动]] — Container Storage Interface 规范、迁移状态、核心能力全景
- [[22-概念/04-存储/cloud-native-storage-systems.md|云原生存储系统对比]] — Longhorn/Rook-Ceph/OpenEBS/JuiceFS 架构与选型
- [[22-概念/04-存储/storage-performance-optimization.md|存储性能优化]] — 基准测试、NVMe 调优、存储 QoS
- [[22-概念/04-存储/storage-data-protection.md|存储数据保护与灾难恢复]] — Velero 最佳实践、不可变备份、DR 策略
- [[22-概念/12-研究/storage-tool-evolution.md|存储工具演进]] — Rook/Longhorn/Velero 版本演进

## 实体与工具

| 工具 | 定位 | 最新版本 |
|------|------|---------|
| Longhorn | 轻量分布式块存储 | v1.7.x |
| Rook/Ceph | 企业级分布式存储 Operator | v1.14+ / Ceph Reef/Squid |
| OpenEBS | NVMe-oF 块存储 | v4.0+ (Mayastor) |
| JuiceFS | 分布式 POSIX 文件系统 | v1.2.x |
| Velero | K8S 备份与灾难恢复 | v1.14-1.15 |
| Portworx | 多云存储平台 | latest |
| NetApp Trident | ONTAP CSI 驱动 | latest |

## 矛盾与开放问题

1. **Longhorn vs OpenEBS Mayastor 性能** — Longhorn 基于 iSCSI 有 10-15% 开销，
   Mayastor 基于 NVMe-oF 延迟更低，但 Mayastor 成熟度不足。
   生产选择需权衡稳定性 vs 性能。

2. **存储 QoS 标准化进度** — VolumeAttributesClass 已 GA，但实际 CSI 驱动支持度参差不齐。
   需跟踪各驱动的适配进度。

3. **多云存储联邦成本** — 跨云存储复制的出口成本是主要障碍，
   同步复制受 50-200ms 延迟限制。异步复制 + 元数据联邦是更现实的方案。

## 来源页面

- [[22-概念/04-存储/csi-drivers.md|CSI 驱动]] — Kubernetes 官方文档、kubernetes-csi.github.io
- [[22-概念/04-存储/cloud-native-storage-systems.md|云原生存储系统对比]] — Longhorn/Rook/OpenEBS/JuiceFS 官方文档
- [[22-概念/04-存储/storage-performance-optimization.md|存储性能优化]] — K8S 官方文档、社区基准测试
- [[22-概念/04-存储/storage-data-protection.md|存储数据保护与灾难恢复]] — Velero 官方文档、AWS S3 文档

## 研究统计

| 指标 | 值 |
|------|-----|
| 研究轮次 | 3 |
| 搜索查询 | 10 |
| 抓取页面 | 8+ |
| 创建概念页 | 4 |
| 更新概念页 | 1 |
| 创建合成页 | 1 |

---

## 跨域关联

- [[22-概念/05-安全/k8s-security-compliance.md|k8s security compliance]] — 存储加密（etcd 加密、PV 加密）与零信任安全架构的深度耦合
- [[22-概念/12-研究/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — AI/ML 工作负载对高性能存储（GPUDirect Storage、NVMe-oF）的需求驱动存储架构变革
- [[22-概念/08-可靠性与运维/finops-greenops-practices.md|finops greenops practices]] — 存储成本优化（分层存储、数据生命周期管理）是 FinOps 实践的关键组成部分
- [[22-概念/08-可靠性与运维/multi-cluster-dr-automation.md|multi cluster dr automation]] — 跨集群存储复制与灾难恢复策略（Velero、Kasten）保障业务连续性

## Related

- research/ — tag hub


<!-- risk-assessed -->
