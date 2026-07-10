---
title: NFS CSI 生产部署指南
description: 'NFS CSI Driver 在 Kubernetes 上的生产部署：安装配置、NFS Server Provisioner、权限安全、性能调优与高可用架构'
summary: 'NFS CSI Driver 在 Kubernetes 上的生产部署：安装配置、NFS Server Provisioner、权限安全、性能调优与高可用架构'
category: storage-data
tags:
- storage
- k8s
- nfs
- csi
- performance
- high-availability
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
- NFS CSI 生产部署 是什么
- 如何 NFS CSI Driver 安装配置
- NFS 性能调优 rsize wsize
- 高可用 NFS 架构
trigger_keywords:
- NFS
- CSI Driver
- NFS Server
- rsize
- wsize
- nfsvers
- 高可用
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# NFS CSI 生产部署指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **NFS CSI Driver**: v4.x | **最后更新**: 2026-07
> **文档定位**: NFS 是最广泛支持的共享文件系统协议。本文覆盖 NFS CSI Driver 部署、性能调优、安全配置和高可用架构。

## 1. 架构概览

### 1.1 NFS CSI 架构

```
┌─────────────────────────────────────────────────────────┐
│               Kubernetes Cluster                         │
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  NFS CSI Driver                                      ││
│  │  ┌──────────────┐  ┌──────────────────────────────┐ ││
│  │  │ CSI          │  │ Node Driver                  │ ││
│  │  │ Controller   │  │ (每节点 DaemonSet)           │ ││
│  │  │  ├─ Provision│  │  ├─ mount.nfs4               │ ││
│  │  │  ├─ Delete   │  │  ├─ umount                   │ ││
│  │  │  └─ Expand   │  │  └─ 权限管理                 │ ││
│  │  └──────────────┘  └──────────────────────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  Application Pods                                    ││
│  │  ┌─────┐ ┌─────┐ ┌─────┐                            ││
│  │  │Pod A│ │Pod B│ │Pod C│  ← 共享 NFS 卷            ││
│  │  └─────┘ └─────┘ └─────┘                            ││
│  └─────────────────────────────────────────────────────┘│
│                        │ NFSv4                           │
│                        ▼                                 │
│  ┌─────────────────────────────────────────────────────┐│
│  │  NFS Server (外部或集群内)                           ││
│  │  ┌──────────────┐  ┌──────────────┐                 ││
│  │  │ NFS Server 1 │  │ NFS Server 2 │ (HA)           ││
│  │  └──────────────┘  └──────────────┘                 ││
│  └─────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────┘
```

### 1.2 NFS 协议版本对比

| 版本 | 特性 | 性能 | 安全性 | 适用场景 |
|------|------|------|--------|---------|
| **NFSv3** | 无状态、UDP/TCP | 中等 | 弱（AUTH_SYS） | 遗留系统兼容 |
| **NFSv4.0** | 有状态、TCP | 好 | 中等（Kerberos 可选） | 通用场景 |
| **NFSv4.1** | pNFS、多路径 | 更好 | 中等 | 大规模并行访问 |
| **NFSv4.2** | 服务端复制、稀疏文件 | 最好 | 强 | 生产环境首选 |

## 2. NFS CSI Driver 安装

### 2.1 Helm 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add csi-driver-nfs https://raw.githubusercontent.com/kubernetes-csi/csi-driver-nfs/master/charts
helm repo update

# 安装 NFS CSI Driver
helm install csi-driver-nfs csi-driver-nfs/csi-driver-nfs \
  --namespace kube-system \
  --version v4.9.0 \
  --set kubeletDir=/var/lib/kubelet \
  --set controller.replicas=2 \
  --set controller.resources.limits.cpu=500m \
  --set controller.resources.limits.memory=512Mi \
  --set node.resources.limits.cpu=500m \
  --set node.resources.limits.memory=256Mi \
  --wait

# 验证安装
kubectl get pods -n kube-system -l app=csi-nfs-controller
kubectl get pods -n kube-system -l app=csi-nfs-node
```
### 2.2 StorageClass 配置

```yaml
# nfs-storageclass.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/data
  # 挂载选项（性能调优关键）
  mountOptions:
    - nfsvers=4.2           # 使用 NFSv4.2
    - rsize=1048576         # 读缓冲区 1MB
    - wsize=1048576         # 写缓冲区 1MB
    - hard                  # 硬挂载（推荐）
    - timeo=600             # 超时 60 秒
    - retrans=2             # 重试次数
    - noatime               # 不更新访问时间
    - sync                  # 同步写入（数据安全）
    # 或 async（性能更好，但有数据丢失风险）
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
---
# 只读共享 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi-readonly
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/shared
mountOptions:
  - nfsvers=4.2
  - rsize=1048576
  - wsize=1048576
  - ro
  - noatime
reclaimPolicy: Retain
volumeBindingMode: Immediate
```

### 2.3 PVC 创建

```yaml
# nfs-pvc.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nfs-shared-data
  namespace: default
spec:
  accessModes:
    - ReadWriteMany
  storageClassName: nfs-csi
  resources:
    requests:
      storage: 100Gi
---
# 多 Pod 共享示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: shared-app
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: shared
  template:
    metadata:
      labels:
        app: shared
    spec:
      containers:
        - name: app
          image: nginx:alpine
          volumeMounts:
            - name: shared-vol
              mountPath: /data
      volumes:
        - name: shared-vol
          persistentVolumeClaim:
            claimName: nfs-shared-data
```

## 3. NFS Server Provisioner

### 3.1 集群内 NFS Server（开发测试）

```yaml
# nfs-server-provisioner.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: nfs-server
  namespace: nfs-system
spec:
  serviceName: nfs-server
  replicas: 1
  selector:
    matchLabels:
      app: nfs-server
  template:
    metadata:
      labels:
        app: nfs-server
    spec:
      containers:
        - name: nfs-server
          image: k8s.gcr.io/volume-nfs:0.8
          ports:
            - containerPort: 2049
              name: nfs
            - containerPort: 20048
              name: mountd
            - containerPort: 111
              name: rpcbind
          securityContext:
            privileged: true
          volumeMounts:
            - name: nfs-data
              mountPath: /exports
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "1"
              memory: 1Gi
  volumeClaimTemplates:
    - metadata:
        name: nfs-data
      spec:
        accessModes: [ReadWriteOnce]
        storageClassName: local-storage
        resources:
          requests:
            storage: 200Gi
---
apiVersion: v1
kind: Service
metadata:
  name: nfs-server
  namespace: nfs-system
spec:
  ports:
    - name: nfs
      port: 2049
    - name: mountd
      port: 20048
    - name: rpcbind
      port: 111
  selector:
    app: nfs-server
```

### 3.2 使用集群内 NFS Server 的 StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-internal
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.nfs-system.svc.cluster.local
  share: /
mountOptions:
  - nfsvers=4.2
  - rsize=1048576
  - wsize=1048576
  - hard
  - noatime
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

## 4. 权限与安全配置

### 4.1 NFS 导出配置

```bash
# /etc/exports 配置示例（NFS Server 端）
# 格式: /export/path  client(options)

# 允许 K8s 节点访问
/exports/data    10.0.0.0/24(rw,sync,no_subtree_check,no_root_squash)
/exports/shared  10.0.0.0/24(ro,sync,no_subtree_check,root_squash)

# 使用 Kerberos 安全（推荐生产环境）
/exports/secure  10.0.0.0/24(rw,sync,no_subtree_check,sec=krb5p)

# 应用导出配置
sudo exportfs -ra

# 验证导出
sudo exportfs -v
```

### 4.2 NFS 挂载安全选项

```yaml
# 安全加固的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi-secure
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/secure
mountOptions:
  - nfsvers=4.2
  - sec=krb5p              # Kerberos 加密
  - hard
  - timeo=600
  - retrans=2
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

### 4.3 Kerberos 配置

```yaml
# Kerberos 配置 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: krb5-config
  namespace: kube-system
data:
  krb5.conf: |
    [libdefaults]
      default_realm = EXAMPLE.COM
      dns_lookup_realm = false
      dns_lookup_kdc = false
      ticket_lifetime = 24h
      renew_lifetime = 7d
      forwardable = true
    
    [realms]
      EXAMPLE.COM = {
        kdc = kdc.example.com
        admin_server = kdc.example.com
      }
    
    [domain_realm]
      .example.com = EXAMPLE.COM
      example.com = EXAMPLE.COM
---
# 在 Node Driver 中挂载 Kerberos 配置
# 需要修改 CSI Driver DaemonSet
```

### 4.4 RBAC 配置

```yaml
# nfs-csi-rbac.yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: nfs-csi-provisioner
rules:
  - apiGroups: [""]
    resources: ["persistentvolumes"]
    verbs: ["get", "list", "watch", "create", "delete"]
  - apiGroups: [""]
    resources: ["persistentvolumeclaims"]
    verbs: ["get", "list", "watch", "update"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses"]
    verbs: ["get", "list", "watch"]
  - apiGroups: [""]
    resources: ["events"]
    verbs: ["list", "watch", "create", "update", "patch"]
  - apiGroups: ["snapshot.storage.k8s.io"]
    resources: ["volumesnapshots"]
    verbs: ["get", "list"]
  - apiGroups: ["snapshot.storage.k8s.io"]
    resources: ["volumesnapshotcontents"]
    verbs: ["get", "list"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["csinodes"]
    verbs: ["get", "list", "watch"]
```

## 5. 性能调优

### 5.1 NFS 挂载参数详解

| 参数 | 默认值 | 说明 | 生产建议 |
|------|--------|------|---------|
| `nfsvers` | 4.2 | NFS 协议版本 | 4.2（推荐） |
| `rsize` | 1MB | 读缓冲区大小 | 1048576 (1MB) |
| `wsize` | 1MB | 写缓冲区大小 | 1048576 (1MB) |
| `hard` | hard | 硬/软挂载 | hard（数据安全） |
| `timeo` | 600 (60s) | 超时时间 | 600 |
| `retrans` | 2 | 重试次数 | 2-3 |
| `noatime` | - | 不更新访问时间 | 开启（性能提升） |
| `sync` | sync | 同步/异步写 | sync（数据安全） |
| `nconnect` | 1 | TCP 连接数 | 8（NFSv4.1+） |
| `max_connect` | 1 | 最大连接数 | 16（内核 5.4+） |

### 5.2 高性能 StorageClass

```yaml
# nfs-high-performance.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-high-perf
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/highperf
mountOptions:
  # 协议版本
  - nfsvers=4.2
  
  # I/O 缓冲区（大值提升吞吐量）
  - rsize=1048576         # 1MB 读缓冲
  - wsize=1048576         # 1MB 写缓冲
  
  # 多路径（NFSv4.1+）
  - nconnect=8            # 8 个 TCP 连接
  
  # 超时与重试
  - hard                  # 硬挂载
  - timeo=600             # 60 秒超时
  - retrans=3             # 3 次重试
  
  # 性能优化
  - noatime               # 不更新访问时间
  - async                 # 异步写（性能优先，有数据风险）
  
  # 读写优化
  - rdirplus              # READDIRPLUS 优化目录遍历
  - lookupcache=positive  # 正向查找缓存
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

### 5.3 NFS Server 端调优

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# /etc/nfs.conf (NFS Server 配置)
[nfsd]
  threads=32              # NFS 线程数（根据 CPU 核数调整）
  tcp=y                   # 启用 TCP
  udp=n                   # 禁用 UDP（生产环境推荐 TCP）
  
  # NFSv4.2 特性
  vers4.2=y
  
[exportfs]
  # 导出选项
  fsid=0                  # 根导出 ID

# 系统级调优
# /etc/sysctl.conf
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
net.core.netdev_max_backlog = 5000

# 应用配置
sudo sysctl -p
sudo systemctl restart nfs-server
```
### 5.4 fio 基准测试

```yaml
# nfs-fio-test.yaml
apiVersion: v1
kind: Pod
metadata:
  name: nfs-fio
  namespace: benchmark
spec:
  containers:
    - name: fio
      image: ljishen/fio:latest
      command: ["sleep", "3600"]
      volumeMounts:
        - name: nfs-vol
          mountPath: /data
  volumes:
    - name: nfs-vol
      persistentVolumeClaim:
        claimName: nfs-fio-test
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试 PVC
kubectl create namespace benchmark
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nfs-fio-test
  namespace: benchmark
spec:
  accessModes: [ReadWriteMany]
  storageClassName: nfs-high-perf
  resources:
    requests:
      storage: 50Gi
EOF

# 顺序写测试
kubectl exec -n benchmark nfs-fio -- fio \
  --name=seq-write \
  --ioengine=libaio \
  --direct=1 \
  --bs=1M \
  --size=5G \
  --numjobs=4 \
  --runtime=60 \
  --rw=write \
  --filename=/data/test-seq-write

# 顺序读测试
kubectl exec -n benchmark nfs-fio -- fio \
  --name=seq-read \
  --ioengine=libaio \
  --direct=1 \
  --bs=1M \
  --size=5G \
  --numjobs=4 \
  --runtime=60 \
  --rw=read \
  --filename=/data/test-seq-read

# 随机读写
kubectl exec -n benchmark nfs-fio -- fio \
  --name=rand-rw \
  --ioengine=libaio \
  --direct=1 \
  --bs=4k \
  --size=5G \
  --numjobs=8 \
  --runtime=120 \
  --rw=randrw \
  --rwmixread=70 \
  --filename=/data/test-rand-rw
```
### 5.5 预期性能指标

| 场景 | 吞吐量 | IOPS | 延迟 | 配置 |
|------|--------|------|------|------|
| **顺序读** | 500-1000 MB/s | - | 2-10ms | 10GbE, rsize=1MB |
| **顺序写** | 300-800 MB/s | - | 5-20ms | 10GbE, wsize=1MB, async |
| **随机读** | - | 10K-30K | 1-5ms | SSD 后端 |
| **随机写** | - | 5K-15K | 5-20ms | SSD 后端, async |
| **多客户端** | 线性扩展 | 线性扩展 | 稳定 | nconnect=8 |

> **注意**: NFS 性能受网络带宽、服务端磁盘、客户端数量等多因素影响。以上数据为单客户端参考值。

## 6. 高可用 NFS 架构

### 6.1 高可用架构设计

```
┌─────────────────────────────────────────────────────────┐
│              High Availability NFS Architecture          │
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  Kubernetes Nodes                                    ││
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐               ││
│  │  │Node 1│ │Node 2│ │Node 3│ │Node 4│               ││
│  │  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘               ││
│  └─────┼────────┼────────┼────────┼────────────────────┘│
│        │        │        │        │                      │
│        └────────┴────┬───┴────────┘                      │
│                      │ NFS                               │
│                      ▼                                   │
│  ┌─────────────────────────────────────────────────────┐│
│  │  Load Balancer (VIP)                                 ││
│  │  10.0.0.100:2049                                     ││
│  └─────────────────────────────────────────────────────┘│
│                      │                                   │
│        ┌─────────────┴─────────────┐                    │
│        ▼                           ▼                    │
│  ┌──────────────┐           ┌──────────────┐            │
│  │ NFS Server 1 │           │ NFS Server 2 │            │
│  │ (Active)     │◄─────────►│ (Standby)    │            │
│  │              │  DRBD/    │              │            │
│  │              │  GlusterFS│              │            │
│  └──────────────┘           └──────────────┘            │
│        │                           │                    │
│        ▼                           ▼                    │
│  ┌──────────────┐           ┌──────────────┐            │
│  │ Storage      │           │ Storage      │            │
│  │ (RAID/SSD)   │           │ (RAID/SSD)   │            │
│  └──────────────┘           └──────────────┘            │
└─────────────────────────────────────────────────────────┘
```

### 6.2 NFS Ganesha 高可用部署

```yaml
# nfs-ganesha-ha.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: nfs-ganesha
  namespace: nfs-system
spec:
  serviceName: nfs-ganesha
  replicas: 2
  selector:
    matchLabels:
      app: nfs-ganesha
  template:
    metadata:
      labels:
        app: nfs-ganesha
    spec:
      containers:
        - name: ganesha
          image: nfs-ganesha:latest
          ports:
            - containerPort: 2049
              name: nfs
            - containerPort: 875
              name: rquota
          securityContext:
            privileged: true
          volumeMounts:
            - name: ganesha-config
              mountPath: /etc/ganesha
            - name: nfs-data
              mountPath: /export
          resources:
            requests:
              cpu: "1"
              memory: 2Gi
            limits:
              cpu: "2"
              memory: 4Gi
      volumes:
        - name: ganesha-config
          configMap:
            name: ganesha-config
  volumeClaimTemplates:
    - metadata:
        name: nfs-data
      spec:
        accessModes: [ReadWriteOnce]
        storageClassName: local-storage
        resources:
          requests:
            storage: 500Gi
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: ganesha-config
  namespace: nfs-system
data:
  ganesha.conf: |
    EXPORT
    {
        Export_Id = 1;
        Path = /export;
        Pseudo = /export;
        Access_Type = RW;
        Squash = No_Root_Squash;
        Protocols = 4;
        Transports = TCP;
        SecType = sys;
        
        FSAL {
            Name = VFS;
        }
    }
    
    NFS_CORE_PARAM {
        NFS_Port = 2049;
        NFS_Protocols = 4;
        Enable_RQUOTA = false;
    }
```

### 6.3 VIP 配置

```yaml
# 使用 MetalLB 或 Keepalived 实现 VIP
# MetalLB L2 配置示例
apiVersion: v1
kind: Service
metadata:
  name: nfs-ganesha-vip
  namespace: nfs-system
  annotations:
    metallb.universe.tf/allow-shared-ip: nfs
spec:
  type: LoadBalancer
  loadBalancerIP: 10.0.0.100
  ports:
    - name: nfs
      port: 2049
      targetPort: 2049
  selector:
    app: nfs-ganesha
```

### 6.4 使用 HA VIP 的 StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-ha
provisioner: nfs.csi.k8s.io
parameters:
  server: 10.0.0.100          # VIP 地址
  share: /export
mountOptions:
  - nfsvers=4.2
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=3
  - noatime
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

## 7. 监控与告警

### 7.1 Prometheus 监控

```yaml
# nfs-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: nfs-csi-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: csi-nfs-controller
  namespaceSelector:
    matchNames:
      - kube-system
  endpoints:
    - port: metrics
      interval: 15s
```

### 7.2 NFS Server 监控

```yaml
# nfs-exporter.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nfs-exporter
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: nfs-exporter
  template:
    metadata:
      labels:
        app: nfs-exporter
    spec:
      containers:
        - name: exporter
          image: prometheus/nfs-exporter:latest
          ports:
            - containerPort: 9347
          args:
            - --nfs-server=nfs-server.example.com
```

### 7.3 告警规则

```yaml
# nfs-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: nfs-alerts
  namespace: monitoring
spec:
  groups:
    - name: nfs.rules
      rules:
        # NFS Server 不可用
        - alert: NFSServerDown
          expr: probe_success{job="nfs-server"} == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "NFS Server {{ $labels.instance }} 不可用"

        # NFS 挂载延迟高
        - alert: NFSMountLatencyHigh
          expr: nfs_mount_latency_seconds > 1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "NFS 挂载延迟超过 1 秒"

        # NFS I/O 错误
        - alert: NFSIOErrors
          expr: rate(nfs_io_errors_total[5m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "NFS I/O 错误率异常"

        # NFS 连接数过高
        - alert: NFSConnectionsHigh
          expr: nfs_connections > 1000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "NFS 连接数超过 1000，可能存在性能问题"
```

## 8. 故障排查

### 8.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| 挂载超时 | 网络或防火墙问题 | 检查网络连通性和端口 2049/111 |
| 权限拒绝 | NFS 导出配置或 squash 设置 | 检查 /etc/exports 和 root_squash |
| 性能差 | rsize/wsize 或网络带宽 | 调整缓冲区大小和网络配置 |
| Stale file handle | 服务端重启或导出变更 | 重新挂载或重启客户端 Pod |
| 写入慢 | sync 写入或磁盘瓶颈 | 使用 async 或优化服务端存储 |

### 8.2 诊断命令

```bash
# 检查 NFS 挂载状态
mount | grep nfs

# 查看 NFS 统计信息
cat /proc/mounts | grep nfs
nfsstat -m

# 检查 NFS Server 连接
showmount -e nfs-server.example.com

# 测试 NFS 性能
fio --name=nfs-test --ioengine=posixaio --rw=randrw --bs=4k \
    --size=1G --numjobs=4 --runtime=60 --filename=/mnt/nfs/test

# 查看 NFS 客户端日志
dmesg | grep nfs

# 检查网络连通性
rpcinfo -p nfs-server.example.com

# 重新挂载（不重启 Pod）
mount -o remount /mnt/nfs
```

## 9. 生产最佳实践

### 9.1 容量规划

```yaml
capacity_planning:
  # NFS Server 存储
  server_storage:
    type: "SSD/NVMe RAID"
    usable_ratio: 0.7  # RAID 和快照预留
  
  # 网络带宽
  network:
    minimum: "10Gbps"
    recommended: "25Gbps+"
  
  # 客户端并发
  clients:
    per_server: 100  # 单 NFS Server 最大客户端数
    threads: 32      # NFS 线程数
```

### 9.2 备份策略

```yaml
backup_strategy:
  # NFS Server 快照
  snapshots:
    schedule: "0 */4 * * *"
    retention: 48
  
  # Velero 备份
  velero:
    schedule: "0 2 * * *"
    retention: 30d
  
  # 异地复制
  replication:
    method: "rsync/rclone"
    schedule: "0 */6 * * *"
    target: "dr-nfs-server"
```

### 9.3 安全加固

```yaml
security:
  # 协议安全
  protocol:
    - 使用 NFSv4.2
    - 启用 Kerberos 认证 (sec=krb5p)
    - 禁用 NFSv3
  
  # 网络安全
  network:
    - 使用 NetworkPolicy 限制访问
    - NFS 流量走专用 VLAN
    - 防火墙规则限制源 IP
  
  # 访问控制
  access:
    - 使用 root_squash（生产环境）
    - 配置适当的文件权限
    - 定期审计导出列表
```

---

## Related

- [[03-longhorn-production|Longhorn 生产部署]]
- [[04-openebs-production|OpenEBS 生产部署]]

## See Also

- [NFS CSI Driver](https://github.com/kubernetes-csi/csi-driver-nfs)
- [NFS Ganesha](https://nfs-ganesha.github.io/)
- [NFS 最佳实践](https://www.kernel.org/doc/Documentation/filesystems/nfs/)


<!-- risk-assessed -->
