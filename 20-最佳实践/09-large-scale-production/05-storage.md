---
title: 存储最佳实践
description: 大规模 Kubernetes 集群存储的 StorageClass 设计、CSI 驱动、卷类型选型、备份快照、容量治理与有状态应用的生产级最佳实践
summary: 覆盖 StorageClass 分层设计、CSI 与卷类型选型矩阵、快照备份体系、PV/PVC 容量治理、有状态应用存储实践
category: references
tags:
- k8s
- best-practices
- storage
- csi
- backup
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 存储工程师
- 平台工程师
estimated_read_time: 20min
---

# 存储最佳实践

> 存储是 Kubernetes 生产中"出问题最难救"的层：网络抖动能恢复，数据坏了救不回。大规模集群的存储实践核心是：**分层设计、容量治理、备份先行**。

## 1. StorageClass 分层设计

### 1.1 按性能/成本分层

| 层级 | 典型实现 | 适用场景 | reclaimPolicy |
|---|---|---|---|
| 高性能 | 本地 NVMe（TopoLVM/OpenEBS local）、ESSD PL2+/io2 | 数据库、消息队列、ES | Retain |
| 通用 | 云 SSD（ESSD PL1/gp3）、Ceph RBD | 常规有状态服务 | Delete（配合备份）或 Retain |
| 容量型 | HDD 云盘、CephFS、对象存储网关 | 日志、归档、备份介质 | Delete |
| 共享文件 | NAS/EFS/FSx、CephFS | 多 Pod 共享读写（RWX） | Retain |

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ssd-high
  annotations:
    storageclass.kubernetes.io/is-default-class: "false"   # 谨慎设置默认 SC
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "16000"
  throughput: "1000"
  encrypted: "true"          # 静态加密必须开
reclaimPolicy: Retain        # 高性能层建议 Retain，防止误删 PVC 丢数据
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer   # 关键：延迟绑定，避免调度与 AZ 错配
```

### 1.2 关键设计点

- **`volumeBindingMode: WaitForFirstConsumer`**：跨 AZ 集群必须，先调度再供给卷，避免"卷在 A 区、Pod 调度到 B 区"
- **默认 StorageClass 唯一**：集群中只能有一个默认 SC，多个默认会导致供给行为不可预测
- `allowVolumeExpansion: true`：所有云盘类 SC 开启在线扩容
- `reclaimPolicy`：核心数据层用 `Retain` + 人工回收流程；临时数据用 `Delete`

## 2. CSI 驱动治理

- 只部署经生产验证的 CSI 驱动；controller 插件（provisioner/attacher/resizer）要有 HA 副本与资源 requests
- node 插件 DaemonSet 资源受限保护——CSI 异常（如 mount 泄漏）不能拖垮节点
- 关注挂载点泄漏：节点上 `mount | grep kubelet` 数量异常增长是 CSI 故障信号
- 升级 CSI 驱动与集群版本保持兼容矩阵，纳入升级检查项

## 3. 卷类型选型矩阵

| 需求 | 推荐 | 避免 |
|---|---|---|
| 数据库（MySQL/PG） | 本地 NVMe + 应用层复制，或高性能云盘 | 共享文件存储、普通 HDD |
| 消息队列（Kafka） | 本地盘 + 多副本，或高性能云盘 | 网络文件系统 |
| ES/搜索 | 本地 SSD 或高性能云盘 | HDD |
| AI 训练数据 | 并行文件系统/对象存储 + 缓存层（JuiceFS/Fluid） | 单点 NAS |
| 共享配置/模型 | RWX 文件存储、ConfigMap（<1 MiB） | 用 PVC 当配置中心 |
| 临时缓存 | emptyDir（内存介质 `medium: Memory` 注意计入容器内存 limit） | hostPath（除非必要且受控） |

## 4. 备份与快照体系

### 4.1 三层备份

1. **应用层备份**（首选）：数据库自带备份（mysqldump/binlog、pg_basebackup、Kafka MirrorMaker）——恢复粒度最细、一致性最好
2. **卷快照**：CSI VolumeSnapshot，秒级生成，适合快速回滚；注意快照**不是**备份（同存储域，存储整体故障时快照一起丢）
3. **集群级备份**：Velero + restic/kopia，覆盖 etcd 资源 + PV 数据，支持跨集群/跨云恢复

### 4.2 快照实践

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
driver: ebs.csi.aws.com
deletionPolicy: Retain     # 删除 VolumeSnapshot 对象时保留底层快照
```

- 快照策略：核心业务每 4–6 小时一次，保留 7–30 天，异地/跨账号复制
- **每季度恢复演练**：从快照恢复 PVC → 挂载验证数据完整性
- Velero 备份验证：定期做"备份 → 恢复到隔离集群"的端到端演练

## 5. 容量治理（大规模重点）

- **PVC 使用率监控**：`kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes`，水位 > 80% 告警并触发扩容流程
- 在线扩容流程：改 PVC `spec.resources.requests.storage` → 文件系统扩展（ext4/xfs 自动）→ 应用无感知；**只增不减**
- 孤儿资源治理：定期扫描 Bound 但无 Pod 引用的 PVC、Available 状态的 PV、快照残留——大集群里这些是显著成本泄漏
- 配额：`ResourceQuota` 中管控 `persistentvolumeclaims` 数量与 `requests.storage` 总量（见 [[03-workload#1.3 集群级治理]]）
- 节点本地盘治理：emptyDir/日志写满 nodefs 会触发整机驱逐——应用日志必须有轮转与上限

## 6. 有状态应用存储实践

- StatefulSet + volumeClaimTemplates 是标准姿势；删除 Pod **不会**删 PVC，重建时原卷复用
- 扩缩容与缩容后的 PVC 残留要纳入 SOP（缩容后手动清理或 operator 管理）
- 数据库类优先使用 **Operator**（如 CloudNativePG、Strimzi、Percona Operator）管理存储生命周期，避免手工 PV/PVC 漂移
- 本地盘场景：配合节点亲和与污点，防止 Pod 漂移到无数据节点；本地盘节点纳入"不可随意回收"清单
- 跨 AZ 数据：存储层复制（如 EBS 不跨 AZ——需应用层复制）+ `topologySpreadConstraints` 保证副本分布

## 7. 监控指标清单

| 指标 | 告警阈值建议 |
|---|---|
| PVC 使用率 | > 80% 警告，> 90% 严重 |
| 卷 IO 延迟（P99） | 云盘 > 10ms 关注，> 50ms 严重 |
| 卷 IOPS/吞吐打满率 | > 80% 持续 5 分钟 |
| CSI 操作失败率（attach/mount/provision） | 持续非零 |
| 快照任务成功率 | 任何失败即告警 |
| etcd db size / 配额使用率 | > 70%（见 [[02-cluster-configuration]]） |

## 8. 常见反模式

| 反模式 | 后果 |
|---|---|
| 用快照当备份 | 存储域故障时快照与数据一起丢 |
| Immediate 绑定跨 AZ | 卷与 Pod 分区错配，Pod 永久 Pending |
| 无 PVC 水位监控 | 磁盘写满 → 数据库崩溃 → 恢复复杂 |
| hostPath 随意挂载 | 节点状态污染、安全风险、Pod 漂移即丢数据 |
| 数据库上 K8s 无应用层复制 | 单卷故障即数据不可用 |
| 只做备份不做恢复演练 | 出事时发现备份不可用 |

## Related

- [[02-cluster-configuration|集群配置最佳实践（etcd 部分）]]
- [[07-pre-production-checklist|生产上线前检查项（灾备演练）]]
- [[20-最佳实践/07-scenarios/storage-issues|存储问题场景]]
- [[20-最佳实践/07-scenarios/backup-restore|备份恢复场景]]
