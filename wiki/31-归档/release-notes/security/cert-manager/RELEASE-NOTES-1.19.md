---
title: rook v1.19 Release Notes
description: rook v1.19 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.19 Release Notes 是什么
- 如何 rook v1.19 Release Notes
trigger_keywords:
- rook
- v1.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Rook|rook]] v1.19 Release Notes

Source: [v1.19.3](https://github.com/rook/rook/releases/tag/v1.19.3)

# Improvements
Rook v1.19.3 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- csi: Use ceph-csi-operator to deploy Ceph-CSI/NVMe-oF (#17154, @nixpanic)
- csi: Update ceph-csi image to v3.16.2 (#17184, @black-dragon74)
- csi: Update CSI sidecars to latest versions available (#17119, @iPraveenParihar)
- pool: Clean up erasure code profile on pool deletion (#17208, @OdedViner)
- pool: Set EC pool status to ready after reconcile (#17200, @OdedViner)
- pool: Skip mirroring if the data pool is erasure-coded (#17143, @parth-gr)
- exporter: Delete orphaned ceph-exporter [[Deployments|deployments]] on reconcile (#17165, @adilGhaffarDev)
- exporter: Reconcile as best effort during deletion and ensure all clusters reconciled (#17164, @travisn)
- exporter: Add configurable port for ceph exporter (#17116, @OdedViner)
- rgw: Create correct IPv6 formatted secret for object store users (#17161, @parth-gr)
- [[Helm|helm]]: Allow annotations and labels for CephCluster (#17046, @sathieu)
- osd: Check devlinks while cleaning osd disks (#17123, @sp98)
- osd: Update lockbox key rotation for encrypted OSDs (#17112, @BlaineEXE)
- osd: Set device-type label on update (#17113, @satoru-takeuchi)
- rgw: Support new RGW pools in shared pools zone json config (#17102, @arttor)
- rgw: ObjectStore controller to wait until zone and sharedPools are reconciled (#17101, @arttor)


<!-- risk-assessed -->
