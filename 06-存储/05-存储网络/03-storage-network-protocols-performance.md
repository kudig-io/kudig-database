---
title: Kubernetes Storage Networking — iSCSI, NFS, Ceph, and CSI Network Paths
description: K8s 存储网络 — iSCSI/NFS/Ceph 网络存储架构、CSI 网络路径、存储性能调优、多路径、加密传输
summary: 深入理解 Kubernetes 网络存储的网络路径、协议选型与性能调优实践
category: reference
tags:
- storage-networking
- iscsi
- nfs
- ceph
- csi
- performance
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: storage
---
# Kubernetes 存储网络深度解析

> 网络存储协议、CSI 数据路径与性能调优。

## 存储网络架构

```
┌─────────────────────────────────────────────────────────────┐
│  Pod                                                         │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  应用进程 → VFS → FUSE/内核文件系统                   │   │
│  └───────────────────────┬─────────────────────────────┘   │
│                          │                                  │
│  ┌───────────────────────┴─────────────────────────────┐   │
│  │  挂载点 (/var/lib/kubelet/pods/<uid>/volumes/...)    │   │
│  └───────────────────────┬─────────────────────────────┘   │
└──────────────────────────┼──────────────────────────────────┘
                           │
┌──────────────────────────┼──────────────────────────────────┐
│  Node                    │                                  │
│  ┌───────────────────────┴─────────────────────────────┐   │
│  │  kubelet → Volume Manager → CSI Node Plugin          │   │
│  └───────────────────────┬─────────────────────────────┘   │
│                          │                                  │
│  ┌───────────────────────┴─────────────────────────────┐   │
│  │  存储协议层                                          │   │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐    │   │
│  │  │ NFS  │ │iSCSI │ │ Ceph │ │ FC   │ │ NVMe │    │   │
│  │  │      │ │      │ │ RBD  │ │      │ │ /oF  │    │   │
│  │  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘    │   │
│  └─────┼────────┼────────┼────────┼────────┼──────────┘   │
└────────┼────────┼────────┼────────┼────────┼───────────────┘
         │        │        │        │        │
    ┌────┴────────┴────────┴────────┴────────┴────┐
    │  存储网络 (专用网络 / 共享网络)               │
    │  ┌──────────────────────────────────────┐   │
    │  │  以太网 (10/25/100 GbE)              │   │
    │  │  或 FC 网络 (16/32/64 Gbps)          │   │
    │  │  或 RDMA (RoCE / InfiniBand)         │   │
    │  └──────────────────────────────────────┘   │
    └────────────────────┬────────────────────────┘
                         │
    ┌────────────────────┴────────────────────────┐
    │  存储后端                                    │
    │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐     │
    │  │ NAS  │ │ SAN  │ │ Ceph │ │ 云块 │     │
    │  │      │ │      │ │ 集群 │ │ 存储 │     │
    │  └──────┘ └──────┘ └──────┘ └──────┘     │
    └─────────────────────────────────────────────┘
```

## 存储协议对比

| 协议 | 类型 | 延迟 | 吞吐 | 适用场景 | K8s 支持 |
|------|------|------|------|----------|----------|
| NFS v4.1 | 文件 | 0.5-2ms | 中 | 共享文件/RWX | 内置 |
| iSCSI | 块 | 0.2-1ms | 高 | 块存储/RWO | 内置 |
| Ceph RBD | 块 | 0.1-0.5ms | 高 | 分布式块 | CSI |
| CephFS | 文件 | 0.2-1ms | 中 | 分布式文件 | CSI |
| FC | 块 | 0.1-0.3ms | 最高 | 企业 SAN | 内置 |
| NVMe-oF | 块 | <0.1ms | 最高 | 高性能 | CSI |
| 云块存储 | 块 | 0.5-2ms | 高 | 云环境 | CSI |

## NFS 存储网络

### NFS CSI 配置

```yaml
# NFS CSI Driver StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.storage.svc.cluster.local
  share: /exports/k8s
  # 性能参数
  mountOptions: "nfsvers=4.1,rsize=1048576,wsize=1048576,hard,timeo=600,retrans=2"
mountOptions:
  - nfsvers=4.1
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=2
  - noresvport
reclaimPolicy: Delete
volumeBindingMode: Immediate
---
# PVC 使用
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-data
spec:
  accessModes: ["ReadWriteMany"]  # NFS 支持 RWX
  storageClassName: nfs-csi
  resources:
    requests:
      storage: 100Gi
```

### NFS 性能调优

```bash
# 服务端调优 (/etc/sysctl.d/nfs.conf)
# 增加 NFS 线程数
fs.nfs.nlm_tcpport = 0
fs.nfs.nlm_udpport = 0
# 增加 RPC 缓冲
sunrpc.tcp_slot_table_entries = 128
sunrpc.udp_slot_table_entries = 128

# 客户端挂载优化
# 大文件顺序写: rsize=1048576,wsize=1048576
# 小文件随机: rsize=32768,wsize=32768
# 高延迟网络: hard,timeo=600,retrans=3
# 低延迟网络: soft,timeo=100,retrans=1

# 验证 NFS 性能
fio --name=nfs-test --directory=/mnt/nfs --rw=randwrite \
  --bs=4k --size=1G --numjobs=4 --runtime=60 --group_reporting
```

## iSCSI 存储网络

### iSCSI 配置

```yaml
# iSCSI PV（静态供给）
apiVersion: v1
kind: PersistentVolume
metadata:
  name: iscsi-pv
spec:
  capacity:
    storage: 500Gi
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: Retain
  storageClassName: iscsi
  iscsi:
    targetPortal: "10.0.1.100:3260"
    iqn: "iqn.2024-01.com.storage:target1"
    lun: 0
    fsType: "ext4"
    readOnly: false
    chapAuthSession: true
    secretRef:
      name: iscsi-secret
      namespace: kube-system
---
apiVersion: v1
kind: Secret
metadata:
  name: iscsi-secret
  namespace: kube-system
type: kubernetes.io/iscsi-chap
data:
  discovery.sendtargets.auth.username: dXNlcg==
  discovery.sendtargets.auth.password: cGFzcw==
  node.session.auth.username: dXNlcg==
  node.session.auth.password: cGFzcw==
```

### 多路径（Multipath）

```bash
# /etc/multipath.conf
defaults {
    user_friendly_names yes
    find_multipaths yes
}
devices {
    device {
        vendor "STORAGE_VENDOR"
        product "PRODUCT"
        path_grouping_policy "multibus"
        path_selector "round-robin 0"
        failback immediate
        rr_weight uniform
        no_path_retry 5
    }
}

# 验证多路径
multipath -ll
# mpatha (36001405...) dm-2 VENDOR,PRODUCT
# size=500G features='1 queue_if_no_path' hwhandler='0' wp=rw
# |-+- policy='round-robin 0' prio=1 status=active
# | |- 2:0:0:1 sdb 8:16 active ready running
# | `- 3:0:0:1 sdc 8:32 active ready running
# `-+- policy='round-robin 0' prio=1 status=enabled
#   |- 2:0:1:1 sdd 8:48 active ready running
#   `- 3:0:1:1 sde 8:64 active ready running
```

## Ceph 存储网络

### Ceph 集群网络架构

```
┌─────────────────────────────────────────────────────────┐
│  Ceph 集群                                              │
│                                                         │
│  客户端网络 (Public Network): 10/25 GbE                 │
│  ┌─────┐  ┌─────┐  ┌─────┐                           │
│  │ MON │  │ MDS │  │ RGW │  ← 客户端访问              │
│  └─────┘  └─────┘  └─────┘                           │
│                                                         │
│  集群网络 (Cluster Network): 25/100 GbE 或 RDMA        │
│  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐                 │
│  │ OSD │  │ OSD │  │ OSD │  │ OSD │  ← 数据复制      │
│  │ 1   │  │ 2   │  │ 3   │  │ N   │                 │
│  └─────┘  └─────┘  └─────┘  └─────┘                 │
│                                                         │
│  网络分离: Public ≠ Cluster（避免复制流量影响客户端）    │
└─────────────────────────────────────────────────────────┘
```

### Rook-Ceph StorageClass

```yaml
apiVersion: ceph.rook.io/v1
kind: CephBlockPool
metadata:
  name: replicapool
  namespace: rook-ceph
spec:
  failureDomain: host          # 跨主机副本
  replicated:
    size: 3                    # 3 副本
    requireSafeReplicaSize: true
  compression:
    mode: none                 # 或 passive/aggressive
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  imageFormat: "2"
  imageFeatures: layering,exclusive-lock,object-map,fast-diff
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
  csi.storage.k8s.io/fstype: ext4
reclaimPolicy: Delete
allowVolumeExpansion: true
mountOptions:
  - discard  # TRIM 支持
```

## 存储网络性能调优

### 网络参数

```bash
# 存储网络专用调优 (/etc/sysctl.d/storage-network.conf)

# 大缓冲区（高带宽）
net.core.rmem_max = 67108864
net.core.wmem_max = 67108864
net.core.rmem_default = 67108864
net.core.wmem_default = 67108864

# TCP 窗口
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864

# 队列
net.core.netdev_max_backlog = 30000
net.core.optmem_max = 40960

# Jumbo Frame（如果网络支持）
# ip link set eth1 mtu 9000

# RDMA (RoCE) 配置
# 启用 PFC (Priority Flow Control)
# mlnx_qos -i eth1 --pfc 0,0,0,1,0,0,0,0
```

### 存储网络隔离

```yaml
# 存储网络专用 VLAN/接口
# 确保存储流量不与业务流量竞争

# 节点网络配置示例:
# eth0: 业务网络 (10 GbE) — Pod 通信
# eth1: 存储网络 (25 GbE) — iSCSI/NFS/Ceph
# eth2: 管理网络 (1 GbE) — SSH/IPMI

# K8s 中通过 CSI 驱动指定存储网络接口
# 例: Ceph CSI 使用 clusterNetwork 参数
```

## 故障排查

| 问题 | 诊断 | 解决 |
|------|------|------|
| NFS 挂载超时 | `showmount -e <server>` | 检查防火墙/NFS 服务 |
| iSCSI 连接失败 | `iscsiadm -m session` | 检查 target/CHAP |
| Ceph 慢 IO | `ceph osd perf` | 检查 OSD/网络 |
| 多路径故障 | `multipath -ll` | 检查路径/光纤 |
| 存储网络丢包 | `ethtool -S <iface>` | 检查 MTU/缓冲 |
| PV 挂载失败 | `kubectl describe pod` | 检查 CSI 日志 |
| 扩容失败 | `kubectl describe pvc` | 检查 StorageClass |

## 最佳实践

| 实践 | 说明 |
|------|------|
| 网络分离 | 存储流量走专用网络 |
| Jumbo Frame | 存储网络启用 MTU 9000 |
| 多路径 | 块存储必须配置 multipath |
| 加密传输 | 敏感数据用加密协议 |
| 监控延迟 | 存储网络延迟 < 1ms |
| 容量规划 | 预留 30% 带宽余量 |
| 故障域 | 副本跨机架/AZ 分布 |
| 定期测试 | fio 基准测试验证性能 |

## Related

- [[06-存储/05-存储网络/index.md|存储网络]]
- [[06-存储/05-存储网络/02-csi-driver-architecture.md|CSI 驱动架构]]
- [[06-存储/index.md|存储]]
- [[17-系统基础/01-Linux/02-linux-kernel-container-fundamentals.md|Linux 内核基础]]
