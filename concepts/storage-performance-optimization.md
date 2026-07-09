---
title: 存储性能优化
summary: 存储性能优化是 Kubernetes 数据密集型工作负载（数据库、AI/ML 训练、日志分析）的关键环节。本文涵盖基准测试方法论、硬件级优化、云厂商特化配置以及存储
  QoS 策略。
category: concepts
tags:
- storage
- performance
- nvme
- benchmark
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 存储性能优化

## 概述

存储性能优化是 Kubernetes 数据密集型工作负载（数据库、AI/ML 训练、日志分析）的关键环节。本文涵盖基准测试方法论、硬件级优化、云厂商特化配置以及存储 QoS 策略。

相关：[[concepts/csi-drivers.md|csi drivers]] | [[concepts/storageclass.md|storageclass]] | [[生态参考/98-merged-indexes/index.md|index]]

---

## 1. 基准测试工具

### 1.1 fio DaemonSet（推荐首选）

fio 是存储性能测试的事实标准。以 DaemonSet 方式运行可直接在每个节点上测试本地磁盘和网络存储。

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fio-benchmark
spec:
  selector:
    matchLabels:
      app: fio
  template:
    metadata:
      labels:
        app: fio
    spec:
      containers:
      - name: fio
        image: ljishen/fio:latest
        command: ["sleep", "infinity"]
        volumeMounts:
        - name: test-vol
          mountPath: /testdata
      volumes:
      - name: test-vol
        persistentVolumeClaim:
          claimName: fio-test-pvc
```

典型测试场景：

| 测试类型 | fio 参数 | 目标指标 |
|---------|---------|---------|
| 顺序读 | `--rw=read --bs=1M` | 吞吐量 (MB/s) |
| 随机读 (4K) | `--rw=randread --bs=4k --iodepth=64` | IOPS |
| 随机写 (4K) | `--rw=randwrite --bs=4k --iodepth=64` | IOPS |
| 混合读写 | `--rw=randrw --rwmixread=70 --bs=4k` | IOPS + 延迟 |
| 顺序写 | `--rw=write --bs=1M` | 吞吐量 (MB/s) |

### 1.2 kube-burner

kube-burner 侧重于 Kubernetes API 层面的性能测试，可测量 PVC 创建/绑定延迟、StorageClass provisioning 时间等控制面指标。

```bash
kube-burner init -c storage-workload.yml --uuid=$(uuidgen)
```

### 1.3 PerfKitBenchmarker

Google 开源的跨云基准测试框架，内置 100+ 基准测试，包括存储类测试（fio、bonnie++、dd 等），适合多云横向对比。

```bash
./pkb.py --cloud=gcp --benchmarks=fio --fio_parameters='--size=10G'
```

---

## 2. 关键指标基线

### 2.1 IOPS 基线参考

| 存储类型 | 随机读 IOPS (4K) | 随机写 IOPS (4K) | 延迟 (P99) |
|---------|-----------------|-----------------|-----------|
| NVMe 本地盘 (直通) | 500K – 2M | 200K – 1M | < 100μs |
| 云 SSD (gp3/io2) | 3K – 16K | 3K – 16K | 200μs – 1ms |
| Ceph RBD (NVMe 后端) | 50K – 200K | 30K – 150K | 500μs – 2ms |
| Ceph RBD (HDD 后端) | 1K – 5K | 500 – 2K | 5ms – 20ms |
| NFS (网络) | 5K – 30K | 3K – 20K | 1ms – 5ms |

### 2.2 吞吐量基线

| 存储类型 | 顺序读 (MB/s) | 顺序写 (MB/s) |
|---------|-------------|-------------|
| NVMe 本地盘 | 3,000 – 7,000 | 2,000 – 5,000 |
| 云 SSD | 125 – 1,000 | 125 – 1,000 |
| Ceph RBD (NVMe) | 500 – 2,000 | 300 – 1,500 |

> **注意**：云 SSD 的 IOPS/吞吐量通常与卷大小或预配置 IOPS 挂钩，需按需调整。

---

## 3. NVMe / SSD 优化

### 3.1 Raw Block Volumes

跳过文件系统层，消除 I/O 栈开销，适合数据库等对延迟敏感的场景。

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-block-pvc
spec:
  volumeMode: Block          # 关键：使用 Block 而非 Filesystem
  accessModes: [ReadWriteOnce]
  storageClassName: local-nvme
  resources:
    requests:
      storage: 100Gi
```

Pod 挂载：

```yaml
volumeDevices:
- name: data
  devicePath: /dev/xvda      # 直接暴露块设备
```

### 3.2 I/O 调度器配置

NVMe 设备应使用 `none`（noop）调度器，避免不必要的排序开销。

```bash
# 查看当前调度器
cat /sys/block/nvme0n1/queue/scheduler

# 设置为 none
echo none > /sys/block/nvme0n1/queue/scheduler

# 通过 DaemonSet 批量设置
# 或在 node 初始化脚本中配置 udev 规则
echo 'ACTION=="add|change", KERNEL=="nvme[0-9]*", ATTR{queue/scheduler}="none"' \
  > /etc/udev/rules.d/60-nvme-scheduler.rules
```

### 3.3 NUMA-Aware 调度

NVMe 设备绑定到特定 NUMA 节点，跨 NUMA 访问会增加 50-100% 延迟。

```yaml
# Pod 使用 topology hints 确保调度到 NVMe 所在 NUMA 节点
spec:
  containers:
  - name: db
    resources:
      limits:
        example.com/nvme: 1    # 设备插件暴露的 NVMe 资源
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
```

### 3.4 内核参数调优

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 增大请求队列深度
echo 1024 > /sys/block/nvme0n1/queue/nr_requests

# 关闭 readahead（NVMe 内部已有预取）
echo 0 > /sys/block/nvme0n1/queue/read_ahead_kb

# 调整脏页写回参数（减少写突发）
sysctl -w vm.dirty_ratio=10
sysctl -w vm.dirty_background_ratio=5
sysctl -w vm.dirty_expire_centisecs=500

# NVMe 多队列确认
cat /sys/block/nvme0n1/queue/nr_queues  # 应等于 CPU 核数
```

---

## 4. 云厂商特定优化

### 4.1 AWS（i3 / i4i 实例）

- **实例存储**：i3/i4i 提供本地 NVMe，零网络延迟
- **io2 Block Express**：最高 256K IOPS，亚毫秒延迟
- **gp3**：基线 3K IOPS + 125MB/s，可独立预配置

```yaml
# gp3 预配置 IOPS 的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-high-iops
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "16000"              # 预配置 IOPS
  throughput: "1000"          # MB/s
  fsType: ext4
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 4.2 GCP（Local SSD）

- **Local SSD**：最高 375GB/块，最多 24 块 = 9TB，350K 读 IOPS
- **pd-extreme**：最高 120K IOPS（预配置）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GKE 节点池启用 Local SSD
gcloud container node-pools create high-io-pool \
  --local-ssd-count=4 \
  --machine-type=n2-standard-32 \
  --cluster=my-cluster
```
### 4.3 Azure（Lsv2 系列）

- **Lsv2**：本地 NVMe 实例，最多 19.2TB 本地存储
- **Ultra Disk**：最高 160K IOPS，2,000 MB/s

```yaml
# Ultra Disk StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ultra-disk
provisioner: disk.csi.azure.com
parameters:
  skuName: UltraSSD_LRS
  cachingMode: None            # Ultra Disk 不支持缓存
  DiskIOPSReadWrite: "20000"
  DiskMBpsReadWrite: "1000"
volumeBindingMode: WaitForFirstConsumer
```

---

## 5. 存储 QoS

### 5.1 VolumeAttributesClass（KEP-3751）

Kubernetes 1.31+ 引入 VolumeAttributesClass，允许在不重建 PVC 的情况下动态修改卷的性能参数。

```yaml
apiVersion: storage.k8s.io/v1alpha1
kind: VolumeAttributesClass
metadata:
  name: gold-performance
driverName: ebs.csi.aws.com
parameters:
  iops: "16000"
  throughput: "1000"
---
# PVC 引用 VolumeAttributesClass
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-pvc
spec:
  volumeAttributesClassName: gold-performance
  accessModes: [ReadWriteOnce]
  resources:
    requests:
      storage: 100Gi
```

### 5.2 CSI 级 QoS

部分 CSI 驱动内置 QoS 能力：

- **OpenStack Cinder**：通过 volume type 的 QoS specs 控制 IOPS/带宽上限
- **Ceph CSI**：通过 Ceph QoS（mClock 调度器）限制客户端 IOPS
- **Netapp Trident**：基于 QoS Policy Group 的吞吐量/ IOPS 限制

### 5.3 Linux blkio cgroup

在节点层面通过 cgroup v2 限制 Pod 的块 I/O：

```yaml
# Pod 级别的 I/O 限制（需要 cgroup v2）
apiVersion: v1
kind: Pod
metadata:
  name: low-priority-worker
spec:
  containers:
  - name: worker
    image: busybox
    resources:
      limits:
        cpu: "2"
        memory: "1Gi"
    # 通过 runtimeClass 或 node-level 配置 blkio
```

cgroup v2 blkio 控制文件：

```bash
# 限制读 IOPS（设备号 major:minor）
echo "259:0 rbps=104857600 riops=5000" > /sys/fs/cgroup/kubepods/burstable/pod<uid>/io.max

# 限制写带宽为 100MB/s
echo "259:0 wbps=104857600" > /sys/fs/cgroup/kubepods/burstable/pod<uid>/io.max
```

---

## 6. 优化检查清单

- [ ] 确认 I/O 调度器为 `none`（NVMe）或 `mq-deadline`（SSD）
- [ ] 验证 NUMA 拓扑，Pod 调度到 NVMe 所在 NUMA 节点
- [ ] 数据库工作负载优先使用 Raw Block Volumes
- [ ] 云 SSD 按需预配置 IOPS 和吞吐量
- [ ] 内核脏页参数已调优
- [ ] 基准测试覆盖顺序读写 + 随机读写 + 混合场景
- [ ] 存储 QoS 策略防止"吵闹邻居"
- [ ] VolumeAttributesClass 用于动态性能调整（1.31+）

---

## 参考资料

- [fio 官方文档](https://fio.readthedocs.io/)
- [KEP-3751: VolumeAttributesClass](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/3751-volume-attributes-class)
- [AWS EBS CSI 性能指南](https://docs.aws.amazon.com/ebs/latest/userguide/ebs-io-characteristics.html)
- [GCP Local SSD 性能](https://cloud.google.com/compute/docs/disks/local-ssd-performance)

## Related

- [[concepts/csi-drivers.md|csi drivers]] — CSI 驱动规范与实现
- [[concepts/cloud-native-storage-systems.md|cloud native storage systems]] — 云原生存储系统架构
- [[concepts/finops-greenops-practices.md|finops greenops practices]] — FinOps 与绿色运维实践


<!-- risk-assessed -->
