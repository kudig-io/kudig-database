---
title: 存储故障排查
description: '# 存储故障排查'
category: skills
tags:
- k8s
- troubleshooting
- structural
- storage
- etcd
- kubelet
- scheduler
- prometheus
- daemonset
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 存储故障排查 是什么
- 如何 存储故障排查
trigger_keywords:
- 存储故障排查
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
created: "2026-05-23"
---

# 存储故障排查

### 01 Pv Pvc Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **PVC 状态**：`kubectl get pvc -A -o wide`，关注 Pending/Bound；`kubectl describe pvc <name>` 看事件。
2. **PV/SC 对齐**：`kubectl get pv`、`kubectl get sc`，确认 StorageClass、`volumeBindingMode`、`reclaimPolicy` 是否符合预期。
3. **附件状态**：`kubectl get volumeattachment -o wide`，判断 Attach 是否卡住或已挂在旧节点。
4. **节点挂载**：在节点上 `findmnt -t csi`、`ls -l /dev/disk/by-id/ | grep <pv>`，确认设备与挂载存在。
5. **多点挂载**：若 `Multi-Attach`，先确认旧 Pod 是否已删除，必要时清理僵尸附件。
6. **快速缓解**：
   - Pending：检查后端配额/可用区/StorageClass，必要时扩容或调整 Topology。
   - Attach 卡住：谨慎删除 `VolumeAttachment` 并在云控制台确认解绑。
7. **证据留存**：保存 PVC/PV/VA 描述、CSI controller 日志与节点挂载快照。

---

#### 排查方法与步骤

1. **确认 PVC/PV/StorageClass 状态**：核对绑定关系与 `volumeBindingMode`。
2. **检查 VolumeAttachment**：确认卷是否仍附着在旧节点。
3. **节点侧挂载排查**：查看设备、挂载点与 [[kubelet|kubelet]] 日志。
4. **区分控制面与数据面**：判断是 Provision/Attach 还是 Mount 阶段问题。
5. **验证修复结果**：Pod 启动、挂载点可读写、监控指标恢复。

#### 常见修复策略

- **Pending**：补齐 StorageClass/拓扑配置，校验后端配额与可用区。
- **Multi-Attach**：清理旧 `VolumeAttachment` 并在云控制台确认解绑。
- **挂载失败**：修复节点依赖（文件系统工具/内核模块）并重试挂载。

---

### 02 Csi Troubleshooting

#### 0. 10 分钟快速诊断

1. **CSI 组件就绪**：`kubectl get [[Pods|pods]] -n kube-system | grep -E "csi|storage"`，确认 controller/node 插件均 Running。
2. **驱动注册**：`kubectl get csinode <node> -o yaml`，确认驱动条目存在；节点上检查 `/var/lib/kubelet/plugins/` Socket。
3. **控制面日志**：`kubectl logs -n kube-system <csi-controller> -c csi-provisioner|csi-attacher`，定位 Create/Attach 失败原因。
4. **Node 侧日志**：`kubectl logs -n kube-system <csi-node-pod> -c <driver>`，查看 NodePublish/NodeStage 错误。
5. **超时/限流**：若 `DeadlineExceeded`，关注后端 API 速率限制与网络抖动。
6. **快速缓解**：
   - Socket 异常：重启 csi-node Pod，确认 hostPath 与 kubelet 根目录一致。
   - 资源过载：提升 controller 副本和资源限制，削峰 PVC 创建。
7. **证据留存**：保存 csi-controller/node 日志、CSINode 状态与失败请求时间点。

---

#### 排查方法与步骤

1. **确认驱动注册**：检查 `CSINode` 与节点 Socket 路径。
2. **控制面日志定位**：查看 `csi-provisioner/attacher/resizer/snapshotter` 日志。
3. **节点侧挂载定位**：确认 Node Pod 日志与挂载点状态。
4. **后端 API 状态核对**：检查云平台配额、速率限制与卷状态。
5. **修复验证**：PVC 绑定、Pod 挂载、快照/扩容状态恢复。

#### 常见修复策略

- **驱动未注册**：修正 kubelet 根目录与 DaemonSet 的 `hostPath` 映射。
- **调用超时**：提升超时阈值并优化后端 API 调用频率。
- **挂载失败**：修复节点权限/工具链后重试挂载。

---

### 03 Snapshot Backup Troubleshooting

#### 0. 10 分钟快速诊断

1. **Snapshot 组件存活**：`kubectl get pods -n kube-system | grep snapshot`，确认 snapshot-controller 和 snapshot-validation-webhook 运行正常。
2. **CRD 检查**：`kubectl get crd volumesnapshots.snapshot.storage.k8s.io volumesnapshotcontents.snapshot.storage.k8s.io volumesnapshotclasses.snapshot.storage.k8s.io`，确认 CSI Snapshot CRD 已安装。
3. **SnapshotClass 配置**：`kubectl get volumesnapshotclass`，确认存在可用的 SnapshotClass 且 `driver` 字段与 CSI 驱动匹配。
4. **快照状态检查**：`kubectl get volumesnapshot -A`，观察 `READYTOUSE` 列状态。
5. **Sidecar 日志**：查看 CSI 驱动的 snapshotter sidecar 容器日志，定位 `CreateSnapshot`/`DeleteSnapshot` 错误。
6. **快速缓解**：
   - 快照创建卡住：检查 CSI 驱动后端存储配额和快照数量限制。
   - 恢复失败：确认源 PVC 已删除（如需从快照创建新 PVC）或快照 `READYTOUSE=true`。
7. **证据留存**：保存 VolumeSnapshot、VolumeSnapshotContent 的 YAML 状态、CSI 驱动日志、后端存储快照列表。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

CSI 快照的工作流程涉及多个组件的协同：

```
用户创建 VolumeSnapshot
        │
        ▼
┌─────────────────────┐
│ snapshot-controller │  ──► 监听 VolumeSnapshot，创建 VolumeSnapshotContent
│ (external-snapshotter)│
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ CSI Snapshot Sidecar │ ──► 调用 CSI 驱动的 CreateSnapshot RPC
│ (csi-snapshotter)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   CSI Driver         │ ──► 与存储后端交互，实际创建快照
│ (vendor specific)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   存储后端           │ ──► 物理快照（LVM/COW/ROW/copy-on-write）
│ (cloud/block/nfs)    │
└─────────────────────┘
```

**关键概念**：
- **Crash Consistency**：快照捕获的是文件系统在某一刻的状态，类似于断电时的状态。对于数据库等有状态应用，可能需要额外操作确保一致性。
- **Application Consistency**：通过 pre-snapshot hook（如 `fsfreeze`、`pg_dump`、`LOCK TABLES`）确保应用在快照时处于静默状态。
- **VolumeSnapshotContent**：快照的实际后端资源映射，由 snapshot-controller 动态创建。

---

### 04 Storage Performance Troubleshooting

#### 0. 10 分钟快速诊断

1. **确认症状**：应用报 `I/O timeout`、`database slow query`、`write stalled`，或 Pod 事件中出现 `VolumeMount` 延迟。
2. **节点 I/O 指标**：`iostat -x 1 10` 查看 `%util`、`await`、`svctm`，确认物理磁盘是否饱和。
3. **PVC 延迟**：`kubectl top pvc`（如 metrics-server 支持）或检查 CSI 驱动的 Prometheus 指标（`csi_sidecar_operations_seconds`）。
4. **存储类差异**：对比不同 StorageClass 的 PVC 性能，确认是否因存储类型选择不当导致。
5. **文件系统检查**：`df -i` 检查 inode 使用率，`dumpe2fs`/`xfs_info` 检查文件系统参数。
6. **快速缓解**：
   - 临时迁移到本地 SSD 或更高性能存储类。
   - 对数据库类应用，增加 `fsync` 间隔或切换到异步写入模式（需评估数据安全风险）。
7. **证据留存**：保存 `iostat`、`fio` 测试结果、PVC YAML、StorageClass 参数、应用慢查询日志。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

Kubernetes 存储 I/O 路径涉及多个层次，性能问题可能发生在任何一层：

```
应用层 (Pod)
    │  write()/read()
    ▼
文件系统层 (ext4/XFS)
    │  page cache, journal, inode allocation
    ▼
块设备层 (Block Device)
    │  I/O scheduler (mq-deadline/none/bfq)
    ▼
卷层 (PVC <-> PV)
    │  CSI 驱动, device mapper, LVM
    ▼
存储协议层
    │  Local SCSI/SATA/NVMe | iSCSI | NFS | FC
    ▼
存储后端层
    │  本地磁盘 | SAN | NAS | 云盘 (EBS/SSD/Premium)
    ▼
物理介质层
       SSD/HDD/NVMe
```

**关键性能指标**：
- **IOPS**：每秒 I/O 操作数，随机小 I/O 场景的关键指标
- **吞吐量 (Throughput)**：每秒传输的数据量，大文件顺序读写场景的关键指标
- **延迟 (Latency)**：单个 I/O 操作的完成时间，数据库等延迟敏感场景的关键指标
- **I/O 队列深度**：等待处理的 I/O 请求数量，高并发场景的关键指标

---

### 05 Storageclass Troubleshooting

#### 0. 10 分钟快速诊断

1. **StorageClass 存在性**：`kubectl get storageclass`，确认目标 StorageClass 存在且未标记为已弃用。
2. **Provisioner 注册**：`kubectl get csidriver` 或查看 StorageClass 的 `provisioner` 字段，确认对应驱动已注册。
3. **PVC 事件**：`kubectl describe pvc <name>`，关注与 StorageClass 相关的事件（如 `ProvisioningFailed`、`WaitForFirstConsumer`）。
4. **默认类冲突**：`kubectl get storageclass`，检查是否多个 StorageClass 带有 `(default)` 标记。
5. **参数验证**：`kubectl get storageclass <name> -o yaml`，核对 `parameters` 是否符合后端存储要求。
6. **快速缓解**：
   - PVC Pending 因 StorageClass 缺失：创建正确的 StorageClass 或修改 PVC 的 `storageClassName`。
   - 默认类冲突：移除多余的 `storageclass.kubernetes.io/is-default-class` annotation。
   - 拓扑不匹配：确认 `volumeBindingMode: WaitForFirstConsumer` 与 Pod 调度拓扑一致。
7. **证据留存**：保存 StorageClass YAML、PVC/PV 事件、CSI provisioner 日志、后端存储控制台状态。

---

#### 2. 排查方法与步骤



#### 2.2 动态供给失败排查

#### 2.2.1 排查逻辑决策树

```
PVC 处于 Pending，事件显示 ProvisioningFailed
    │
    ├─ 1. 检查 StorageClass 存在性
    │       ├─ StorageClass 不存在 → 创建或修改 PVC 指向正确的类
    │       └─ StorageClass 存在 → 进入 2
    │
    ├─ 2. 检查 Provisioner 注册
    │       ├─ 内置 Provisioner（如 kubernetes.io/aws-ebs）→ 检查云厂商集成
    │       ├─ CSI Provisioner → 检查 CSIDriver 和 CSI Controller Pod
    │       └─ Provisioner 未注册 → 部署对应的 CSI 驱动
    │
    ├─ 3. 检查参数正确性
    │       ├─ parameters 错误 → 对照后端文档修正
    │       └─ 参数正确 → 进入 4
    │
    ├─ 4. 检查后端状态
    │       ├─ 配额耗尽 → 申请提升配额或释放资源
    │       ├─ 可用区不可用 → 更换可用区或 StorageClass
    │       └─ API 限流 → 降低 PVC 创建速率
    │
    └─ 5. 检查 CSI Controller 日志
            └─ 具体错误信息 → 针对性修复
```

#### 2.2.2 Provisioner 注册检查

```bash
# 检查 CSIDriver 是否存在（CSI 场景）
kubectl get csidriver
# 预期输出：
# NAME                       ATTACHREQUIRED   PODINFOONMOUNT   STORAGECAPACITY   TOKENREQUESTS   REQUIRESREPUBLISH   MODES        AGE
# ebs.csi.aws.com            true             false            false             <unset>        false               Persistent   30d
# diskplugin.csi.alibabacloud.com   true       false            false             <unset>        false               Persistent   30d

# 检查 CSI Controller Pod 是否 Running
kubectl get pods -n kube-system | grep -E "csi-provisioner|csi-controller"

# 检查 CSI Controller 日志
kubectl logs -n kube-system <csi-controller-pod> -c csi-provisioner --tail=200

# 对于内置 Provisioner（非 CSI），检查云厂商控制器
kubectl get pods -n kube-system | grep -E "cloud-contro
...(截断)

## 相关链接

- [[skills/manage-persistent-storage|持久化存储管理]]
- [[skills/backup-restore-etcd|etcd 备份恢复]]

## Related

- [[skills/ts-cluster-operations|ts-cluster-operations]] — 集群运维故障排查
- [[entities/kubelet|kubelet]] — kubelet
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
