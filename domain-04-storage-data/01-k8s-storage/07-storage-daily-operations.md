---
title: 07 - 存储日常运维操作手册
description: '# 07 - 存储日常运维操作手册'
category: storage
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- helm
- daemonset
- job
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 存储日常运维操作手册 是什么
- 如何 存储日常运维操作手册
- Kubernetes 6 storage 最佳实践
trigger_keywords:
- 存储日常运维操作手册
- storage
prerequisites:
- kubectl-basics
- storage-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
---

# 07 - 存储日常运维操作手册

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **运维重点**: 日常操作、故障处理、性能监控

<!-- chunk: 目录 -->
## 目录

1. [存储资源查看与监控](#存储资源查看与监控)
2. [PVC创建与管理](#pvc创建与管理)
3. [存储扩容操作](#存储扩容操作)
4. [存储备份与恢复](#存储备份与恢复)
5. [CSI驱动运维](#csi驱动运维)
6. [存储性能调优](#存储性能调优)
7. [日常巡检脚本](#日常巡检脚本)
8. [应急处理流程](#应急处理流程)

---

<!-- chunk: 存储资源查看与监控 -->
## 存储资源查看与监控

### 基础资源查询命令

```bash
# 1. 查看所有StorageClass
kubectl get storageclass
kubectl get sc -o wide

# 2. 查看所有PV状态
kubectl get pv
kubectl get pv -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,CAPACITY:.spec.capacity.storage,ACCESS:.spec.accessModes,CLASS:.spec.storageClassName

# 3. 查看所有PVC状态
kubectl get pvc --all-namespaces
kubectl get pvc -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,STATUS:.status.phase,CAPACITY:.spec.resources.requests.storage,ACCESS:.spec.accessModes

# 4. 查看CSI驱动状态
kubectl get csidriver
kubectl get csinode

# 5. 查看VolumeAttachment状态
kubectl get volumeattachment
```

### 存储使用率监控

```bash
# 查看PVC使用详情
kubectl get pvc -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,USAGE:.status.capacity.storage,REQUEST:.spec.resources.requests.storage

# 查看节点存储分配
kubectl describe nodes | grep -A 5 "Allocated resources" | grep ephemeral-storage
```

---

<!-- chunk: PVC创建与管理 -->
## PVC创建与管理

### 标准PVC创建模板

```yaml
# 标准生产环境PVC模板
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data-pvc
  namespace: production
  annotations:
    # 添加描述信息便于管理
    description: "应用数据存储卷"
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: fast-ssd-pl2
  resources:
    requests:
      storage: 100Gi
  # 可选：指定特定PV
  # selector:
  #   matchLabels:
  #     type: ssd
```

### PVC批量创建脚本

```bash
#!/bin/bash
# batch-create-pvc.sh

NAMESPACE="production"
STORAGE_CLASS="fast-ssd-pl2"
BASE_NAME="app-data"
COUNT=10
SIZE="100Gi"

for i in $(seq 1 $COUNT); do
  PVC_NAME="${BASE_NAME}-${i}"
  
  cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: $PVC_NAME
  namespace: $NAMESPACE
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: $STORAGE_CLASS
  resources:
    requests:
      storage: $SIZE
EOF
  
  echo "Created PVC: $PVC_NAME"
done
```

### PVC状态管理

```bash
# 1. 查看Pending状态的PVC及其原因
kubectl get pvc --all-namespaces --field-selector=status.phase=Pending -o wide

# 2. 查看PVC详细事件信息
kubectl describe pvc <pvc-name> -n <namespace> | grep -A 20 "Events:"

# 3. 强制删除卡住的PVC
kubectl patch pvc <pvc-name> -p '{"metadata":{"finalizers":null}}' -n <namespace>
kubectl delete pvc <pvc-name> -n <namespace> --force --grace-period=0
```

---

<!-- chunk: 存储扩容操作 -->
## 存储扩容操作

### 在线扩容前提检查

```bash
# 1. 检查StorageClass是否支持扩容
kubectl get sc <storage-class-name> -o jsonpath='{.allowVolumeExpansion}'

# 2. 检查PVC当前状态
kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.status.phase}'

# 3. 检查PV是否支持扩容
kubectl get pv <pv-name> -o jsonpath='{.spec.csi.driver}'
```

### 执行扩容操作

```bash
# 方法1: 直接编辑PVC
kubectl edit pvc <pvc-name> -n <namespace>
# 修改 spec.resources.requests.storage 字段

# 方法2: 使用patch命令
kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 方法3: 通过YAML文件更新
cat <<EOF | kubectl apply -f -
apiVersion: vvc
kind: PersistentVolumeClaim
metadata:
  name: <pvc-name>
  namespace: <namespace>
spec:
  resources:
    requests:
      storage: 200Gi
EOF
```

### 扩容验证步骤

```bash
# 1. 监控扩容过程
kubectl get pvc <pvc-name> -n <namespace> -w

# 2. 检查扩容状态变化
# Resizing -> FileSystemResizePending -> Bound

# 3. 验证文件系统大小（需要进入Pod）
kubectl exec -it <pod-name> -n <namespace> -- df -h | grep <mount-path>

# 4. 某些情况下需要重启Pod完成文件系统扩容
kubectl rollout restart deployment/<deployment-name> -n <namespace>
```

---

<!-- chunk: 存储备份与恢复 -->
## 存储备份与恢复

### 快照备份操作

```yaml
# 1. 创建VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: default-snapshot-class
driver: diskplugin.csi.alibabacloud.com
deletionPolicy: Delete
parameters:
  # 阿里云特定参数
  # instantAccess: "true"
  # retentionDays: "7"

---
# 2. 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: app-data-backup-$(date +%Y%m%d)
  namespace: production
spec:
  volumeSnapshotClassName: default-snapshot-class
  source:
    persistentVolumeClaimName: app-data-pvc
```

### 备份管理脚本

```bash
#!/bin/bash
# backup-management.sh

NAMESPACE="production"
PVC_NAME="app-data-pvc"
BACKUP_PREFIX="app-backup"

# 创建快照
create_snapshot() {
  TIMESTAMP=$(date +%Y%m%d-%H%M%S)
  SNAPSHOT_NAME="${BACKUP_PREFIX}-${TIMESTAMP}"
  
  cat <<EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: $SNAPSHOT_NAME
  namespace: $NAMESPACE
spec:
  volumeSnapshotClassName: default-snapshot-class
  source:
    persistentVolumeClaimName: $PVC_NAME
EOF
  
  echo "Created snapshot: $SNAPSHOT_NAME"
}

# 清理旧快照（保留最近7天）
cleanup_old_snapshots() {
  kubectl get volumesnapshot -n $NAMESPACE -o json | \
    jq -r '.items[] | select(.metadata.creationTimestamp < "'$(date -d '7 days ago' --iso-8601)'") | .metadata.name' | \
    xargs -I {} kubectl delete volumesnapshot {} -n $NAMESPACE
}

# 列出所有快照
list_snapshots() {
  kubectl get volumesnapshot -n $NAMESPACE
}

case "$1" in
  create)
    create_snapshot
    ;;
  cleanup)
    cleanup_old_snapshots
    ;;
  list)
    list_snapshots
    ;;
  *)
    echo "Usage: $0 {create|cleanup|list}"
    exit 1
    ;;
esac
```

### 从快照恢复数据

```yaml
# 从快照创建新的PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data-restored
  namespace: production
spec:
  storageClassName: fast-ssd-pl2
  dataSource:
    name: app-backup-20260204-120000
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

---

<!-- chunk: CSI驱动运维 -->
## CSI驱动运维

### CSI组件状态检查

```bash
# 1. 检查CSI驱动注册状态
kubectl get csidriver

# 2. 检查节点上的CSI插件
kubectl get csinode

# 3. 检查CSI控制器Pod状态
kubectl get pods -n kube-system | grep csi

# 4. 检查CSI节点插件Pod状态
kubectl get daemonset -n kube-system | grep csi
```

### CSI日志查看

```bash
# 1. 查看CSI控制器日志
kubectl logs -n kube-system -l app=csi-controller -c csi-provisioner

# 2. 查看CSI节点插件日志
kubectl logs -n kube-system -l app=csi-node -c csi-driver

# 3. 查看特定节点的CSI日志
NODE_NAME="worker-node-1"
kubectl logs -n kube-system ds/csi-node -c csi-driver --tail=100 -n kube-system --selector kubernetes.io/hostname=$NODE_NAME
```

### CSI驱动升级流程

```bash
# 1. 检查当前版本
kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver -o jsonpath='{.items[*].spec.containers[*].image}'

# 2. 备份当前配置
kubectl get csidriver -o yaml > csi-driver-backup.yaml

# 3. 执行升级（具体步骤取决于CSI驱动提供商）
# 通常通过Helm或apply新版本YAML

# 4. 验证升级结果
kubectl get csidriver
kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver
```

---

<!-- chunk: 存储性能调优 -->
## 存储性能调优

### 性能监控指标收集

```bash
# 1. 收集I/O统计信息
kubectl exec -it <pod-name> -n <namespace> -- iostat -x 1 5

# 2. 检查文件系统性能
kubectl exec -it <pod-name> -n <namespace> -- dd if=/dev/zero of=/data/testfile bs=1M count=1000 oflag=direct

# 3. 监控网络存储延迟
kubectl exec -it <pod-name> -n <namespace> -- ping <storage-endpoint>
```

### 挂载参数优化

```yaml
# 优化的存储挂载配置
apiVersion: v1
kind: PersistentVolume
metadata:
  name: optimized-pv
spec:
  mountOptions:
    - noatime          # 不更新访问时间戳
    - nodiratime       # 目录不更新访问时间戳
    - discard          # 启用TRIM支持
    - barrier=0        # 禁用写屏障（谨慎使用）
    - data=ordered     # 数据写入顺序保证
  csi:
    driver: diskplugin.csi.alibabacloud.com
    fsType: ext4
    volumeAttributes:
      performanceLevel: "PL2"
```

### 性能测试脚本

```bash
#!/bin/bash
# storage-performance-test.sh

POD_NAME=$1
NAMESPACE=${2:-"default"}
TEST_FILE="/data/performance-test"

echo "开始存储性能测试..."
echo "测试Pod: $POD_NAME"
echo "命名空间: $NAMESPACE"
echo ""

# 1. 顺序写入测试
echo "=== 顺序写入性能测试 ==="
kubectl exec -it $POD_NAME -n $NAMESPACE -- \
  dd if=/dev/zero of=$TEST_FILE bs=1M count=1000 oflag=direct 2>&1

# 2. 顺序读取测试
echo ""
echo "=== 顺序读取性能测试 ==="
kubectl exec -it $POD_NAME -n $NAMESPACE -- \
  dd if=$TEST_FILE of=/dev/null bs=1M count=1000 iflag=direct 2>&1

# 3. 随机读写测试
echo ""
echo "=== 随机读写性能测试 ==="
kubectl exec -it $POD_NAME -n $NAMESPACE -- \
  fio --name=randtest --filename=$TEST_FILE --rw=randrw --bs=4k --size=100M --numjobs=4 --iodepth=32 --direct=1

# 4. 清理测试文件
kubectl exec -it $POD_NAME -n $NAMESPACE -- rm -f $TEST_FILE

echo ""
echo "性能测试完成"
```

---

<!-- chunk: 日常巡检脚本 -->
## 日常巡检脚本

```bash
#!/bin/bash
# daily-storage-inspection.sh

REPORT_FILE="/tmp/storage-inspection-$(date +%Y%m%d).log"
exec > >(tee -a "$REPORT_FILE") 2>&1

echo "=========================================="
echo "Kubernetes 存储系统日常巡检报告"
echo "检查时间: $(date)"
echo "=========================================="

# 1. 基础资源状态检查
echo ""
echo "1. 存储资源状态检查"
echo "--------------------"
echo "StorageClass数量: $(kubectl get sc 2>/dev/null | wc -l)"
echo "PV总数: $(kubectl get pv 2>/dev/null | wc -l)"
echo "PVC总数: $(kubectl get pvc --all-namespaces 2>/dev/null | wc -l)"

# 2. 异常状态检查
echo ""
echo "2. 异常状态检查"
echo "----------------"
PENDING_PVC=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Pending 2>/dev/null | wc -l)
if [ "$PENDING_PVC" -gt 0 ]; then
  echo "⚠️  发现 $PENDING_PVC 个Pending状态的PVC"
  kubectl get pvc --all-namespaces --field-selector=status.phase=Pending
fi

LOST_PVC=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Lost 2>/dev/null | wc -l)
if [ "$LOST_PVC" -gt 0 ]; then
  echo "❌ 发现 $LOST_PVC 个Lost状态的PVC"
  kubectl get pvc --all-namespaces --field-selector=status.phase=Lost
fi

FAILED_PV=$(kubectl get pv --field-selector=status.phase=Failed 2>/dev/null | wc -l)
if [ "$FAILED_PV" -gt 0 ]; then
  echo "❌ 发现 $FAILED_PV 个Failed状态的PV"
  kubectl get pv --field-selector=status.phase=Failed
fi

# 3. 高使用率检查
echo ""
echo "3. 存储使用率检查"
echo "------------------"
HIGH_USAGE_PVC=$(kubectl get pvc --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.status.capacity.storage}{"\t"}{.spec.resources.requests.storage}{"\n"}{end}' 2>/dev/null | awk '$3/$4 > 0.85')
if [ -n "$HIGH_USAGE_PVC" ]; then
  echo "⚠️  发现高使用率PVC (>85%)"
  echo -e "$HIGH_USAGE_PVC"
fi

# 4. CSI驱动状态检查
echo ""
echo "4. CSI驱动状态检查"
echo "-------------------"
CSI_PODS=$(kubectl get pods -n kube-system 2>/dev/null | grep -c "csi")
echo "CSI相关Pod数量: $CSI_PODS"

CSI_ERRORS=$(kubectl get pods -n kube-system 2>/dev/null | grep "csi" | grep -v "Running")
if [ -n "$CSI_ERRORS" ]; then
  echo "❌ 发现异常的CSI Pod"
  echo "$CSI_ERRORS"
fi

echo ""
echo "=========================================="
echo "巡检完成，请查看详细报告: $REPORT_FILE"
echo "=========================================="
```

---

<!-- chunk: 应急处理流程 -->
## 应急处理流程

### 存储故障应急响应

```bash
#!/bin/bash
# storage-emergency-response.sh

EMERGENCY_TYPE=$1

emergency_response() {
  case "$EMERGENCY_TYPE" in
    "pvc-pending")
      handle_pvc_pending
      ;;
    "mount-failure")
      handle_mount_failure
      ;;
    "csi-down")
      handle_csi_down
      ;;
    "data-loss")
      handle_data_loss
      ;;
    *)
      echo "未知应急类型: $EMERGENCY_TYPE"
      echo "支持的类型: pvc-pending, mount-failure, csi-down, data-loss"
      exit 1
      ;;
  esac
}

handle_pvc_pending() {
  echo "处理PVC Pending问题..."
  # 1. 检查StorageClass
  kubectl get sc
  # 2. 检查CSI驱动状态
  kubectl get pods -n kube-system | grep csi
  # 3. 检查资源配额
  kubectl get resourcequota --all-namespaces
}

handle_mount_failure() {
  echo "处理挂载失败问题..."
  # 1. 检查VolumeAttachment
  kubectl get volumeattachment
  # 2. 检查节点状态
  kubectl get nodes
  # 3. 检查Pod事件
  kubectl describe pod <pod-name> -n <namespace>
}

handle_csi_down() {
  echo "处理CSI驱动故障..."
  # 1. 重启CSI Pod
  kubectl delete pods -n kube-system -l app=csi-driver
  # 2. 检查节点插件
  kubectl get daemonset -n kube-system | grep csi
}

handle_data_loss() {
  echo "处理数据丢失问题..."
  # 1. 立即停止相关应用
  kubectl scale deployment <deployment-name> -n <namespace> --replicas=0
  # 2. 从备份恢复
  # 3. 验证数据完整性
}

emergency_response
```

### 故障排查检查清单

```markdown
<!-- chunk: 存储故障排查检查清单 -->
## 存储故障排查检查清单

### 🔍 初步诊断
- [ ] 检查集群整体状态：`kubectl get nodes`
- [ ] 检查存储相关Pod状态：`kubectl get pods -n kube-system | grep csi`
- [ ] 检查PVC/PV状态：`kubectl get pvc,pv --all-namespaces`

### 📊 详细检查
- [ ] 查看详细事件信息：`kubectl describe pvc <name>`
- [ ] 检查StorageClass配置：`kubectl get sc <name> -o yaml`
- [ ] 查看CSI驱动日志：`kubectl logs -n kube-system <csi-pod>`
- [ ] 检查节点存储状态：`kubectl describe node <node-name>`

### ⚡ 应急措施
- [ ] 隔离故障应用：暂停相关Deployment
- [ ] 数据保护：立即创建快照备份
- [ ] 降级处理：切换到备用存储方案
- [ ] 通知相关人员：发送故障告警

### 📈 根因分析
- [ ] 检查云服务商状态面板
- [ ] 分析监控指标异常时间点
- [ ] 审查近期变更记录
- [ ] 复现问题场景
```

---
**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-04-storage-data/MOC.md|domain-04-storage-data MOC]]
- [[domain-04-storage-data/README.md|Storage Domain 存储领域知识库]]
- [[domain-04-storage-data/00-open-source-projects-index.md|Domain-6 存储 — 开源项目索引]]
- [[domain-04-storage-data/01-storage-architecture-overview.md|存储架构概览与核心组件]]
- [[domain-04-storage-data/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]]
- [[domain-04-storage-data/03-pvc-patterns-practices.md|03 - PVC使用模式与最佳实践]]
- [[domain-04-storage-data/04-storageclass-dynamic-provisioning.md|StorageClass 动态供给与多租户管理]]
- [[domain-04-storage-data/05-csi-drivers-integration.md|05 - CSI驱动集成与运维管理]]
- [[domain-04-storage-data/06-storage-fundamental-concepts.md|06 - 存储基础概念详解]]
- [[domain-04-storage-data/08-storage-performance-tuning.md|08 - 存储性能调优与优化策略]]
- [[domain-04-storage-data/09-pv-pvc-troubleshooting.md|09 - PV/PVC故障排查与解决方案]]
- [[domain-04-storage-data/10-storage-backup-disaster-recovery.md|10 - 存储备份与灾难恢复]]

## See Also

- [[domain-04-storage-data/05-csi-drivers-integration.md|05-csi-drivers-integration]]
- [[domain-04-storage-data/06-storage-fundamental-concepts.md|06-storage-fundamental-concepts]]
- [[domain-04-storage-data/08-storage-performance-tuning.md|08-storage-performance-tuning]]
- [[domain-04-storage-data/09-pv-pvc-troubleshooting.md|09-pv-pvc-troubleshooting]]

## Related

- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
