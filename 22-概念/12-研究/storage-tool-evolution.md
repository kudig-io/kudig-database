---
title: 存储工具演进
description: '# 存储工具演进'
summary: 'Rook 将分布式存储系统（Ceph 等）编排到 Kubernetes 原生环境中。'
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
tier: core
created: '2026-05-23'
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
status: stable
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 存储工具演进

> 本文档综合了 `生态参考/_archived-release-notes/storage/` 目录下 Rook、Longhorn 和 Velero 三大存储/备份工具的 76 个版本发布说明 ^[inferred]

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
- 自动问题恢复
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

## 源码实现分析

### Rook Operator 调谐循环

```go
// github.com/rook/rook/pkg/operator/ceph/cluster/controller.go
// Rook Operator 监听 CephCluster CR，编排 Ceph 组件生命周期
func (c *ClusterController) reconcileCluster(cluster *cephv1.CephCluster) error {
    // 1. 验证集群配置
    if err := c.validateCluster(cluster); err != nil {
        return err
    }
    // 2. 创建/更新 Ceph MON（监控守护进程）
    if err := c.mons.Start(cluster); err != nil {
        return errors.Wrap(err, "failed to start mons")
    }
    // 3. 创建 OSD（对象存储守护进程）
    if err := c.osds.Start(cluster); err != nil {
        return errors.Wrap(err, "failed to start osds")
    }
    // 4. 更新 CephCluster Status
    c.updateStatus(cluster, cephv1.ConditionReady)
    return nil
}
```

### Velero 备份流程

```go
// github.com/vmware-tanzu/velero/pkg/backup/backup.go
// Velero 备份控制器：序列化 K8s 资源 + 卷快照
func (b *kubernetesBackupper) Backup(backup *api.Backup) error {
    // 1. 收集所有需要备份的资源
    resources := b.itemCollector.GetItems(backup.Spec)
    // 2. 序列化每个资源为 YAML/JSON
    for _, item := range resources {
        b.backupItem(tarWriter, item)
    }
    // 3. 对 PVC 触发 VolumeSnapshotter 插件
    for _, pvc := range pvcs {
        snapshotID := b.volumeSnapshotter.CreateSnapshot(pv)
        b.backupStore.PutSnapshot(backup, snapshotID)
    }
    // 4. 上传到对象存储（S3/GCS/Azure Blob）
    b.backupStore.PutBackup(backup)
}
```

### 存储工具架构对比

```
┌───────────────────────────────────────────────────────────┐
│              存储工具架构对比                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Rook (Operator 模式)                                    │
│  ────────────────────                                    │
│  CephCluster CR → Rook Operator → MON/OSD/MDS Pods     │
│       │                                                  │
│       └→ CSI Driver → PVC/PV → Pod 挂载                 │
│                                                           │
│  Longhorn (微服务模式)                                   │
│  ────────────────────                                    │
│  LonghornManager DaemonSet → 每卷独立 Engine/Replica   │
│       │                                                  │
│       └→ CSI Driver → iSCSI/NFS → Pod 挂载              │
│                                                           │
│  Velero (备份模式)                                       │
│  ────────────────────                                    │
│  Backup CR → Velero Server → 序列化资源 + 卷快照       │
│       │                                                  │
│       └→ 对象存储 (S3/GCS/Azure) → Restore 恢复        │
└───────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：Rook Ceph 集群部署（🔴 生产存储基础设施）

```yaml
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v18.2
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
    allowMultiplePerNode: false
  storage:
    useAllNodes: false
    nodes:
    - name: storage-node-1
      devices:
      - name: /dev/sdb
      - name: /dev/sdc
  resources:
    osd:
      requests:
        cpu: "2"
        memory: 4Gi
      limits:
        memory: 8Gi
```

### 场景二：Velero 定时备份（🟡 创建备份任务）

```bash
# 创建每日凌晨 2 点的全集群备份
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --include-namespaces="production,staging" \
  --snapshot-volumes=true \
  --ttl=168h  # 保留 7 天

# 🟢 查看备份状态
velero backup get
velero schedule get daily-backup -o yaml

# 🔴 从备份恢复（影响生产）
velero restore create --from-backup daily-backup-20260711
```

### 场景三：Longhorn 卷快照与恢复（🟡 修改存储状态）

```bash
# 🟢 查看 Longhorn 卷状态
kubectl get volumes.longhorn.io -n longhorn-system

# 🟡 创建卷快照
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: Snapshot
metadata:
  name: pre-upgrade-snap
  namespace: longhorn-system
spec:
  volume: pvc-abc123
EOF

# 🔴 从快照恢复（会覆盖当前数据）
# 先停止 Pod，再恢复卷，最后重启 Pod
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Rook 就是 Ceph | Rook 是编排器，还支持其他存储后端 |
| Longhorn 不适合生产 | v1.4+ 已支持企业级 HA 和备份 |
| Velero 只备份 YAML | 还支持 PV 快照、Restic/Kopia 文件备份 |
| 备份不需要测试恢复 | 必须定期恢复演练，否则备份无意义 |
| Rook 升级无风险 | Ceph 升级需严格按版本顺序，跳版本可能损坏 |
| 存储工具不需要监控 | 必须监控 OSD 状态、卷健康、备份成功率 |

## 面试要点

1. **Rook 与直接使用 Ceph 的区别？**
   - Rook 是 K8s Operator，自动化 Ceph 生命周期管理
   - 自部署 Ceph 需手动管理 MON/OSD/MDS 配置
   - Rook 提供自愈、滚动升级、CSI 集成

2. **Velero 备份架构的核心组件？**
   - Velero Server：备份控制器 + 调度器
   - VolumeSnapshotter 插件：卷快照抽象
   - ObjectStore 插件：S3/GCS/Azure 存储后端
   - Restic/Kopia：文件级备份（无需 CSI 快照）

3. **Longhorn vs Rook 如何选型？**
   - Longhorn：轻量、简单、小团队、块存储为主
   - Rook/Ceph：企业级、多存储类型、大规模、复杂需求
   - 关键因素：团队能力、规模、存储类型需求

4. **生产环境存储备份策略如何设计？**
   - 3-2-1 原则：3 份副本、2 种介质、1 份异地
   - Velero 定时备份 + 卷快照 + 异地复制
   - 定期恢复演练（至少季度一次）

## 来源文档

- 生态参考/_archived-release-notes/storage/rook/（29 个文件）
- 生态参考/_archived-release-notes/storage/longhorn/（19 个文件）
- 生态参考/_archived-release-notes/storage/velero/（28 个文件）

## Related

- [[22-概念/12-研究/observability-stack-evolution.md|observability-stack-evolution]] — 可观测性栈演进
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[rook]] — Rook
- [[longhorn]] — Longhorn


<!-- risk-assessed -->
