# 02 - PV/PVC核心概念与企业级实践

> **适用版本**: Kubernetes v1.25 - v1.32 | **运维重点**: 企业级配置、生产环境最佳实践、故障预防 | **最后更新**: 2026-02

## 目录

1. [PV存储架构分层模型](#pv存储架构分层模型)
2. [PV核心规格字段详解](#pv核心规格字段详解)
3. [PVC声明与绑定机制](#pvc声明与绑定机制)
4. [企业级配置模板](#企业级配置模板)
5. [生产环境最佳实践](#生产环境最佳实践)
6. [容量管理与优化](#容量管理与优化)
7. [监控与告警配置](#监控与告警配置)
8. [故障预防与自愈](#故障预防与自愈)

---

## 1. PV 存储架构分层模型

```
┌─────────────────────────────────────────────────────────────────┐
│                      应用层 (Application Layer)                  │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐            │
│  │   Pod   │  │   Pod   │  │   Pod   │  │   Pod   │            │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘            │
│       │            │            │            │                  │
├───────┼────────────┼────────────┼────────────┼──────────────────┤
│       ▼            ▼            ▼            ▼                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              PVC 层 (PersistentVolumeClaim)              │   │
│  │   声明式存储请求：容量、访问模式、StorageClass           │   │
│  └─────────────────────────┬───────────────────────────────┘   │
│                            │ 绑定 (Binding)                     │
├────────────────────────────┼────────────────────────────────────┤
│                            ▼                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              PV 层 (PersistentVolume)                    │   │
│  │   集群级存储资源：容量、访问模式、回收策略、存储后端     │   │
│  └─────────────────────────┬───────────────────────────────┘   │
│                            │                                    │
├────────────────────────────┼────────────────────────────────────┤
│                            ▼                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              CSI 驱动层 (CSI Driver Layer)               │   │
│  │   Provisioner │ Attacher │ Resizer │ Snapshotter        │   │
│  └─────────────────────────┬───────────────────────────────┘   │
│                            │                                    │
├────────────────────────────┼────────────────────────────────────┤
│                            ▼                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              存储后端 (Storage Backend)                  │   │
│  │   云盘(EBS/ESSD) │ NFS │ Ceph │ Local │ iSCSI           │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. PV 核心规格字段详解

| 字段 | 类型 | 必填 | 说明 |
|:---|:---|:---:|:---|
| `capacity.storage` | Quantity | 是 | 存储容量，如 `100Gi` |
| `accessModes` | []string | 是 | 访问模式：RWO/ROX/RWX/RWOP |
| `persistentVolumeReclaimPolicy` | string | 否 | 回收策略：Retain/Delete/Recycle |
| `storageClassName` | string | 否 | 关联的 StorageClass 名称 |
| `volumeMode` | string | 否 | 卷模式：Filesystem(默认)/Block |
| `mountOptions` | []string | 否 | 挂载选项，如 `["noatime","discard"]` |
| `nodeAffinity` | NodeAffinity | 否 | 节点亲和性约束（Local PV必须） |
| `csi` | CSIPersistentVolumeSource | 否 | CSI 卷配置 |

---

## 3. PV 生命周期状态机

```
                    ┌─────────────────────────────────────────────┐
                    │                                             │
                    ▼                                             │
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐     │
│ Pending  │───▶│Available │───▶│  Bound   │───▶│ Released │─────┤
└──────────┘    └──────────┘    └──────────┘    └──────────┘     │
     │               │               │               │            │
     │               │               │               ├──▶ Retain ─┘
     │               │               │               │    (手动回收)
     │               │               │               │
     │               │               │               ├──▶ Delete
     │               │               │               │    (自动删除)
     │               │               │               │
     ▼               ▼               ▼               ▼
┌──────────────────────────────────────────────────────────────┐
│                        Failed                                 │
│              (CSI驱动错误/存储后端故障)                       │
└──────────────────────────────────────────────────────────────┘
```

### 状态说明

| 状态 (Phase) | 含义 | 触发条件 |
|:---|:---|:---|
| **Pending** | 等待中 | PV 创建中，后端存储尚未就绪 |
| **Available** | 可用 | PV 已就绪，等待 PVC 绑定 |
| **Bound** | 已绑定 | PV 已与 PVC 绑定 |
| **Released** | 已释放 | PVC 删除后，PV 等待回收 |
| **Failed** | 失败 | 自动回收失败或后端错误 |

---

## 4. 访问模式 (Access Modes) 深度解析

| 模式 | 全称 | 说明 | 典型场景 |
|:---:|:---|:---|:---|
| **RWO** | ReadWriteOnce | 单节点读写 | 数据库、有状态应用 |
| **ROX** | ReadOnlyMany | 多节点只读 | 静态资源、配置文件 |
| **RWX** | ReadWriteMany | 多节点读写 | 共享存储、日志收集 |
| **RWOP** | ReadWriteOncePod | 单Pod读写(v1.22+) | 严格单实例应用 |

### 存储后端访问模式支持矩阵

| 存储类型 | RWO | ROX | RWX | RWOP |
|:---|:---:|:---:|:---:|:---:|
| AWS EBS | ✅ | ❌ | ❌ | ✅ |
| 阿里云 ESSD | ✅ | ❌ | ❌ | ✅ |
| 阿里云 NAS | ✅ | ✅ | ✅ | ✅ |
| GCP Persistent Disk | ✅ | ✅ | ❌ | ✅ |
| Azure Disk | ✅ | ❌ | ❌ | ✅ |
| Azure Files | ✅ | ✅ | ✅ | ✅ |
| NFS | ✅ | ✅ | ✅ | ❌ |
| Ceph RBD | ✅ | ✅ | ❌ | ✅ |
| CephFS | ✅ | ✅ | ✅ | ✅ |
| Local PV | ✅ | ❌ | ❌ | ✅ |
| iSCSI | ✅ | ✅ | ❌ | ✅ |

---

## 5. 回收策略 (Reclaim Policy) 详解

| 策略 | 行为 | 适用场景 | 风险 |
|:---|:---|:---|:---|
| **Retain** | 保留数据，需手动清理 | 生产环境、重要数据 | 存储泄漏 |
| **Delete** | 自动删除 PV 和后端存储 | 临时数据、测试环境 | 数据丢失 |
| **Recycle** | 清空数据后重新可用 | 已废弃(v1.14) | 不推荐 |

### 生产环境建议

```yaml
# 生产环境：Retain 策略 + 定期备份
apiVersion: v1
kind: PersistentVolume
metadata:
  name: prod-mysql-pv
  labels:
    env: production
    backup: required
spec:
  capacity:
    storage: 500Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain  # 生产必须
  storageClassName: alicloud-disk-essd-pl1
  csi:
    driver: diskplugin.csi.alibabacloud.com
    volumeHandle: d-bp1xxxxxxxxxxxxx
    fsType: ext4
```

---

## 6. PV 绑定机制与算法

### 绑定流程

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────┐
│  PVC 创建   │────▶│  PV Controller   │────▶│  匹配算法   │
└─────────────┘     └──────────────────┘     └──────┬──────┘
                                                    │
                    ┌───────────────────────────────┘
                    ▼
    ┌───────────────────────────────────────────────────┐
    │              PV 匹配条件检查                       │
    │  1. StorageClass 匹配                             │
    │  2. AccessModes 包含                              │
    │  3. Capacity >= 请求容量                          │
    │  4. Selector 标签匹配 (如有)                      │
    │  5. VolumeMode 匹配                               │
    │  6. NodeAffinity 满足 (WaitForFirstConsumer)     │
    └───────────────────────────────────────────────────┘
                    │
                    ▼
    ┌───────────────────────────────────────────────────┐
    │              绑定优先级排序                        │
    │  1. 精确容量匹配优先                              │
    │  2. 最小满足容量优先                              │
    │  3. 先创建的 PV 优先                              │
    └───────────────────────────────────────────────────┘
```

### 绑定延迟模式 (VolumeBindingMode)

| 模式 | 说明 | 优点 | 缺点 |
|:---|:---|:---|:---|
| **Immediate** | PVC 创建时立即绑定 | 快速 | 可能跨可用区 |
| **WaitForFirstConsumer** | Pod 调度时绑定 | 拓扑感知 | 稍慢 |

```yaml
# 推荐：WaitForFirstConsumer 避免跨可用区问题
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: topology-aware-essd
provisioner: diskplugin.csi.alibabacloud.com
volumeBindingMode: WaitForFirstConsumer
parameters:
  type: cloud_essd
  performanceLevel: PL1
allowedTopologies:
  - matchLabelExpressions:
      - key: topology.diskplugin.csi.alibabacloud.com/zone
        values:
          - cn-hangzhou-h
          - cn-hangzhou-i
```

---

## 7. Local PV 配置详解

### Local PV 架构特点

| 特性 | 说明 |
|:---|:---|
| **数据本地性** | 数据存储在节点本地磁盘，无网络开销 |
| **节点绑定** | Pod 必须调度到 PV 所在节点 |
| **无高可用** | 节点故障 = 数据不可用 |
| **手动管理** | 需要预先创建，不支持动态供给 |

### Local PV 完整配置

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-node1-ssd
  labels:
    storage-tier: nvme
spec:
  capacity:
    storage: 1Ti
  volumeMode: Filesystem
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-nvme
  local:
    path: /mnt/disks/nvme0n1
  nodeAffinity:  # Local PV 必须配置
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: kubernetes.io/hostname
              operator: In
              values:
                - node-1
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-nvme
provisioner: kubernetes.io/no-provisioner  # 静态供给
volumeBindingMode: WaitForFirstConsumer    # 必须
reclaimPolicy: Retain
```

### Local PV 自动发现 (local-static-provisioner)

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: local-volume-provisioner
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: local-volume-provisioner
  template:
    metadata:
      labels:
        app: local-volume-provisioner
    spec:
      serviceAccountName: local-volume-provisioner
      containers:
        - name: provisioner
          image: registry.k8s.io/sig-storage/local-volume-provisioner:v2.5.0
          env:
            - name: MY_NODE_NAME
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
          volumeMounts:
            - name: local-disks
              mountPath: /mnt/disks
              mountPropagation: HostToContainer
            - name: provisioner-config
              mountPath: /etc/provisioner/config
      volumes:
        - name: local-disks
          hostPath:
            path: /mnt/disks
        - name: provisioner-config
          configMap:
            name: local-provisioner-config
```

---

## 8. PV 监控与告警

### Prometheus 监控指标

| 指标 | 说明 | 告警阈值建议 |
|:---|:---|:---|
| `kube_persistentvolume_status_phase` | PV 状态分布 | Failed > 0 |
| `kube_persistentvolume_capacity_bytes` | PV 容量 | - |
| `kubelet_volume_stats_used_bytes` | 已使用空间 | > 85% |
| `kubelet_volume_stats_available_bytes` | 可用空间 | < 10Gi |
| `kubelet_volume_stats_inodes_used` | inode 使用量 | > 90% |

### 告警规则配置

```yaml
groups:
  - name: pv-alerts
    rules:
      - alert: PersistentVolumeFailed
        expr: kube_persistentvolume_status_phase{phase="Failed"} == 1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "PV {{ $labels.persistentvolume }} 状态异常"
          
      - alert: PersistentVolumeUsageHigh
        expr: |
          kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "PV 使用率超过 85%"
          
      - alert: PersistentVolumeInodeExhaustion
        expr: |
          kubelet_volume_stats_inodes_used / kubelet_volume_stats_inodes > 0.90
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "PV inode 使用率超过 90%"
```

---

## 9. 常见问题排查

### 问题诊断命令

```bash
# 查看 PV 状态
kubectl get pv -o wide

# 查看 PV 详情
kubectl describe pv <pv-name>

# 查看 PV 事件
kubectl get events --field-selector involvedObject.kind=PersistentVolume

# 查看 CSI 驱动日志
kubectl logs -n kube-system -l app=csi-provisioner --tail=100

# 检查节点存储状态
kubectl get csinodes
kubectl describe csinode <node-name>
```

### 常见问题与解决

| 问题 | 可能原因 | 解决方案 |
|:---|:---|:---|
| PV 一直 Pending | CSI 驱动未就绪 | 检查 CSI Pod 状态 |
| PVC 无法绑定 PV | 容量/访问模式不匹配 | 检查 PV 规格 |
| Pod 挂载失败 | 节点无权限访问存储 | 检查 IAM/安全组 |
| 删除 PV 卡住 | Finalizer 未清除 | 检查是否有残留引用 |
| 扩容失败 | 存储类型不支持 | 确认 CSI 支持 ExpandVolume |

---
---
## 企业级配置模板

### 标准化PV配置模板

```yaml
# 企业级PV配置模板库
apiVersion: v1
kind: List
items:
# 高性能数据库PV模板
- apiVersion: v1
  kind: PersistentVolume
  metadata:
    name: db-high-performance-template
    labels:
      template: database
      performance-tier: high
      environment: production
    annotations:
      description: "高性能数据库存储模板"
      backup-policy: "hourly-snapshot"
      retention-days: "30"
  spec:
    capacity:
      storage: 1Ti
    accessModes:
      - ReadWriteOnce
    persistentVolumeReclaimPolicy: Retain
    storageClassName: fast-ssd-pl3
    volumeMode: Filesystem
    mountOptions:
      - noatime
      - nodiratime
      - discard
      - barrier=0
    nodeAffinity:
      required:
        nodeSelectorTerms:
        - matchExpressions:
          - key: node-role.kubernetes.io/database
            operator: Exists
    csi:
      driver: diskplugin.csi.alibabacloud.com
      fsType: ext4
      volumeAttributes:
        type: "cloud_essd"
        performanceLevel: "PL3"
        encrypted: "true"
        kmsKeyId: "kms-key-for-db"

# 标准应用PV模板
- apiVersion: v1
  kind: PersistentVolume
  metadata:
    name: app-standard-template
    labels:
      template: application
      performance-tier: standard
      environment: production
  spec:
    capacity:
      storage: 500Gi
    accessModes:
      - ReadWriteOnce
    persistentVolumeReclaimPolicy: Retain
    storageClassName: standard-ssd-pl1
    volumeMode: Filesystem
    mountOptions:
      - noatime
      - discard
    csi:
      driver: diskplugin.csi.alibabacloud.com
      fsType: ext4
      volumeAttributes:
        type: "cloud_essd"
        performanceLevel: "PL1"
        encrypted: "true"

# 共享存储PV模板
- apiVersion: v1
  kind: PersistentVolume
  metadata:
    name: shared-storage-template
    labels:
      template: shared
      access-mode: rwx
      environment: production
  spec:
    capacity:
      storage: 2Ti
    accessModes:
      - ReadWriteMany
    persistentVolumeReclaimPolicy: Retain
    storageClassName: shared-nas
    volumeMode: Filesystem
    mountOptions:
      - vers=4.1
      - rsize=1048576
      - wsize=1048576
      - hard
      - timeo=600
    csi:
      driver: nasplugin.csi.alibabacloud.com
      volumeHandle: "nas-server:/shared/path"
```

### PVC标准化声明模板

```yaml
# PVC标准化模板
apiVersion: v1
kind: List
items:
# 数据库PVC模板
- apiVersion: v1
  kind: PersistentVolumeClaim
  metadata:
    name: database-pvc-template
    namespace: production
    labels:
      app: database
      tier: backend
    annotations:
      volume.beta.kubernetes.io/storage-provisioner: diskplugin.csi.alibabacloud.com
      description: "数据库存储卷声明"
      backup-required: "true"
      sla-tier: "platinum"
  spec:
    accessModes:
      - ReadWriteOnce
    storageClassName: fast-ssd-pl3
    resources:
      requests:
        storage: 500Gi
    volumeMode: Filesystem

# 应用PVC模板
- apiVersion: v1
  kind: PersistentVolumeClaim
  metadata:
    name: application-pvc-template
    namespace: production
    labels:
      app: application
      tier: frontend
    annotations:
      description: "应用存储卷声明"
      backup-required: "false"
      sla-tier: "gold"
  spec:
    accessModes:
      - ReadWriteOnce
    storageClassName: standard-ssd-pl1
    resources:
      requests:
        storage: 100Gi
    volumeMode: Filesystem

# 共享PVC模板
- apiVersion: v1
  kind: PersistentVolumeClaim
  metadata:
    name: shared-pvc-template
    namespace: production
    labels:
      app: shared
      access-mode: rwx
    annotations:
      description: "共享存储卷声明"
  spec:
    accessModes:
      - ReadWriteMany
    storageClassName: shared-nas
    resources:
      requests:
        storage: 1Ti
    volumeMode: Filesystem
```

---
## 生产环境最佳实践

### 容量规划与预留策略

```python
# 智能容量规划算法
class StorageCapacityPlanner:
    def __init__(self):
        self.safety_margins = {
            'database': 0.3,      # 30% 安全边际
            'application': 0.2,   # 20% 安全边际
            'shared': 0.25        # 25% 安全边际
        }
        
        self.growth_factors = {
            'database': 1.2,      # 月增长率20%
            'application': 1.1,   # 月增长率10%
            'shared': 1.15        # 月增长率15%
        }
    
    def calculate_required_capacity(self, current_usage_gb, workload_type, forecast_months=12):
        """计算所需存储容量"""
        safety_margin = self.safety_margins[workload_type]
        growth_factor = self.growth_factors[workload_type]
        
        # 计算预测用量
        projected_usage = current_usage_gb * (growth_factor ** forecast_months)
        
        # 添加安全边际
        required_capacity = projected_usage * (1 + safety_margin)
        
        return {
            'current_usage': current_usage_gb,
            'projected_usage': round(projected_usage, 2),
            'required_capacity': round(required_capacity, 2),
            'safety_buffer': round(required_capacity - projected_usage, 2),
            'buffer_percentage': round(safety_margin * 100, 1)
        }

# 使用示例
planner = StorageCapacityPlanner()
result = planner.calculate_required_capacity(500, 'database', 12)
print(f"数据库12个月后需要容量: {result['required_capacity']} GB")
```

### 自动化健康检查脚本

```bash
#!/bin/bash
# enterprise-pv-health-check.sh

LOG_FILE="/var/log/pv-health-check.log"
ALERT_EMAIL="sre-team@company.com"

# 存储健康检查函数
check_pv_health() {
    echo "$(date): 开始PV健康检查" >> $LOG_FILE
    
    # 1. 检查PV状态异常
    FAILED_PV=$(kubectl get pv --field-selector=status.phase=Failed -o name)
    if [ -n "$FAILED_PV" ]; then
        echo "❌ 发现Failed状态的PV: $FAILED_PV" >> $LOG_FILE
        echo "Subject: PV Health Alert - Failed PV Detected" | \
            mail -s "PV Health Alert" $ALERT_EMAIL <<< "Failed PVs detected: $FAILED_PV"
    fi
    
    # 2. 检查长时间Pending的PVC
    LONG_PENDING_PVC=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Pending \
        -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name} {.metadata.creationTimestamp}{"\n"}{end}' | \
        awk -v cutoff="$(date -d '1 hour ago' -u +%Y-%m-%dT%H:%M:%SZ)" '$2 < cutoff')
    
    if [ -n "$LONG_PENDING_PVC" ]; then
        echo "⚠️  发现长时间Pending的PVC:" >> $LOG_FILE
        echo "$LONG_PENDING_PVC" >> $LOG_FILE
    fi
    
    # 3. 检查高使用率的PVC
    HIGH_USAGE_PVC=$(kubectl get pvc --all-namespaces -o json | \
        jq -r '.items[] | select(.status.capacity.storage and .spec.resources.requests.storage) |
               .usage_ratio = (.status.capacity.storage | split("Gi")[0] | tonumber) /
                             (.spec.resources.requests.storage | split("Gi")[0] | tonumber) |
               select(.usage_ratio > 0.9) |
               "\(.metadata.namespace)/\(.metadata.name): \(.usage_ratio*100)%"')
    
    if [ -n "$HIGH_USAGE_PVC" ]; then
        echo "🚨 高使用率PVC (>90%):" >> $LOG_FILE
        echo "$HIGH_USAGE_PVC" >> $LOG_FILE
    fi
    
    # 4. 检查CSI驱动状态
    CSI_PODS_UNHEALTHY=$(kubectl get pods -n kube-system | grep csi | grep -v Running)
    if [ -n "$CSI_PODS_UNHEALTHY" ]; then
        echo "❌ CSI驱动Pod异常:" >> $LOG_FILE
        echo "$CSI_PODS_UNHEALTHY" >> $LOG_FILE
    fi
    
    echo "$(date): PV健康检查完成" >> $LOG_FILE
}

# 定时执行（每30分钟）
while true; do
    check_pv_health
    sleep 1800
done
```

### 存储资源配额管理

```yaml
# Namespace级别的存储资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: storage-quota
  namespace: production
spec:
  hard:
    # 存储总量限制
    requests.storage: 10Ti
    # PVC数量限制
    persistentvolumeclaims: 100
    # 各StorageClass的限制
    requests.storage-class/fast-ssd-pl3: 2Ti
    requests.storage-class/standard-ssd-pl1: 5Ti
    requests.storage-class/shared-nas: 3Ti
  scopeSelector:
    matchExpressions:
    - scopeName: PriorityClass
      operator: In
      values:
      - production
```

---
## 容量管理与优化

### 智能容量回收策略

```python
# 容量回收优化器
class StorageReclamationOptimizer:
    def __init__(self):
        self.reclamation_rules = {
            'idle_threshold_days': 30,
            'low_utilization_threshold': 0.1,  # 10% 使用率
            'candidate_age_days': 7
        }
    
    def identify_reclamation_candidates(self, pvc_list):
        """识别可回收的存储资源"""
        candidates = []
        
        for pvc in pvc_list:
            # 检查是否闲置
            if hasattr(pvc, 'last_access_time'):
                idle_days = (datetime.now() - pvc.last_access_time).days
                if idle_days > self.reclamation_rules['idle_threshold_days']:
                    candidates.append({
                        'pvc': pvc.name,
                        'reason': 'long_idle',
                        'idle_days': idle_days,
                        'recommendation': 'consider_archival'
                    })
            
            # 检查低利用率
            if hasattr(pvc, 'utilization_ratio'):
                if pvc.utilization_ratio < self.reclamation_rules['low_utilization_threshold']:
                    candidates.append({
                        'pvc': pvc.name,
                        'reason': 'low_utilization',
                        'utilization': f"{pvc.utilization_ratio*100:.1f}%",
                        'recommendation': 'rightsizing_or_consolidation'
                    })
        
        return candidates

# 使用示例
optimizer = StorageReclamationOptimizer()
candidates = optimizer.identify_reclamation_candidates(active_pvcs)
for candidate in candidates:
    print(f"PVC {candidate['pvc']}: {candidate['reason']} - {candidate['recommendation']}")
```

### 容量优化自动化脚本

```bash
#!/bin/bash
# storage-optimization-automation.sh

# 存储容量优化主函数
optimize_storage_capacity() {
    echo "🔧 开始存储容量优化..."
    
    # 1. 识别过度分配的PVC
    echo "🔍 识别过度分配的存储..."
    OVER_ALLOCATED=$(kubectl get pvc --all-namespaces -o json | \
        jq -r '.items[] | select(.status.capacity.storage and .spec.resources.requests.storage) |
               .allocation_ratio = (.spec.resources.requests.storage | split("Gi")[0] | tonumber) /
                                  (.status.capacity.storage | split("Gi")[0] | tonumber) |
               select(.allocation_ratio > 2.0) |
               "\(.metadata.namespace)/\(.metadata.name): 请求\(.spec.resources.requests.storage), 实际\(.status.capacity.storage)"')
    
    if [ -n "$OVER_ALLOCATED" ]; then
        echo "发现过度分配的PVC:"
        echo "$OVER_ALLOCATED"
        # 生成优化建议...
    fi
    
    # 2. 识别可合并的小容量PVC
    echo "🔄 寻找可合并的存储卷..."
    SMALL_PVC=$(kubectl get pvc --all-namespaces -o json | \
        jq -r '.items[] | select(.spec.resources.requests.storage) |
               .size_gb = (.spec.resources.requests.storage | split("Gi")[0] | tonumber) |
               select(.size_gb < 10) |
               "\(.metadata.namespace)/\(.metadata.name): \(.size_gb)Gi"')
    
    if [ -n "$SMALL_PVC" ]; then
        echo "发现小容量PVC (<10Gi):"
        echo "$SMALL_PVC"
        # 建议合并策略...
    fi
    
    # 3. 清理Released状态的PV
    echo "🧹 清理已释放的PV..."
    RELEASED_PV=$(kubectl get pv --field-selector=status.phase=Released -o name)
    if [ -n "$RELEASED_PV" ]; then
        echo "发现Released状态的PV，建议清理:"
        echo "$RELEASED_PV"
        # 提供清理指导...
    fi
    
    echo "✅ 容量优化分析完成"
}

# 执行优化
optimize_storage_capacity
```

---
## 监控与告警配置

### 核心监控指标定义

```yaml
# 存储监控指标配置
storage_monitoring_config:
  metrics_collection:
    interval: 30s
    timeout: 10s
    scrape_limit: 1000
    
  key_metrics:
    # 容量相关
    - name: pvc_usage_percentage
      query: |
        (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) * 100
      thresholds:
        warning: 80
        critical: 95
      labels: [namespace, persistentvolumeclaim]
      
    - name: pv_allocation_efficiency
      query: |
        avg(kubelet_volume_stats_used_bytes / kube_persistentvolume_capacity_bytes)
      thresholds:
        warning: 0.3
        critical: 0.1
      labels: [persistentvolume]
      
    # 性能相关
    - name: storage_io_latency
      query: |
        rate(storage_operation_duration_seconds_sum[5m]) / 
        rate(storage_operation_duration_seconds_count[5m])
      thresholds:
        warning: 0.002  # 2ms
        critical: 0.005  # 5ms
      labels: [operation_name, volume_plugin]
      
    # 状态相关
    - name: pvc_binding_duration
      query: |
        histogram_quantile(0.95, rate(persistentvolumeclaim_binding_duration_seconds_bucket[5m]))
      thresholds:
        warning: 30
        critical: 60
      labels: [storageclass]

  alert_rules:
    - name: PVCUsageHigh
      severity: warning
      condition: pvc_usage_percentage > 85
      duration: 10m
      summary: "PVC使用率过高"
      description: "{{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 使用率 {{ $value }}%"
      
    - name: PVCUsageCritical
      severity: critical
      condition: pvc_usage_percentage > 95
      duration: 5m
      summary: "PVC使用率达到临界值"
      description: "{{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 使用率 {{ $value }}%，请立即处理"
```

### 自动化监控部署脚本

```bash
#!/bin/bash
# storage-monitoring-deployment.sh

# 部署存储监控配置
deploy_storage_monitoring() {
    echo "📈 部署存储监控配置..."
    
    # 1. 创建ServiceMonitor
    cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: storage-monitoring
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: storage-exporter
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
EOF
    
    # 2. 部署告警规则
    cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-alerts
  namespace: monitoring
spec:
  groups:
  - name: storage.rules
    rules:
    - alert: StorageHighUsage
      expr: (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) * 100 > 90
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "存储使用率过高 (instance {{ \$labels.instance }})"
        description: "{{ \$labels.namespace }}/{{ \$labels.persistentvolumeclaim }} 使用率 {{ \$value }}%"
        
    - alert: StorageCriticalError
      expr: kube_persistentvolume_status_phase{phase="Failed"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "存储卷状态异常"
        description: "PV {{ \$labels.persistentvolume }} 状态为Failed"
EOF
    
    echo "✅ 存储监控配置部署完成"
}

# 执行部署
deploy_storage_monitoring
```

---
## 故障预防与自愈

### 存储自愈机制

```yaml
# 存储故障自愈Operator配置
apiVersion: storage.k8s.io/v1
kind: StorageSelfHealingOperator
metadata:
  name: storage-healing-operator
spec:
  healingPolicies:
    # PVC Pending自愈
    - condition: PVCStatus == "Pending" && Age > 10m
      actions:
        - checkStorageClassExistence
        - verifyCSIProvisionerHealth
        - validateResourceQuotas
        - notifyAdminIfNotResolved
      
    # PV Failed自愈
    - condition: PVStatus == "Failed"
      actions:
        - attemptRecreation
        - fallbackToAlternativeStorageClass
        - createIncidentTicket
      
    # 高使用率预警
    - condition: PVCUsage > 90%
      actions:
        - sendEarlyWarning
        - triggerAutoScaling
        - recommendCapacityPlanning
      
    # CSI驱动异常
    - condition: CSIPodsUnhealthy
      actions:
        - restartCSIPods
        - validateNodeRegistration
        - checkCloudProviderConnectivity

  notificationChannels:
    - type: email
      recipients: ["sre-team@company.com", "storage-admin@company.com"]
    - type: webhook
      url: "https://alert-system.company.com/webhook/storage"
    - type: slack
      channel: "#storage-alerts"
```

### 故障预防检查清单

```markdown
## 📋 存储系统故障预防检查清单

### 🔧 基础设施检查
- [ ] 存储节点磁盘健康状态检查
- [ ] 网络连接稳定性验证
- [ ] 云服务商配额和限制确认
- [ ] 备份存储可用性验证

### 🛡️ 配置合规性检查
- [ ] StorageClass配置标准化审核
- [ ] PVC命名规范一致性检查
- [ ] 安全策略（加密、访问控制）合规性
- [ ] 资源配额设置合理性评估

### 📊 性能基线建立
- [ ] 正常IOPS/吞吐量基准值设定
- [ ] 延迟指标正常范围确定
- [ ] 容量使用趋势分析
- [ ] 故障恢复时间目标(RTO)验证

### 🔄 自动化机制验证
- [ ] 自动扩容策略有效性测试
- [ ] 故障转移机制演练
- [ ] 监控告警准确性验证
- [ ] 备份恢复流程测试

### 👥 运维流程确认
- [ ] 故障响应流程文档化
- [ ] 关键人员联系方式更新
- [ ] 值班安排和交接机制
- [ ] 知识库和文档时效性检查
```

---
| **标签管理** | 添加 `env`、`app`、`backup` 等标签便于管理 |
| **回收策略** | 生产环境使用 `Retain`，测试环境可用 `Delete` |
| **绑定模式** | 使用 `WaitForFirstConsumer` 避免跨可用区 |
| **容量规划** | 预留 20% 余量，配置扩容告警 |
| **监控告警** | 监控使用率、inode、状态异常 |
| **定期备份** | Retain 策略 + VolumeSnapshot 定期快照 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
