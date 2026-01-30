# Kubernetes 存储(PV/PVC/StorageClass)从入门到实战

> **适用环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK | **版本**: Kubernetes v1.25-v1.32  
> **文档类型**: PPT演示文稿内容 | **目标受众**: 开发者、运维工程师、架构师  

---

## 目录

1. [存储基础概念](#1-存储基础概念)
2. [PV/PVC核心机制](#2-pvpvc核心机制)
3. [StorageClass详解](#3-storageclass详解)
4. [阿里云存储实践](#4-阿里云存储实践)
5. [ACK产品集成](#5-ack产品集成)
6. [高级特性与配置](#6-高级特性与配置)
7. [生产最佳实践](#7-生产最佳实践)
8. [监控与故障排查](#8-监控与故障排查)
9. [总结与Q&A](#9-总结与qa)

---

## 1. 存储基础概念

### 1.1 为什么需要持久化存储？

**无持久化存储的痛点**
```
❌ 容器重启后数据丢失
❌ Pod重建导致数据清空
❌ 无法在多个Pod间共享数据
❌ 难以实现数据备份和恢复
❌ 存储容量无法弹性扩展
```

**使用持久化存储的优势**
```
✅ 数据持久保存，独立于Pod生命周期
✅ 支持多Pod共享访问(RWX)
✅ 可实现数据备份、快照、克隆
✅ 支持在线扩容和性能调优
✅ 统一的存储管理和策略控制
```

### 1.2 Kubernetes存储架构

```
[应用Pod] → [PVC] → [PV] → [StorageClass] → [CSI Driver] → [云存储后端]
    ↑         ↑       ↑         ↑              ↑              ↑
  业务使用  抽象声明  实体卷   动态供给    存储接口    阿里云EBS/NAS/OSS
```

**核心组件**
- **Volume**: Pod级别的存储卷
- **PV (PersistentVolume)**: 集群级别的存储资源
- **PVC (PersistentVolumeClaim)**: 存储资源申请
- **StorageClass**: 存储类定义，支持动态供给
- **CSI (Container Storage Interface)**: 标准存储接口

### 1.3 存储类型对比

| 存储类型 | 访问模式 | 性能 | 适用场景 | 成本 |
|---------|---------|------|----------|------|
| **HostPath** | RWO | 最高 | 单节点测试 | 低 |
| **Local PV** | RWO | 高 | 高性能本地存储 | 中 |
| **云盘(EBS/EBS)** | RWO | 中-高 | 数据库、通用应用 | 中 |
| **NAS/EFS** | RWX | 中 | 共享文件、日志存储 | 中 |
| **对象存储(OSS/S3)** | - | 低 | 大文件、归档存储 | 低 |

---

## 2. PV/PVC核心机制

### 2.1 PV生命周期管理

```
Available → Bound → Released → Available/Recycled
    ↑                                     ↓
    └─────────── Recycle/Delete ──────────┘
```

**PV状态说明**
- **Available**: 可用，未被任何PVC绑定
- **Bound**: 已绑定到某个PVC
- **Released**: PVC被删除，但PV仍存在
- **Failed**: 回收失败

### 2.2 PVC绑定机制

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce  # 访问模式
  storageClassName: alicloud-disk-essd  # 存储类
  resources:
    requests:
      storage: 100Gi  # 容量请求
```

**访问模式详解**
| 模式 | 缩写 | 说明 | 适用场景 |
|------|------|------|----------|
| **ReadWriteOnce** | RWO | 单节点读写 | 数据库、单实例应用 |
| **ReadOnlyMany** | ROX | 多节点只读 | 配置文件、静态资源 |
| **ReadWriteMany** | RWX | 多节点读写 | 共享存储、日志收集 |
| **ReadWriteOncePod** | RWOP | 单Pod独占 | v1.22+，更强隔离 |

### 2.3 静态供应 vs 动态供应

#### 静态供应 (Static Provisioning)
```yaml
# 1. 管理员预先创建PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-static-001
spec:
  capacity:
    storage: 100Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  csi:
    driver: diskplugin.csi.alibabacloud.com
    volumeHandle: d-xxxxxxxxx  # 已存在的云盘ID

---
# 2. 用户创建PVC绑定已有PV
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data
spec:
  volumeName: pv-static-001  # 指定PV名称
  resources:
    requests:
      storage: 100Gi
```

#### 动态供应 (Dynamic Provisioning) - 推荐
```yaml
# 1. 定义StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
allowVolumeExpansion: true

---
# 2. 用户只需创建PVC，自动创建PV
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: dynamic-pvc
spec:
  storageClassName: alicloud-disk-essd  # 自动供给
  resources:
    requests:
      storage: 100Gi
```

### 2.4 PV与Pod绑定方式

#### 方式一：Deployment直接挂载
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.24
        volumeMounts:
        - name: data
          mountPath: /usr/share/nginx/html
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: web-data  # PVC名称
```

#### 方式二：StatefulSet volumeClaimTemplates (推荐)
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql
  replicas: 3
  volumeClaimTemplates:  # 为每个Pod自动创建PVC
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 100Gi
  # 自动创建: data-mysql-0, data-mysql-1, data-mysql-2
```

#### 方式三：多Pod共享存储(RWX)
```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-storage
spec:
  accessModes:
    - ReadWriteMany  # 必须是RWX
  storageClassName: alicloud-nas
  resources:
    requests:
      storage: 100Gi
```

---

## 3. StorageClass详解

### 3.1 StorageClass核心参数

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: production-storage
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  # 云盘类型和性能
  type: cloud_essd
  performanceLevel: PL1
  
  # 文件系统类型
  fsType: ext4
  
  # 加密配置
  encrypted: "true"
  kmsKeyId: "key-xxxxxx"
  
  # 可用区配置
  zoneId: cn-hangzhou-a,cn-hangzhou-b
  
# 重要配置项
reclaimPolicy: Retain  # Delete/Retain/Recycle
allowVolumeExpansion: true  # 支持扩容
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定
mountOptions:
  - noatime
  - nodiratime
```

### 3.2 参数详解

| 参数 | 说明 | 可选值 | 默认值 |
|------|------|--------|--------|
| **provisioner** | CSI驱动名称 | 各厂商不同 | 必填 |
| **reclaimPolicy** | 回收策略 | Delete/Retain/Recycle | Delete |
| **allowVolumeExpansion** | 是否允许扩容 | true/false | false |
| **volumeBindingMode** | 绑定模式 | Immediate/WaitForFirstConsumer | Immediate |
| **mountOptions** | 挂载选项 | 文件系统参数 | 无 |

### 3.3 回收策略对比

| 策略 | 行为 | 适用场景 |
|------|------|----------|
| **Delete** | PVC删除时同时删除PV和后端存储 | 开发测试环境 |
| **Retain** | PVC删除后PV保留，需手动清理 | 生产环境 |
| **Recycle** | 清空PV内容后变为Available | 已废弃 |

### 3.4 绑定模式详解

#### Immediate (立即绑定)
```yaml
volumeBindingMode: Immediate
```
- PVC创建后立即绑定PV
- 可能导致跨AZ调度问题
- 适用于无地域要求的场景

#### WaitForFirstConsumer (延迟绑定) - 推荐
```yaml
volumeBindingMode: WaitForFirstConsumer
```
- 等待第一个使用PVC的Pod调度后再绑定
- 自动匹配Pod所在AZ
- 避免跨AZ挂载问题

---

## 4. 阿里云存储实践

### 4.1 专有云 vs 公共云差异

| 特性 | 专有云(Apsara Stack) | 公共云(ACK) |
|------|---------------------|-------------|
| **存储后端** | 本地存储+EBS模拟 | 真实云盘服务 |
| **网络环境** | 私有网络 | 公网+私网 |
| **安全管控** | 本地化策略 | 云安全中心 |
| **运维模式** | 本地运维 | 托管运维 |
| **计费模式** | 按资源池计费 | 按量付费 |

### 4.2 阿里云存储类型选择

#### 云盘(EBS)系列
```yaml
# 高效云盘 - 入门级
type: cloud_efficiency
特点: 成本低，性能一般
适用: 开发测试、非核心应用

# SSD云盘
type: cloud_ssd
特点: 性能均衡
适用: 一般生产应用

# ESSD云盘 - 推荐生产
type: cloud_essd
performanceLevel: PL1  # PL0/PL1/PL2/PL3
特点: 高性能，支持IOPS调整
适用: 核心数据库、高并发应用
```

#### NAS文件存储
```yaml
# 标准型NAS
provisioner: nasplugin.csi.alibabacloud.com
parameters:
  volumeAs: subpath
  server: "xxx.cn-hangzhou.nas.aliyuncs.com"

# 极速型NAS - 高性能
parameters:
  server: "xxx.cn-hangzhou.extreme.nas.aliyuncs.com"
  protocolType: "NFS"
```

#### OSS对象存储
```yaml
provisioner: ossplugin.csi.alibabacloud.com
parameters:
  bucket: "my-bucket"
  url: "oss-cn-hangzhou.aliyuncs.com"
  # 适用于大文件存储、归档场景
```

### 4.3 性能等级选择指南

| 等级 | IOPS | 吞吐量 | 延迟 | 适用场景 | 成本系数 |
|------|------|--------|------|----------|----------|
| **PL0** | 10K | 180MB/s | 1ms | 开发测试 | 1.0x |
| **PL1** | 50K | 350MB/s | <1ms | **标准生产**(MySQL) | 1.5x |
| **PL2** | 100K | 750MB/s | <1ms | 高并发数据库 | 3.0x |
| **PL3** | 1M | 4GB/s | <1ms | 极高性能OLTP | 10.0x |

**选择建议**:
- 小于100GB: PL0足够
- 100GB-1TB: PL1推荐
- 大于1TB且高并发: PL2
- 极端性能要求: PL3

---

## 5. ACK产品集成

### 5.1 ACK托管版存储配置

```yaml
# ACK托管集群默认StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-available
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: available  # 自动选择最优类型
  regionId: cn-hangzhou
  zoneId: cn-hangzhou-a
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 5.2 专有版ACK存储配置

```yaml
# 专有云环境下需要特殊配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: apsara-disk-standard
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  # 专有云特定参数
  type: cloud_ssd
  zoneId: cn-beijing-a
  # 可能需要额外的认证配置
  accessToken: "xxxxxx"
mountOptions:
  - _netdev  # 网络设备标识
```

### 5.3 多可用区部署策略

```yaml
# 多AZ高可用部署
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-ha
spec:
  replicas: 3
  template:
    spec:
      # Pod反亲和，分散到不同AZ
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: mysql
            topologyKey: topology.kubernetes.io/zone
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd-pl1
      resources:
        requests:
          storage: 100Gi
```

### 5.4 存储加密配置

```yaml
# KMS加密存储
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: encrypted-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"
  kmsKeyId: "key-1234567890abcdef"  # 指定KMS密钥
```

---

## 6. 高级特性与配置

### 6.1 在线扩容 (Volume Expansion)

```yaml
# 1. StorageClass启用扩容
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: expandable-storage
allowVolumeExpansion: true  # 关键配置

---
# 2. 扩容PVC
# 方法一：kubectl patch
kubectl patch pvc mysql-data -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 方法二：编辑PVC
kubectl edit pvc mysql-data
# 修改 spec.resources.requests.storage: 200Gi
```

**扩容注意事项**:
- 只能增大，不能缩小
- 需要文件系统支持在线扩容(XFS/ext4)
- StatefulSet中PVC扩容后可能需要重启Pod

### 6.2 快照与克隆

#### VolumeSnapshot创建
```yaml
# 1. 定义VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: alicloud-snapshot
driver: diskplugin.csi.alibabacloud.com
deletionPolicy: Delete

---
# 2. 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-snapshot-20260130
spec:
  volumeSnapshotClassName: alicloud-snapshot
  source:
    persistentVolumeClaimName: mysql-data
```

#### 从快照恢复
```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-restored
spec:
  storageClassName: alicloud-disk-essd
  dataSource:
    name: mysql-snapshot-20260130
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 100Gi  # 必须>=源PVC
```

### 6.3 存储拓扑感知

```yaml
# 拓扑感知StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: topology-aware
provisioner: diskplugin.csi.alibabacloud.com
volumeBindingMode: WaitForFirstConsumer  # 必须
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - cn-hangzhou-a
    - cn-hangzhou-b
```

### 6.4 临时卷(Ephemeral Volumes)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: data-processor
spec:
  containers:
  - name: processor
    image: data-processor:v1
    volumeMounts:
    - name: scratch
      mountPath: /tmp/work
  volumes:
  - name: scratch
    ephemeral:  # 临时卷，Pod删除时自动清理
      volumeClaimTemplate:
        spec:
          accessModes: ["ReadWriteOnce"]
          storageClassName: alicloud-disk-efficiency
          resources:
            requests:
              storage: 50Gi
```

---

## 7. 生产最佳实践

### 7.1 命名规范

```bash
# PVC命名规范
{应用名}-{用途}-{环境}
例子:
mysql-data-prod      # 生产环境MySQL数据
redis-cache-test     # 测试环境Redis缓存
elasticsearch-logs-dev  # 开发环境ES日志
```

### 7.2 容量规划建议

| 应用类型 | 预留空间 | 扩容阈值 | 监控指标 |
|----------|----------|----------|----------|
| **数据库** | 50% | 80% | 存储使用率、inode使用率 |
| **日志存储** | 30% | 90% | 磁盘空间、inode |
| **文件存储** | 40% | 85% | 存储使用率 |
| **缓存存储** | 20% | 95% | 空间使用率 |

### 7.3 高可用架构设计

```yaml
# MySQL主从高可用
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
spec:
  replicas: 3
  serviceName: mysql
  template:
    spec:
      # 节点亲和，确保分布在不同故障域
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: mysql
            topologyKey: kubernetes.io/hostname
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: failure-domain.beta.kubernetes.io/zone
                operator: In
                values:
                - cn-hangzhou-a
                - cn-hangzhou-b
```

### 7.4 备份策略

```yaml
# 定期快照CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: storage-backup
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: bitnami/kubectl:latest
            command:
            - /bin/sh
            - -c
            - |
              # 为所有关键PVC创建快照
              kubectl get pvc -n production -l backup=enabled -o jsonpath='{.items[*].metadata.name}' | \
              xargs -I {} kubectl apply -f - <<EOF
              apiVersion: snapshot.storage.k8s.io/v1
              kind: VolumeSnapshot
              metadata:
                name: {}-$(date +%Y%m%d)
                namespace: production
              spec:
                volumeSnapshotClassName: alicloud-snapshot
                source:
                  persistentVolumeClaimName: {}
              EOF
          restartPolicy: OnFailure
```

### 7.5 安全配置

```yaml
# 安全的StorageClass配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: secure-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"  # 启用加密
  kmsKeyId: "alias/production-key"
mountOptions:
  - noatime
  - nodiratime
  - defaults
reclaimPolicy: Retain  # 生产环境保留数据
```

---

## 8. 监控与故障排查

### 8.1 关键监控指标

| 指标类别 | 具体指标 | 告警阈值 | 说明 |
|----------|----------|----------|------|
| **容量监控** | 存储使用率 | >80% | 磁盘空间不足 |
| **容量监控** | inode使用率 | >90% | 文件数量过多 |
| **性能监控** | IOPS | >预设值的80% | 性能瓶颈 |
| **性能监控** | 吞吐量 | >预设值的80% | 带宽瓶颈 |
| **可用性监控** | PVC状态 | Pending/Failed | 供给失败 |
| **可用性监控** | PV状态 | Released/Failed | 回收异常 |

### 8.2 常见故障诊断流程

```
PVC状态异常?
    │
    ├── Pending? ──检查StorageClass是否存在
    │       │
    │       ├── 不存在 ──创建正确的StorageClass
    │       └── 存在 ──检查CSI驱动状态
    │
    ├── Bound但Pod挂载失败? ──检查挂载参数和权限
    │
    └── Lost? ──检查PV和后端存储状态
```

### 8.3 常用诊断命令

```bash
# 基础检查
kubectl get pvc -A -o wide
kubectl get pv -o wide
kubectl get sc

# 详细信息
kubectl describe pvc <pvc-name> -n <namespace>
kubectl describe pv <pv-name>

# CSI驱动状态
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system <csi-provisioner-pod>

# 存储事件
kubectl get events --field-selector involvedObject.kind=PersistentVolumeClaim
kubectl get events --field-selector reason=ProvisioningFailed
```

### 8.4 阿里云特定诊断

```bash
# 检查云盘状态
aliyun ecs DescribeDisks --DiskIds '["d-xxxxxxxxx"]'

# 检查NAS挂载点
aliyun nas DescribeMountTargets --FileSystemId <fs-id>

# 检查快照状态
aliyun ecs DescribeSnapshots --SnapshotIds '["s-xxxxxxxxx"]'
```

---

## 9. 总结与Q&A

### 9.1 核心要点回顾

✅ **基础概念**: PV/PVC/StorageClass是K8s存储的核心抽象  
✅ **动态供给**: 推荐使用StorageClass实现自动化存储管理  
✅ **阿里云集成**: 合理选择EBS/NAS/OSS满足不同业务需求  
✅ **生产实践**: 注重高可用、安全性、监控告警  
✅ **故障排查**: 系统化诊断流程，快速定位问题根源  

### 9.2 常见问题解答

**Q: 如何选择合适的存储类型？**
A: 数据库用ESSD PL1，共享文件用NAS，大文件归档用OSS

**Q: PVC扩容后为什么容量没变化？**
A: 需要重启Pod或手动执行文件系统扩容命令

**Q: 如何实现跨AZ的存储高可用？**
A: 使用StatefulSet配合Pod反亲和，每个Pod独立存储

**Q: 快照和备份有什么区别？**
A: 快照是存储级别，备份是应用级别，建议两者结合使用

### 9.3 学习建议

1. **理论学习**: 先掌握PV/PVC基本概念和生命周期
2. **动手实践**: 在测试环境部署各种存储场景
3. **生产应用**: 从小规模开始，逐步扩大使用范围
4. **持续优化**: 根据监控数据调整配置和策略

---