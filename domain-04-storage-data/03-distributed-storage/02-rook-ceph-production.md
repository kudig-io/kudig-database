---
title: Rook-Ceph Kubernetes 生产部署与运维
description: 在阿里云与专有云 Kubernetes 上部署 Rook-Ceph 分布式存储，覆盖架构、部署、Pool与 StorageClass 配置、OSD 故障排查、扩容、性能调优及与阿里云/专有云场景的集成
category: storage
tags:
- k8s
- rook
- ceph
- distributed-storage
- osd
- pool
- storageclass
- alicloud
- apsara-stack
- performance-tuning
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 35min
intent_queries:
- Rook-Ceph K8s 生产部署
- 阿里云 K8s 使用 Rook-Ceph
- Rook-Ceph OSD 故障排查与扩容
trigger_keywords:
- Rook
- Ceph
- 分布式存储
- OSD
- Ceph Pool
- StorageClass
prerequisites:
- kubectl-basics
- storage-basics
- ceph-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-06-26"
updated: "2026-06-26"
---

# Rook-Ceph Kubernetes 生产部署与运维

> **适用版本**: Kubernetes v1.28 - v1.32 | **Rook**: v1.13+ | **Ceph**: Reef (18.x) / Quincy (17.x) | **最后更新**: 2026-06
> **文档定位**: 聚焦 Rook-Ceph 在阿里云/专有云裸金属或 ECS 集群上的生产部署与运维。ACK 托管集群推荐使用阿里云云盘/NAS/OSS CSI；Rook-Ceph 更适合专有云自建存储池、边缘节点或需要统一 Ceph 接口的场景。

<!-- chunk: 目录 -->
## 目录

1. [架构概述](#架构概述)
2. [前置条件与资源规划](#前置条件与资源规划)
3. [部署 Rook-Ceph](#部署-rook-ceph)
4. [Pool 与 StorageClass 配置](#pool-与-storageclass-配置)
5. [OSD 故障排查](#osd-故障排查)
6. [扩容操作](#扩容操作)
7. [性能调优](#性能调优)
8. [监控与告警](#监控与告警)
9. [阿里云/专有云场景集成](#阿里云专有云场景集成)
10. [常见问题排查](#常见问题排查)
11. [最佳实践检查清单](#最佳实践检查清单)

---

<!-- chunk: 1. 架构概述 -->
## 1. 架构概述

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          Kubernetes Cluster                              │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                         Rook Operator                            │   │
│   │              管理 CephCluster / CephBlockPool / CephFS           │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │
│   │   MON x3    │  │   MGR x2    │  │   OSD xN    │  │   MDS x2    │   │
│   │  集群元数据  │  │  监控管理   │  │  对象存储   │  │  CephFS元数据│   │
│   └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘   │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │              CSI Plugin (RBD + CephFS)                          │   │
│   │          provisioner + nodeplugin + snapshotter                 │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │           StorageClass: rook-ceph-block / rook-cephfs           │   │
│   └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

| 组件 | 数量建议 | 说明 |
|:---|:---:|:---|
| MON | 3 或 5 | 必须是奇数，建议 3 节点跨可用区部署 |
| MGR | 2（Active/Standby） | 管理监控与指标 |
| OSD | 每节点 1-N | 每 OSD 独占一块盘，禁止与系统盘混用 |
| MDS | 2+ | CephFS 元数据服务 |
| CSI Plugin | DaemonSet + Deployment | RBD 与 CephFS 独立部署 |

---

<!-- chunk: 2. 前置条件 -->
## 2. 前置条件与资源规划

### 2.1 节点要求

| 项目 | 最低要求 | 生产建议 |
|:---|:---|:---|
| 节点数 | 3 | ≥ 3，MON/OSD 分离或共置 |
| CPU/内存 | 4C8G | 8C16G 以上 |
| 数据盘 | 每 OSD 100GB | SSD/NVMe，每 OSD 独占 |
| 网络 | 万兆内网 | 推荐独立存储网络 |
| Kernel | 4.x | 5.x 以上，启用 `rbd` 模块 |

### 2.2 加载内核模块

```bash
# 所有节点执行
modprobe rbd
modprobe nbd

echo "rbd" >> /etc/modules-load.d/rook-ceph.conf
echo "nbd" >> /etc/modules-load.d/rook-ceph.conf
```

### 2.3 准备裸盘

```bash
# 确认磁盘无分区、无文件系统
lsblk -f
wipefs -a /dev/vdb
```

### 2.4 阿里云/专有云网络要求

| 要求 | 说明 |
|:---|:---|
| 节点内网互通 | MON/OSD 之间低延迟通信 |
| 安全组放行 | 6789（MON）、6800-7300（OSD/MGR/MDS） |
| 专有云网络 | 确保天基内网可达镜像仓库与 Yum 源 |

---

<!-- chunk: 3. 部署 Rook-Ceph -->
## 3. 部署 Rook-Ceph

### 3.1 下载 Rook 部署清单

```bash
git clone --single-branch --branch v1.13.2 https://github.com/rook/rook.git
cd rook/deploy/examples
```

### 3.2 部署 Operator 与 CRD

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl create -f crds.yaml -f common.yaml
kubectl create -f operator.yaml

kubectl get pods -n rook-ceph
```

### 3.3 创建 CephCluster

```yaml
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v18.2.1
    allowUnsupported: false
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
    allowMultiplePerNode: false
  mgr:
    count: 2
    allowMultiplePerNode: false
  dashboard:
    enabled: true
    ssl: false
  storage:
    useAllNodes: false
    useAllDevices: false
    nodes:
      - name: "k8s-node-01"
        devices:
          - name: "vdb"
      - name: "k8s-node-02"
        devices:
          - name: "vdb"
      - name: "k8s-node-03"
        devices:
          - name: "vdb"
    resources:
      osd:
        requests:
          cpu: "500m"
          memory: "1Gi"
        limits:
          cpu: "2000m"
          memory: "4Gi"
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl apply -f cluster.yaml
kubectl get cephcluster -n rook-ceph
# 等待 HEALTH_OK
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph status
```

### 3.4 部署 Toolbox

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl apply -f toolbox.yaml
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph status
```

---

<!-- chunk: 4. Pool 与 StorageClass -->
## 4. Pool 与 StorageClass 配置

### 4.1 创建 CephBlockPool

```yaml
apiVersion: ceph.rook.io/v1
kind: CephBlockPool
metadata:
  name: replicapool
  namespace: rook-ceph
spec:
  failureDomain: host
  replicated:
    size: 3
    requireSafeReplicaSize: true
  parameters:
    pg_num: "128"
    pgp_num: "128"
```

### 4.2 创建 StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  imageFormat: "2"
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
  csi.storage.k8s.io/fstype: ext4
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: Immediate
```

### 4.3 创建 CephFS

```yaml
apiVersion: ceph.rook.io/v1
kind: CephFilesystem
metadata:
  name: myfs
  namespace: rook-ceph
spec:
  metadataPool:
    replicated:
      size: 3
  dataPools:
    - name: replicated
      replicated:
        size: 3
  metadataServer:
    activeCount: 1
    activeStandby: true
```

---

<!-- chunk: 5. OSD 故障排查 -->
## 5. OSD 故障排查

### 5.1 查看 Ceph 状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph status
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph osd tree
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph osd df tree
```

### 5.2 OSD 常见故障

| 现象 | 根因 | 修复命令 |
|:---|:---|:---|
| OSD 状态 down | 节点重启 / 磁盘故障 | `ceph osd up osd.0` 或替换 OSD |
| OSD 状态 out | 数据重平衡中 | 等待重平衡完成，必要时 `ceph osd in osd.0` |
| OSD 占用率 > 85% | 集群容量不足 | 扩容 OSD 或调整 reweight |
| OSD 启动失败 | 磁盘被占用 / LVM 签名残留 | `wipefs -a /dev/vdb` 后重启 operator |
| PG 状态 degraded | OSD 数量不足或 down | 修复 OSD，等待 recovery |

### 5.3 替换故障 OSD

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 标记 osd.0 为 out
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph osd out osd.0

# 2. 停止 osd.0 Pod
kubectl scale deployment rook-ceph-osd-0 -n rook-ceph --replicas=0

# 3. 移除 CRUSH map
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph osd crush remove osd.0
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph auth del osd.0
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- ceph osd rm osd.0

# 4. 清理磁盘并删除对应 OSD deployment
kubectl delete deployment rook-ceph-osd-0 -n rook-ceph
wipefs -a /dev/vdb

# 5. 更新 CephCluster CR，触发 operator 重新创建 OSD
kubectl edit cephcluster rook-ceph -n rook-ceph
```

---

<!-- chunk: 6. 扩容 -->
## 6. 扩容操作

### 6.1 横向扩容 OSD

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 新增节点 k8s-node-04，挂载 /dev/vdb
kubectl edit cephcluster rook-ceph -n rook-ceph
```

```yaml
spec:
  storage:
    nodes:
      - name: "k8s-node-01"
        devices:
          - name: "vdb"
      - name: "k8s-node-02"
        devices:
          - name: "vdb"
      - name: "k8s-node-03"
        devices:
          - name: "vdb"
      - name: "k8s-node-04"
        devices:
          - name: "vdb"
```

### 6.2 纵向扩容 Pool

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 增加 PG 数（谨慎操作）
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- \
  ceph osd pool set replicapool pg_num 256
```

### 6.3 扩容 PVC

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 编辑 PVC 增大 storage 请求
kubectl edit pvc mysql-data -n production
# RBD StorageClass 需开启 allowVolumeExpansion: true
```

---

<!-- chunk: 7. 性能调优 -->
## 7. 性能调优

### 7.1 关键参数

| 参数 | 默认值 | 生产建议 | 说明 |
|:---|:---:|:---:|:---|
| `osd_memory_target` | 4GB | 根据节点内存调整 | OSD 内存上限 |
| `osd_op_num_threads_per_shard` | 5 | 8-16 | 并发 OP 线程 |
| `rbd_cache` | true | true | RBD 客户端缓存 |
| `rbd_cache_max_dirty` | 25165824 | 100MB | 脏数据上限 |
| `pg_num` | 自动 | 按 OSD 数×100 / 副本数 | 避免 PG 过少 |

### 7.2 应用调优配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- \
  ceph config set osd osd_memory_target 8589934592

kubectl exec -it deploy/rook-ceph-tools -n rook-ceph -- \
  ceph config set osd osd_op_num_threads_per_shard 8
```

### 7.3 网络与调度优化

| 优化项 | 配置 |
|:---|:---|
| 独立存储网络 | 为 OSD 绑定独立网卡或 VLAN |
| CPU 独占 | 为 OSD 配置 `cpuManagerPolicy: static` |
| 节点反亲和 | MON/OSD 分散在不同物理机架 |
| 内存 HugePage | 启用透明大页提升性能 |

---

<!-- chunk: 8. 监控与告警 -->
## 8. 监控与告警

```yaml
groups:
  - name: rook-ceph-alerts
    rules:
      - alert: CephClusterUnhealthy
        expr: ceph_health_status != 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Ceph 集群状态异常"

      - alert: CephOSDDown
        expr: ceph_osd_up == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Ceph OSD 下线"

      - alert: CephOSDUtilizationHigh
        expr: ceph_osd_utilization > 0.85
        for: 30m
        labels:
          severity: warning
        annotations:
          summary: "Ceph OSD 使用率超过 85%"

      - alert: CephPGDegraded
        expr: ceph_pg_degraded > 0
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Ceph PG 处于 Degraded 状态"
```

---

<!-- chunk: 9. 阿里云/专有云场景集成 -->
## 9. 阿里云/专有云场景集成

### 9.1 阿里云场景

| 场景 | 推荐方案 | 说明 |
|:---|:---|:---|
| ACK 托管集群生产存储 | 阿里云云盘/NAS/OSS CSI | 免运维、SLA 保障 |
| 自建 K8s 裸金属集群 | Rook-Ceph | 需要块/文件/对象统一接口 |
| 边缘节点本地盘 | Rook-Ceph + local PV | 成本敏感、延迟敏感 |
| 专有云（Apsara Stack） | Rook-Ceph 或飞天盘古对接 | 取决于底座存储能力 |

### 9.2 专有云注意事项

- **镜像仓库**：使用专有云内部 Harbor 镜像仓库，离线加载 Rook/Ceph 镜像
- **网络隔离**：MON 之间、OSD 之间必须保证低延迟内网互通
- **底座对接**：若专有云已提供分布式存储（如盘古），优先通过 CSI 对接，避免重复建设 Ceph
- **天基监控**：将 Ceph 告警接入专有云监控平台（SLS/ARMS/天基）
- **离线部署**：提前下载所有镜像并推送至专有云 Harbor，避免部署时拉取失败

### 9.3 阿里云 CLI 检查节点与磁盘

```bash
# 查看 ECS 实例
aliyun ecs DescribeInstances --RegionId cn-hangzhou \
  --InstanceIds '["i-xxxxxxxxxxxxx"]'

# 查看云盘
aliyun ecs DescribeDisks --RegionId cn-hangzhou \
  --InstanceId i-xxxxxxxxxxxxx
```

---

<!-- chunk: 10. 常见问题排查 -->
## 10. 常见问题排查

| 问题 | 排查命令 | 解决方案 |
|:---|:---|:---|
| CephCluster 一直 Creating | `kubectl describe cephcluster -n rook-ceph` | 检查磁盘、内核模块、镜像拉取 |
| PVC 一直 Pending | `kubectl describe pvc mysql-data` | 检查 Pool、StorageClass、provisioner 日志 |
| RBD 挂载失败 | `kubectl logs -n rook-ceph -l app=csi-rbdplugin` | 检查节点 rbd 模块、密钥 |
| Dashboard 无法访问 | `kubectl get svc -n rook-ceph` | 配置 NodePort 或 Ingress |
| 镜像拉取失败 | `kubectl describe pod -n rook-ceph` | 使用专有云 Harbor 镜像地址 |

---

<!-- chunk: 11. 最佳实践检查清单 -->
## 11. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| Ceph 状态健康 | HEALTH_OK | `ceph status` |
| MON 为奇数且跨节点 | 3 或 5 | `ceph mon dump` |
| OSD 独占数据盘 | 无系统盘混用 | `lsblk` |
| Pool 副本数 ≥ 3 | 生产 3 副本 | `ceph osd pool get replicapool size` |
| StorageClass 允许扩容 | allowVolumeExpansion: true | `kubectl get sc` |
| 内核模块加载 | rbd/nbd 已加载 | `lsmod \| grep rbd` |
| 镜像仓库可访问 | 专有云 Harbor 正常 | `crictl pull` |
| 定期备份 Ceph 配置 | 导出 ceph.conf 与 keyring | 备份脚本 |
| 容量告警阈值 | OSD 80% 告警 | PrometheusRule |
| 离线部署包准备 | 镜像已同步 | Harbor 仓库检查 |

---

## Related

- [[domain-04-storage-data/01-k8s-storage/10-storage-backup-disaster-recovery|10 - 存储备份与灾难恢复]]
- [[domain-04-storage-data/README|Storage Domain 存储领域知识库]]
- [[domain-12-cloud-providers/01-alibaba-cloud/apsara-stack-components|专有云组件索引]]

## See Also

- [[domain-04-storage-data/03-distributed-storage/01-velero-backup-recovery|Velero 阿里云专有云备份恢复实战]]
- [[domain-04-storage-data/03-distributed-storage/03-longhorn-production|Longhorn 生产指南]]
- [[domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns|有状态应用存储模式]]

---

## Ceph Dashboard 与 Prometheus 指标接入

Rook 默认启用 Ceph Dashboard，可通过端口转发或阿里云 SLB 暴露。生产环境建议仅允许白名单 IP 或 VPN 访问，并修改默认 admin 密码。

```bash
# 端口转发访问 Dashboard
kubectl -n rook-ceph port-forward svc/rook-ceph-mgr-dashboard 8443:8443

# 获取 Dashboard 密码
kubectl -n rook-ceph get secret rook-ceph-dashboard-password \
  -o jsonpath="{.data.password}" | base64 -d
```

### Prometheus 抓取配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: rook-ceph-metrics
  namespace: monitoring
spec:
  namespaceSelector:
    matchNames:
      - rook-ceph
  selector:
    matchLabels:
      app: rook-ceph-mgr
  endpoints:
    - port: http-metrics
      path: /metrics
      interval: 30s
```

### 关键指标

| 指标 | 含义 | 告警阈值 |
|:---|:---|:---:|
| `ceph_health_status` | 集群健康状态 | > 0 异常 |
| `ceph_osd_up` | OSD 是否在线 | == 0 异常 |
| `ceph_osd_utilization` | OSD 使用率 | > 80% 警告 |
| `ceph_pool_percent_used` | Pool 使用率 | > 80% 警告 |
| `ceph_pg_active_clean` | PG 状态 | 小于总 PG 数 |

---

## BlueStore 内存与 OSD 资源规划

BlueStore 是 Ceph 默认后端，OSD 内存直接影响元数据缓存命中率和 I/O 延迟。

| 场景 | 单 OSD 内存建议 | 说明 |
|:---|:---:|:---|
| 通用业务 | 4 GB | 默认配置，适用大多数场景 |
| 高性能数据库 | 8 GB+ | 提升元数据缓存，降低延迟 |
| 大容量归档 | 2 GB | 冷数据访问频率低，可降低内存 |

```yaml
spec:
  storage:
    nodes:
      - name: k8s-node-01
        resources:
          osd:
            limits:
              memory: "8Gi"
              cpu: "2"
            requests:
              memory: "4Gi"
              cpu: "1"
```
