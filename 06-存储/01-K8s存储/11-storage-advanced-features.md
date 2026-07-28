---
title: 11 - 存储高级特性与优化策略
description: 'title: 11 - 存储高级特性与优化策略'
summary: 'title: 11 - 存储高级特性与优化策略'
category: general
tags:
- k8s
- storage
- pv
- pvc
- opa
- minio
- redis
- mysql
- elasticsearch
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 存储是什么？
- 如何使用存储？
- 存储的最佳实践是什么？
trigger_keywords:
- 存储高级特性与优化策略
- storage
- data
prerequisites:
- kubectl-basics
- storage-basics
- redis-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 11 - 存储高级特性与优化策略
description: '# 11 - 存储高级特性与优化策略'
category: storage
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- opa
- minio
- redis
- mysql
- elasticsearch
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 存储高级特性与优化策略 是什么
- 如何 存储高级特性与优化策略
- [[kubernetes|Kubernetes]] 6 storage 最佳实践
trigger_keywords:
- 存储高级特性与优化策略
- storage
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
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

# 11 - 存储高级特性与优化策略

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **运维重点**: 高级功能、性能优化、容量规划

<!-- chunk: 目录 -->
## 目录

1. [存储快照与克隆](#存储快照与克隆)
2. [存储扩容与收缩](#存储扩容与收缩)
3. [存储QoS与限速](#存储qos与限速)
4. [存储加密与安全](#存储加密与安全)
5. [存储分层与缓存](#存储分层与缓存)
6. [存储多路径与高可用](#存储多路径与高可用)
7. [存储性能基准测试](#存储性能基准测试)
8. [存储成本优化策略](#存储成本优化策略)

---

<!-- chunk: 存储快照与克隆 -->
## 存储快照与克隆

### VolumeSnapshot高级配置

```yaml
# 生产级VolumeSnapshotClass配置
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: production-snapshot-class
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: diskplugin.csi.alibabacloud.com
deletionPolicy: Retain  # 生产环境建议Retain
parameters:
  # 阿里云特有参数
  instantAccess: "true"  # 即时访问快照
  instantAccessRetentionDays: "1"
  # AWS EBS参数示例
  # tagSpecification_1: "Environment=Production"
  # tagSpecification_2: "Team=Database"
```

### 快照策略管理

```yaml
# 自动化快照策略
apiVersion: batch/v1
kind: CronJob
metadata:
  name: automated-snapshots
  namespace: production
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-operator
          containers:
          - name: snapshot-creator
            image: bitnami/kubectl:latest
            command:
            - /bin/sh
            - -c
            - |
              # 生成时间戳
              TIMESTAMP=$(date +%Y%m%d-%H%M%S)
              
              # 为关键应用创建快照
              APPS=("mysql-data" "redis-data" "elasticsearch-data")
              
              for app in "${APPS[@]}"; do
                cat <<EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: ${app}-snapshot-${TIMESTAMP}
  namespace: production
  labels:
    app: ${app}
    type: automated-backup
spec:
  volumeSnapshotClassName: production-snapshot-class
  source:
    persistentVolumeClaimName: ${app}-pvc
EOF
              done
              
              # 清理7天前的快照
              kubectl get volumesnapshot -n production -o json | \
                jq -r '.items[] | select(.metadata.creationTimestamp < "'$(date -d '7 days ago' --iso-8601)'") | .metadata.name' | \
                xargs -I {} kubectl delete volumesnapshot {} -n production
          restartPolicy: OnFailure
```

### 存储克隆操作

```yaml
# 从现有PVC克隆新PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cloned-app-data
  namespace: staging
spec:
  storageClassName: fast-ssd-pl2
  dataSource:
    name: production-app-data-pvc  # 源PVC名称
    kind: PersistentVolumeClaim
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 200Gi  # 可以大于等于源PVC大小
```

### 增量快照优化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# incremental-snapshot-manager.sh

NAMESPACE="production"
BASE_SNAPSHOT="weekly-full-backup"
INCREMENTAL_PREFIX="daily-incr"

create_incremental_snapshot() {
  BASE_TIMESTAMP=$(kubectl get volumesnapshot $BASE_SNAPSHOT -n $NAMESPACE -o jsonpath='{.metadata.creationTimestamp}')
  
  cat <<EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: ${INCREMENTAL_PREFIX}-$(date +%Y%m%d)
  namespace: $NAMESPACE
  annotations:
    snapshot.storage.k8s.io/base-snapshot: $BASE_SNAPSHOT
    snapshot.storage.k8s.io/base-timestamp: $BASE_TIMESTAMP
spec:
  volumeSnapshotClassName: production-snapshot-class
  source:
    persistentVolumeClaimName: app-data-pvc
EOF
}

# 每周创建全量快照，每日增量快照
case "$(date +%u)" in
  1)  # 周一创建全量快照
    create_full_snapshot
    ;;
  *)  # 其他日期创建增量快照
    create_incremental_snapshot
    ;;
esac
```
---

<!-- chunk: 存储扩容与收缩 -->
## 存储扩容与收缩

### 在线扩容最佳实践

```yaml
# 支持在线扩容的StorageClass配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: resizable-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true  # 启用扩容功能
allowedTopologies:
- matchLabelExpressions:
  - key: topology.diskplugin.csi.alibabacloud.com/zone
    values:
    - cn-hangzhou-a
    - cn-hangzhou-b
```

### 扩容操作流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# safe-volume-expansion.sh

PVC_NAME=$1
NAMESPACE=$2
NEW_SIZE=$3

if [ $# -ne 3 ]; then
  echo "Usage: $0 <pvc-name> <namespace> <new-size>"
  echo "Example: $0 mysql-data production 200Gi"
  exit 1
fi

# 1. 预检查
echo "🔍 执行预检查..."
CURRENT_SIZE=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.resources.requests.storage}')
STORAGE_CLASS=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.storageClassName}')

echo "当前大小: $CURRENT_SIZE"
echo "目标大小: $NEW_SIZE"
echo "StorageClass: $STORAGE_CLASS"

# 检查StorageClass是否支持扩容
if [ "$(kubectl get sc $STORAGE_CLASS -o jsonpath='{.allowVolumeExpansion}')" != "true" ]; then
  echo "❌ StorageClass $STORAGE_CLASS 不支持扩容"
  exit 1
fi

# 2. 创建扩容前快照
echo "📸 创建扩容前快照..."
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
SNAPSHOT_NAME="pre-expansion-${PVC_NAME}-${TIMESTAMP}"

kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: $SNAPSHOT_NAME
  namespace: $NAMESPACE
spec:
  volumeSnapshotClassName: production-snapshot-class
  source:
    persistentVolumeClaimName: $PVC_NAME
EOF

# 等待快照完成
echo "⏳ 等待快照创建完成..."
while [ "$(kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o jsonpath='{.status.readyToUse}')" != "true" ]; do
  sleep 5
done

# 3. 执行扩容
echo "🚀 执行扩容操作..."
kubectl patch pvc $PVC_NAME -n $NAMESPACE -p '{"spec":{"resources":{"requests":{"storage":"'$NEW_SIZE'"}}}}'

# 4. 监控扩容状态
echo "📊 监控扩容进度..."
kubectl get pvc $PVC_NAME -n $NAMESPACE -w

# 5. 验证扩容结果
echo "✅ 验证扩容结果..."
FINAL_SIZE=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.status.capacity.storage}')
echo "最终大小: $FINAL_SIZE"

# 6. 文件系统扩容（如需要）
echo "🔄 检查是否需要文件系统扩容..."
POD_NAME=$(kubectl get pods -n $NAMESPACE -o json | jq -r '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == "'$PVC_NAME'") | .metadata.name' | head -1)

if [ -n "$POD_NAME" ]; then
  echo "Pod $POD_NAME 使用此PVC，可能需要重启以完成文件系统扩容"
  read -p "是否重启Pod? (y/n): " RESTART
  if [ "$RESTART" = "y" ]; then
    DEPLOYMENT_NAME=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.metadata.ownerReferences[0].name}')
    kubectl rollout restart deployment/$DEPLOYMENT_NAME -n $NAMESPACE
  fi
fi

echo "🎉 扩容操作完成！"
```
### 存储收缩限制说明

```markdown
<!-- chunk: 存储收缩限制 -->
## 存储收缩限制

⚠️ **重要提醒**: Kubernetes目前不支持PVC收缩操作

### 技术限制原因
1. **文件系统限制**: 大多数文件系统不支持在线收缩
2. **数据安全**: 收缩可能导致数据丢失
3. **应用兼容性**: 应用程序可能无法处理存储空间减少

### 替代方案

#### 方案1: 数据迁移
```bash
# 1. 创建新的小容量PVC
# 2. 在应用层面迁移数据
# 3. 切换应用到新的PVC
# 4. 删除旧的PVC
```

#### 方案2: 文件系统级别操作
```bash
# 仅适用于特定场景，风险较高
# 1. 备份数据
# 2. 缩小文件系统（需要卸载）
# 3. 缩小底层卷（云服务商支持）
# 4. 重新挂载并验证
```
```

---

<!-- chunk: 存储QoS与限速 -->
## 存储QoS与限速

### 存储I/O限速配置

```yaml
# 带I/O限速的StorageClass（阿里云示例）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: qos-controlled-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  # I/O限速参数
  maxIOPS: "3000"      # 最大IOPS限制
  maxThroughput: "120" # 最大吞吐量(MB/s)
  minIOPS: "1000"      # 最小IOPS保证
  burstEnabled: "true" # 启用突发性能
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 应用级别存储QoS

```yaml
# 使用ResourceQuota限制存储使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: storage-quota
  namespace: production
spec:
  hard:
    requests.storage: 1000Gi    # 总存储请求限制
    persistentvolumeclaims: 50   # PVC数量限制
    requests.storageclass/fast-ssd-pl3.storage: 500Gi  # 特定存储类限制
---
# 使用LimitRange设置默认存储限制
apiVersion: v1
kind: LimitRange
metadata:
  name: storage-limits
  namespace: production
spec:
  limits:
  - type: PersistentVolumeClaim
    max:
      storage: 500Gi    # 最大PVC大小
    min:
      storage: 10Gi     # 最小PVC大小
    defaultRequest:
      storage: 50Gi     # 默认请求大小
```

### 存储性能监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# storage-qos-monitor.sh

NAMESPACE="production"
THRESHOLD_IOPS=5000
THRESHOLD_LATENCY_MS=50

monitor_storage_performance() {
  # 收集存储性能指标
  PODS=$(kubectl get pods -n $NAMESPACE -o jsonpath='{.items[*].metadata.name}')
  
  for POD in $PODS; do
    # 检查I/O统计
    IOPS=$(kubectl exec -it $POD -n $NAMESPACE -- iostat -x 1 1 | tail -1 | awk '{print $4+$5}')
    
    # 检查延迟
    LATENCY=$(kubectl exec -it $POD -n $NAMESPACE -- dd if=/dev/zero of=/tmp/test bs=4k count=1000 oflag=direct 2>&1 | grep bytes | awk '{print $NF}' | sed 's/[^0-9.]//g')
    
    # 告警判断
    if (( $(echo "$IOPS > $THRESHOLD_IOPS" | bc -l) )); then
      echo "🚨 告警: Pod $POD IOPS过高 ($IOPS)"
    fi
    
    if (( $(echo "$LATENCY > $THRESHOLD_LATENCY_MS" | bc -l) )); then
      echo "🚨 告警: Pod $POD 延迟过高 (${LATENCY}ms)"
    fi
  done
}

# 每5分钟执行一次监控
while true; do
  monitor_storage_performance
  sleep 300
done
```
---

<!-- chunk: 存储加密与安全 -->
## 存储加密与安全

### 静态数据加密

```yaml
# 启用静态加密的StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: encrypted-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"  # 启用加密
  kmsKeyId: "kms-key-12345678"  # 指定KMS密钥ID
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 加密密钥管理

```yaml
# Kubernetes Secret存储加密密钥
apiVersion: v1
kind: Secret
metadata:
  name: storage-encryption-keys
  namespace: kube-system
type: Opaque
data:
  # Base64编码的密钥材料
  master-key: <base64-encoded-master-key>
  kms-config: <base64-encoded-kms-config>
```

### 加密状态验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# encryption-verification.sh

verify_encryption_status() {
  echo "🔐 存储加密状态验证"
  echo "===================="
  
  # 1. 检查StorageClass加密配置
  echo "1. 检查加密StorageClass..."
  kubectl get sc -o json | jq -r '.items[] | select(.parameters.encrypted=="true") | .metadata.name'
  
  # 2. 检查加密PV数量
  echo ""
  echo "2. 加密PV统计..."
  ENCRYPTED_PV_COUNT=$(kubectl get pv -o json | jq -r '[.items[] | select(.spec.csi.volumeAttributes.encrypted=="true")] | length')
  TOTAL_PV_COUNT=$(kubectl get pv --no-headers | wc -l)
  echo "加密PV数量: $ENCRYPTED_PV_COUNT / $TOTAL_PV_COUNT"
  
  # 3. 检查KMS服务状态
  echo ""
  echo "3. KMS服务状态检查..."
  KMS_TYPE=$(kubectl get storageclass -o json | jq -r '[.items[] | select(.parameters.encrypted=="true" or .parameters."csi.storage.k8s.io/node-publish-secret-name"!=null)] | .[0].provisioner // "unknown"' 2>/dev/null)
  case "$KMS_TYPE" in
    disk|csi-disk)
      echo "  [阿里云KMS] 检查密钥服务..."
      if command -v aliyun &>/dev/null; then
        KMS_KEYS=$(kubectl get storageclass -o json | jq -r '[.items[].parameters."disk-encryption-kms-key-id" // empty] | unique[]')
        for KEY_ID in $KMS_KEYS; do
          KEY_STATUS=$(aliyun kms DescribeKey --KeyId "$KEY_ID" 2>/dev/null | jq -r '.KeyMetadata.KeyState // "unknown"')
          if [ "$KEY_STATUS" = "Enabled" ]; then
            echo "  ✅ 密钥 $KEY_ID 状态: $KEY_STATUS"
          else
            echo "  ❌ 密钥 $KEY_ID 状态: $KEY_STATUS (需关注)"
          fi
        done
      else
        echo "  aliyun CLI 未安装，请手动执行: aliyun kms DescribeKey --KeyId <KEY_ID>"
      fi
      ;;
    ebs.csi.aws.com)
      echo "  [AWS KMS] 检查密钥服务..."
      if command -v aws &>/dev/null; then
        KMS_KEYS=$(kubectl get storageclass -o json | jq -r '[.items[].parameters."kms-key-id" // empty] | unique[]')
        for KEY_ID in $KMS_KEYS; do
          KEY_STATUS=$(aws kms describe-key --key-id "$KEY_ID" --query 'KeyMetadata.KeyState' --output text 2>/dev/null)
          if [ "$KEY_STATUS" = "Enabled" ]; then
            echo "  ✅ 密钥 $KEY_ID 状态: $KEY_STATUS"
          else
            echo "  ❌ 密钥 $KEY_ID 状态: $KEY_STATUS (需关注)"
          fi
        done
      else
        echo "  aws CLI 未安装，请手动执行: aws kms describe-key --key-id <KEY_ID>"
      fi
      ;;
    *)
      echo "  通用KMS检查: 验证CSI驱动Secret配置..."
      kubectl get secrets -n kube-system -l app=csi-driver -o json | \
        jq -r '.items[] | "  Secret: \(.metadata.name) - 创建: \(.metadata.creationTimestamp)"' 2>/dev/null || \
        echo "  未发现CSI驱动Secret，请检查KMS集成配置"
      ;;
  esac
  
  # 4. 生成加密合规报告
  echo ""
  echo "4. 生成合规报告..."
  REPORT_FILE="/tmp/encryption-compliance-$(date +%Y%m%d).txt"
  cat > $REPORT_FILE <<EOF
存储加密合规报告
生成时间: $(date)
总PV数量: $TOTAL_PV_COUNT
加密PV数量: $ENCRYPTED_PV_COUNT
加密覆盖率: $((ENCRYPTED_PV_COUNT * 100 / TOTAL_PV_COUNT))%
EOF
  
  echo "合规报告已生成: $REPORT_FILE"
}

verify_encryption_status
```
---

<!-- chunk: 存储分层与缓存 -->
## 存储分层与缓存

### 存储分层策略

```yaml
# 多层级StorageClass配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: tiered-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  # 热数据层 - 高性能SSD
  hot-tier:
    type: cloud_essd
    performanceLevel: PL3
    iops: "1000000"
    
  # 温数据层 - 标准SSD
  warm-tier:
    type: cloud_essd
    performanceLevel: PL1
    iops: "50000"
    
  # 冷数据层 - 低成本存储
  cold-tier:
    type: cloud_efficiency
    iops: "3000"
```

### 自动分层存储实现

```yaml
# 使用Stork实现存储分层（示例）
apiVersion: stork.libopenstorage.org/v1alpha1
kind: Migration
metadata:
  name: storage-tiering-migration
  namespace: production
spec:
  # 源存储配置
  sourceStorageClass: hot-tier-storage
  # 目标存储配置
  destinationStorageClass: warm-tier-storage
  # 数据迁移策略
  migrationSchedule: "0 2 * * *"  # 每天凌晨2点执行
  # 条件触发
  triggers:
    - condition: storageUtilization < 30%
      action: migrateToLowerTier
    - condition: accessFrequency < 10/day
      action: migrateToLowerTier
```

### 存储缓存优化

```yaml
# Redis缓存层配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: redis-cache-layer
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: redis-cache
  template:
    metadata:
      labels:
        app: redis-cache
    spec:
      containers:
      - name: redis
        image: redis:7-alpine
        ports:
        - containerPort: 6379
        volumeMounts:
        - name: redis-data
          mountPath: /data
        resources:
          requests:
            memory: 2Gi
            cpu: 1
          limits:
            memory: 4Gi
            cpu: 2
      volumes:
      - name: redis-data
        persistentVolumeClaim:
          claimName: redis-local-ssd  # 使用本地SSD获得最佳性能
---
# 本地SSD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-ssd-cache
provisioner: kubernetes.io/no-provisioner
volumeBindingMode: WaitForFirstConsumer
reclaimPolicy: Delete
```

---

<!-- chunk: 存储多路径与高可用 -->
## 存储多路径与高可用

### 多路径I/O配置

```yaml
# 多路径存储配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: multipath-config
  namespace: kube-system
data:
  multipath.conf: |
    defaults {
        user_friendly_names yes
        find_multipaths yes
        polling_interval 10
    }
    
    devices {
        device {
            vendor "ALIBABA"
            product "Cloud_Disk"
            path_grouping_policy "group_by_prio"
            prio "alua"
            path_checker "tur"
            hardware_handler "1 alua"
            failback "immediate"
            rr_weight "priorities"
            no_path_retry "queue"
        }
    }
```

### 存储高可用配置

```yaml
# 高可用存储StatefulSet配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: ha-storage-cluster
  namespace: production
spec:
  serviceName: ha-storage
  replicas: 3
  selector:
    matchLabels:
      app: ha-storage
  template:
    metadata:
      labels:
        app: ha-storage
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - ha-storage
            topologyKey: kubernetes.io/hostname
      containers:
      - name: storage-node
        image: storage/node:latest
        volumeMounts:
        - name: data
          mountPath: /data
        readinessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - "storage-cli health-check"
          initialDelaySeconds: 30
          periodSeconds: 10
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd-pl2
      resources:
        requests:
          storage: 500Gi
```

---

<!-- chunk: 存储性能基准测试 -->
## 存储性能基准测试

### 标准性能测试套件

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# storage-benchmark-suite.sh

TEST_NAMESPACE="benchmark"
PVC_NAME="benchmark-pvc"
STORAGE_CLASS="fast-ssd-pl2"

# 1. 创建测试环境
setup_benchmark_environment() {
  cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: $PVC_NAME
  namespace: $TEST_NAMESPACE
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: $STORAGE_CLASS
  resources:
    requests:
      storage: 100Gi
---
apiVersion: v1
kind: Pod
metadata:
  name: benchmark-pod
  namespace: $TEST_NAMESPACE
spec:
  containers:
  - name: fio-benchmark
    image: ljishen/fio
    command: ["sleep", "3600"]
    volumeMounts:
    - name: benchmark-volume
      mountPath: /data
  volumes:
  - name: benchmark-volume
    persistentVolumeClaim:
      claimName: $PVC_NAME
EOF
}

# 2. 执行FIO基准测试
run_fio_benchmark() {
  echo "🏃‍♂️ 执行FIO基准测试..."
  
  # 顺序读写测试
  kubectl exec -it benchmark-pod -n $TEST_NAMESPACE -- \
    fio --name=seq-read --filename=/data/testfile --rw=read --bs=1M --size=10G --numjobs=1 --iodepth=32 --direct=1 --runtime=60 --time_based --group_reporting
  
  kubectl exec -it benchmark-pod -n $TEST_NAMESPACE -- \
    fio --name=seq-write --filename=/data/testfile --rw=write --bs=1M --size=10G --numjobs=1 --iodepth=32 --direct=1 --runtime=60 --time_based --group_reporting
  
  # 随机读写测试
  kubectl exec -it benchmark-pod -n $TEST_NAMESPACE -- \
    fio --name=rand-read --filename=/data/testfile --rw=randread --bs=4k --size=10G --numjobs=16 --iodepth=32 --direct=1 --runtime=60 --time_based --group_reporting
  
  kubectl exec -it benchmark-pod -n $TEST_NAMESPACE -- \
    fio --name=rand-write --filename=/data/testfile --rw=randwrite --bs=4k --size=10G --numjobs=16 --iodepth=32 --direct=1 --runtime=60 --time_based --group_reporting
}

# 3. 生成性能报告
generate_report() {
  REPORT_FILE="/tmp/storage-benchmark-$(date +%Y%m%d-%H%M%S).html"
  
  cat > $REPORT_FILE <<EOF
<!DOCTYPE html>
<html>
<head>
    <title>存储性能基准测试报告</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .test-result { margin: 20px 0; padding: 15px; border: 1px solid #ddd; }
        .metric { display: inline-block; width: 200px; }
        .value { font-weight: bold; color: #0066cc; }
    </style>
</head>
<body>
    <h1>存储性能基准测试报告</h1>
    <p>测试时间: $(date)</p>
    <p>StorageClass: $STORAGE_CLASS</p>
    
    <div class="test-result">
        <h3>顺序读取性能</h3>
        <div class="metric">带宽:</div><div class="value">$(kubectl exec benchmark-pod -n $TEST_NAMESPACE -- fio --name=test --filename=/data/testfile --rw=read --bs=1M --size=1G --direct=1 --minimal | cut -d',' -f7) MB/s</div><br>
        <div class="metric">IOPS:</div><div class="value">$(kubectl exec benchmark-pod -n $TEST_NAMESPACE -- fio --name=test --filename=/data/testfile --rw=read --bs=1M --size=1G --direct=1 --minimal | cut -d',' -f8)</div>
    </div>
    
    <div class="test-result">
        <h3>随机读取性能</h3>
        <div class="metric">IOPS:</div><div class="value">$(kubectl exec benchmark-pod -n $TEST_NAMESPACE -- fio --name=test --filename=/data/testfile --rw=randread --bs=4k --size=1G --direct=1 --minimal | cut -d',' -f8)</div><br>
        <div class="metric">延迟:</div><div class="value">$(kubectl exec benchmark-pod -n $TEST_NAMESPACE -- fio --name=test --filename=/data/testfile --rw=randread --bs=4k --size=1G --direct=1 --minimal | cut -d',' -f40) ms</div>
    </div>
</body>
</html>
EOF
  
  echo "📊 性能报告已生成: $REPORT_FILE"
}

# 执行完整测试流程
main() {
  setup_benchmark_environment
  sleep 30  # 等待Pod启动
  run_fio_benchmark
  generate_report
  # 清理测试资源
  kubectl delete pod benchmark-pod -n $TEST_NAMESPACE
  kubectl delete pvc $PVC_NAME -n $TEST_NAMESPACE
}

main
```
---

<!-- chunk: 存储成本优化策略 -->
## 存储成本优化策略

### 成本分析与优化

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# storage-cost-optimizer.sh

analyze_storage_costs() {
  echo "💰 存储成本分析报告"
  echo "==================="
  
  # 1. 按StorageClass统计使用量
  echo "1. StorageClass使用统计:"
  kubectl get pvc --all-namespaces -o json | \
    jq -r '.items[] | "\(.spec.storageClassName)\t\(.spec.resources.requests.storage)"' | \
    sort | uniq -c
  
  # 2. 识别闲置存储
  echo ""
  echo "2. 高闲置率PVC识别:"
  kubectl get pvc --all-namespaces -o json | \
    jq -r '.items[] | select(.status.capacity.storage and .spec.resources.requests.storage) | 
           "\(.metadata.namespace)/\(.metadata.name)\t\(.status.capacity.storage)\t\(.spec.resources.requests.storage)"' | \
    awk '{usage=$2/$3; if(usage<0.3) print $1"\t使用率:"usage}'
  
  # 3. 成本计算（示例）
  echo ""
  echo "3. 预估月度成本:"
  FAST_SSD_COST_PER_GB=0.15  # 元/GB/月
  STANDARD_SSD_COST_PER_GB=0.10
  ECONOMY_SSD_COST_PER_GB=0.07
  
  FAST_USAGE=$(kubectl get pvc --all-namespaces -o json | jq '[.items[] | select(.spec.storageClassName=="fast-ssd-pl2")] | map(.spec.resources.requests.storage | rtrimstr("Gi") | tonumber) | add')
  STANDARD_USAGE=$(kubectl get pvc --all-namespaces -o json | jq '[.items[] | select(.spec.storageClassName=="standard-ssd-pl1")] | map(.spec.resources.requests.storage | rtrimstr("Gi") | tonumber) | add')
  ECONOMY_USAGE=$(kubectl get pvc --all-namespaces -o json | jq '[.items[] | select(.spec.storageClassName=="economy-ssd-pl0")] | map(.spec.resources.requests.storage | rtrimstr("Gi") | tonumber) | add')
  
  echo "高性能SSD: ${FAST_USAGE}Gi × ¥${FAST_SSD_COST_PER_GB} = ¥$(echo "${FAST_USAGE} * ${FAST_SSD_COST_PER_GB}" | bc)"
  echo "标准SSD: ${STANDARD_USAGE}Gi × ¥${STANDARD_SSD_COST_PER_GB} = ¥$(echo "${STANDARD_USAGE} * ${STANDARD_SSD_COST_PER_GB}" | bc)"
  echo "经济型SSD: ${ECONOMY_USAGE}Gi × ¥${ECONOMY_SSD_COST_PER_GB} = ¥$(echo "${ECONOMY_USAGE} * ${ECONOMY_SSD_COST_PER_GB}" | bc)"
  echo "总成本估算: ¥$(echo "${FAST_USAGE} * ${FAST_SSD_COST_PER_GB} + ${STANDARD_USAGE} * ${STANDARD_SSD_COST_PER_GB} + ${ECONOMY_USAGE} * ${ECONOMY_SSD_COST_PER_GB}" | bc)"
}

optimize_storage_costs() {
  echo ""
  echo "💡 成本优化建议:"
  echo "================"
  
  # 1. 降级建议
  echo "1. 存储类型降级建议:"
  kubectl get pvc --all-namespaces -o json | \
    jq -r '.items[] | select(.spec.storageClassName=="fast-ssd-pl2" and (.status.capacity.storage | rtrimstr("Gi") | tonumber) < 50) | 
           "Namespace: \(.metadata.namespace), PVC: \(.metadata.name), 建议降级至standard-ssd-pl1"'
  
  # 2. 缩容建议
  echo ""
  echo "2. 存储容量优化建议:"
  kubectl get pvc --all-namespaces -o json | \
    jq -r '.items[] | select(.status.capacity.storage and .spec.resources.requests.storage) | 
           usage=(.status.capacity.storage | rtrimstr("Gi") | tonumber) / (.spec.resources.requests.storage | rtrimstr("Gi") | tonumber) |
           select(usage < 0.5) | 
           "Namespace: \(.metadata.namespace), PVC: \(.metadata.name), 当前使用率: \(usage*100)%，建议缩容"'
  
  # 3. 生命周期管理
  echo ""
  echo "3. 生命周期优化:"
  echo "• 实施数据分层存储策略"
  echo "• 建立定期清理机制"
  echo "• 使用对象存储归档冷数据"
  echo "• 实施快照保留策略"
}

analyze_storage_costs
optimize_storage_costs
```
---
**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 存储 KUDIG Database — Global MOC
- [[06-存储/README.md|Storage Domain 存储领域知识库]]
- Domain-6 存储 — 开源项目索引
- 存储架构概览与核心组件
- PV/PVC 核心概念与企业级实践
- 03 - PVC使用模式与最佳实践
- StorageClass 动态供给与多租户管理
- 05 - CSI驱动集成与运维管理
- 06 - 存储基础概念详解
- 07 - 存储日常运维操作手册
- 08 - 存储性能调优与优化策略
- 09 - PV/PVC故障排查与解决方案

## See Also

- 09-pv-pvc-troubleshooting
- 10-storage-backup-disaster-recovery
- 12-storage-monitoring-alerting
- 13-storage-security-compliance

## Related

- [[21-生态参考/03-领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
