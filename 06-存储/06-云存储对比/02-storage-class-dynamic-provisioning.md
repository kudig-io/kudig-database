---
title: Kubernetes StorageClass and Dynamic Provisioning Deep Dive
description: K8s 存储类与动态供给 — StorageClass 配置、CSI 驱动、卷快照、扩容、拓扑感知、多租户存储策略
summary: Kubernetes 动态存储供给的完整实践，涵盖主流 CSI 驱动配置与生产调优
category: practice
tags:
- storageclass
- csi
- dynamic-provisioning
- volume-snapshot
- persistent-volume
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: storage
---
# Kubernetes StorageClass 与动态供给深度实践

> 从 StorageClass 配置到生产级存储策略的完整指南。

## 存储供给模型

```
PVC 请求 → StorageClass 匹配 → CSI Driver 创建卷 → PV 绑定 → Pod 挂载
    │              │                    │
    │         参数/供给者           云 API / 本地存储
    │              │                    │
    └──── 动态供给（自动）──── vs ──── 静态供给（手动）
```

## StorageClass 配置

### AWS EBS（gp3）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-encrypted
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
  kmsKeyId: arn:aws:kms:us-east-1:123456789:key/xxx
volumeBindingMode: WaitForFirstConsumer  # 拓扑感知
allowVolumeExpansion: true
reclaimPolicy: Retain
mountOptions:
  - noatime
  - nodiratime
```

### GCP PD（pd-ssd）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-ssd-regional
provisioner: pd.csi.storage.gke.io
parameters:
  type: pd-ssd
  replication-type: regional  # 跨 zone 复制
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
```

### Azure Disk（Premium SSD v2）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: premium-ssd-v2
provisioner: disk.csi.azure.com
parameters:
  skuName: PremiumV2_LRS
  diskIOPSReadWrite: "3000"
  diskMBpsReadWrite: "125"
  cachingMode: None
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 本地存储（Local PV）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-ssd
provisioner: kubernetes.io/no-provisioner
volumeBindingMode: WaitForFirstConsumer
reclaimPolicy: Retain
---
# 需要手动创建 PV 或使用 local-volume-provisioner
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-node1-ssd1
spec:
  capacity:
    storage: 500Gi
  volumeMode: Filesystem
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-ssd
  local:
    path: /mnt/disks/ssd1
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: kubernetes.io/hostname
              operator: In
              values: ["node-1"]
```

## 卷快照与克隆

### VolumeSnapshotClass

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapshot
driver: ebs.csi.aws.com
deletionPolicy: Retain
parameters:
  tagSpecification_1: "Environment=production"
  tagSpecification_2: "ManagedBy=kubernetes"
```

### 创建快照与恢复

```yaml
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-20260721
  namespace: production
spec:
  volumeSnapshotClassName: ebs-snapshot
  source:
    persistentVolumeClaimName: postgres-data
---
# 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data-restored
  namespace: production
spec:
  storageClassName: gp3-encrypted
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: db-snapshot-20260721
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

### 定时快照（VolumeSnapshotContent CronJob）

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: volume-snapshot-cron
  namespace: production
spec:
  schedule: "0 */6 * * *"  # 每 6 小时
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator
          containers:
            - name: snapshot
              image: bitnami/kubectl:1.30
              command:
                - /bin/sh
                - -c
                - |
                  TIMESTAMP=$(date +%Y%m%d-%H%M)
                  cat <<EOF | kubectl apply -f -
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: postgres-snap-${TIMESTAMP}
                    namespace: production
                  spec:
                    volumeSnapshotClassName: ebs-snapshot
                    source:
                      persistentVolumeClaimName: postgres-data
                  EOF
                  # 清理 7 天前的快照
                  kubectl get volumesnapshot -n production -o name | \
                    grep postgres-snap | head -n -28 | \
                    xargs -r kubectl delete -n production
          restartPolicy: OnFailure
```

## 卷扩容

```yaml
# 在线扩容 PVC（需 StorageClass allowVolumeExpansion: true）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
  namespace: production
spec:
  resources:
    requests:
      storage: 200Gi  # 从 100Gi 扩到 200Gi（只能扩不能缩）
```

```bash
# 监控扩容状态
kubectl get pvc postgres-data -n production -o jsonpath='{.status.conditions}'
# 如果文件系统需要扩展，Pod 重启后自动 resize
kubectl describe pvc postgres-data -n production | grep -A5 Conditions
```

## 多租户存储策略

### ResourceQuota 限制存储

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: storage-quota
  namespace: team-a
spec:
  hard:
    requests.storage: 500Gi
    persistentvolumeclaims: "20"
    gp3-encrypted.storageclass.storage.k8s.io/requests.storage: 300Gi
    standard.storageclass.storage.k8s.io/requests.storage: 200Gi
```

### LimitRange 限制单 PVC 大小

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: pvc-limits
  namespace: team-a
spec:
  limits:
    - type: PersistentVolumeClaim
      max:
        storage: 100Gi
      min:
        storage: 1Gi
```

## 存储性能基准

| 存储类型 | IOPS | 吞吐 | 延迟 | 适用场景 |
|----------|------|------|------|----------|
| AWS gp3 | 3,000-16,000 | 125-1,000 MB/s | ~1ms | 通用数据库 |
| AWS io2 | 最高 256,000 | 最高 4,000 MB/s | < 1ms | 高性能数据库 |
| GCP pd-ssd | 最高 100,000 | 最高 1,200 MB/s | ~1ms | 高性能工作负载 |
| GCP pd-balanced | 最高 16,000 | 最高 240 MB/s | ~2ms | 通用 |
| Azure Premium v2 | 最高 80,000 | 最高 1,200 MB/s | < 1ms | 高性能 |
| Local NVMe | 最高 1,000,000 | 最高 6,000 MB/s | < 0.1ms | 缓存/临时数据 |
| NFS/EFS | 取决于配置 | 取决于配置 | 5-20ms | 共享文件 |

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| PVC Pending | StorageClass 不存在/配额不足 | `kubectl describe pvc` |
| Pod 挂载超时 | CSI Driver 异常/卷未就绪 | `kubectl logs -n kube-system csi-attacher-*` |
| 扩容失败 | 文件系统不支持/卷类型限制 | 检查 StorageClass allowVolumeExpansion |
| I/O 性能差 | 卷类型 IOPS 不足/节点 EBS 带宽饱和 | `iostat -x 1` + 云监控 |
| 数据丢失 | reclaimPolicy=Delete + PVC 删除 | 改为 Retain + 定期快照 |
| 跨 AZ 挂载失败 | WaitForFirstConsumer 未设置 | 设置 volumeBindingMode |

```bash
# CSI Driver 状态检查
kubectl get csidrivers
kubectl get csinodes
kubectl get pods -n kube-system -l app=ebs-csi-controller
kubectl logs -n kube-system -l app=ebs-csi-node --tail=50

# PVC/PV 状态
kubectl get pvc -A | grep -v Bound
kubectl describe pvc <name> -n <ns>
kubectl get pv | grep Released
```

## 最佳实践

1. **始终设置 `volumeBindingMode: WaitForFirstConsumer`** — 避免跨 AZ 调度失败
2. **生产使用 `reclaimPolicy: Retain`** — 防止误删数据
3. **启用 `allowVolumeExpansion`** — 支持在线扩容
4. **定期快照** — CronJob 自动创建 + 清理
5. **加密** — 所有生产 StorageClass 启用加密
6. **监控 PVC 使用率** — 80% 告警，提前扩容
7. **避免 NFS 做数据库存储** — 延迟高、锁机制弱

## StorageClass 治理与准入控制

### 命名规范与标签策略

```yaml
# 生产级 StorageClass 标准模板
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: <tier>-<type>-<region>  # 如 fast-ssd-us-east-1
  labels:
    storage-tier: fast          # fast | balanced | archive
    environment: production     # production | staging | dev
    cost-center: engineering    # 成本归属
    data-classification: internal  # public | internal | confidential
  annotations:
    description: "高性能 SSD，适用于 OLTP 数据库"
    owner: "platform-team@company.com"
    sla: "99.99%"
provisioner: ebs.csi.aws.com
parameters:
  type: io2
  iops: "10000"
  encrypted: "true"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
```

### OPA/Gatekeeper 准入策略：禁止非标准 StorageClass

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedStorageClasses
metadata:
  name: restrict-storage-classes
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["PersistentVolumeClaim"]
    excludedNamespaces:
      - kube-system
  parameters:
    storageClasses:
      - fast-ssd
      - balanced-ssd
      - standard-hdd
      - shared-nfs
---
# 禁止使用默认 StorageClass（强制显式指定）
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequireExplicitStorageClass
metadata:
  name: no-default-storageclass
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["PersistentVolumeClaim"]
  parameters:
    message: "生产环境必须显式指定 storageClassName，禁止依赖默认值"
```

## 动态供给性能调优

### CSI Driver 资源与并发配置

```yaml
# EBS CSI Controller 资源调优（生产环境）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ebs-csi-controller
  namespace: kube-system
spec:
  replicas: 2  # 生产至少 2 副本
  template:
    spec:
      containers:
        - name: ebs-plugin
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
        - name: csi-provisioner
          args:
            - --worker-threads=10      # 并发供给线程
            - --timeout=60s            # 供给超时
            - --retry-interval-start=1s
            - --retry-interval-max=30s
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 128Mi
        - name: csi-attacher
          args:
            - --worker-threads=10
            - --timeout=120s           # Attach 超时（云盘挂载较慢）
```

### 供给性能基准与告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-provisioning-alerts
  namespace: monitoring
spec:
  groups:
    - name: storage-provisioning
      rules:
        - alert: PVProvisioningSlow
          expr: |
            histogram_quantile(0.95,
              sum(rate(csi_operations_seconds_bucket{method_name="/csi.v1.Controller/CreateVolume"}[5m])) by (le, driver_name)
            ) > 30
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "CSI 卷供给 P95 延迟 > 30s (driver: {{ $labels.driver_name }})"
            runbook: "检查 CSI Controller 日志、云 API 限流、节点资源"

        - alert: CSIControllerDown
          expr: |
            kube_deployment_status_replicas_available{deployment=~".*csi.*controller.*"} < 1
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "CSI Controller 无可用副本: {{ $labels.deployment }}"
            runbook: "立即检查 kube-system 中 CSI Controller Pod 状态"

        - alert: VolumeAttachSlow
          expr: |
            histogram_quantile(0.95,
              sum(rate(csi_operations_seconds_bucket{method_name="/csi.v1.Controller/ControllerPublishVolume"}[5m])) by (le, driver_name)
            ) > 60
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "卷 Attach P95 延迟 > 60s"
            runbook: "检查云盘配额、节点 Attach 限制、CSI Attacher 日志"
```

## FinOps：存储成本标签与分账

### 成本标签自动化注入

```yaml
# 通过 Mutating Webhook 自动为 PV 添加成本标签
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: storage-cost-labeler
webhooks:
  - name: cost.storage.company.com
    rules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE"]
        resources: ["persistentvolumeclaims"]
    clientConfig:
      service:
        name: cost-labeler-svc
        namespace: platform-system
        path: /mutate-pvc
    admissionReviewVersions: ["v1"]
    sideEffects: None
```

### 存储成本分账 PromQL

```promql
# 按命名空间统计存储成本（假设 $0.10/GiB/月）
sum by (namespace) (
  kube_persistentvolumeclaim_resource_requests_storage_bytes
) / 1024^3 * 0.10

# 按 StorageClass 统计成本分布
sum by (storageclass) (
  kube_persistentvolumeclaim_resource_requests_storage_bytes
) / 1024^3 * 0.10

# 存储成本月环比增长
(
  sum(kube_persistentvolumeclaim_resource_requests_storage_bytes) -
  sum(kube_persistentvolumeclaim_resource_requests_storage_bytes offset 30d)
) / sum(kube_persistentvolumeclaim_resource_requests_storage_bytes offset 30d) * 100
```

## 自动化运维脚本

### StorageClass 健康检查脚本

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# StorageClass 健康检查与合规审计

echo "=== 1. StorageClass 列表与配置 ==="
kubectl get storageclass -o custom-columns=\
NAME:.metadata.name,\
PROVISIONER:.provisioner,\
RECLAIM:.reclaimPolicy,\
BINDING:.volumeBindingMode,\
EXPANSION:.allowVolumeExpansion,\
DEFAULT:.metadata.annotations.storageclass\.kubernetes\.io/is-default-class

echo ""
echo "=== 2. CSI Driver 状态 ==="
kubectl get csidrivers -o custom-columns=NAME:.metadata.name,ATTACH:.spec.attachRequired,MODES:.spec.volumeLifecycleModes

echo ""
echo "=== 3. CSI Node 注册状态 ==="
kubectl get csinodes -o custom-columns=NODE:.metadata.name,DRIVERS:.drivers[*].name

echo ""
echo "=== 4. PVC 状态异常检查 ==="
kubectl get pvc -A --field-selector=status.phase!=Bound -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,STATUS:.status.phase,SC:.spec.storageClassName

echo ""
echo "=== 5. PV 容量使用率 Top 10 ==="
kubectl get pvc -A -o json | jq -r '
  .items[] |
  select(.status.phase == "Bound") |
  "\(.metadata.namespace)/\(.metadata.name) \(.spec.resources.requests.storage)"
' | sort -k2 -h -r | head -10

echo ""
echo "=== 6. 默认 StorageClass 检查 ==="
DEFAULT_COUNT=$(kubectl get storageclass -o json | jq '[.items[] | select(.metadata.annotations["storageclass.kubernetes.io/is-default-class"] == "true")] | length')
if [ "$DEFAULT_COUNT" -gt 1 ]; then
  echo "⚠️  存在多个默认 StorageClass，可能导致 PVC 供给不确定"
  kubectl get storageclass -o json | jq -r '.items[] | select(.metadata.annotations["storageclass.kubernetes.io/is-default-class"] == "true") | .metadata.name'
elif [ "$DEFAULT_COUNT" -eq 0 ]; then
  echo "ℹ️  无默认 StorageClass（生产环境建议显式指定）"
else
  echo "✅ 默认 StorageClass 配置正常"
fi
```

### 批量 PVC 扩容脚本

```bash
#!/bin/bash
# 🟡 中风险：修改 PVC 大小
# 批量扩容指定命名空间下使用率 > 80% 的 PVC

NAMESPACE=${1:-production}
THRESHOLD=${2:-80}
EXPAND_RATIO=${3:-1.5}  # 扩容倍数

echo "检查命名空间 $NAMESPACE 中 PVC 使用率 > ${THRESHOLD}% 的卷..."

kubectl get pvc -n "$NAMESPACE" -o json | jq -r '
  .items[] |
  select(.status.phase == "Bound") |
  "\(.metadata.name) \(.spec.resources.requests.storage) \(.spec.storageClassName)"
' | while read -r PVC_NAME CURRENT_SIZE SC_NAME; do
  # 检查 StorageClass 是否支持扩容
  EXPANSION=$(kubectl get storageclass "$SC_NAME" -o jsonpath='{.allowVolumeExpansion}')
  if [ "$EXPANSION" != "true" ]; then
    echo "⚠️  $PVC_NAME: StorageClass $SC_NAME 不支持扩容，跳过"
    continue
  fi
  
  # 获取实际使用率
  USAGE=$(kubectl exec -n "$NAMESPACE" deploy/metrics-collector -- \
    df /data 2>/dev/null | tail -1 | awk '{print $5}' | tr -d '%')
  
  if [ -n "$USAGE" ] && [ "$USAGE" -gt "$THRESHOLD" ]; then
    # 计算新大小
    CURRENT_GI=$(echo "$CURRENT_SIZE" | sed 's/Gi//')
    NEW_GI=$(echo "$CURRENT_GI * $EXPAND_RATIO" | bc | cut -d. -f1)
    echo "📈 扩容 $PVC_NAME: ${CURRENT_SIZE} → ${NEW_GI}Gi (使用率: ${USAGE}%)"
    kubectl patch pvc "$PVC_NAME" -n "$NAMESPACE" --type merge -p \
      "{\"spec\":{\"resources\":{\"requests\":{\"storage\":\"${NEW_GI}Gi\"}}}}"
  fi
done
```

## 存储供给故障排查决策树

```
PVC Pending
├── StorageClass 不存在？
│   └── kubectl get sc → 创建正确的 StorageClass
├── CSI Driver 未运行？
│   └── kubectl get pods -n kube-system -l app=*csi* → 修复 CSI Pod
├── 云 API 配额不足？
│   └── 检查云控制台磁盘配额 → 申请扩容
├── ResourceQuota 限制？
│   └── kubectl describe resourcequota -n <ns> → 调整配额
└── WaitForFirstConsumer + 无合适节点？
    └── 检查节点拓扑标签 → 确认 AZ/Zone 匹配

PVC Bound 但 Pod 挂载失败
├── VolumeAttachment 超时？
│   └── kubectl get volumeattachment → 检查 Attach/Detach 状态
├── 节点 CSI Node Plugin 异常？
│   └── kubectl logs -n kube-system -l app=*csi-node* → 重启 DaemonSet
├── 文件系统损坏？
│   └── 节点 dmesg | grep -i error → fsck 修复
└── 权限/SELinux/AppArmor 阻止？
    └── 检查节点审计日志 → 调整安全上下文
```

## Related

- [[06-存储/06-云存储对比/index.md|云存储对比]]
- [[06-存储/06-云存储对比/01-cloud-storage-comparison.md|云存储选型]]
- [[06-存储/05-存储网络/02-csi-driver-architecture.md|CSI 驱动架构]]
