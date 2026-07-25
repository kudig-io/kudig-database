---
title: Storage Area Network (SAN)
description: 存储网络深度指南 — SAN/NAS 架构、iSCSI/FC 协议、K8s 集成、生产实践
summary: 存储网络完整指南，涵盖 SAN vs NAS 架构对比、iSCSI/FC/FCoE 协议、K8s CSI 集成、多路径、生产配置
tags:
- san
- nas
- iscsi
- fibre-channel
- storage-network
difficulty: advanced
domain: 存储
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# 存储网络深度指南 SAN/NAS

## 1. 存储网络架构概述

### 1.1 SAN vs NAS

| 特性 | SAN (Storage Area Network) | NAS (Network Attached Storage) |
|------|---------------------------|-------------------------------|
| 访问方式 | 块级 (Block) | 文件级 (File) |
| 协议 | iSCSI, FC, FCoE | NFS, SMB/CIFS |
| 性能 | 低延迟、高 IOPS | 中等延迟、高吞吐 |
| 适用场景 | 数据库、虚拟化 | 文件共享、备份 |
| K8s 集成 | CSI Driver | CSI Driver / NFS Provisioner |

### 1.2 架构拓扑

```
┌─────────────────────────────────────────────────────────┐
│                      SAN 架构                            │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────┐  │
│  │  Server  │────│  Switch  │────│  Storage Array   │  │
│  │  (HBA)   │    │  (FC)    │    │  (LUN/Volume)    │  │
│  └──────────┘    └──────────┘    └──────────────────┘  │
│                                                         │
│                      NAS 架构                            │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────┐  │
│  │  Client  │────│  Network │────│  NAS Head        │  │
│  │  (NFS)   │    │  (TCP/IP)│    │  (File System)   │  │
│  └──────────┘    └──────────┘    └──────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 2. iSCSI 协议

### 2.1 核心概念

- **Initiator**：发起连接的一端（通常是服务器）
- **Target**：提供存储的一端（存储阵列）
- **LUN**：逻辑单元号，映射到具体的存储卷
- **IQN**：iSCSI Qualified Name，唯一标识符

### 2.2 Linux iSCSI Initiator 配置

```bash
# 安装 open-iscsi
apt-get install open-iscsi  # Debian/Ubuntu
yum install iscsi-initiator-utils  # RHEL/CentOS

# 配置 Initiator Name
echo "InitiatorName=iqn.2026-07.com.example:node1" > /etc/iscsi/initiatorname.iscsi

# 发现 Target
iscsiadm -m discovery -t sendtargets -p 192.168.1.100

# 登录 Target
iscsiadm -m node --login

# 查看会话
iscsiadm -m session

# 登出
iscsiadm -m node --logout
```

### 2.3 K8s iSCSI CSI

```yaml
# iSCSI PV 示例
apiVersion: v1
kind: PersistentVolume
metadata:
  name: iscsi-pv
spec:
  capacity:
    storage: 100Gi
  accessModes:
    - ReadWriteOnce
  iscsi:
    targetPortal: 192.168.1.100:3260
    iqn: iqn.2026-07.com.example:storage.lun1
    lun: 1
    fsType: ext4
    readOnly: false
    chapAuthDiscovery: true
    chapAuthSession: true
    secretRef:
      name: iscsi-secret
---
apiVersion: v1
kind: Secret
metadata:
  name: iscsi-secret
type: kubernetes.io/iscsi-chap
data:
  discovery.sendtargets.auth.username: dXNlcg==
  discovery.sendtargets.auth.password: cGFzc3dvcmQ=
```

## 3. Fibre Channel (FC)

### 3.1 FC 架构

| 组件 | 说明 |
|------|------|
| HBA | Host Bus Adapter，服务器端 FC 卡 |
| FC Switch | 光纤交换机，构建 FC Fabric |
| Target Port | 存储阵列的 FC 端口 |
| WWN | World Wide Name，FC 设备唯一标识 |
| Zoning | 访问控制，隔离不同主机 |

### 3.2 FC 配置

```bash
# 查看 HBA 信息
cat /sys/class/fc_host/host*/port_name
cat /sys/class/fc_host/host*/port_state

# 查看 FC 设备
lsscsi -f
fc-list

# 多路径配置
multipath -ll
multipath -a  # 添加设备
```

### 3.3 FC 多路径

```bash
# /etc/multipath.conf
defaults {
    user_friendly_names yes
    find_multipaths yes
}

blacklist {
    devnode "^(ram|raw|loop|fd|md|dm-|sr|scd|st)[0-9]*"
}

multipaths {
    multipath {
        wwid 3600507680c82004cf800000000000001
        alias storage-lun1
    }
}
```

## 4. NAS 与 NFS

### 4.1 NFS 版本对比

| 版本 | 状态 | 特性 |
|------|------|------|
| NFSv3 | 稳定 | 无状态、UDP/TCP |
| NFSv4 | 推荐 | 有状态、仅 TCP、集成 Kerberos |
| NFSv4.1 | 推荐 | pNFS、会话、多路径 |
| NFSv4.2 | 最新 | 服务器端复制、稀疏文件 |

### 4.2 K8s NFS CSI

```yaml
# NFS StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/k8s
reclaimPolicy: Delete
volumeBindingMode: Immediate
mountOptions:
  - nfsvers=4.1
  - hard
  - timeo=600
  - retrans=2
```

## 5. 生产最佳实践

### 5.1 多路径配置

```yaml
# 多路径 PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: multipath-pv
spec:
  capacity:
    storage: 500Gi
  accessModes:
    - ReadWriteOnce
  iscsi:
    targetPortal: 192.168.1.100:3260
    portals:
      - 192.168.1.101:3260  # 多路径
    iqn: iqn.2026-07.com.example:storage.lun1
    lun: 1
    fsType: ext4
```

### 5.2 性能调优

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| nr_requests | 256 | I/O 队列深度 |
| read_ahead_kb | 4096 | 预读大小 |
| scheduler | mq-deadline | I/O 调度器 |
| max_sectors_kb | 1024 | 最大 I/O 大小 |

### 5.3 监控指标

```promql
# iSCSI 会话状态
node_iscsi_sessions

# 多路径状态
node_multipath_paths{state="active"}

# NFS 延迟
nfs_rpc_ops_total
nfs_rpc_rtt_seconds
```

## 6. 存储性能基准测试

### fio 测试命令

```bash
# 🟢 只读：顺序读吐吐量测试
fio --name=seq-read --rw=read --bs=1M --size=1G \
  --numjobs=4 --runtime=60 --group_reporting \
  --filename=/mnt/test/fio-test --direct=1

# 🟢 只读：随机 IOPS 测试 (4K block)
fio --name=rand-iops --rw=randread --bs=4k --size=1G \
  --numjobs=8 --runtime=60 --group_reporting \
  --filename=/mnt/test/fio-test --direct=1 --iodepth=32

# 🟢 只读：混合读写测试 (70/30)
fio --name=mixed --rw=randrw --rwmixread=70 --bs=4k \
  --size=1G --numjobs=4 --runtime=60 --group_reporting \
  --filename=/mnt/test/fio-test --direct=1

# 🟢 只读：延迟测试
fio --name=latency --rw=randwrite --bs=4k --size=256M \
  --numjobs=1 --runtime=30 --group_reporting \
  --filename=/mnt/test/fio-test --direct=1 --iodepth=1
```

### 存储性能基线

| 存储类型 | 顺序读 | 随机 IOPS (4K) | P99 延迟 | 适用场景 |
|----------|--------|--------------|----------|----------|
| NVMe SSD | 3-7 GB/s | 100K-1M | < 1ms | 数据库/高性能 |
| SATA SSD | 500 MB/s | 50K-100K | < 2ms | 通用工作负载 |
| HDD (RAID10) | 200-400 MB/s | 1K-5K | 5-15ms | 备份/归档 |
| NFS (NAS) | 100-500 MB/s | 5K-20K | 2-10ms | 文件共享 |
| iSCSI (SAN) | 200-800 MB/s | 10K-50K | 1-5ms | 块存储 |

## 7. K8s 存储故障排查

### 诊断命令集

```bash
# 🟢 只读：PV/PVC 状态总览
kubectl get pv,pvc -A | grep -v Bound

# 🟢 只读：检查 VolumeAttachment
kubectl get volumeattachment | grep -v true

# 🟢 只读：CSI 驱动状态
kubectl get csidrivers
kubectl get csinodes
kubectl get pods -n kube-system -l app=csi-provisioner
kubectl get pods -n kube-system -l app=csi-nodeplugin

# 🟢 只读：CSI 驱动日志
kubectl logs -n kube-system -l app=csi-provisioner --tail=50
kubectl logs -n kube-system -l app=csi-nodeplugin --tail=50

# 🟢 只读：节点存储状态
kubectl describe node <node> | grep -A10 "Conditions" | grep -i disk

# 🟢 只读：检查挂载点
kubectl exec <pod> -- mount | grep -E "ext4|xfs|nfs"

# 🟢 只读：检查文件系统健康
kubectl exec <pod> -- df -h
kubectl exec <pod> -- df -i  # inode 使用率
```

### 故障排查决策树

```
存储异常
├── PVC Pending?
│   ├── Yes → StorageClass 存在? 配额足够? Provisioner 运行?
│   └── No
│       ├── Pod 挂载失败?
│       │   ├── Yes → VolumeAttachment 状态? CSI NodePlugin 正常?
│       │   └── No
│       │       ├── I/O 性能差?
│       │       │   ├── Yes → 存储后端负载? 多路径状态? 网络延迟?
│       │       │   └── No → 应用层问题
```

### 常见故障与修复

| 故障 | 根因 | 修复 |
|------|------|------|
| Multi-Attach Error | RWO 卷被多节点挂载 | 确认旧 Pod 完全终止，检查 VolumeAttachment |
| Mount Timeout | CSI NodePlugin 崩溃 | 重启节点上的 CSI DaemonSet Pod |
| NFS Stale Handle | NFS Server 重启/导出变更 | 重新挂载 Pod，检查 NFS 导出配置 |
| I/O Hang | 存储后端过载/网络中断 | 检查多路径状态，确认存储阵列健康 |
| Permission Denied | fsGroup/SELinux 不匹配 | 设置 Pod securityContext.fsGroup |

## 8. 高可用存储架构

### 存储高可用设计

```
┌────────────────────────────────────────────┐
│          高可用存储架构                      │
│                                              │
│  ┌──────────┐      ┌──────────┐      │
│  │ App Pod 1 │      │ App Pod 2 │      │
│  └────┬─────┘      └────┬─────┘      │
│       │                  │              │
│  ┌────▼──────────────────▼───────────┐  │
│  │     多路径 (Multipath I/O)       │  │
│  │   Path A ──── FC Switch 1 ───┐   │  │
│  │   Path B ──── FC Switch 2 ───┤   │  │
│  └───────────────────────────────┼───┘  │
│                                  │      │
│  ┌───────────────────────────────▼───┐  │
│  │  Storage Array (Active-Active)    │  │
│  │  Controller A ←──→ Controller B  │  │
│  │  (RAID 10 / Erasure Coding)      │  │
│  └───────────────────────────────────┘  │
└────────────────────────────────────────────┘
```

### 存储容灾策略

| 策略 | RPO | RTO | 实现方式 |
|------|-----|-----|----------|
| 同步复制 | 0 | < 1min | 存储阵列同步镜像 |
| 异步复制 | < 5min | < 5min | 存储阵列异步镜像 |
| 快照 + 异地 | < 1h | < 30min | VolumeSnapshot + 跨区域复制 |
| 应用层复制 | 可变 | 可变 | PostgreSQL Streaming/MySQL Replication |

## 9. 存储监控告警

### PrometheusRule — 存储网络监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-network-alerts
  namespace: monitoring
spec:
  groups:
    - name: storage-network
      rules:
        - alert: MultipathDegraded
          expr: |
            node_multipath_paths{state="faulty"} > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "多路径降级: {{ $labels.device }} 有故障路径"

        - alert: NFSLatencyHigh
          expr: |
            rate(nfs_rpc_rtt_seconds_sum[5m]) / rate(nfs_rpc_rtt_seconds_count[5m]) > 0.05
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "NFS 平均延迟 > 50ms，检查网络或 NAS 负载"

        - alert: ISCSISessionDown
          expr: |
            node_iscsi_sessions == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "iSCSI 会话断开，存储访问可能中断"

        - alert: DiskIOLatencyHigh
          expr: |
            rate(node_disk_write_time_seconds_total[5m]) / rate(node_disk_writes_completed_total[5m]) > 0.02
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "磁盘 {{ $labels.device }} 写延迟 > 20ms"
```

## Related

- [[06-存储/05-存储网络/index.md|存储网络索引]]
- [[06-存储/06-云存储对比/index.md|云存储对比]]
- [[06-存储/README.md|存储知识域]]
