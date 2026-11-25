---
title: JuiceFS on Kubernetes 生产部署指南
description: 'JuiceFS 分布式文件系统在 K8s 上的生产部署：元数据引擎选型、对象存储后端配置、CSI Driver 安装、缓存策略与性能调优'
summary: 'JuiceFS 分布式文件系统在 K8s 上的生产部署：元数据引擎选型、对象存储后端配置、CSI Driver 安装、缓存策略与性能调优'
category: storage-data
tags:
- storage
- k8s
- juicefs
- distributed-filesystem
- csi
- s3
- cache
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
- JuiceFS on K8s 是什么
- 如何 JuiceFS 元数据引擎选型
- JuiceFS CSI Driver 安装配置
- JuiceFS 缓存策略调优
trigger_keywords:
- JuiceFS
- 分布式文件系统
- 元数据引擎
- TiKV
- 对象存储
- CSI Driver
- 缓存策略
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


# JuiceFS on Kubernetes 生产部署指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **JuiceFS**: v1.1+ | **CSI Driver**: v0.24+ | **最后更新**: 2026-07
> **文档定位**: JuiceFS 是云原生分布式文件系统，将元数据与数据分离存储，支持多种后端。本文聚焦 K8s 生产环境部署。

## 1. 架构概览

### 1.1 JuiceFS 核心架构

```
┌─────────────────────────────────────────────────────────┐
│                    JuiceFS Architecture                  │
│                                                          │
│  ┌──────────────┐     ┌──────────────────────────────┐  │
│  │  JuiceFS     │     │  Metadata Engine              │  │
│  │  Client      │◄───►│  ┌────────┬────────┬────────┐│  │
│  │  (FUSE/K8s)  │     │  │ TiKV   │ Redis  │ MySQL  ││  │
│  └──────┬───────┘     │  │ (推荐)  │ (快速) │ (稳定) ││  │
│         │             │  └────────┴────────┴────────┘│  │
│         │             └──────────────────────────────┘  │
│         │                                                │
│         │             ┌──────────────────────────────┐  │
│         └────────────►│  Data Storage (Object Store)  │  │
│                       │  ┌────────┬────────┬────────┐│  │
│                       │  │ S3     │ OSS    │ MinIO  ││  │
│                       │  └────────┴────────┴────────┘│  │
│                       └──────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

**核心设计原则**:
- **元数据与数据分离**: 元数据存储在专用数据库，数据存储在对象存储
- **POSIX 兼容**: 支持标准文件系统接口，应用无需修改
- **多级缓存**: 内核缓存 → 本地磁盘缓存 → 分布式缓存 → 对象存储
- **强一致性**: 默认提供强一致性语义

### 1.2 元数据引擎对比

| 引擎 | 一致性 | 性能 | 运维复杂度 | 适用场景 |
|------|--------|------|-----------|---------|
| **TiKV** | 强一致（Raft） | 高 | 中等 | 生产环境首选，大规模集群 |
| **Redis** | 异步/同步 | 极高 | 低 | 小规模集群、开发测试 |
| **MySQL** | 强一致 | 中等 | 低 | 已有 MySQL 基础设施 |
| **PostgreSQL** | 强一致 | 中等 | 低 | 已有 PG 基础设施 |
| **SQLite** | 本地一致 | 高 | 最低 | 单节点测试 |

### 1.3 K8s 集成架构

```
┌─────────────────────────────────────────────────────┐
│               Kubernetes Cluster                     │
│                                                      │
│  ┌─────────────────────────────────────────────────┐│
│  │  JuiceFS CSI Driver                              ││
│  │  ┌──────────────┐  ┌──────────────────────────┐ ││
│  │  │ Controller   │  │ Node Driver              │ ││
│  │  │ (Provisioner)│  │ (每节点 DaemonSet)       │ ││
│  │  │  ├─ 创建 PVC │  │  ├─ 挂载 JuiceFS         │ ││
│  │  │  └─ 删除 PVC │  │  └─ 缓存管理             │ ││
│  │  └──────────────┘  └──────────────────────────┘ ││
│  └─────────────────────────────────────────────────┘│
│                                                      │
│  ┌─────────────────────────────────────────────────┐│
│  │  Application Pods                                ││
│  │  ┌─────┐ ┌─────┐ ┌─────┐                        ││
│  │  │Pod A│ │Pod B│ │Pod C│  ← 共享 JuiceFS 卷     ││
│  │  └─────┘ └─────┘ └─────┘                        ││
│  └─────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────┘
```

## 2. 环境准备

### 2.1 元数据引擎部署

#### TiKV 部署（推荐）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 TiDB Operator 部署 TiKV
# 1. 安装 TiDB Operator
helm repo add pingcap https://charts.pingcap.org/
helm install tidb-operator pingcap/tidb-operator \
  --namespace tidb-admin \
  --create-namespace \
  --version v1.6.0

# 2. 部署 TiKV 集群
cat <<EOF | kubectl apply -f -
apiVersion: pingcap.com/v1alpha1
kind: TidbCluster
metadata:
  name: tikv-juicefs
  namespace: tikv
spec:
  version: v7.5.0
  pd:
    replicas: 3
    requests:
      storage: 10Gi
    config:
      schedule:
        max-merge-region-size: 20
        max-merge-region-keys: 200000
  tikv:
    replicas: 3
    requests:
      storage: 100Gi
    config:
      storage:
        reserve-space: "0MB"
      raftstore:
        raft-log-gc-count-limit: 100000
      rocksdb:
        max-open-files: 10000
EOF

# 3. 验证 TiKV 集群
kubectl get pods -n tikv
kubectl get tidbcluster tikv-juicefs -n tikv
```
#### Redis 部署

```yaml
# redis-juicefs.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-juicefs
  namespace: juicefs
spec:
  serviceName: redis-juicefs
  replicas: 1
  selector:
    matchLabels:
      app: redis-juicefs
  template:
    metadata:
      labels:
        app: redis-juicefs
    spec:
      containers:
        - name: redis
          image: redis:7-alpine
          ports:
            - containerPort: 6379
          command:
            - redis-server
            - --appendonly yes
            - --maxmemory 4gb
            - --maxmemory-policy allkeys-lru
            - --save 60 1000
          volumeMounts:
            - name: data
              mountPath: /data
          resources:
            requests:
              cpu: "1"
              memory: 4Gi
            limits:
              cpu: "2"
              memory: 6Gi
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: [ReadWriteOnce]
        storageClassName: local-storage
        resources:
          requests:
            storage: 20Gi
---
apiVersion: v1
kind: Service
metadata:
  name: redis-juicefs
  namespace: juicefs
spec:
  ports:
    - port: 6379
  selector:
    app: redis-juicefs
```

### 2.2 对象存储配置

```yaml
# juicefs-secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: juicefs-secret
  namespace: juicefs
type: Opaque
stringData:
  # 元数据引擎配置
  # TiKV 格式: tikv://pd-0.pd.tikv.svc:2379,pd-1.pd.tikv.svc:2379,pd-2.pd.tikv.svc:2379
  # Redis 格式: redis://redis-juicefs.juicefs.svc:6379/0
  # MySQL 格式: mysql://user:password@tcp(mysql.host:3306)/juicefs
  metaurl: "tikv://pd-0.pd.tikv.svc:2379,pd-1.pd.tikv.svc:2379,pd-2.pd.tikv.svc:2379"
  
  # 对象存储配置（以 S3 为例）
  name: "myjfs"
  storage: "s3"
  bucket: "https://s3.amazonaws.com/my-bucket/juicefs"
  access-key: "AKIAIOSFODNN7EXAMPLE"
  secret-key: "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
  
  # 其他对象存储示例:
  # 阿里云 OSS:
  # storage: "oss"
  # bucket: "https://oss-cn-hangzhou.aliyuncs.com/my-bucket"
  
  # MinIO:
  # storage: "minio"
  # bucket: "http://minio.juicefs.svc:9000/my-bucket"
  
  # 格式化参数
  format-options: "--block-size 4096 --compress lz4"
```

### 2.3 格式化 JuiceFS 卷

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用临时 Pod 格式化卷
kubectl run juicefs-format --rm -it \
  --namespace juicefs \
  --image juicedata/mount:ee-5.1.2 \
  --command -- bash

# 在容器内执行格式化
juicefs format \
  --storage s3 \
  --bucket https://s3.amazonaws.com/my-bucket/juicefs \
  --access-key AKIAIOSFODNN7EXAMPLE \
  --secret-key wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY \
  tikv://pd-0.pd.tikv.svc:2379,pd-1.pd.tikv.svc:2379,pd-2.pd.tikv.svc:2379 \
  myjfs

# 验证格式化成功
juicefs status tikv://pd-0.pd.tikv.svc:2379,pd-1.pd.tikv.svc:2379,pd-2.pd.tikv.svc:2379
```
## 3. CSI Driver 安装

### 3.1 Helm 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add juicefs https://juicedata.github.io/charts/
helm repo update

# 安装 CSI Driver
helm install juicefs-csi-driver juicefs/juicefs-csi-driver \
  --namespace juicefs \
  --create-namespace \
  --version 0.25.0 \
  --set controller.replicas=2 \
  --set controller.resources.limits.cpu=1 \
  --set controller.resources.limits.memory=1Gi \
  --set node.resources.limits.cpu=500m \
  --set node.resources.limits.memory=1Gi \
  --wait

# 验证安装
kubectl get pods -n juicefs
# juicefs-csi-controller-0   5/5     Running
# juicefs-csi-node-xxxxx     3/3     Running
```
### 3.2 StorageClass 配置

```yaml
# juicefs-storageclass.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-sc
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: juicefs
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: juicefs
  
  # JuiceFS 特有参数
  juicefs/mount-cpu-request: 500m
  juicefs/mount-memory-request: 1Gi
  juicefs/mount-cpu-limit: "1"
  juicefs/mount-memory-limit: 2Gi
  
  # 缓存配置
  juicefs/cache-dir: /var/jfsCache
  juicefs/cache-size: "102400"    # 100GB 缓存
  juicefs-cache-partial: "true"   # 启用部分缓存
  
  # 挂载选项
  juicefs/mount-options: "cache-group=k8s-cluster,max-uploads=50,writeback"
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
---
# 共享卷 StorageClass（ReadWriteMany）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-shared
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: juicefs
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: juicefs
reclaimPolicy: Retain
volumeBindingMode: Immediate
```

### 3.3 PVC 创建

```yaml
# juicefs-pvc.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: jfs-shared-data
  namespace: default
spec:
  accessModes:
    - ReadWriteMany
  storageClassName: juicefs-shared
  resources:
    requests:
      storage: 1Pi    # JuiceFS 实际容量由后端决定，这里填写一个大值
---
# 使用示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-server
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
        - name: nginx
          image: nginx:alpine
          volumeMounts:
            - name: shared-data
              mountPath: /usr/share/nginx/html
      volumes:
        - name: shared-data
          persistentVolumeClaim:
            claimName: jfs-shared-data
```

## 4. 缓存策略

### 4.1 缓存架构

```
┌─────────────────────────────────────────────────────┐
│              JuiceFS Multi-Level Cache               │
│                                                      │
│  Level 1: Kernel Cache (Page Cache)                  │
│  ┌──────────────────────────────────────────────┐   │
│  │  最快，但进程重启后失效                       │   │
│  └──────────────────────────────────────────────┘   │
│           │ cache-miss                               │
│           ▼                                          │
│  Level 2: Local Disk Cache                          │
│  ┌──────────────────────────────────────────────┐   │
│  │  /var/jfsCache/  (SSD/NVMe 推荐)             │   │
│  │  可配置大小，进程重启后保留                   │   │
│  └──────────────────────────────────────────────┘   │
│           │ cache-miss                               │
│           ▼                                          │
│  Level 3: Distributed Cache (可选)                   │
│  ┌──────────────────────────────────────────────┐   │
│  │  Redis Cluster / 共享缓存组                   │   │
│  │  多节点共享热数据                             │   │
│  └──────────────────────────────────────────────┘   │
│           │ cache-miss                               │
│           ▼                                          │
│  Level 4: Object Storage (S3/OSS/MinIO)             │
│  ┌──────────────────────────────────────────────┐   │
│  │  源数据，最高延迟                             │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### 4.2 本地缓存配置

```yaml
# 带缓存优化的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-cached
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: juicefs
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: juicefs
  
  # 本地缓存配置
  juicefs/cache-dir: /var/jfsCache
  juicefs/cache-size: "204800"    # 200GB 本地缓存
  
  # 挂载选项
  juicefs/mount-options: >-
    cache-dir=/var/jfsCache,
    cache-size=204800,
    cache-full-ratio=0.9,
    free-space-ratio=0.1,
    writeback,
    upload-limit=100,
    download-limit=200
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

### 4.3 分布式缓存配置

```yaml
# 使用 Redis 作为分布式缓存
# 在 Secret 中配置缓存组
apiVersion: v1
kind: Secret
metadata:
  name: juicefs-secret
  namespace: juicefs
type: Opaque
stringData:
  # ... 其他配置 ...
  
  # 分布式缓存配置
  cache-group: "k8s-production"
  # 或使用独立 Redis 作为缓存
  # cache-group: "redis://redis-cache.juicefs.svc:6379/1"
```

```bash
# 挂载时指定分布式缓存
juicefs mount ... /mnt/jfs \
  --cache-group k8s-production \
  --warmup /mnt/jfs/hot-data

# 预热常用数据到缓存
juicefs warmup /mnt/jfs/frequently-accessed-data
```

### 4.4 缓存策略选型建议

| 场景 | 缓存策略 | 配置建议 |
|------|---------|---------|
| **读密集型** | 大本地缓存 | cache-size = 50%+ 磁盘容量 |
| **写密集型** | 启用 writeback | writeback + upload-limit |
| **共享数据** | 分布式缓存 | cache-group + 预热 |
| **混合负载** | 本地 + 分布式 | cache-dir + cache-group |
| **冷数据** | 最小缓存 | cache-size = 10GB |

## 5. 性能调优

### 5.1 挂载选项调优

```yaml
# 高性能 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-high-perf
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: juicefs
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: juicefs
  
  juicefs/mount-options: >-
    # I/O 并发
    max-uploads=100,
    max-downloads=200,
    buffer-size=500,
    
    # 元数据缓存
    entry-cache=60,
    dir-entry-cache=60,
    attr-cache=60,
    
    # 写优化
    writeback,
    upload-limit=200,
    upload-delay=1,
    
    # 读优化
    prefetch=50,
    read-ahead=200,
    
    # 缓存配置
    cache-dir=/var/jfsCache,
    cache-size=204800,
    cache-full-ratio=0.95,
    free-space-ratio=0.05
reclaimPolicy: Delete
volumeBindingMode: Immediate
```

### 5.2 关键参数说明

| 参数 | 默认值 | 说明 | 调优建议 |
|------|--------|------|---------|
| `max-uploads` | 20 | 最大并发上传数 | 生产环境设为 50-100 |
| `max-downloads` | 20 | 最大并发下载数 | 设为 100-200 |
| `buffer-size` | 100 | 读写缓冲区 (MB) | 设为 300-500 |
| `prefetch` | 1 | 预读块数 | 设为 20-50 |
| `writeback` | false | 异步写回 | 写密集场景开启 |
| `cache-size` | 100MB | 本地缓存大小 (MB) | 设为磁盘容量的 50-80% |
| `entry-cache` | 1s | 元数据缓存时间 | 只读场景设为 60s |

### 5.3 fio 基准测试

```yaml
# juicefs-fio-test.yaml
apiVersion: v1
kind: Pod
metadata:
  name: juicefs-fio
  namespace: benchmark
spec:
  containers:
    - name: fio
      image: ljishen/fio:latest
      command: ["sleep", "3600"]
      volumeMounts:
        - name: jfs-vol
          mountPath: /data
  volumes:
    - name: jfs-vol
      persistentVolumeClaim:
        claimName: jfs-fio-test
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 顺序写测试
kubectl exec -n benchmark juicefs-fio -- fio \
  --name=seq-write \
  --ioengine=libaio \
  --direct=0 \
  --bs=1M \
  --size=5G \
  --numjobs=4 \
  --runtime=60 \
  --rw=write \
  --directory=/data

# 随机读测试
kubectl exec -n benchmark juicefs-fio -- fio \
  --name=rand-read \
  --ioengine=libaio \
  --direct=0 \
  --bs=4k \
  --size=5G \
  --numjobs=8 \
  --runtime=60 \
  --rw=randread \
  --directory=/data

# 混合负载
kubectl exec -n benchmark juicefs-fio -- fio \
  --name=mixed \
  --ioengine=libaio \
  --direct=0 \
  --bs=64k \
  --size=5G \
  --numjobs=4 \
  --runtime=120 \
  --rw=randrw \
  --rwmixread=70 \
  --directory=/data
```
### 5.4 预期性能指标

| 场景 | 吞吐量 | IOPS | 延迟 | 配置 |
|------|--------|------|------|------|
| **顺序读（缓存命中）** | 2000+ MB/s | - | <1ms | 本地 SSD 缓存 |
| **顺序写** | 500+ MB/s | - | 5-20ms | writeback 模式 |
| **随机读（缓存命中）** | - | 100K+ | <1ms | NVMe 缓存 |
| **随机写** | - | 20K+ | 5-50ms | writeback 模式 |
| **混合负载** | 300+ MB/s | 30K+ | 5-20ms | 缓存 + writeback |

## 6. 监控与运维

### 6.1 Prometheus 监控

```yaml
# juicefs-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: juicefs-csi
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: juicefs-csi-driver
  namespaceSelector:
    matchNames:
      - juicefs
  endpoints:
    - port: metrics
      interval: 15s
```

### 6.2 关键监控指标

```yaml
# juicefs-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: juicefs-alerts
  namespace: monitoring
spec:
  groups:
    - name: juicefs.rules
      rules:
        # 缓存命中率
        - alert: JuiceFSLowCacheHitRate
          expr: |
            juicefs_client_cache_hit / (juicefs_client_cache_hit + juicefs_client_cache_miss) < 0.7
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "JuiceFS 缓存命中率低于 70%"

        # 缓存空间不足
        - alert: JuiceFSCacheSpaceLow
          expr: |
            juicefs_client_cache_used_bytes / juicefs_client_cache_total_bytes > 0.9
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "JuiceFS 本地缓存使用率超过 90%"

        # 上传队列积压
        - alert: JuiceFSUploadBacklog
          expr: juicefs_client_uploads_pending > 1000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "JuiceFS 上传队列积压超过 1000"

        # 元数据延迟
        - alert: JuiceFSMetadataLatencyHigh
          expr: juicefs_client_metadata_latency_seconds > 0.1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "JuiceFS 元数据操作延迟超过 100ms"
```

### 6.3 运维命令

```bash
# 查看 JuiceFS 卷状态
juicefs status <meta-url>

# 检查挂载点
mount | grep juicefs

# 查看缓存使用情况
du -sh /var/jfsCache/

# 预热数据到缓存
juicefs warmup /path/to/hot-data

# 清理缓存
juicefs warmup --evict /path/to/cold-data

# 查看文件系统统计
juicefs stats /mnt/jfs

# 检查一致性
juicefs fsck <meta-url>
```

## 7. 故障排查

### 7.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| Pod 挂载超时 | CSI Node Driver 异常 | 检查 DaemonSet Pod 状态 |
| I/O 错误 | 对象存储连接问题 | 检查网络和凭据 |
| 性能差 | 缓存配置不当 | 调整缓存大小和策略 |
| 元数据慢 | TiKV/Redis 性能问题 | 检查元数据引擎资源 |
| 磁盘空间不足 | 缓存目录满 | 调整 cache-size 或清理 |

### 7.2 诊断步骤

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 CSI Driver 状态
kubectl get pods -n juicefs

# 2. 查看 CSI Node Driver 日志
kubectl logs -n juicefs -l app=juicefs-csi-node --tail=100

# 3. 检查 PVC 绑定状态
kubectl get pvc | grep juicefs

# 4. 检查挂载点
kubectl exec <pod> -- df -h | grep juicefs

# 5. 测试 I/O
kubectl exec <pod> -- dd if=/dev/zero of=/mnt/test bs=1M count=100

# 6. 检查元数据引擎连接
kubectl exec <pod> -- juicefs status <meta-url>

# 7. 查看详细日志
export JUICEFS_LOG_LEVEL=DEBUG
```
## 8. 生产最佳实践

### 8.1 容量规划

```yaml
capacity_planning:
  # 元数据存储
  metadata:
    tikv:
      storage: "100Gi+ per node"
      nodes: 3  # 最少 3 节点保证高可用
  
  # 缓存存储
  cache:
    local: "50-80% of node disk"
    distributed: "根据热数据量规划"
  
  # 对象存储
  object_storage:
    # 预留 20% 空间
    usable_ratio: 0.8
```

### 8.2 备份策略

```yaml
backup_strategy:
  # 元数据备份
  metadata:
    tikv:
      schedule: "0 2 * * *"
      retention: 30d
      method: "TiKV Backup"
    redis:
      schedule: "0 */6 * * *"
      retention: 7d
      method: "RDB Snapshot"
  
  # 数据备份（通过对象存储版本控制）
  data:
    s3_versioning: true
    lifecycle_rules:
      - transition: "30d -> IA"
      - expiration: "365d"
```

### 8.3 安全配置

```yaml
security:
  # 传输加密
  encryption_in_transit: true
  # 使用 HTTPS 访问对象存储
  
  # 静态加密
  encryption_at_rest:
    enabled: true
    method: "S3 SSE-KMS"
  
  # 访问控制
  rbac:
    - 限制 Secret 访问权限
    - 使用独立 ServiceAccount
  
  # 网络隔离
  network_policy:
    - 限制 juicefs 命名空间访问
    - 元数据引擎走专用网络
```

---

## Related

- [[03-longhorn-production|Longhorn 生产部署]]
- [[04-openebs-production|OpenEBS 生产部署]]

## See Also

- [JuiceFS 官方文档](https://juicefs.com/docs/zh/community/introduction)
- [JuiceFS CSI Driver](https://github.com/juicedata/juicefs-csi-driver)
- [JuiceFS 最佳实践](https://juicefs.com/docs/zh/cloud/best_practices)


<!-- risk-assessed -->
