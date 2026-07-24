---
title: CSI Drivers
description: CSI Drivers — Kubernetes 生产运维知识库
summary: CSI Drivers — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- csi
- storage
- plugin
- volume
- provisioning
- ceph
- statefulset
- daemonset
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CSI Drivers 是什么
- 如何 CSI Drivers
trigger_keywords:
- CSI
- Drivers
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CSI Drivers

> CSI (Container Storage Interface) 是 Kubernetes 存储插件标准接口，定义了 K8s 与存储厂商之间的 gRPC 通信规范，替代了早期的 in-tree 和 FlexVolume 插件。

## 基本信息

| 属性 | 值 |
|------|------|
| 规范 | CSI Spec v1.9+ |
| 接口 | gRPC (Identity/Controller/Node) |
| K8s 版本 | 1.13+ GA |
| 替代 | In-tree plugins, FlexVolume |
| 注册 | CSIDriver 对象 |

## CSI 架构

```
┌─────────────────────────────────────────────────────┐
│              Kubernetes Control Plane               │
│  ┌────────────────────────────────────────────┐  │
│  │  External Provisioner / Attacher / Resizer  │  │
│  │  (Sidecar Containers)                       │  │
│  └─────────────────────┬──────────────────────┘  │
│                        │ gRPC                      │
└────────────────────────┼───────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        ▼                                  ▼
┌─────────────────┐            ┌─────────────────┐
│ CSI Controller  │            │   CSI Node      │
│ (Deployment/    │            │   (DaemonSet)   │
│  StatefulSet)   │            │                 │
│                 │            │ NodeStage       │
│ CreateVolume    │            │ NodePublish     │
│ DeleteVolume    │            │ NodeGetInfo     │
│ ControllerPub   │            │ NodeExpand      │
│ CreateSnapshot  │            │                 │
└────────┬────────┘            └────────┬────────┘
         │                               │
         ▼                               ▼
┌─────────────────────────────────────────────────────┐
│              存储后端                              │
│  AWS EBS / Ceph RBD / NFS / Azure Disk / GCE PD  │
└─────────────────────────────────────────────────────┘
```

## CSI 组件职责

| 组件 | 运行方式 | 职责 |
|------|----------|------|
| CSI Controller | Deployment/StatefulSet | CreateVolume, DeleteVolume, ControllerPublish, CreateSnapshot |
| CSI Node | DaemonSet (每个节点) | NodeStage, NodePublish, NodeExpand |
| External Provisioner | Sidecar | 监听 PVC，调用 CreateVolume |
| External Attacher | Sidecar | 监听 VolumeAttachment，调用 ControllerPublish |
| External Resizer | Sidecar | 监听 PVC 扩容，调用 ControllerExpand |
| External Snapshotter | Sidecar | 监听 VolumeSnapshot，调用 CreateSnapshot |
| Node Driver Registrar | Sidecar | 向 kubelet 注册 CSI 驱动 |

## 主流 CSI 驱动对比

| 驱动 | 后端 | 访问模式 | 特性 |
|------|------|----------|------|
| AWS EBS CSI | Amazon EBS | RWO | gp3/io2, 加密, 快照 |
| Azure Disk CSI | Azure Managed Disk | RWO | Premium SSD, 加密 |
| GCE PD CSI | Google PD | RWO | pd-ssd, 快照 |
| Ceph RBD CSI | Ceph RBD | RWO | 分布式, 快照, 克隆 |
| CephFS CSI | CephFS | RWX | 共享文件系统 |
| NFS CSI | NFS Server | RWX | 文件共享 |
| Longhorn | 本地磁盘 | RWO | 分布式块存储, 备份 |
| OpenEBS | 本地/网络 | RWO/RWX | 多种引擎 |
| TopoLVM | 本地 LVM | RWO | 本地卷管理 |

## 卷生命周期

```
1. Provision (创建)
   PVC 创建 → External Provisioner → CSI CreateVolume → 存储后端创建卷
       │
2. Attach (挂载到节点)
   Pod 调度 → External Attacher → CSI ControllerPublish → 卷挂载到节点
       │
3. Stage (格式化/挂载到临时目录)
   kubelet → CSI NodeStageVolume → 格式化 + 挂载到全局目录
       │
4. Publish (绑定到 Pod)
   kubelet → CSI NodePublishVolume → bind mount 到 Pod 目录
       │
5. Unpublish/Unstage (Pod 删除时反向操作)
       │
6. Detach (从节点卸载)
       │
7. Delete (删除卷, reclaimPolicy=Delete 时)
```

## StorageClass 配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
mountOptions: []
---
# Ceph RBD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-rbd
provisioner: rbd.csi.ceph.com
parameters:
  clusterID: ceph-cluster-1
  pool: rbd-pool
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: csi-rbd-secret
  csi.storage.k8s.io/provisioner-secret-namespace: ceph-system
reclaimPolicy: Delete
allowVolumeExpansion: true
```

## 运维操作

### 常用命令

```bash
# 🟢 查看已安装的 CSI 驱动
kubectl get csidrivers

# 🟢 查看 StorageClass
kubectl get storageclass

# 🟢 查看 PV/PVC
kubectl get pv
kubectl get pvc -A

# 🟢 查看卷详情
kubectl describe pv <pv-name>
kubectl describe pvc <pvc-name> -n <ns>

# 🟢 查看 CSI Node 插件状态
kubectl get csinode
kubectl describe csinode <node-name>

# 🟢 查看 CSI Controller 日志
kubectl logs -n kube-system -l app=ebs-csi-controller --tail=50

# 🟢 查看 CSI Node 日志
kubectl logs -n kube-system -l app=ebs-csi-node --tail=50

# 🟡 扩容 PVC
kubectl patch pvc <pvc-name> -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 🟢 查看快照
kubectl get volumesnapshot -A
kubectl get volumesnapshotcontent
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| PVC Pending | 无匹配 StorageClass/容量不足 | 检查 SC 参数和后端容量 |
| Pod ContainerCreating | 卷挂载失败 | 检查 CSI Node 日志 |
| 卷无法扩容 | SC 不支持/后端限制 | 检查 allowVolumeExpansion |
| 数据丢失 | reclaimPolicy=Delete | 使用 Retain + 备份 |
| 跨 AZ 调度失败 | WaitForFirstConsumer 未配置 | 设置 volumeBindingMode |
| CSI Pod CrashLoop | 权限/配置错误 | 检查 RBAC 和参数 |

### 排查流程

```
1. PVC 状态检查
   kubectl describe pvc <name> -n <ns>
       │
2. 事件查看
   kubectl get events -n <ns> --field-selector involvedObject.name=<pvc>
       │
3. CSI Controller 日志
   kubectl logs -n kube-system -l app=<csi>-controller
       │
4. CSI Node 日志
   kubectl logs -n kube-system -l app=<csi>-node --tail=50
       │
5. 存储后端确认
   检查实际卷状态
```

## 生产案例

### 案例1：EBS 卷跨 AZ 挂载失败

**症状：** Pod 调度后卷挂载超时

**根因：** EBS 卷在 us-east-1a，Pod 调度到 us-east-1b

**解决：** StorageClass 设置 `volumeBindingMode: WaitForFirstConsumer`

### 案例2：PVC 扩容后文件系统未扩展

**症状：** PVC 显示 100Gi，但 Pod 内 df 仍显示 50Gi

**根因：** 需要重启 Pod 触发 NodeExpandVolume

**解决：** 删除并重建 Pod，或等待 CSI Resizer 自动处理

## 检查清单

- [ ] 理解 CSI 架构 (Controller + Node)
- [ ] 掌握卷生命周期 7 个阶段
- [ ] 能配置 StorageClass
- [ ] 掌握 PVC 扩容操作
- [ ] 能排查卷挂载失败问题
- [ ] 理解 WaitForFirstConsumer 重要性
- [ ] 了解快照和克隆操作

## Related

- [[grpc]] — gRPC
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/storage-model.md|Persistent Storage Model]]
- [[技能/存储/csi-storage/manage-persistent-storage.md|Manage Persistent Storage]]
- [[实体/statefulset.md|StatefulSet]]

<!-- risk-assessed -->
