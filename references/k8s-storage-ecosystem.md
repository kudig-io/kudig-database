---
title: 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
description: '# 存储体系'
category: reference
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- backup
- etcd
- scheduler
- ceph
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复 是什么
- 如何 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
trigger_keywords:
- 存储体系：PV
- PVC
- StorageClass
- CSI
- 驱动与灾备恢复
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

# 存储体系

## PV/PVC 绑定机制

- **PV（PersistentVolume）**：集群级存储资源
- **PVC（PersistentVolumeClaim）**：用户对存储的请求
- 绑定条件：accessModes + storageClassName + capacity 匹配

回收策略：Retain（保留）、Delete（删除）、Recycle（已废弃）。

## StorageClass 动态供给

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp3
  iops: "3000"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
```

## CSI 驱动架构

CSI（Container Storage Interface）三组件：
- **External Provisioner**：创建/删除存储卷
- **External Attacher**：挂载/卸载存储卷到节点
- **Node Driver**：节点级别挂载操作

主流驱动：AWS EBS CSI、Azure Disk CSI、GCE PD CSI、Ceph RBD CSI、NFS CSI。

## 灾备恢复（Velero）

Velero 备份策略：
- **集群资源备份**：所有 K8s 对象（etcd 快照）
- **PV 数据备份**：VolumeSnapshot 或 restic 文件级备份
- **跨集群恢复**：支持不同集群间恢复

---

> 来源：.zread/wiki/drafts/10-cun-chu-ti-xi-pv-pvc-storageclass-csi-qu-dong-yu-zai-bei-hui-fu.md

## Related

- [[references/k8s-control-plane-deep-dive.md|k8s-control-plane-deep-dive]] — 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[synthesis/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]