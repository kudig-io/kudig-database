---
title: 存储 I/O 性能故障排查指南 [topic-structural-trouble-shooting]
description: 'title: 存储 I/O 性能故障排查指南'
summary: 'title: 存储 I/O 性能故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- storage
- performance
- kubelet
- scheduler
- prometheus
- docker
- mysql
- postgresql
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 存储 I/O 性能故障排查指南 是什么
- 如何 存储 I/O 性能故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 存储 I/O 性能故障排查指南 故障排查
- 存储 I/O 性能故障排查指南 排障步骤
trigger_keywords:
- 存储
- 性能故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- mysql-basics
- logging-basics
---



title: 存储 I/O 性能故障排查指南
description: '# 存储 I/O 性能故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- scheduler
- [[Prometheus|prometheus]]
- mysql
- postgresql
- elasticsearch
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 存储 I/O 性能故障排查指南 是什么
- 如何 存储 I/O 性能故障排查指南
- 存储 I/O 性能故障排查指南 故障排查
- 存储 I/O 性能故障排查指南 排障步骤
trigger_keywords:
- 存储
- 性能故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 存储 I/O 性能故障排查指南

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

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

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 高延迟 I/O

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 数据库查询缓慢 | `slow query: execution time > 1000ms` | 数据库日志 | MySQL/PostgreSQL 慢查询日志 |
| 文件操作超时 | `context deadline exceeded` | 应用日志 | 应用 Pod 日志 |
| 容器启动缓慢 | `MountVolume.SetUp failed: ... timeout` | kubelet Events | `kubectl describe pod` |
| 日志写入阻塞 | `rsyslog: action suspended` | 系统日志 | 节点 `journalctl` |
| 页面加载缓慢 | `File read took 5+ seconds` | 应用日志 | 应用性能监控 (APM) |

#### 1.1.2 吞吐量瓶颈

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 大文件传输缓慢 | `transfer speed < 10MB/s` | 应用/工具输出 | `pv`、`dd` 命令 |
| 备份任务超时 | `backup job exceeded deadline` | CronJob Events | `kubectl describe job` |
| 索引构建缓慢 | `index creation progress stalled` | 数据库日志 | Elasticsearch/MongoDB 日志 |
| 批量写入队列堆积 | `wal flush queue depth > 1000` | 数据库指标 | Prometheus 指标 |

#### 1.1.3 存储饱和与抖动

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 磁盘 I/O 利用率 100% | `%util = 100.00` | `iostat` | 节点 `iostat -x` |
| I/O 等待时间飙升 | `await > 100ms` | `iostat` | 节点 `iostat -x` |
| 存储后端限流 | `throttling: rate limit exceeded` | CSI 驱动日志 | CSI driver Pod 日志 |
| 网络存储延迟波动 | `NFS/ISCSI latency spikes` | 网络监控 | `ping`、`nfsstat` |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **电商大促期间数据库卡死** | 订单服务响应时间从 10ms 飙升至 5s+ | 共享存储阵列 IOPS 被其他业务挤占 | 使用本地 SSD 或 dedicated storage pool |
| **日志系统写入阻塞** | Fluentd/Fluent Bit 出现 `buffer full` | 日志 PVC 吞吐不足，单盘写入饱和 | 增加 buffer 分区，使用更高性能存储类 |
| **CI/CD 构建缓存失效** | 构建时间从 3 分钟增加到 30 分钟 | 缓存 PVC 使用网络存储，高并发下延迟激增 | 将缓存迁移到节点本地 ephemeral storage |
| **监控数据写入丢失** | Prometheus `wal truncation` 失败 | TSDB 存储磁盘 I/O 不足 | 为 Prometheus 配置独立的高性能磁盘 |

### 1.2 报错查看方式汇总

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 节点级 I/O 监控
iostat -x 1 10
iotop -aoP

# 文件系统检查
df -h
df -i
xfs_info /mnt/data  # XFS 文件系统
dumpe2fs /dev/sdb1  # ext4 文件系统

# PVC 和 Pod 存储事件
kubectl get events --field-selector reason=FailedMount --sort-by='.lastTimestamp'
kubectl get events --field-selector reason=FailedMapVolume --sort-by='.lastTimestamp'

# CSI 驱动性能指标（如已部署监控）
curl -s http://csi-metrics-endpoint:8080/metrics | grep csi_sidecar_operations_seconds

# 容器内 I/O 测试（需要特权或特定镜像）
kubectl exec -it <pod> -- ioping -c 10 /data
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

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

### 2.2 排查逻辑决策树

```
存储 I/O 性能问题
    ├── 高延迟 (>50ms)
    │   ├── 文件系统层
    │   │   ├── inode 耗尽？──► 扩容或清理小文件
    │   │   ├── journal 模式导致？──► 切换为 writeback（权衡安全性）
    │   │   └── 碎片严重？──► 整理碎片或重建文件系统
    │   ├── 块设备/调度器层
    │   │   ├── I/O scheduler 不合适？──► NVMe 用 none，HDD 用 mq-deadline
    │   │   └── read-ahead 不足？──► 调大 /sys/block/xxx/queue/read_ahead_kb
    │   ├── 协议/网络层（网络存储）
    │   │   ├── 网络延迟高？──► 优化网络拓扑或使用 RDMA
    │   │   └── NFS 参数不当？──► 调整 rsize/wsize, async
    │   └── 存储后端层
    │       ├── 云盘 IOPS 限制？──► 升级云盘类型或开启 burst
    │       └── 存储池争用？──► 使用 dedicated pool 或本地盘
    ├── 低吞吐量 (<预期值)
    │   ├── 应用 I/O 模式不匹配？──► 优化应用为顺序读写
    │   ├── 单线程瓶颈？──► 增加并发 I/O 线程
    │   └── 存储带宽限制？──► 升级存储规格或条带化
    └── I/O 饱和/抖动
        ├── 多 Pod 争抢同一磁盘？──► 分散到不同节点/磁盘
        ├── 节点级 I/O 风暴？──► 限制 Pod I/O cgroup (blkio)
        └── 后台任务影响？──► 调整 cron 时间窗口或限速
```

### 2.3 详细诊断命令

#### 节点级 I/O 诊断

```bash
#!/bin/bash
# 节点级 I/O 诊断脚本
# 建议在节点上直接运行

echo "=== 节点级 I/O 诊断 ==="

# 1. 块设备列表和类型
echo "1. 块设备信息:"
lsblk -d -o NAME,SIZE,TYPE,ROTA,MODEL,SCHED

# 2. 磁盘 I/O 统计（扩展统计）
echo ""
echo "2. 磁盘 I/O 统计 (iostat -x 1 5):"
iostat -x 1 5 | tail -n +4

# 3. 实时 I/O 进程排名
echo ""
echo "3. I/O 进程排名 (iotop):"
if command -v iotop &>/dev/null; then
  sudo iotop -aoP -n 5 -d 1 2>/dev/null | head -20
else
  echo "  ⚠ iotop 未安装，使用 pidstat 替代:"
  pidstat -d 1 5 2>/dev/null | tail -n +4 | head -20
fi

# 4. 文件系统挂载参数
echo ""
echo "4. 文件系统挂载参数:"
findmnt -lo TARGET,SOURCE,FSTYPE,OPTIONS | grep -E "ext4|xfs|btrfs"

# 5. I/O 调度器
echo ""
echo "5. I/O 调度器配置:"
for disk in $(lsblk -d -n -o NAME | grep -E "^sd|^nvme|^vd"); do
  if [ -f /sys/block/$disk/queue/scheduler ]; then
    SCHEDULER=$(cat /sys/block/$disk/queue/scheduler | grep -oP '\[\K[^]]+')
    READ_AHEAD=$(cat /sys/block/$disk/queue/read_ahead_kb)
    NR_REQUESTS=$(cat /sys/block/$disk/queue/nr_requests)
    echo "  $disk: scheduler=$SCHEDULER, read_ahead=${READ_AHEAD}KB, nr_requests=$NR_REQUESTS"
  fi
done

# 6. 内存和缓存状态
echo ""
echo "6. 内存/缓存状态:"
free -h
echo "  Dirty/Writeback:"
cat /proc/meminfo | grep -E "^(Dirty|Writeback|Cached|Buffers)"

# 7. 检查是否有 I/O 错误
echo ""
echo "7. 块设备 I/O 错误:"
dmesg | grep -iE "I/O error|sector|buffer" | tail -10
```

#### PVC 级 I/O 基准测试 (fio)

```bash
#!/bin/bash
# PVC 级 I/O 基准测试脚本
# 在目标 Pod 中运行，需要 fio 工具

TEST_DIR=${1:-/data}
TEST_SIZE=${2:-1G}

echo "=== PVC I/O 基准测试 ==="
echo "测试目录: $TEST_DIR"
echo "测试数据量: $TEST_SIZE"

# 检查 fio
if ! command -v fio &>/dev/null; then
  echo "✗ fio 未安装，正在尝试安装..."
  apt-get update &>/dev/null && apt-get install -y fio &>/dev/null || \
    yum install -y fio &>/dev/null || \
    echo "✗ 无法自动安装 fio，请手动安装"
fi

cd $TEST_DIR || exit 1

# 1. 随机读 IOPS 测试 (4K)
echo ""
echo "1. 随机读 IOPS (4K, 随机, 深度 32):"
fio --name=randread --directory=$TEST_DIR --ioengine=libaio \
    --iodepth=32 --rw=randread --bs=4k --direct=1 --size=$TEST_SIZE \
    --numjobs=4 --runtime=60 --group_reporting \
    --output-format=json | jq '.jobs[0].read.iops'

# 2. 随机写 IOPS 测试 (4K)
echo ""
echo "2. 随机写 IOPS (4K, 随机, 深度 32):"
fio --name=randwrite --directory=$TEST_DIR --ioengine=libaio \
    --iodepth=32 --rw=randwrite --bs=4k --direct=1 --size=$TEST_SIZE \
    --numjobs=4 --runtime=60 --group_reporting \
    --output-format=json | jq '.jobs[0].write.iops'

# 3. 顺序读吞吐测试 (1M)
echo ""
echo "3. 顺序读吞吐 (1M, 顺序):"
fio --name=seqread --directory=$TEST_DIR --ioengine=libaio \
    --iodepth=16 --rw=read --bs=1m --direct=1 --size=$TEST_SIZE \
    --numjobs=2 --runtime=60 --group_reporting \
    --output-format=json | jq '.jobs[0].read.bw_bytes / 1024 / 1024'

# 4. 顺序写吞吐测试 (1M)
echo ""
echo "4. 顺序写吞吐 (1M, 顺序):"
fio --name=seqwrite --directory=$TEST_DIR --ioengine=libaio \
    --iodepth=16 --rw=write --bs=1m --direct=1 --size=$TEST_SIZE \
    --numjobs=2 --runtime=60 --group_reporting \
    --output-format=json | jq '.jobs[0].write.bw_bytes / 1024 / 1024'

# 5. 混合读写测试 (70/30)
echo ""
echo "5. 混合读写延迟 (70% 读, 30% 写, 4K):"
fio --name=rwmix --directory=$TEST_DIR --ioengine=libaio \
    --iodepth=1 --rw=randrw --rwmixread=70 --bs=4k --direct=1 \
    --size=$TEST_SIZE --numjobs=1 --runtime=60 --group_reporting \
    --output-format=json | jq '{read_lat_ms: .jobs[0].read.lat_ns.mean / 1000000, write_lat_ms: .jobs[0].write.lat_ns.mean / 1000000}'

# 清理测试文件
rm -f $TEST_DIR/randread.* $TEST_DIR/randwrite.* $TEST_DIR/seqread.* $TEST_DIR/seqwrite.* $TEST_DIR/rwmix.*

echo ""
echo "=== 测试完成 ==="
```

#### CSI 驱动性能指标采集

```bash
#!/bin/bash
# CSI 驱动性能指标采集

echo "=== CSI 驱动性能指标 ==="

# 查找 CSI 驱动的 metrics endpoint
for pod in $(kubectl get pods -n kube-system -o name | grep -E "csi|driver"); do
  # 检查是否有 metrics 端口暴露
  PORTS=$(kubectl get $pod -n kube-system -o json | jq -r '.spec.containers[].ports[]?.containerPort' 2>/dev/null | tr '\n' ' ')
  if [ -n "$PORTS" ]; then
    echo "$pod 端口: $PORTS"
  fi
done

# 如果使用 Prometheus，查询关键指标
echo ""
echo "Prometheus 查询示例:"
echo "  1. CSI 操作延迟: histogram_quantile(0.99, rate(csi_sidecar_operations_seconds_bucket[5m]))"
echo "  2. CSI 操作错误率: rate(csi_sidecar_operations_total{status=\"Failed\"}[5m])"
echo "  3. PV 创建延迟: histogram_quantile(0.99, rate(storage_operation_duration_seconds_bucket{operation_name=\"volume_provision\"}[5m]))"

# 通过 kubelet metrics 查看卷操作延迟
echo ""
echo "Kubelet 卷操作指标:"
kubectl get --raw /api/v1/nodes/$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')/proxy/metrics 2>/dev/null | \
  grep -E "storage_operation_duration_seconds|volume_manager" | head -10
```

---

## 3. 解决方案与风险控制

### 3.1 存储类性能优化

#### 方案一：高性能 StorageClass 配置

```yaml
# 高性能本地 NVMe StorageClass（适用于延迟敏感型应用）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-nvme-fast
provisioner: kubernetes.io/no-provisioner  # 使用 local-static-provisioner
volumeBindingMode: WaitForFirstConsumer
# local 卷不支持动态供给，需预先创建 PV
---
# 云厂商高性能 SSD StorageClass（AWS EBS io2 示例）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-io2-high-performance
provisioner: ebs.csi.aws.com
parameters:
  type: io2
  # IOPS 与容量比例，io2 最高支持 500 IOPS/GB
  iopsPerGB: "100"
  # 加密（可选）
  encrypted: "true"
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
# reclaimPolicy: Retain  # 如需保留数据
---
# 通用 SSD StorageClass（吞吐优化）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: premium-ssd-throughput
provisioner: pd.csi.storage.gke.io  # GKE 示例
parameters:
  type: pd-ssd
  replication-type: regional
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

#### 方案二：Pod I/O 隔离与限制

```yaml
# 使用 blkio 限制 Pod I/O（需要容器运行时支持）
apiVersion: v1
kind: Pod
metadata:
  name: io-limited-app
spec:
  containers:
  - name: app
    image: postgres:15
    resources:
      limits:
        # CPU/内存限制
        cpu: "4"
        memory: "16Gi"
        # 部分容器运行时支持 blkio 限制
        # ephemeral-storage: "100Gi"
    volumeMounts:
    - name: data
      mountPath: /var/lib/postgresql/data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: postgres-pvc
  # 节点亲和性：确保调度到具有本地 SSD 的节点
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: node.kubernetes.io/instance-type
            operator: In
            values:
            - "i3.2xlarge"  # AWS I3 实例带 NVMe SSD
```

#### 方案三：文件系统调优

```bash
#!/bin/bash
# 文件系统调优脚本（在节点上以 root 执行）
# 适用于高性能数据库/消息队列场景

DISK=${1:-nvme0n1}
MOUNT_POINT=${2:-/data}
FS_TYPE=$(findmnt -n -o FSTYPE $MOUNT_POINT)

echo "=== 文件系统调优: $MOUNT_POINT ($FS_TYPE on $DISK) ==="

if [ "$FS_TYPE" = "xfs" ]; then
  echo "1. XFS 文件系统调优:"
  # 增加日志缓冲区大小
  xfs_admin -l $MOUNT_POINT 2>/dev/null
  
  # 挂载选项优化
  echo "  建议挂载选项:"
  echo "    noatime,nodiratime,logbufs=8,logbsize=256k,largeio,inode64"
  
  # 如果尚未挂载，使用优化选项重新挂载
  mount -o remount,noatime,nodiratime $MOUNT_POINT
  echo "  ✓ 已重新挂载为 noatime,nodiratime"

elif [ "$FS_TYPE" = "ext4" ]; then
  echo "1. ext4 文件系统调优:"
  # 关闭日志（仅适用于可重建的数据，如缓存）
  # tune2fs -O ^has_journal /dev/$DISK
  
  # 增加预留块百分比（默认 5%，对于大容量盘可减少）
  tune2fs -m 1 /dev/$DISK
  echo "  ✓ 预留块比例已调整为 1%"
  
  # 挂载选项优化
  mount -o remount,noatime,nodiratime,data=writeback $MOUNT_POINT
  echo "  ✓ 已重新挂载为 noatime,nodiratime,data=writeback"
fi

# 2. I/O 调度器优化
echo ""
echo "2. I/O 调度器优化:"
if "$DISK" == nvme*; then
  # NVMe 设备使用 'none' 调度器（多队列原生优化）
  echo none > /sys/block/$DISK/queue/scheduler
  echo "  ✓ $DISK 调度器已设置为 none (NVMe 优化)"
else
  # SATA/SAS SSD 使用 'mq-deadline'
  echo mq-deadline > /sys/block/$DISK/queue/scheduler
  echo "  ✓ $DISK 调度器已设置为 mq-deadline"
fi

# 3. 增加 read-ahead
echo ""
echo "3. 读取预读优化:"
# 对于顺序读场景（如日志、大文件），增加 read-ahead
echo 8192 > /sys/block/$DISK/queue/read_ahead_kb
echo "  ✓ read_ahead 已设置为 8192 KB"

# 4. VM 缓存调优
echo ""
echo "4. 系统缓存调优:"
# 降低脏页比例，减少突发写入
echo 5 > /proc/sys/vm/dirty_ratio
echo 2 > /proc/sys/vm/dirty_background_ratio
echo "  ✓ dirty_ratio=5%, dirty_background_ratio=2%"

# 5. 验证调优结果
echo ""
echo "5. 调优结果验证:"
echo "  调度器: $(cat /sys/block/$DISK/queue/scheduler | grep -oP '\[\K[^]]+')"
echo "  read_ahead: $(cat /sys/block/$DISK/queue/read_ahead_kb) KB"
echo "  dirty_ratio: $(cat /proc/sys/vm/dirty_ratio)"
```

### 3.2 网络存储（NFS）性能优化

```yaml
# NFS PVC 性能优化示例
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nfs-high-performance
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteMany
  nfs:
    server: 10.0.0.10
    path: /exports/highperf
    # 高级挂载选项
  mountOptions:
    - hard           # 硬挂载，I/O 失败时重试而非返回错误
    - intr           # 允许中断硬挂载的等待
    - nolock         # 不使用 NFS 锁（如应用自身已处理并发）
    - noatime        # 不更新访问时间
    - nodiratime     # 不更新目录访问时间
    - rsize=1048576  # 读块大小 1MB
    - wsize=1048576  # 写块大小 1MB
    - tcp            # 使用 TCP（比 UDP 更可靠）
    - timeo=600      # 超时时间（deciseconds）
    - retrans=2      # 重试次数
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nfs-highperf-pvc
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: ""  # 空字符串绑定到静态 PV
  resources:
    requests:
      storage: 100Gi
```

### 3.3 应用层 I/O 优化

```yaml
# PostgreSQL I/O 优化配置（ConfigMap 形式）
apiVersion: v1
kind: ConfigMap
metadata:
  name: postgres-io-optimized
  namespace: database
data:
  postgresql.conf: |
    # WAL 配置优化
    wal_level = replica
    wal_buffers = 16MB
    max_wal_size = 2GB
    min_wal_size = 512MB
    checkpoint_completion_target = 0.9
    checkpoint_timeout = 10min
    
    # 异步提交（权衡：轻微数据丢失风险换取更高吞吐）
    synchronous_commit = off  # 或 local
    
    # 随机页面成本（SSD 应降低此值）
    random_page_cost = 1.1    # SSD 推荐 1.1，HDD 推荐 4.0
    seq_page_cost = 1.0
    
    # 并发写入优化
    effective_io_concurrency = 200  # SSD 推荐 200，HDD 推荐 2
    maintenance_io_concurrency = 200
    
    # 大页支持（需节点配置 hugepages）
    huge_pages = try
    shared_buffers = 4GB
```

### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 切换 I/O scheduler | ⭐ 低 | 可能影响 I/O 延迟特性 | 恢复原始 scheduler：`echo xxx > /sys/block/xxx/queue/scheduler` |
| 修改文件系统挂载选项 | ⭐⭐ 中 | `noatime` 不影响功能；`data=writeback` 可能影响崩溃恢复 | 重新挂载为原始选项 |
| 调整 VM dirty_ratio | ⭐ 低 | 影响脏页刷盘频率 | 恢复原始值：`echo xx > /proc/sys/vm/dirty_ratio` |
| 更换 StorageClass | ⭐⭐ 中 | 仅影响新 PVC，现有 PVC 不变 | 恢复默认 StorageClass |
| 关闭 ext4 journal | ⭐⭐⭐ 高 | 崩溃后文件系统可能损坏 | 使用 `tune2fs -O has_journal` 恢复 |
| 应用层异步提交 | ⭐⭐ 中 | 可能丢失最近几秒数据 | 恢复 `synchronous_commit = on` |

### 3.5 验证与监控

#### I/O 性能验证脚本

```bash
#!/bin/bash
# I/O 性能验证脚本
# 在优化前后分别运行，对比结果

TEST_DIR=${1:-/data}
RESULT_FILE="/tmp/io-benchmark-$(date +%Y%m%d-%H%M%S).txt"

echo "=== I/O 性能验证 ===" | tee $RESULT_FILE
echo "测试时间: $(date)" | tee -a $RESULT_FILE
echo "测试目录: $TEST_DIR" | tee -a $RESULT_FILE
echo "" | tee -a $RESULT_FILE

# 1. 磁盘信息
echo "1. 磁盘信息:" | tee -a $RESULT_FILE
lsblk -d -o NAME,SIZE,TYPE,ROTA,MODEL | grep -E "sd|nvme|vd" | tee -a $RESULT_FILE
echo "" | tee -a $RESULT_FILE

# 2. 文件系统信息
echo "2. 文件系统信息:" | tee -a $RESULT_FILE
findmnt -n -o TARGET,SOURCE,FSTYPE,OPTIONS $TEST_DIR | tee -a $RESULT_FILE
echo "" | tee -a $RESULT_FILE

# 3. 快速 fio 测试
echo "3. 快速 I/O 测试 (30 秒):" | tee -a $RESULT_FILE

# 随机读延迟
echo "  随机读延迟 (4K, 深度 1):" | tee -a $RESULT_FILE
READ_LAT=$(fio --name=verify-read --directory=$TEST_DIR --ioengine=libaio \
  --iodepth=1 --rw=randread --bs=4k --direct=1 --size=100M \
  --numjobs=1 --runtime=30 --group_reporting --output-format=json 2>/dev/null | \
  jq '.jobs[0].read.lat_ns.mean / 1000000')
echo "    平均延迟: ${READ_LAT} ms" | tee -a $RESULT_FILE

# 随机写延迟
echo "  随机写延迟 (4K, 深度 1):" | tee -a $RESULT_FILE
WRITE_LAT=$(fio --name=verify-write --directory=$TEST_DIR --ioengine=libaio \
  --iodepth=1 --rw=randwrite --bs=4k --direct=1 --size=100M \
  --numjobs=1 --runtime=30 --group_reporting --output-format=json 2>/dev/null | \
  jq '.jobs[0].write.lat_ns.mean / 1000000')
echo "    平均延迟: ${WRITE_LAT} ms" | tee -a $RESULT_FILE

# 顺序吞吐
echo "  顺序读吞吐 (1M):" | tee -a $RESULT_FILE
SEQ_READ=$(fio --name=verify-seqread --directory=$TEST_DIR --ioengine=libaio \
  --iodepth=8 --rw=read --bs=1m --direct=1 --size=500M \
  --numjobs=1 --runtime=30 --group_reporting --output-format=json 2>/dev/null | \
  jq '.jobs[0].read.bw_bytes / 1024 / 1024')
echo "    吞吐: ${SEQ_READ} MB/s" | tee -a $RESULT_FILE

echo "" | tee -a $RESULT_FILE
echo "结果已保存到: $RESULT_FILE"

# 清理
rm -f $TEST_DIR/verify-*
```

#### Prometheus 存储性能告警

```yaml
# Prometheus 存储性能监控告警
groups:
- name: storage-performance
  rules:
  - alert: HighDiskIOUtilization
    expr: |
      rate(node_disk_io_time_seconds_total[5m]) * 100 > 90
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "磁盘 I/O 利用率过高"
      description: "节点 {{ $labels.instance }} 的磁盘 {{ $labels.device }} I/O 利用率超过 90%"

  - alert: HighDiskLatency
    expr: |
      (
        rate(node_disk_io_time_weighted_seconds_total[5m]) /
        rate(node_disk_ios_completed_total[5m])
      ) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "磁盘 I/O 延迟过高"
      description: "节点 {{ $labels.instance }} 的磁盘 {{ $labels.device }} 平均 I/O 延迟超过 100ms"

  - alert: PVCVolumeFull
    expr: |
      kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "PVC 容量即将耗尽"
      description: "命名空间 {{ $labels.namespace }} 的 PVC {{ $labels.persistentvolumeclaim }} 可用空间不足 10%"

  - alert: PVCVolumeInodesFull
    expr: |
      kubelet_volume_stats_inodes_free / kubelet_volume_stats_inodes < 0.05
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "PVC inode 即将耗尽"
      description: "命名空间 {{ $labels.namespace }} 的 PVC {{ $labels.persistentvolumeclaim }} 可用 inode 不足 5%"

  - alert: SlowStorageOperations
    expr: |
      histogram_quantile(0.99,
        rate(storage_operation_duration_seconds_bucket{status="success"}[5m])
      ) > 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "存储操作延迟过高"
      description: "存储操作 {{ $labels.operation_name }} 的 P99 延迟超过 10 秒"
```

### 3.6 最佳实践

1. **存储类选择矩阵**：
   - **数据库（OLTP）**：本地 NVMe / 云厂商 io2 类型，低延迟优先
   - **大数据/分析**：高吞吐云盘 / NFS，顺序读写优化
   - **日志/缓存**：本地 SSD 或 ephemeral storage，成本优先
   - **通用应用**：通用 SSD（gp3/pd-ssd），平衡性价比

2. **分离存储负载**：将高 I/O 应用（数据库、消息队列）与低 I/O 应用部署在不同节点，避免 I/O 争用

3. **使用节点亲和性**：为高性能存储应用配置 `nodeAffinity`，确保调度到配备 SSD/NVMe 的节点

4. **监控先行**：在应用上线前使用 `fio` 进行基准测试，建立性能基线，便于后续问题定位

5. **分层存储策略**：使用不同 StorageClass 实现热数据（高性能盘）和冷数据（标准盘）的分层

6. **避免过度分片**：ext4 文件系统在小文件场景下注意 inode 消耗，必要时使用 XFS

### 典型问题案例

#### 案例一：MySQL 容器随机 I/O 延迟飙升

**问题描述**：MySQL Pod 在业务高峰期出现大量 `慢查询`，`iostat` 显示 `%util` 接近 100%。

**根本原因**：多个 Pod 共享同一个节点上的 SATA SSD，I/O 调度器为 `mq-deadline`，多队列并发性能不佳。

**解决方案**：
1. 将 MySQL 迁移到配备 NVMe 的专用节点
2. 将 NVMe 设备的 I/O scheduler 改为 `none`
3. 增加 MySQL `innodb_io_capacity` 参数以匹配硬件能力

#### 案例二：NFS 共享卷导致构建服务间歇性卡死

**问题描述**：CI/CD 构建 Pod 在执行 `npm install`/`mvn build` 时间歇性卡死 30-60 秒。

**根本原因**：NFS 默认挂载选项 `rsize/wsize=1024`，大量小文件操作导致网络往返次数过多。

**解决方案**：
1. 将 NFS 挂载选项调整为 `rsize=1048576,wsize=1048576`
2. 对构建缓存使用节点本地 `emptyDir` 或 `local` PV
3. 将 NFS 挂载从 `hard` 改为 `soft,timeo=50` 减少卡死时间

#### 案例三：Prometheus TSDB 写入导致节点级 I/O 风暴

**问题描述**：Prometheus Pod 所在节点 `iowait` 持续高于 50%，影响同节点其他应用。

**根本原因**：Prometheus 的 WAL 和 checkpoint 操作产生大量随机写 I/O，与共享存储争用。

**解决方案**：
1. 为 Prometheus 配置独立的本地 SSD `local` PV
2. 使用 `storage.tsdb.min-block-duration` 和 `storage.tsdb.max-block-duration` 调整块大小
3. 配置节点亲和性确保 Prometheus 独占 NVMe 节点

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md|02-csi-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md|03-snapshot-backup-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md|05-storageclass-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md|01-pv-pvc-troubleshooting]]

```