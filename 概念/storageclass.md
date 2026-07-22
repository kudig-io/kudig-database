---
title: StorageClass
summary: StorageClass 是 Kubernetes 中用于定义存储"类"的资源对象，它描述了存储卷的质量-of-service 级别、备份策略或集群管理员定义的任意策略。通过
  StorageClass，Kubernetes 可以实现存储的动态供给（Dynamic Provisioning）。
category: concepts
tags:
- storage
- storageclass
- dynamic-provisioning
- core
- visibility/public
tier: core
sources:
- concepts/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# StorageClass

## 概述

StorageClass 是 Kubernetes 中用于定义存储"类"的资源对象。它由集群管理员声明，描述一类存储的供应参数：provisioner（谁来创建）、参数（性能/类型/区域）、回收策略、绑定模式、是否可扩容等。它的核心价值是**动态供给（Dynamic Provisioning）**——用户只需在 PVC 里写 `storageClassName: fast-ssd`，系统自动按 StorageClass 模板创建底层 PV，无需管理员手工预创建。一个集群通常定义多个 StorageClass（如 fast-ssd、standard-hdd、cold-blob），并标注一个为默认。

## 架构与工作原理

```
用户 PVC (storageClassName: fast-ssd)
        │ PersistentVolumeClaim
        ▼
StorageClass "fast-ssd"
   provisioner: disk.csi.azure.com
   parameters: {type: Premium_LRS}
   reclaimPolicy: Delete
        │ 触发 provisioner 调用
        ▼
CSI Driver / 外部 Provisioner
        │ 向云/分布式存储 API 请求创建卷
        ▼
真实存储卷（云盘 / NFS / Ceph / 本地）
        │ 创建成功，回调生成 PV
        ▼
PVC ←→ PV 自动绑定（动态）
```

**工作流**：
1. 集群管理员定义 StorageClass（指定 provisioner + 参数 + 策略）。
2. 用户创建 PVC 引用该 StorageClass。
3. 对应的 **CSI 外部 Provisioner**（或 in-tree provisioner）监听到 PVC，调用后端 API 创建真实存储卷。
4. 创建成功后自动生成 PV，并与 PVC 绑定。
5. Pod 通过 PVC 挂载使用；删除 PVC 时按 `reclaimPolicy` 决定 PV/卷命运。

**绑定模式（volumeBindingMode）**：
- `Immediate`：PVC 创建即触发供给与绑定（不看 Pod 调度位置）。适合云盘等独立于节点的存储。
- `WaitForFirstConsumer`：等到第一个使用该 PVC 的 Pod 被调度后，再按 Pod 所在节点/拓扑供给绑定。**多拓扑存储（如本地盘、跨 AZ 云盘）必选**，避免卷和 Pod 在不同 AZ。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `provisioner` | 决定谁来创建卷（CSI driver / in-tree） |
| `parameters` | 传给 provisioner 的参数（磁盘类型、IOPS、区域、fsType） |
| `reclaimPolicy` | Retain / Delete（PVC 删除时 PV 命运） |
| `volumeBindingMode` | Immediate / WaitForFirstConsumer |
| `allowVolumeExpansion` | true 时 PVC 可在线扩容 |
| `mountOptions` | 挂载选项（如 hard,nfsvers=4.1） |
| `metadata.annotations[storageclass.kubernetes.io/is-default-class]` | 标为默认 |

## 配置示例

```yaml
---
# 1. 高性能 SSD（云盘，动态供给）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: disk.csi.azure.com
parameters:
  type: Premium_LRS          # 或 UltraSSD_LRS / StandardSSD_LRS
  cachingmode: ReadOnly
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer   # 跨 AZ 必选
allowVolumeExpansion: true
mountOptions:
- noatime
---
# 2. 本地盘（受拓扑约束，跨节点不能漂移）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-storage
provisioner: kubernetes.io/no-provisioner   # 不动态创建，靠静态 PV
volumeBindingMode: WaitForFirstConsumer     # 必选：按 Pod 调度位置匹配
reclaimPolicy: Retain
---
# 3. NFS 共享存储
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-shared
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs.example.com
  share: /export/k8s
reclaimPolicy: Retain
volumeBindingMode: Immediate
---
# 4. 用户 PVC 引用
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data
  namespace: production
spec:
  storageClassName: fast-ssd
  accessModes: [ReadWriteOnce]
  resources:
    requests: {storage: 200Gi}
```

## 常用操作与命令

```bash
# 查看 StorageClass 与默认
kubectl get sc
kubectl get sc -o custom-columns=NAME:.metadata.name,DEFAULT:.metadata.annotations.storageclass\.kubernetes\.io/is-default-class,PROVISIONER:.provisioner,BINDING:.volumeBindingMode

# 查看动态供给的 PV/PVC
kubectl get pvc,pv -n production

# 设为默认
kubectl patch sc standard -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 在线扩容（要求 allowVolumeExpansion: true）
kubectl patch pvc db-data -p '{"spec":{"resources":{"requests":{"storage":"500Gi"}}}}'
kubectl get pvc db-data -o jsonpath='{.status.conditions}'

# 查看 CSI driver 状态
kubectl get csidriver
kubectl get pods -n kube-system | grep csi

# 排查 PVC Pending
kubectl describe pvc db-data    # 看 Events: failed to provision / no matching topology
```

## 最佳实践

1. **按性能分层定义多个 SC**：fast-ssd / standard-hdd / cold-blob，让用户按需选择。
2. **跨 AZ 用 WaitForFirstConsumer**：避免卷在 AZ-a、Pod 调度到 AZ-b 导致挂载失败。
3. **reclaimPolicy 谨慎选 Delete**：数据库类用 Retain 防误删；临时缓存类用 Delete 自动清理。
4. **开启 allowVolumeExpansion**：未来扩容免重建，按需开。
5. **明确默认 StorageClass**：用户 PVC 不指定 SC 时用默认，避免误用昂贵存储。
6. **CSI driver 版本跟随**：云厂商 CSI 持续迭代，定期升级获新特性（snapshot/clone/扩容）。
7. **命名含性能/拓扑语义**：`fast-ssd-replicated` 比 `sc1` 更可读。

## 远程顾问诊断要点

- 询问用户使用的 provisioner 类型（如 alicloud-disk、csi-plugin）
- 检查 StorageClass 的 VolumeBindingMode（Immediate/WaitForFirstConsumer）
- 确认 reclaimPolicy 和 allowVolumeExpansion 设置

## 常见陷阱

- **PVC 一直 Pending**：provisioner 未安装/异常、参数错误、配额不足、跨拓扑约束无法满足。
- **卷与 Pod 跨 AZ**：Immediate 模式下卷创建在 AZ-a，Pod 调度到 AZ-b 挂载失败；改 WaitForFirstConsumer。
- **扩容失败**：StorageClass 未开 allowVolumeExpansion，或 CSI driver 版本不支持在线扩。
- **删除 PVC 后存储未释放**：reclaimPolicy=Retain 时 PV 保留但底层卷释放取决于 provider。
- **in-tree provisioner 弃用**：K8s 迁移 CSI，旧的 in-tree storage class（如 kubernetes.io/aws-ebs）在新版本失效。
- **本地盘误用**：local-volume 跨节点不可迁移，Pod 漂移后挂载失败，需配合 nodeAffinity 固定。
- **默认 SC 被误改**：切换默认 SC 影响所有未指定 SC 的 PVC，谨慎操作。

## 源码实现分析

### External Provisioner 动态供给流程

```go
// sigs.k8s.io/sig-storage-lib-external-provisioner/controller/controller.go
// CSI External Provisioner 核心逻辑
func (p *csiProvisioner) Provision(ctx context.Context, options controller.ProvisionOptions) (*v1.PersistentVolume, error) {
    // 1. 从 StorageClass 获取 provisioner 名称和参数
    sc := options.StorageClass
    // provisioner: ebs.csi.aws.com / pd.csi.storage.gke.io
    
    // 2. 调用 CSI CreateVolume RPC
    req := &csi.CreateVolumeRequest{
        Name:               pvName,
        CapacityRange:      &csi.CapacityRange{RequiredBytes: size},
        VolumeCapabilities: volCaps,  // RWO/RWX
        Parameters:         sc.Parameters,  // type: gp3, iops: 3000
    }
    resp, err := p.csiClient.CreateVolume(ctx, req)
    
    // 3. 构建 PV 对象并返回
    pv := &v1.PersistentVolume{
        Spec: v1.PersistentVolumeSpec{
            Capacity: v1.ResourceList{v1.ResourceStorage: qty},
            PersistentVolumeSource: v1.PersistentVolumeSource{
                CSI: &v1.CSIPersistentVolumeSource{
                    Driver: sc.Provisioner,
                    VolumeHandle: resp.Volume.VolumeId,
                },
            },
        },
    }
}
```

```
┌─────────────────────────────────────────────────────────┐
│     StorageClass 动态供给架构                        │
├─────────────────────────────────────────────────────────┤
│  PVC Created (storageClassName: gp3-ssd)               │
│       │                                                 │
│       ▼                                                 │
│  ┌───────────────────┐                      │
│  │ External Provisioner │ (Watch PVC)            │
│  └───────────────────┘                      │
│       │                                         │
│       ▼                                         │
│  CSI CreateVolume RPC                           │
│       │                                         │
│       ▼                                         │
│  ┌───────────────────┐                      │
│  │  CSI Driver (node)  │                      │
│  │  ebs.csi.aws.com    │                      │
│  └───────────────────┘                      │
│       │                                         │
│       ▼                                         │
│  Cloud API: CreateVolume (gp3, 100Gi, 3000 IOPS)│
│       │                                         │
│       ▼                                         │
│  PV Created ──▶ PVC Bound                       │
└─────────────────────────────────────────────────────────┘
```

### 生产配置：多层级 StorageClass

```yaml
# 高性能 SSD（数据库）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "false"
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
---
# 低成本 HDD（日志/备份）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: st1-hdd
provisioner: ebs.csi.aws.com
parameters:
  type: st1
  encrypted: "true"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 生产运维：StorageClass 故障诊断

```bash
# 🟢 查看 StorageClass 和默认标记
kubectl get sc
kubectl describe sc <name>

# 🟢 检查 Provisioner 状态
kubectl get pods -n kube-system -l app=csi-provisioner
kubectl logs -n kube-system -l app=csi-provisioner --tail=30

# 🟢 检查 PVC 动态供给事件
kubectl describe pvc <name> -n <ns> | grep -A5 Events

# 🟡 修改默认 StorageClass
kubectl patch sc <old> -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"false"}}}'
kubectl patch sc <new> -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'
```

## 面试要点

1. **StorageClass 的 volumeBindingMode 有什么区别？**
   - Immediate：PVC 创建即触发供给，不考虑 Pod 调度位置
   - WaitForFirstConsumer：等 Pod 调度确定后再供给，保证同 AZ
   - 云环境必须用 WaitForFirstConsumer，否则可能跨 AZ 挂载失败

2. **allowVolumeExpansion 如何工作？**
   - 允许用户修改 PVC.spec.resources.requests.storage 触发扩容
   - CSI 驱动调用 ControllerExpandVolume RPC
   - 块存储扩容后需文件系统 resize（kubelet 自动执行）
   - 注意：只能扩不能缩

3. **reclaimPolicy 对生产的影响？**
   - Delete：PVC 删除后云盘自动删除（数据不可恢复）
   - Retain：PVC 删除后 PV 保留，需手动清理（安全但占资源）
   - 生产建议：数据库用 Retain，临时工作负载用 Delete

4. **如何设计多层级存储方案？**
   - 按性能/成本分层：SSD（数据库）/ HDD（日志）/ 对象存储（备份）
   - 每个层级一个 StorageClass，通过参数区分
   - 配合 VolumeSnapshot 实现跨层级迁移

## 相关链接

- [[概念/pv.md|PersistentVolume]] — 持久化卷
- [[概念/persistent-volume-claim.md|PersistentVolumeClaim]] — 持久化卷声明
- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/statefulset.md|StatefulSet]] — 与 volumeClaimTemplates 配合
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
