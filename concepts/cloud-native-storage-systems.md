---
title: 云原生存储系统对比
summary: 云原生存储系统对比：以及运维团队通过 CLI/GUI 逐卷管理。扩展需要采购新设备，故障域大。
category: concepts
tags:
- storage
- longhorn
- rook
- ceph
- openebs
- juicefs
- k8s
tier: supporting
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
related:
- '[[concepts/csi-drivers.md|csi drivers]]'
- '[[concepts/storageclass.md|storageclass]]'
- '[[concepts/pv.md|pv]]'
- '[[domain-19-landscape-references/98-merged-indexes/index.md|index]]'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云原生存储系统对比

## 1. 概述：云原生存储 vs 传统存储

**传统存储**（SAN、NAS、iSCSI 阵列）为静态工作负载设计，依赖集中式硬件、手动配置、
以及运维团队通过 CLI/GUI 逐卷管理。扩展需要采购新设备，故障域大。

**云原生存储**的设计原则：

| 维度 | 传统存储 | 云原生存储 |
|------|---------|-----------|
| 供给方式 | 人工预配 | CSI 驱动 + StorageClass 动态供给 |
| 扩展模型 | 纵向（换更大阵列） | 横向（加节点/磁盘） |
| 数据调度 | 与计算分离、静态绑定 | 跟随 Pod 调度、拓扑感知 |
| 故障域 | 控制器双活 | 分布式多副本、自动修复 |
| API | SNMP / CLI / GUI | Kubernetes CRD + Operator |
| 升级 | 停机窗口 | 滚动升级、零停机 |

云原生存储通过 [[concepts/csi-drivers.md|csi drivers]] 标准化接口暴露能力，配合 [[concepts/storageclass.md|storageclass]]
实现声明式供给，最终以 [[concepts/pv.md|pv]] 绑定到工作负载。

> 相关领域索引：[[domain-19-landscape-references/98-merged-indexes/index.md|index]]

---

## 2. Longhorn v1.7.x

> Rancher/SUSE 出品，轻量级分布式块存储，CNCF 孵化项目。

### 2.1 架构

```
┌─────────────────────────────────────────┐
│           Longhorn Manager (CSI)        │
│   ┌───────────┐  ┌───────────┐         │
│   │ Volume 1  │  │ Volume 2  │  ...    │
│   │ Replica A │  │ Replica A │         │
│   │ Replica B │  │ Replica B │         │
│   │ Replica C │  │ Replica C │         │
│   └───────────┘  └───────────┘         │
│         iSCSI Target (tgt)              │
│         → 通过 iSCSI 挂载到节点         │
└─────────────────────────────────────────┘
```

- **数据面**：每个卷在 N 个节点创建 N 个副本（默认 3），通过 iSCSI 协议暴露给宿主节点
- **控制面**：Longhorn Manager 以 DaemonSet 运行，Watch CRD 卷状态并驱动副本同步
- **快照/备份**：支持增量快照，可备份到 S3/NFS

### 2.2 生产最佳实践

- 每个磁盘/分区独立挂载点，避免使用根文件系统
- 设置 `storage-overprovisioning-percentage` ≤ 100 防止超额分配
- 开启 `create-default-disk-label` 让 Longhorn 自动发现裸盘
- 启用 `guaranteed-instance-manager-cpu` 预留引擎 CPU（建议 250m）
- 使用 `nodeSelector` 或 `diskSelector` 精确控制副本分布

### 2.3 性能特征

| 指标 | 参考值 |
|------|--------|
| 顺序读开销 | ~5-8%（vs 本地盘） |
| 随机写开销 | ~10-15%（多副本同步写） |
| 推荐副本数 | 3（生产）、2（开发） |
| 最大卷大小 | 实测 10TiB（受 iSCSI 重建时间制约） |
| 快照性能 | 增量快照，对 IOPS 影响 <5% |

瓶颈来源：iSCSI tgt 进程 CPU 开销 + 副本间网络同步写。

### 2.4 HA 模式

- 副本因子 N 决定容忍 N-1 节点故障
- `replicaAutoBalance` 设为 `least-effort` 或 `best-effort` 自动再平衡
- 副本重建在后台执行，带宽限制可配置（避免占满业务带宽）
- 支持 `dataLocality: strict-local`（单副本本地卷，适合临时数据）

### 2.5 升级策略

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://raw.githubusercontent.com/longhorn/longhorn/v1.7.x/deploy/longhorn.yaml
```
- Longhorn 支持滚动升级，引擎逐卷升级（engine image 前后兼容）
- 升级前确保所有卷 healthy，无正在进行的重建
- 可通过 Longhorn UI 或 `kubectl` 触发 volume engine 升级
- 大版本升级（如 1.6→1.7）需阅读 release notes 中的 breaking changes

### 2.6 已知陷阱

1. **iSCSI 性能天花板**：单卷 IOPS 受 tgt 进程单线程限制，不适合高 IOPS OLTP
2. **重建风暴**：节点宕机后 N 个卷同时重建，磁盘和网络瞬间打满 → 设置 `concurrent-replica-rebuild-per-node-limit`
3. **Node Drain 卡住**：需先 evict 副本，否则 PVC detach 超时 → 使用 `node.kubernetes.io/unschedulable` 注解配合
4. **GKE Autopilot 不兼容**：需要特权 iSCSI init 容器
5. **加密卷升级停顿**：旧版加密卷升级需手动转换

---

## 3. Rook/Ceph v1.14+ / Ceph Reef / Squid

> 企业级分布式存储，Ceph 是事实标准；Rook 是 K8s 原生 Operator。

### 3.1 架构

```
┌──────────────────────────────────────────────────┐
│              Rook Operator (CRD 驱动)             │
│                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │   OSD    │  │   OSD    │  │   OSD    │      │
│  │ (Ceph Reef/Squid)      │  │         │      │
│  └──────────┘  └──────────┘  └──────────┘      │
│                                                  │
│  ┌──────────┐  ┌──────────┐                      │
│  │   MGR    │  │  MON x3  │  ← 控制面           │
│  └──────────┘  └──────────┘                      │
│                                                  │
│  ┌──────────┐  ┌──────────┐                      │
│  │   MDS    │  │   RGW    │  ← 文件/对象网关     │
│  │ (CephFS) │  │  (S3)    │                      │
│  └──────────┘  └──────────┘                      │
│                                                  │
│  Ceph CSI → RBD / CephFS StorageClass           │
└──────────────────────────────────────────────────┘
```

- **OSD（Object Storage Daemon）**：每块磁盘一个 OSD，负责数据存储与恢复
- **MON（Monitor）**：集群状态仲裁（Paxos），需要奇数个（3 或 5）
- **MGR（Manager）**：Prometheus exporter + Dashboard + 模块扩展
- **Rook Operator**：Watch CephCluster CRD，自动化 OSD 部署/扩缩/升级

### 3.2 生产最佳实践

- **最少 5 个节点**，每个节点至少 1 块专用 OSD 盘（建议 NVMe/SAS，避免 SATA HDD 混用）
- 使用 `deviceClass` 区分 `hdd`、`ssd`、`nvme`，创建不同 CRUSH 规则
- MON 至少 3 个且分布在不同故障域（zone/rack）
- CephFS 元数据池必须用 SSD/NVMe（`metadataDevice`）
- 配置 `osd_memory_target`（默认 4GB/OSD），按实际内存调整
- 启用 `ceph osd crush rule` 按机架拓扑分布副本
- Rook v1.14+ 支持 `CephObjectStore` 直接暴露 S3

### 3.3 性能

| 场景 | 参考值（NVMe 集群） |
|------|---------------------|
| RBD 顺序读 | 接近本地 NVMe（<5% 开销） |
| RBD 随机 4K 写 | ~50K-100K IOPS / OSD（NVMe） |
| CephFS 吞吐 | 受 MDS 限制，单活 MDS ~2-5 GB/s |
| 对象存储 RGW | 多网关线性扩展 |
| 最大集群规模 | 10,000+ OSD（生产验证） |

Ceph 在大规模集群中吞吐性能最高，但配置复杂度也最高。

### 3.4 HA 模式

- **RBD**：默认 3 副本（可配 erasure coding，如 k=4 m=2 → 耐 2 盘故障，节省 33% 空间）
- **CephFS**：多活 MDS（`active_count: 3`），自动 failover
- **RGW**：多 zone 多网关，支持异地容灾
- **自动恢复**：OSD 宕机后 Ceph 自动 rebalance，恢复速度受 `recovery_max_active` 控制

### 3.5 升级策略

Rook v1.14 升级路径：

1. 升级 Rook Operator（滚动重启 operator Pod）
2. Operator 自动升级 Ceph 镜像（逐 OSD 滚动重启）
3. 验证 `ceph health` 为 `HEALTH_OK` 后继续
4. Ceph 大版本升级（如 Reef → Squid）需设置 `ceph upgrade_policy`

关键注意事项：
- 升级期间集群保持在线，但性能下降 10-20%
- 不要同时升级 Rook 和 Ceph 大版本
- 永远在升级前做 `ceph osd pool` 快照/备份

### 3.6 已知陷阱

1. **最小节点要求**：3 节点集群只有 2 个 OSD 节点时 Ceph 无法维护 3 副本
2. **HDD 混合部署**：慢盘拖垮整个 PG（Placement Group）恢复速度
3. **MDS 内存泄漏**：CephFS 大目录场景下旧版 MDS 有内存问题 → 升级到 Squid
4. **OSD 重启风暴**：升级或节点恢复时多个 OSD 同时重启 → 控制 `maxUnavailable`
5. **CRUSH 规则错误**：错误的故障域配置导致数据分布不均

---

## 4. OpenEBS v4.0+ Mayastor

> 软件定义的 NVMe 存储，追求本地盘级别的低延迟。

### 4.1 架构

```
┌─────────────────────────────────────────────┐
│           OpenEBS Control Plane             │
│   (Kubernetes Operator + CSI Plugin)        │
│                                             │
│   ┌───────────────────────────────────┐     │
│   │     Mayastor (per-node Daemon)    │     │
│   │   NVMe Target + NVMe-oF Exporter │     │
│   │   → SPDK 用户态存储引擎          │     │
│   └───────────────────────────────────┘     │
│                                             │
│   NVMe-oF (RDMA / TCP) 连接池              │
│   ┌────────┐  ┌────────┐  ┌────────┐      │
│   │Node A  │  │Node B  │  │Node C  │      │
│   │NVMe盘  │  │NVMe盘  │  │NVMe盘  │      │
│   └────────┘  └────────┘  └────────┘      │
└─────────────────────────────────────────────┘
```

- **SPDK 引擎**：用户态 NVMe 驱动，绕过内核 IO 路径，延迟极低
- **NVMe-oF**：通过 NVMe over Fabrics（TCP 或 RDMA）将远程盘暴露为本地 NVMe 设备
- **IoEngine**：每个节点运行的 IO 处理进程，负责复制和数据路径
- **Mayastor Control Plane（MCP）**：卷调度、副本放置、故障检测

### 4.2 生产最佳实践

- 节点需支持 `io_uring` 和 NVMe 控制器（裸盘或 NVMe namespace）
- RDMA 网络可显著降低延迟（需 RoCE v2 / InfiniBand 网卡）
- 每节点预留 2-4 CPU 核心给 Mayastor IoEngine
- 使用 `DiskPool` CRD 管理存储池

### 4.3 性能

| 指标 | 参考值 |
|------|--------|
| 本地单副本延迟 | ~10-20 μs（NVMe RDMA） |
| 3 副本远程延迟 | ~100-200 μs（NVMe TCP） |
| IOPS（单引擎） | ~500K+（4K 随机读） |
| 对比本地盘 | <5% 开销（本地副本） |

### 4.4 成熟度说明

- OpenEBS Mayastor 自 v4.0 稳定，但生态和文档仍在追赶 Longhorn/Ceph
- 社区规模较小，生产案例相对有限
- 适合对延迟极度敏感且有 NVMe 硬件预算的场景
- 升级流程尚不如 Ceph/Longhorn 成熟，建议在非关键路径先验证

---

## 5. JuiceFS v1.2.x

> 云原生分布式 POSIX 文件系统，元数据与数据分离架构。

### 5.1 架构

```
┌─────────────────────────────────────────────────┐
│              JuiceFS CSI Driver                 │
│                                                 │
│   ┌──────────────┐    ┌──────────────────────┐ │
│   │  元数据引擎   │    │    数据存储后端       │ │
│   │  Redis/       │    │    S3 / MinIO /      │ │
│   │  TiKV/SQLite  │    │    OSS / COS / HDFS  │ │
│   └──────────────┘    └──────────────────────┘ │
│                                                 │
│   FUSE 客户端（per-Pod Mount）                  │
│   → POSIX 语义，多 Pod 共享同一文件系统          │
└─────────────────────────────────────────────────┘
```

- **元数据**：支持 Redis（生产推荐）、TiKV（大规模）、SQLite（测试）
- **数据**：分块存储到对象存储（S3、MinIO、阿里云 OSS 等）
- **POSIX 兼容**：标准文件操作（`open`/`read`/`write`/`seek`/`mmap`）
- **CSI 驱动**：通过 FUSE 挂载到 Pod

### 5.2 典型场景

**适合：**
- AI/ML 训练共享数据集（多节点只读访问 PB 级数据）
- 日志收集与分析管道（Hadoop/Spark 共享输入输出）
- Web 静态资源分发（高并发只读）

**不适合：**
- 数据库工作负载（随机小 IO、FUSE 延迟 50-200μs vs 块存储 10μs）
- 高频元数据操作（大量 `stat`/`readdir` → 元数据引擎瓶颈）
- 需要强一致快照的场景

### 5.3 性能

| 指标 | 参考值 |
|------|--------|
| 顺序读吞吐 | 取决于对象存储带宽（S3: ~10 GB/s 无上限） |
| 顺序写吞吐 | 受客户端缓存和对象上传限制 |
| 随机读延迟 | 50-200 μs（FUSE 开销）+ 网络 |
| 元数据操作 | Redis 后端 ~10K ops/s |

### 5.4 注意事项

- 元数据引擎是单点故障源 → Redis 必须做 Sentinel/Cluster
- FUSE 挂载在 Pod 内运行，Pod 重启需要重新挂载
- 不支持 `ReadWriteOncePod`（设计为 ReadWriteMany）
- 本地缓存（`--cache-dir`）对热数据性能影响巨大

---

## 6. 对比矩阵

| 维度 | Longhorn | Rook/Ceph | OpenEBS Mayastor | JuiceFS |
|------|----------|-----------|-------------------|---------|
| **部署简单性** | ⭐⭐⭐⭐⭐ 最简单 | ⭐⭐ 复杂 | ⭐⭐⭐ 中等 | ⭐⭐⭐⭐ 较简单 |
| **运维难度** | 低 | 高 | 中 | 中 |
| **最低延迟** | 中等（iSCSI） | 低（NVMe OSD） | ⭐⭐⭐⭐⭐ 最低（SPDK） | 高（FUSE） |
| **最高吞吐** | 中等 | ⭐⭐⭐⭐⭐ 最高 | 高 | 高（对象存储带宽） |
| **最大规模** | ~500 卷/节点 | 10,000+ OSD | 数百节点 | PB 级数据集 |
| **多节点访问** | ❌ RWO | ❌ RWO（RBD）/ ✅ RWX（CephFS） | ❌ RWO | ✅ RWX |
| **数据类型** | 块存储 | 块+文件+对象 | 块存储 | POSIX 文件 |
| **加密支持** | ✅ | ✅ | ✅ | ✅（后端加密） |
| **快照/备份** | ✅ 增量+远程 | ✅ RBD 快照 | ✅ | 对象存储版本控制 |
| **CNCF 状态** | 孵化 | 无（Ceph 独立） | 孵化 | 沙箱 |
| **适用场景** | 通用有状态应用 | 大规模企业存储 | 超低延迟数据库 | AI/ML 共享数据 |
| **不适合** | 高 IOPS OLTP | 小集群(<5节点) | 不需要极致延迟 | 数据库 |

---

## 7. 混合策略建议

在生产集群中，推荐使用多层 StorageClass 匹配不同工作负载：

### 推荐分层方案

```
┌─────────────────────────────────────────────────────────┐
│                     StorageClass 分层                    │
├─────────────────┬───────────────────┬───────────────────┤
│  层级           │  StorageClass     │  工作负载         │
├─────────────────┼───────────────────┼───────────────────┤
│  超低延迟层      │  openebs-mayastor │  etcd、Redis、    │
│  (NVMe)         │  (本地 NVMe)      │  Kafka WAL       │
├─────────────────┼───────────────────┼───────────────────┤
│  通用块存储层    │  longhorn-ssd     │  PostgreSQL、     │
│  (SSD)          │                   │  MySQL、应用数据   │
├─────────────────┼───────────────────┼───────────────────┤
│  高吞吐块存储层  │  ceph-rbd-nvme   │  ClickHouse、     │
│  (NVMe 集群)    │                   │  时序数据库       │
├─────────────────┼───────────────────┼───────────────────┤
│  共享文件层      │  juicefs-ai-data  │  ML 训练数据集、  │
│  (RWX)          │  或 cephfs-ssd    │  日志聚合、HPC    │
├─────────────────┼───────────────────┼───────────────────┤
│  对象存储层      │  ceph-rgw / MinIO │  静态资源、备份、 │
│                 │                   │  归档             │
└─────────────────┴───────────────────┴───────────────────┘
```

### 实施建议

1. **默认 StorageClass** 设为 `longhorn`（简单可靠，覆盖 80% 场景）
2. **数据库类** Pod 通过 `storageClassName` 显式选择低延迟层
3. **AI 训练任务** 使用 `ReadWriteMany` JuiceFS 卷共享数据集
4. **监控/Prometheus** 使用 `local-path` 或 Longhorn（低 IO 需求）
5. **备份** 统一通过 Velero + 对象存储，Longhorn 快照 + Ceph 快照做二级保护

### StorageClass 示例（Longhorn）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: longhorn-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: driver.longhorn.io
parameters:
  numberOfReplicas: "3"
  staleReplicaTimeout: "30"
  fromBackup: ""
  dataLocality: "best-effort"
  fsType: "ext4"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: Immediate
```

### StorageClass 示例（Ceph RBD）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-rbd-nvme
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: nvme-ec-pool
  imageFormat: "2"
  imageFeatures: layering,exclusive-lock,object-map,fast-diff
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer   # 拓扑感知
```

---

## 参考链接

- [Longhorn 官方文档](https://longhorn.io/docs/)
- [Rook Ceph 文档](https://rook.io/docs/rook/latest/)
- [OpenEBS Mayastor](https://openebs.io/docs/)
- [JuiceFS 文档](https://juicefs.com/docs/)
- [[concepts/csi-drivers.md|csi drivers]] — CSI 驱动规范与实现
- [[concepts/storageclass.md|storageclass]] — StorageClass 配置详解
- [[concepts/pv.md|pv]] — 持久卷生命周期
- [[domain-19-landscape-references/98-merged-indexes/index.md|index]] — 存储与数据领域索引

## Related

- [[concepts/csi-drivers.md|csi drivers]] — CSI 驱动规范与实现
- [[concepts/storage-performance-optimization.md|storage performance optimization]] — 存储性能优化策略
- [[concepts/storage-data-protection.md|storage data protection]] — 存储数据保护与灾备

```

<!-- risk-assessed -->
