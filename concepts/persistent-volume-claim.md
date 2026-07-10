---
title: PersistentVolumeClaim
summary: PersistentVolumeClaim（PVC）是 Kubernetes 中用户对持久化存储的声明式请求。Pod 通过挂载 PVC 来使用存储，而无需关心底层存储的具体实现。
category: concepts
tags:
- core-concept
- 存储
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PersistentVolumeClaim

`PersistentVolumeClaim`（PVC）是 Kubernetes 中用户对持久化存储的声明式请求。Pod 通过挂载 PVC 来使用存储，而无需关心底层存储的具体实现。

## PVC 与 PV 的关系

PVC 与 `PersistentVolume`（PV）之间是**声明式绑定**关系：

- **PV** 由管理员或动态供给器创建，代表集群中的实际存储资源
- **PVC** 由用户创建，描述所需的存储规格
- Kubernetes 控制平面负责将满足条件的 PVC 与 PV 进行绑定

绑定完成后，PVC 进入 `Bound` 状态，可被 Pod 通过 `persistentVolumeClaim` 卷类型挂载。

## StorageClass 动态供给

当集群配置了 [[storage-classes]] 时，PVC 可以触发**动态供给**（Dynamic Provisioning）：

1. 用户在 PVC 中指定 `storageClassName`
2. 若不存在匹配的 PV，对应的 StorageClass 会调用 Provisioner 自动创建 PV
3. 新创建的 PV 与 PVC 自动绑定

常见 Provisioner 包括：

- `kubernetes.io/aws-ebs`
- `kubernetes.io/gce-pd`
- `kubernetes.io/azure-disk`
- 阿里云 `diskplugin.csi.alibabacloud.com`

## PVC 关键字段

| 字段 | 说明 |
|------|------|
| `accessModes` | 访问模式：`ReadWriteOnce`（单节点读写）、`ReadOnlyMany`（多节点只读）、`ReadWriteMany`（多节点读写） |
| `resources.requests.storage` | 请求的存储容量，如 `10Gi` |
| `storageClassName` | 指定使用的 StorageClass，空字符串表示使用默认类，不设置则不使用动态供给 |
| `volumeName` | 直接绑定到指定 PV（通常不推荐） |
| `selector` | 通过标签选择匹配的 PV |

## 绑定条件

PVC 与 PV 成功绑定需同时满足：

1. **容量匹配**：PV 的 `capacity` ≥ PVC 的 `resources.requests.storage`
2. **访问模式匹配**：PV 的 `accessModes` 包含 PVC 请求的访问模式
3. **StorageClass 匹配**：PV 的 `storageClassName` 与 PVC 的 `storageClassName` 一致（若 PVC 未设置，则匹配空类 PV）
4. **标签选择器匹配**（若 PVC 设置了 `selector`）

## 阿里云 ACK 存储

在阿里云 ACK 集群中，常见的 PVC 类型包括：

- **云盘 PVC**：基于 ESSD/SSD 云盘，支持 `ReadWriteOnce`，适合数据库等有状态应用
- **NAS PVC**：基于文件存储 NAS，支持 `ReadWriteMany`，适合共享存储场景
- **OSS PVC**：基于对象存储 OSS，适合静态资源、日志归档等大容量场景

不同存储类型的 IOPS、吞吐量和可用区限制各不相同，选型时需结合业务需求。

## 远程顾问诊断要点

PVC 一直停留在 `Pending` 状态是存储类问题的典型表现，诊断思路如下：

- **检查 StorageClass**：确认 `storageClassName` 是否拼写正确，对应的 StorageClass 是否存在且为默认类
- **检查可用 PV**：若未启用动态供给，确认是否有未绑定的 PV 满足容量和访问模式要求
- **检查 Zone 匹配**：云盘类存储通常有可用区约束，确保 Pod 调度节点与 PV/StorageClass 的可用区一致
- **检查资源配额**：确认命名空间的 `ResourceQuota` 是否限制了 PVC 或存储类的使用
- **查看事件**：`kubectl describe pvc <name>` 中的 Events 通常会给出具体失败原因

更多存储排错方法请参考 [[故障诊断/资源排障/14-pvc-storage-troubleshooting.md|pvc-storage-troubleshooting]]。

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
