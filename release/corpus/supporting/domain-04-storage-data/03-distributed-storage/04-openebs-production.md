---
title: OpenEBS 生产部署与运维指南
description: 'OpenEBS Kubernetes 存储平台生产部署：Mayastor NVMe-oF 高性能引擎、Jiva 轻量引擎、Local PV 管理、存储池配置、性能基准测试与监控告警'
summary: 'OpenEBS Kubernetes 存储平台生产部署：Mayastor NVMe-oF 高性能引擎、Jiva 轻量引擎、Local PV 管理、存储池配置、性能基准测试与监控告警'
category: storage-data
tags:
- storage
- k8s
- openebs
- mayastor
- nvme-of
- jiva
- local-pv
- distributed-storage
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- OpenEBS 生产部署 是什么
- 如何 OpenEBS Mayastor 高性能存储
- OpenEBS Jiva 引擎配置
trigger_keywords:
- OpenEBS
- Mayastor
- NVMe-oF
- Jiva
- Local PV
- 存储池
- 性能基准
prerequisites:
- kubectl-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# OpenEBS 生产部署与运维指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **OpenEBS**: v4.x / Mayastor v2.x | **最后更新**: 2026-07
> **文档定位**: OpenEBS 是 CNCF 毕业项目，提供多种存储引擎。本文聚焦生产环境部署，覆盖 Mayastor 高性能引擎、Jiva 轻量引擎和 Local PV 管理。

## 1. 架构概览

### 1.1 OpenEBS 存储引擎对比

| 引擎 | 协议 | 性能 | 适用场景 | 数据冗余 |
|------|------|------|---------|---------|
| **Mayastor** | NVMe-oF / iSCSI | 极高（NVMe 原生） | 数据库、高性能工作负载 | 基于复制因子 |
| **Jiva** | iSCSI | 中等 | 轻量级有状态应用 | 基于副本数 |
| **Local PV** | 本地磁盘 | 最高（无网络开销） | 临时数据、缓存、日志 | 无（依赖上层） |
| **cStor**（已废弃） | iSCSI | 中等 | 迁移遗留环境 | 基于副本数 |

### 1.2 Mayastor 架构

```
┌─────────────────────────────────────────────────────┐
│                   Control Plane                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ API Rest │  │ etcd     │  │ CSI Controller   │   │
│  │ Server   │  │ (状态存储)│  │ (Provisioner)    │   │
│  └──────────┘  └──────────┘  └──────────────────┘   │
├─────────────────────────────────────────────────────┤
│                   Data Plane                         │
│  ┌──────────────────┐  ┌──────────────────┐         │
│  │ io-engine (Node1)│  │ io-engine (Node2)│         │
│  │  ├─ NVMe bdev    │  │  ├─ NVMe bdev    │         │
│  │  ├─ Nexus        │  │  ├─ Replica      │         │
│  │  └─ NVMe-oF tgt  │  │  └─ NVMe-oF tgt  │         │
│  └──────────────────┘  └──────────────────┘         │
│         │ NVMe-oF TCP              │                 │
│         └───────────┬──────────────┘                 │
│                     ▼                                │
│              Pod (Consumer)                          │
└─────────────────────────────────────────────────────┘
```

**核心组件**:
- **io-engine**: 用户态 SPDK 存储引擎，处理 I/O 路径
- **Nexus**: I/O 入口，聚合多个 Replica 提供统一命名空间
- **Replica**: 数据副本，存储在节点本地磁盘上
- **Agent (control-plane)**: 管理卷生命周期、调度决策
- **etcd**: 存储集群元数据和卷拓扑信息

### 1.3 Jiva 架构

```
┌─────────────────────────────────────────┐
│           Jiva Volume Architecture       │
│                                          │
│  ┌──────────────┐                        │
│  │ Target Pod   │ ← iSCSI Target         │
│  │ (Controller) │                        │
│  │  ├─ Frontend  │ ← 接收 I/O            │
│  │  └─ Sync Agent│ ← 副本同步            │
│  └──────┬───────┘                        │
│         │ TCP/iSCSI                       │
│  ┌──────┴───────┐                        │
│  │ Replica Pod  │ ← 数据副本             │
│  │  └─ qcow2    │ ← 精简配置             │
│  └──────────────┘                        │
└─────────────────────────────────────────┘
```

## 2. 环境准备

### 2.1 硬件要求

```yaml
# Mayastor 节点要求
mayastor_node_requirements:
  cpu: "4 cores minimum, 8+ recommended"
  memory: "8Gi minimum, 16Gi+ recommended"
  disks:
    - type: "NVMe SSD 或 SATA SSD"
    - count: "至少 1 块专用磁盘（非系统盘）"
    - size: "根据容量规划"
  network: "10Gbps+ recommended, NVMe-oF 对延迟敏感"
  kernel: "5.10+ (NVMe-oF TCP 支持)"

# Jiva 节点要求
jiva_node_requirements:
  cpu: "2 cores minimum"
  memory: "4Gi minimum"
  disks:
    - type: "任意块设备"
    - count: "至少 1 块"
  network: "1Gbps+ sufficient"
```

### 2.2 内核模块与配置

```bash
# 加载必要内核模块（Mayastor）
sudo modprobe nvme_tcp
sudo modprobe nvmet
sudo modprobe nvme_fabrics

# 持久化配置
cat <<EOF | sudo tee /etc/modules-load.d/openebs-mayastor.conf
nvme_tcp
nvmet
nvme_fabrics
EOF

# 验证模块加载
lsmod | grep nvme

# 检查 Huge Pages（Mayastor 需要）
grep HugePages /proc/meminfo
# 预期输出应显示已配置的 Huge Pages

# 配置 Huge Pages（每个 io-engine 实例需要 512 个 2MB Huge Pages）
echo 512 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# 持久化 Huge Pages
echo "vm.nr_hugepages = 512" | sudo tee /etc/sysctl.d/99-mayastor-hugepages.conf
sudo sysctl -p /etc/sysctl.d/99-mayastor-hugepages.conf
```

### 2.3 节点标签

```bash
# Mayastor 存储节点标签
kubectl label node <node-name> \
  openebs.io/engine=mayastor \
  kubernetes.io/arch=amd64

# Jiva 存储节点标签
kubectl label node <node-name> \
  openebs.io/engine=jiva

# Local PV 节点标签
kubectl label node <node-name> \
  openebs.io/localpv=enabled

# 验证标签
kubectl get nodes --show-labels | grep openebs
```

## 3. OpenEBS 安装

### 3.1 Helm 安装（推荐）

```bash
# 添加 Helm 仓库
helm repo add openebs https://openebs.github.io/openebs
helm repo update

# 查看可用版本
helm search repo openebs/openebs --versions

# 安装 OpenEBS（启用 Mayastor + Local PV）
helm install openebs openebs/openebs \
  --namespace openebs \
  --create-namespace \
  --version 4.1.0 \
  --set mayastor.enabled=true \
  --set mayastor.csi.node.initContainers.enabled=true \
  --set localpv-provisioner.enabled=true \
  --set engines.replicated.jiva.enabled=true \
  --wait

# 验证安装
kubectl get pods -n openebs
```

### 3.2 Mayastor 专用安装

```bash
# 安装 Mayastor Helm Chart（独立安装方式）
helm install mayastor openebs/mayastor \
  --namespace mayastor \
  --create-namespace \
  --version 2.6.0 \
  --set mayastor.io_engine_resources.limits.cpu=4 \
  --set mayastor.io_engine_resources.limits.memory=4Gi \
  --set etcd.replicaCount=3 \
  --wait

# 验证 Mayastor 组件
kubectl get pods -n mayastor -o wide
# 预期输出:
# mayastor-agent-core-xxx        1/1     Running
# mayastor-csi-controller-xxx    5/5     Running
# mayastor-csi-node-xxx          2/2     Running
# mayastor-io-engine-xxx         2/2     Running
# mayastor-etcd-0                1/1     Running
```

### 3.3 创建 Mayastor Pool

```yaml
# mayastor-pool.yaml
apiVersion: openebs.io/v1beta2
kind: DiskPool
metadata:
  name: pool-node1
  namespace: mayastor
spec:
  node: k8s-node-1          # 节点名称
  disks:                     # 磁盘设备路径
    - /dev/nvme0n1           # NVMe 设备（推荐）
  # - /dev/sdb              # SATA 设备也可以
---
apiVersion: openebs.io/v1beta2
kind: DiskPool
metadata:
  name: pool-node2
  namespace: mayastor
spec:
  node: k8s-node-2
  disks:
    - /dev/nvme0n1
---
apiVersion: openebs.io/v1beta2
kind: DiskPool
metadata:
  name: pool-node3
  namespace: mayastor
spec:
  node: k8s-node-3
  disks:
    - /dev/nvme0n1
```

```bash
kubectl apply -f mayastor-pool.yaml

# 验证存储池状态
kubectl get diskpool -n mayastor
# NAME           NODE         STATUS   CAPACITY     USED      AVAILABLE
# pool-node1     k8s-node-1   Online   500GiB       10GiB     490GiB
# pool-node2     k8s-node-2   Online   500GiB       10GiB     490GiB
# pool-node3     k8s-node-3   Online   500GiB       10GiB     490GiB
```

### 3.4 创建 StorageClass

```yaml
# mayastor-storageclass.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: mayastor-nvme
provisioner: io.openebs.csi-mayastor
parameters:
  protocol: nvmf                    # NVMe-oF 协议（高性能）
  repl: "3"                         # 3 副本
  ioTimeoutSecs: "60"
  ioSize: "4096"                    # I/O 大小
  local: "false"                    # 允许跨节点复制
  # 性能调优参数
  targetNsPerSharedSubsystem: "4"   # 每个子系统的命名空间数
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
---
# iSCSI 协议版本（兼容性更好）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: mayastor-iscsi
provisioner: io.openebs.csi-mayastor
parameters:
  protocol: iscsi
  repl: "2"
  ioTimeoutSecs: "30"
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
---
# Jiva StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-jiva-csi
provisioner: jiva.csi.openebs.io
parameters:
  cas-type: jiva
  replication: "3"
  # Jiva 特有参数
  local: "false"
  thinProvision: "true"
reclaimPolicy: Delete
volumeBindingMode: Immediate
---
# Local PV StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-localpv
provisioner: local.csi.openebs.io
parameters:
  storageType: hostpath
  basepath: /var/openebs/local
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
```

```bash
kubectl apply -f mayastor-storageclass.yaml

# 验证 StorageClass
kubectl get sc | grep openebs
```

## 4. 卷管理

### 4.1 创建 PVC

```yaml
# mayastor-pvc.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
  namespace: database
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: mayastor-nvme
  resources:
    requests:
      storage: 100Gi
---
# 测试 Pod
apiVersion: v1
kind: Pod
metadata:
  name: mysql-test
  namespace: database
spec:
  containers:
    - name: mysql
      image: mysql:8.0
      env:
        - name: MYSQL_ROOT_PASSWORD
          value: "testpassword"
      volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: mysql-data
```

### 4.2 卷扩容

```bash
# 在线扩容 PVC（Mayastor 支持在线扩容）
kubectl patch pvc mysql-data -n database \
  -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 查看扩容进度
kubectl get pvc mysql-data -n database -w
```

### 4.3 创建快照

```yaml
# volumesnapshot.yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: mayastor-snapshotclass
driver: io.openebs.csi-mayastor
deletionPolicy: Delete
---
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-data-snapshot
  namespace: database
spec:
  volumeSnapshotClassName: mayastor-snapshotclass
  source:
    persistentVolumeClaimName: mysql-data
```

```bash
kubectl apply -f volumesnapshot.yaml

# 查看快照状态
kubectl get volumesnapshot -n database
# NAME                  READYTOUSE   SOURCEPVC   SOURCESNAPSHOTCONTENT   RESTORESIZE   SNAPSHOTCLASS            AGE
# mysql-data-snapshot   true         mysql-data                          100Gi         mayastor-snapshotclass   30s
```

## 5. 性能基准测试

### 5.1 fio 基准测试

```yaml
# fio-benchmark.yaml
apiVersion: v1
kind: Pod
metadata:
  name: fio-benchmark
  namespace: benchmark
spec:
  containers:
    - name: fio
      image: ljishen/fio:latest
      command: ["sleep", "3600"]
      volumeMounts:
        - name: test-vol
          mountPath: /data
  volumes:
    - name: test-vol
      persistentVolumeClaim:
        claimName: fio-test-pvc
```

```bash
# 创建测试 PVC
kubectl create namespace benchmark
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: fio-test-pvc
  namespace: benchmark
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: mayastor-nvme
  resources:
    requests:
      storage: 50Gi
EOF

# 顺序写测试
kubectl exec -n benchmark fio-benchmark -- fio \
  --name=seq-write \
  --ioengine=libaio \
  --direct=1 \
  --bs=128k \
  --size=10G \
  --numjobs=4 \
  --runtime=60 \
  --rw=write \
  --filename=/data/test-seq-write

# 顺序读测试
kubectl exec -n benchmark fio-benchmark -- fio \
  --name=seq-read \
  --ioengine=libaio \
  --direct=1 \
  --bs=128k \
  --size=10G \
  --numjobs=4 \
  --runtime=60 \
  --rw=read \
  --filename=/data/test-seq-read

# 随机读写混合（模拟数据库）
kubectl exec -n benchmark fio-benchmark -- fio \
  --name=rand-rw \
  --ioengine=libaio \
  --direct=1 \
  --bs=4k \
  --size=10G \
  --numjobs=8 \
  --runtime=120 \
  --rw=randrw \
  --rwmixread=70 \
  --iodepth=32 \
  --filename=/data/test-rand-rw
```

### 5.2 预期性能指标

| 引擎 | 配置 | 顺序读 (MB/s) | 顺序写 (MB/s) | 随机读 IOPS | 随机写 IOPS | 延迟 (μs) |
|------|------|--------------|--------------|------------|------------|----------|
| Mayastor NVMe-oF | 3 副本, NVMe SSD | 2000+ | 1500+ | 200K+ | 100K+ | 100-200 |
| Mayastor iSCSI | 3 副本, NVMe SSD | 1200+ | 800+ | 100K+ | 60K+ | 200-400 |
| Jiva | 3 副本, SATA SSD | 500+ | 300+ | 30K+ | 15K+ | 500-1000 |
| Local PV | 单节点, NVMe SSD | 3000+ | 2500+ | 500K+ | 300K+ | 50-100 |

> **注意**: 以上数据为参考值，实际性能取决于硬件配置、网络带宽和集群负载。

### 5.3 性能调优

```yaml
# 高性能 StorageClass 配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: mayastor-high-perf
provisioner: io.openebs.csi-mayastor
parameters:
  protocol: nvmf
  repl: "2"                         # 减少副本数提升写性能
  ioTimeoutSecs: "30"
  ioSize: "4096"
  local: "false"
  targetNsPerSharedSubsystem: "2"
  # NVMe-oF 特定优化
  ctrlLossTmoSecs: "15"
  keepAliveTmoMs: "10000"
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

## 6. 监控与告警

### 6.1 Prometheus 监控配置

```yaml
# openebs-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: mayastor-io-engine
  namespace: monitoring
  labels:
    release: prometheus
spec:
  selector:
    matchLabels:
      app: mayastor-io-engine
  namespaceSelector:
    matchNames:
      - mayastor
  endpoints:
    - port: metrics
      interval: 15s
      path: /metrics
---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: mayastor-agent
  namespace: monitoring
  labels:
    release: prometheus
spec:
  selector:
    matchLabels:
      app: mayastor-agent-core
  namespaceSelector:
    matchNames:
      - mayastor
  endpoints:
    - port: metrics
      interval: 15s
```

### 6.2 关键监控指标

```yaml
# openebs-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: openebs-alerts
  namespace: monitoring
spec:
  groups:
    - name: openebs.rules
      rules:
        # 存储池容量告警
        - alert: OpenEBSPoolCapacityHigh
          expr: |
            (openebs_pool_capacity_used_bytes / openebs_pool_capacity_total_bytes) > 0.85
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "OpenEBS 存储池 {{ $labels.pool }} 使用率超过 85%"
            description: "存储池使用率 {{ $value | humanizePercentage }}，建议扩容或清理数据"

        - alert: OpenEBSPoolCapacityCritical
          expr: |
            (openebs_pool_capacity_used_bytes / openebs_pool_capacity_total_bytes) > 0.95
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "OpenEBS 存储池 {{ $labels.pool }} 使用率超过 95%"
            description: "存储池即将满载，新卷创建可能失败"

        # 卷健康状态告警
        - alert: OpenEBSVolumeDegraded
          expr: openebs_volume_status{status="degraded"} > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "OpenEBS 卷 {{ $labels.volume }} 处于降级状态"
            description: "卷的副本数不足，可能存在节点或磁盘故障"

        - alert: OpenEBSVolumeFaulted
          expr: openebs_volume_status{status="faulted"} > 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "OpenEBS 卷 {{ $labels.volume }} 处于故障状态"
            description: "卷不可用，需要立即处理"

        # I/O 性能告警
        - alert: OpenEBSHighLatency
          expr: |
            openebs_volume_read_latency_seconds > 0.1
            or openebs_volume_write_latency_seconds > 0.1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "OpenEBS 卷 {{ $labels.volume }} I/O 延迟超过 100ms"

        # 节点 io-engine 状态
        - alert: OpenEBSIoEngineDown
          expr: up{job="mayastor-io-engine"} == 0
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "Mayastor io-engine 节点 {{ $labels.instance }} 不可用"
```

### 6.3 Grafana Dashboard

```json
{
  "dashboard": {
    "title": "OpenEBS Storage Overview",
    "panels": [
      {
        "title": "Pool Capacity Usage",
        "type": "gauge",
        "targets": [
          {
            "expr": "openebs_pool_capacity_used_bytes / openebs_pool_capacity_total_bytes * 100"
          }
        ]
      },
      {
        "title": "Volume IOPS",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(openebs_volume_read_count[5m])",
            "legendFormat": "Read IOPS - {{ volume }}"
          },
          {
            "expr": "rate(openebs_volume_write_count[5m])",
            "legendFormat": "Write IOPS - {{ volume }}"
          }
        ]
      },
      {
        "title": "Volume Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "openebs_volume_read_latency_seconds",
            "legendFormat": "Read Latency - {{ volume }}"
          },
          {
            "expr": "openebs_volume_write_latency_seconds",
            "legendFormat": "Write Latency - {{ volume }}"
          }
        ]
      }
    ]
  }
}
```

## 7. 运维操作

### 7.1 存储池维护

```bash
# 查看存储池详细信息
kubectl describe diskpool pool-node1 -n mayastor

# 将存储池置为维护模式（用于磁盘更换）
kubectl patch diskpool pool-node1 -n mayastor \
  --type=merge -p '{"spec":{"maintenanceMode":true}}'

# 等待所有卷迁移到其他节点
kubectl get volumes -n mayastor -o json | \
  jq '.items[] | select(.spec.target.node=="k8s-node-1")'

# 完成维护后退出维护模式
kubectl patch diskpool pool-node1 -n mayastor \
  --type=merge -p '{"spec":{"maintenanceMode":false}}'
```

### 7.2 节点替换

```bash
# 1. 标记节点不可调度
kubectl cordon k8s-node-old

# 2. 将存储池置于维护模式
kubectl patch diskpool pool-old -n mayastor \
  --type=merge -p '{"spec":{"maintenanceMode":true}}'

# 3. 等待数据重建完成
kubectl get rebuilds -n mayastor

# 4. 在新节点创建存储池
cat <<EOF | kubectl apply -f -
apiVersion: openebs.io/v1beta2
kind: DiskPool
metadata:
  name: pool-new
  namespace: mayastor
spec:
  node: k8s-node-new
  disks:
    - /dev/nvme0n1
EOF

# 5. 删除旧存储池
kubectl delete diskpool pool-old -n mayastor

# 6. 解除旧节点调度
kubectl uncordon k8s-node-old
```

### 7.3 升级流程

```bash
# 查看当前版本
helm list -n mayastor

# 更新 Helm 仓库
helm repo update

# 查看可用版本
helm search repo openebs/mayastor --versions

# 升级（先在测试环境验证）
helm upgrade mayastor openebs/mayastor \
  --namespace mayastor \
  --version 2.7.0 \
  --reuse-values \
  --wait

# 验证升级
kubectl get pods -n mayastor
kubectl get diskpool -n mayastor
```

## 8. 故障排查

### 8.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| PVC Pending | 存储池容量不足 | 扩容存储池或清理数据 |
| 卷 Degraded | 节点或磁盘故障 | 检查节点状态和磁盘健康 |
| I/O 错误 | 网络或磁盘问题 | 检查网络连通性和磁盘 SMART 信息 |
| 创建卷超时 | etcd 性能问题 | 检查 etcd 集群健康状态 |
| 性能下降 | 碎片化或重建中 | 等待重建完成或优化配置 |

### 8.2 诊断命令

```bash
# 检查 Mayastor 组件状态
kubectl get pods -n mayastor -o wide

# 查看 io-engine 日志
kubectl logs -n mayastor -l app=mayastor-io-engine --tail=100

# 检查存储池状态
kubectl get diskpool -n mayastor -o yaml

# 查看卷详细信息
kubectl get volumes -n mayastor -o yaml

# 检查 CSI 驱动状态
kubectl get csidrivers | grep openebs

# 查看卷重建进度
kubectl get rebuilds -n mayastor

# 检查 etcd 集群健康
kubectl exec -n mayastor mayastor-etcd-0 -- etcdctl endpoint health
```

## 9. 生产最佳实践

### 9.1 容量规划

```yaml
capacity_planning:
  # 预留 20% 空间用于重建和快照
  usable_ratio: 0.8
  
  # 3 副本场景
  raw_to_usable: 0.267  # 1/3 * 0.8
  
  # 计算公式
  # 所需原始容量 = 应用数据量 / raw_to_usable
  # 示例: 1TB 应用数据需要 ~3.75TB 原始容量
```

### 9.2 备份策略

```yaml
backup_strategy:
  # 定时快照
  snapshots:
    schedule: "0 */6 * * *"    # 每 6 小时
    retention: 24               # 保留 24 个
  
  # Velero 备份
  velero:
    schedule: "0 2 * * *"      # 每天凌晨 2 点
    retention: 30d              # 保留 30 天
    storage_location: s3-backup
  
  # 跨区域复制
  replication:
    enabled: true
    target_region: "cn-east-2"
    sync_interval: "1h"
```

### 9.3 安全加固

```yaml
security_best_practices:
  # 网络隔离
  network:
    - 使用 NetworkPolicy 限制 mayastor 命名空间的网络访问
    - NVMe-oF 流量走专用网络/VLAN
  
  # 访问控制
  rbac:
    - 限制 DiskPool 和 Volume 的创建权限
    - 使用 ServiceAccount 隔离组件权限
  
  # 加密
  encryption:
    - 传输加密: NVMe-oF TLS（需要内核 5.15+）
    - 静态加密: 依赖底层磁盘加密或 dm-crypt
```

---

## Related

- [[02-rook-ceph-production|Rook Ceph 生产部署]]
- [[03-longhorn-production|Longhorn 生产部署]]

## See Also

- [OpenEBS 官方文档](https://openebs.io/docs)
- [Mayastor 架构文档](https://openebs.io/docs/user-guides/replicated-storage/mayastor)
- [OpenEBS GitHub](https://github.com/openebs/openebs)
