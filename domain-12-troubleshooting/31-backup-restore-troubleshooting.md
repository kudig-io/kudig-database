# 31 - 备份恢复故障排查 (Backup and Restore Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Velero Backup](https://velero.io/docs/), [etcd Backup](https://etcd.io/docs/)

---

## 1. 备份恢复故障诊断总览 (Backup and Restore Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **备份失败** | BackupPhase失败 | 数据保护缺失 | P0 - 紧急 |
| **恢复失败** | RestorePhase失败 | 灾难恢复失败 | P0 - 紧急 |
| **备份数据损坏** | 校验和不匹配 | 数据完整性受损 | P0 - 紧急 |
| **备份过期** | 保留策略问题 | 历史数据丢失 | P1 - 高 |
| **存储空间不足** | 备份无法写入 | 持续保护中断 | P1 - 高 |

### 1.2 备份恢复架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  备份恢复故障诊断架构                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       数据源层                                       │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   etcd数据库 │   │   PV存储     │   │   配置资源   │              │  │
│  │  │   (Control) │   │ (Persistent)│   │ (Resources) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   备份代理   │   │   快照管理   │   │   增量同步   │                   │
│  │ (Velero)    │   │ (etcdbrctl) │   │ (Restic)    │                   │
│  │   控制器     │   │   工具      │   │   备份      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      备份编排层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     BackupController                          │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   调度管理   │  │   策略执行   │  │   状态跟踪   │           │  │  │
│  │  │  │ (Schedule)  │  │ (Policy)    │  │  (Status)   │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   对象存储   │   │   块存储     │   │   文件系统   │                   │
│  │ (S3/MinIO)  │   │ (EBS/Disk)  │   │ (NFS/Local) │                   │
│  │   存储备份   │   │   快照      │   │   存储备份   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      恢复执行层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     RestoreController                         │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   数据提取   │  │   转换处理   │  │   应用恢复   │           │  │  │
│  │  │  │ (Extract)   │  │ (Transform) │  │  (Apply)    │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 备份系统状态检查 (Backup System Status Check)

### 2.1 Velero备份组件验证

```bash
# ========== 1. Velero组件状态 ==========
# 检查Velero部署状态
kubectl get deployment -n velero

# 查看Velero Pod状态
kubectl get pods -n velero -l component=velero

# 验证Velero版本
kubectl exec -n velero deploy/velero -- velero version

# 检查备份存储位置
kubectl get backupstoragelocation -n velero

# 查看卷快照位置
kubectl get volumesnapshotlocation -n velero

# ========== 2. 备份配置检查 ==========
# 检查备份策略
kubectl get schedules -n velero

# 查看备份配置详情
kubectl describe backupstoragelocation default -n velero

# 验证存储凭据
kubectl get secret -n velero cloud-credentials

# 检查Restic守护进程集
kubectl get daemonset -n velero restic

# ========== 3. 存储连接验证 ==========
# 测试对象存储连接
kubectl exec -n velero deploy/velero -- velero backup-location get

# 验证存储桶访问
BUCKET_NAME=$(kubectl get backupstoragelocation default -n velero -o jsonpath='{.spec.objectStorage.bucket}')
kubectl exec -n velero deploy/velero -- aws s3 ls s3://$BUCKET_NAME

# 检查存储使用情况
kubectl exec -n velero deploy/velero -- aws s3 ls s3://$BUCKET_NAME --recursive | wc -l
```

### 2.2 etcd备份状态检查

```bash
# ========== etcd备份组件状态 ==========
# 检查etcd备份Pod状态
kubectl get pods -n kube-system | grep etcd-backup

# 查看etcd备份配置
kubectl get configmap -n kube-system etcd-backup-config

# 验证备份脚本
kubectl get configmap -n kube-system etcd-backup-script -o yaml

# 检查备份CronJob
kubectl get cronjob -n kube-system | grep etcd

# ========== 备份文件验证 ==========
# 检查本地备份文件
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- ls -la /var/lib/etcd-backup/

# 验证备份文件完整性
kubectl exec -n kube-system $ETCD_POD -- sh -c "
cd /var/lib/etcd-backup/
for file in *.db; do
    echo \"Checking \$file\"
    etcdctl --data-dir=/var/lib/etcd snapshot status \$file
done
"

# 检查备份压缩文件
kubectl exec -n kube-system $ETCD_POD -- find /var/lib/etcd-backup/ -name "*.tar.gz" -exec ls -lh {} \;
```

---

## 3. 备份失败问题排查 (Backup Failure Troubleshooting)

### 3.1 备份执行失败分析

```bash
# ========== 1. 备份状态检查 ==========
# 查看失败的备份
kubectl get backups -n velero --field-selector=status.phase=Failed

# 分析备份失败原因
kubectl describe backup <backup-name> -n velero

# 查看备份详细日志
kubectl logs -n velero -l component=velero --tail=100 | grep -A20 "backup.*error"

# 检查特定备份的日志
kubectl logs -n velero deploy/velero --since=1h | grep <backup-name>

# ========== 2. 资源备份问题 ==========
# 检查API资源访问权限
kubectl auth can-i list pods --all-namespaces
kubectl auth can-i list services --all-namespaces

# 验证自定义资源定义备份
kubectl get crds | head -10
kubectl get <crd-name> --all-namespaces

# 检查资源大小限制
kubectl api-resources --verbs=list --namespaced -o name | xargs -I {} kubectl get {} --all-namespaces --no-headers 2>/dev/null | wc -l

# ========== 3. 存储相关问题 ==========
# 检查存储空间
kubectl exec -n velero deploy/velero -- df -h /

# 验证存储桶权限
kubectl exec -n velero deploy/velero -- aws s3api head-bucket --bucket $BUCKET_NAME

# 检查网络连接
kubectl exec -n velero deploy/velero -- nc -zv s3.amazonaws.com 443

# 测试上传功能
kubectl exec -n velero deploy/velero -- sh -c "
echo 'test data' > /tmp/test-file
aws s3 cp /tmp/test-file s3://$BUCKET_NAME/test-upload
"
```

### 3.2 卷备份问题诊断

```bash
# ========== Restic备份状态 ==========
# 检查Restic守护进程状态
kubectl get pods -n velero -l name=restic

# 查看Restic仓库状态
kubectl exec -n velero deploy/velero -- velero restic repo get

# 验证Restic密码
kubectl get secret -n velero restic-credentials

# 检查卷备份注解
kubectl get pods -A -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.annotations."backup\.velero\.io/backup-volumes"
}{
        "\n"
}{
    end
}' | grep -v '<none>'

# ========== 卷备份失败分析 ==========
# 查看Restic备份日志
kubectl logs -n velero -l name=restic --tail=100 | grep -i error

# 检查Pod卷挂载状态
kubectl describe pod <pod-name> -n <namespace> | grep -A10 "Volumes:"

# 验证PVC状态
kubectl get pvc -n <namespace> <pvc-name> -o jsonpath='{
    .status.phase
}{
        "\t"
}{
        .status.capacity.storage
}'

# 测试卷读写权限
kubectl exec -n <namespace> <pod-name> -- sh -c "
dd if=/dev/urandom of=/data/test-file bs=1M count=10
ls -lh /data/test-file
"
```

---

## 4. 恢复失败问题排查 (Restore Failure Troubleshooting)

### 4.1 恢复执行失败分析

```bash
# ========== 1. 恢复状态检查 ==========
# 查看失败的恢复
kubectl get restores -n velero --field-selector=status.phase=Failed

# 分析恢复失败原因
kubectl describe restore <restore-name> -n velero

# 查看恢复详细日志
kubectl logs -n velero deploy/velero --since=1h | grep -A20 "restore.*error"

# 检查资源冲突
kubectl get restores -n velero -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.warnings
}{
        "\t"
}{
        .status.errors
}{
        "\n"
}{
    end
}'

# ========== 2. 命名空间和资源问题 ==========
# 检查目标命名空间状态
kubectl get namespace <target-namespace>

# 验证资源配额
kubectl describe resourcequota -n <target-namespace>

# 检查网络策略影响
kubectl get networkpolicy -n <target-namespace>

# 验证RBAC权限
kubectl auth can-i create pods -n <target-namespace>
kubectl auth can-i create services -n <target-namespace>

# ========== 3. 依赖关系问题 ==========
# 检查服务账户存在性
kubectl get serviceaccount -n <target-namespace>

# 验证ConfigMap和Secret
kubectl get configmap -n <target-namespace>
kubectl get secret -n <target-namespace>

# 检查存储类可用性
kubectl get storageclass
kubectl get storageclass | grep "(default)"
```

### 4.2 数据一致性验证

```bash
# ========== 恢复数据校验 ==========
# 验证恢复的应用状态
kubectl get deployments -n <target-namespace>
kubectl get pods -n <target-namespace> --field-selector=status.phase=Running

# 检查服务端点
kubectl get endpoints -n <target-namespace>
kubectl describe service <service-name> -n <target-namespace>

# 验证数据完整性
kubectl exec -n <target-namespace> <pod-name> -- sh -c "
# 执行应用特定的数据校验命令
# 例如：数据库校验、文件校验等
"

# ========== 配置对比验证 ==========
# 对比备份前后的资源配置
BACKUP_RESOURCES=$(kubectl get resources --from-backup <backup-name> -o yaml)
CURRENT_RESOURCES=$(kubectl get resources -n <target-namespace> -o yaml)

# 使用diff工具比较
diff <(echo "$BACKUP_RESOURCES") <(echo "$CURRENT_RESOURCES")

# 检查关键配置项
kubectl get configmap -n <target-namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .data
}{
        "\n"
}{
    end
}'
```

---

## 5. 备份数据完整性验证 (Backup Data Integrity Verification)

### 5.1 备份文件校验

```bash
# ========== 1. 校验和验证 ==========
# 生成备份文件校验和
kubectl exec -n velero deploy/velero -- sh -c "
cd /backups
for file in *.tar.gz; do
    md5sum \$file > \${file}.md5
done
"

# 验证校验和
kubectl exec -n velero deploy/velero -- sh -c "
cd /backups
for file in *.tar.gz; do
    md5sum -c \${file}.md5
done
"

# ========== 2. 备份内容验证 ==========
# 列出备份内容
kubectl exec -n velero deploy/velero -- velero backup describe <backup-name> --details

# 验证关键资源包含
kubectl exec -n velero deploy/velero -- velero backup describe <backup-name> | grep -E "(Namespaces|Resources|PVs)"

# 检查备份大小
kubectl get backup <backup-name> -n velero -o jsonpath='{
    .status.completionTimestamp
}{
        "\t"
}{
        .status.phase
}{
        "\t"
}{
        .status.progress.itemsBackedUp
}'

# ========== 3. 跨区域复制验证 ==========
# 检查异地备份状态
kubectl get backupstoragelocation -n velero -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.provider
}{
        "\t"
}{
        .status.lastValidationTime
}{
        "\n"
}{
    end
}'

# 验证复制延迟
kubectl exec -n velero deploy/velero -- aws s3 ls s3://$BUCKET_NAME/ --recursive | tail -5
```

### 5.2 定期验证策略

```bash
# ========== 自动化验证脚本 ==========
cat <<'EOF' > backup-validation-script.sh
#!/bin/bash

BACKUP_NAME=$1
VALIDATION_NAMESPACE="backup-validation"

if [ -z "$BACKUP_NAME" ]; then
    echo "Usage: $0 <backup-name>"
    exit 1
fi

echo "=== Backup Validation for: $BACKUP_NAME ==="

# 1. 创建验证命名空间
kubectl create namespace $VALIDATION_NAMESPACE 2>/dev/null || echo "Namespace already exists"

# 2. 执行恢复到验证环境
kubectl exec -n velero deploy/velero -- velero restore create validation-restore-$(date +%Y%m%d-%H%M%S) \
    --from-backup $BACKUP_NAME \
    --namespace-mappings default:$VALIDATION_NAMESPACE \
    --exclude-resources events,replicasets,pods \
    --wait

# 3. 验证恢复状态
RESTORE_NAME=$(kubectl get restores -n velero --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1:].metadata.name}')
kubectl wait -n velero --for=condition=complete restore/$RESTORE_NAME --timeout=300s

# 4. 检查恢复结果
echo "=== Recovery Results ==="
kubectl get deployments -n $VALIDATION_NAMESPACE
kubectl get services -n $VALIDATION_NAMESPACE
kubectl get pods -n $VALIDATION_NAMESPACE --no-headers | wc -l

# 5. 应用健康检查
echo "=== Application Health Check ==="
HEALTHY_PODS=$(kubectl get pods -n $VALIDATION_NAMESPACE --field-selector=status.phase=Running --no-headers | wc -l)
TOTAL_PODS=$(kubectl get pods -n $VALIDATION_NAMESPACE --no-headers | wc -l)

if [ $HEALTHY_PODS -eq $TOTAL_PODS ] && [ $TOTAL_PODS -gt 0 ]; then
    echo "✓ All pods are healthy ($HEALTHY_PODS/$TOTAL_PODS)"
    VALIDATION_RESULT="PASS"
else
    echo "✗ Some pods are unhealthy ($HEALTHY_PODS/$TOTAL_PODS)"
    VALIDATION_RESULT="FAIL"
fi

# 6. 清理验证环境
kubectl delete namespace $VALIDATION_NAMESPACE

# 7. 记录验证结果
echo "Validation Result: $VALIDATION_RESULT" | tee -a /var/log/backup-validation.log

echo "Validation completed for backup: $BACKUP_NAME"
EOF

chmod +x backup-validation-script.sh

# ========== 定期验证CronJob ==========
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-validator
  namespace: velero
spec:
  schedule: "0 2 * * 0"  # 每周日凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: validator
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              # 获取最新的备份
              LATEST_BACKUP=\$(kubectl get backups -n velero --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1:].metadata.name}')
              
              # 执行验证脚本
              /scripts/backup-validation-script.sh \$LATEST_BACKUP
            volumeMounts:
            - name: scripts
              mountPath: /scripts
            - name: logs
              mountPath: /var/log
          volumes:
          - name: scripts
            configMap:
              name: backup-validation-scripts
          - name: logs
            emptyDir: {}
          restartPolicy: OnFailure
EOF

# ========== 验证脚本ConfigMap ==========
kubectl create configmap backup-validation-scripts -n velero \
    --from-file=backup-validation-script.sh=./backup-validation-script.sh
```

---

## 6. 灾难恢复演练 (Disaster Recovery Drills)

### 6.1 恢复演练流程

```bash
# ========== 完整恢复演练脚本 ==========
cat <<'EOF' > disaster-recovery-drill.sh
#!/bin/bash

DRILL_NAME="full-dr-$(date +%Y%m%d-%H%M%S)"
TARGET_NAMESPACE=${1:-dr-test}
BACKUP_NAME=${2:-latest}

echo "=== Disaster Recovery Drill: $DRILL_NAME ==="
echo "Target namespace: $TARGET_NAMESPACE"
echo "Source backup: $BACKUP_NAME"

# 1. 准备阶段
echo "1. Preparation Phase"
kubectl create namespace $TARGET_NAMESPACE 2>/dev/null || echo "Namespace already exists"

# 记录开始时间
START_TIME=$(date +%s)

# 2. 执行恢复
echo "2. Recovery Execution"
if [ "$BACKUP_NAME" = "latest" ]; then
    BACKUP_NAME=$(kubectl get backups -n velero --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1:].metadata.name}')
fi

kubectl exec -n velero deploy/velero -- velero restore create $DRILL_NAME \
    --from-backup $BACKUP_NAME \
    --namespace-mappings default:$TARGET_NAMESPACE \
    --wait

# 3. 验证恢复
echo "3. Recovery Validation"
kubectl wait -n velero --for=condition=complete restore/$DRILL_NAME --timeout=600s

# 检查恢复状态
RESTORE_STATUS=$(kubectl get restore $DRILL_NAME -n velero -o jsonpath='{.status.phase}')
echo "Restore status: $RESTORE_STATUS"

# 4. 应用验证
echo "4. Application Validation"
DEPLOYMENTS=$(kubectl get deployments -n $TARGET_NAMESPACE --no-headers | wc -l)
PODS=$(kubectl get pods -n $TARGET_NAMESPACE --no-headers | wc -l)
RUNNING_PODS=$(kubectl get pods -n $TARGET_NAMESPACE --field-selector=status.phase=Running --no-headers | wc -l)

echo "Deployments: $DEPLOYMENTS"
echo "Total Pods: $PODS"
echo "Running Pods: $RUNNING_PODS"

# 5. 功能测试
echo "5. Functional Testing"
# 这里可以添加特定应用的功能测试脚本
for svc in $(kubectl get svc -n $TARGET_NAMESPACE -o jsonpath='{.items[*].metadata.name}'); do
    echo "Testing service: $svc"
    # 添加服务连通性测试
done

# 6. 性能基准测试
echo "6. Performance Baseline"
# 记录恢复时间
END_TIME=$(date +%s)
RECOVERY_TIME=$((END_TIME - START_TIME))
echo "Total recovery time: ${RECOVERY_TIME} seconds"

# 7. 清理阶段
echo "7. Cleanup Phase"
read -p "Clean up test namespace $TARGET_NAMESPACE? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    kubectl delete namespace $TARGET_NAMESPACE
    echo "Test namespace cleaned up"
else
    echo "Test namespace preserved for manual inspection"
fi

# 8. 生成报告
echo "8. Generating Report"
cat > /tmp/dr-report-$DRILL_NAME.txt <<REPORT
Disaster Recovery Drill Report: $DRILL_NAME
=======================================
Start Time: $(date -d @$START_TIME)
End Time: $(date -d @$END_TIME)
Duration: ${RECOVERY_TIME} seconds

Backup Used: $BACKUP_NAME
Target Namespace: $TARGET_NAMESPACE

Results:
- Deployments Recovered: $DEPLOYMENTS
- Pods Recovered: $PODS
- Running Pods: $RUNNING_PODS
- Restore Status: $RESTORE_STATUS

Recommendations:
[Add recommendations based on drill results]

REPORT

echo "Report saved to /tmp/dr-report-$DRILL_NAME.txt"
cat /tmp/dr-report-$DRILL_NAME.txt
EOF

chmod +x disaster-recovery-drill.sh

# ========== 恢复时间目标(RTO)测试 ==========
cat <<'EOF' > rto-measurement.sh
#!/bin/bash

echo "=== RTO Measurement Test ==="

# 测试不同场景的恢复时间
SCENARIOS=(
    "single-namespace"
    "full-cluster"
    "critical-applications"
)

for scenario in "${SCENARIOS[@]}"; do
    echo "Testing scenario: $scenario"
    
    START_TIME=$(date +%s)
    
    case $scenario in
        "single-namespace")
            # 恢复单个命名空间
            kubectl exec -n velero deploy/velero -- velero restore create test-$scenario \
                --from-backup latest \
                --include-namespaces production \
                --wait
            ;;
        "full-cluster")
            # 全集群恢复（模拟）
            echo "Simulating full cluster recovery..."
            sleep 300  # 模拟5分钟恢复时间
            ;;
        "critical-applications")
            # 关键应用恢复
            kubectl exec -n velero deploy/velero -- velero restore create test-$scenario \
                --from-backup latest \
                --include-resources deployments,services,configmaps,secrets \
                --wait
            ;;
    esac
    
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo "Scenario '$scenario' completed in ${DURATION} seconds"
    echo "$scenario: $DURATION" >> /tmp/rto-results.txt
done

echo "=== RTO Results ==="
cat /tmp/rto-results.txt
EOF

chmod +x rto-measurement.sh
```

### 6.2 恢复点目标(RPO)验证

```bash
# ========== RPO验证工具 ==========
cat <<'EOF' > rpo-validator.sh
#!/bin/bash

echo "=== RPO Validation Tool ==="

# 获取备份时间线
kubectl get backups -n velero -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.startTimestamp
}{
        "\t"
}{
        .status.completionTimestamp
}{
        "\t"
}{
        .status.phase
}{
        "\n"
}{
    end
}' | sort -k2 > /tmp/backup-timeline.txt

echo "=== Backup Timeline ==="
cat /tmp/backup-timeline.txt

# 计算备份间隔
echo "=== Backup Intervals ==="
awk 'NR>1 {
    prev_time = $2
    curr_time = $2
    interval = (curr_time - prev_time) / 60  # 转换为分钟
    printf "Interval %d: %.2f minutes\n", NR-1, interval
}' /tmp/backup-timeline.txt

# 验证RPO目标
TARGET_RPO_MINUTES=${1:-60}  # 默认1小时RPO
echo "Target RPO: ${TARGET_RPO_MINUTES} minutes"

awk -v target=$TARGET_RPO_MINUTES 'NR>1 {
    prev_time = $2
    curr_time = $2
    interval = (curr_time - prev_time) / 60
    if (interval > target) {
        printf "⚠️  RPO violation at backup %s: %.2f minutes (> %d)\n", $1, interval, target
    } else {
        printf "✓ Backup %s meets RPO: %.2f minutes\n", $1, interval
    }
}' /tmp/backup-timeline.txt

# 清理临时文件
rm /tmp/backup-timeline.txt
EOF

chmod +x rpo-validator.sh

# ========== 数据一致性检查 ==========
cat <<'EOF' > data-consistency-checker.sh
#!/bin/bash

BACKUP_NAME=$1
TEST_NAMESPACE="consistency-test"

if [ -z "$BACKUP_NAME" ]; then
    echo "Usage: $0 <backup-name>"
    exit 1
fi

echo "=== Data Consistency Check for: $BACKUP_NAME ==="

# 1. 创建测试数据
echo "1. Creating test data"
kubectl create namespace $TEST_NAMESPACE
kubectl create configmap test-data -n $TEST_NAMESPACE \
    --from-literal=data="$(date):$(uuidgen)"

# 2. 执行备份
echo "2. Creating backup with test data"
kubectl exec -n velero deploy/velero -- velero backup create consistency-check-$BACKUP_NAME \
    --include-namespaces $TEST_NAMESPACE \
    --wait

# 3. 修改测试数据
echo "3. Modifying test data"
kubectl patch configmap test-data -n $TEST_NAMESPACE \
    -p "{\"data\":{\"data\":\"$(date):modified-$(uuidgen)\"}}"

# 4. 执行恢复
echo "4. Restoring from backup"
kubectl exec -n velero deploy/velero -- velero restore create consistency-restore-$BACKUP_NAME \
    --from-backup consistency-check-$BACKUP_NAME \
    --namespace-mappings $TEST_NAMESPACE:restored-$TEST_NAMESPACE \
    --wait

# 5. 验证数据一致性
echo "5. Verifying data consistency"
ORIGINAL_DATA=$(kubectl get configmap test-data -n $TEST_NAMESPACE -o jsonpath='{.data.data}')
RESTORED_DATA=$(kubectl get configmap test-data -n restored-$TEST_NAMESPACE -o jsonpath='{.data.data}')

echo "Original data: $ORIGINAL_DATA"
echo "Restored data: $RESTORED_DATA"

if [ "$ORIGINAL_DATA" = "$RESTORED_DATA" ]; then
    echo "✓ Data consistency verified"
    CONSISTENCY_RESULT="PASS"
else
    echo "✗ Data inconsistency detected"
    CONSISTENCY_RESULT="FAIL"
fi

# 6. 清理测试环境
kubectl delete namespace $TEST_NAMESPACE restored-$TEST_NAMESPACE

echo "Consistency check result: $CONSISTENCY_RESULT"
EOF

chmod +x data-consistency-checker.sh
```

---

## 7. 备份策略优化 (Backup Strategy Optimization)

### 7.1 智能备份策略

```bash
# ========== 分层备份策略 ==========
cat <<EOF > tiered-backup-strategy.yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: hourly-backup
  namespace: velero
spec:
  schedule: "0 * * * *"  # 每小时执行
  template:
    includedNamespaces:
    - production
    excludedResources:
    - events
    - nodes
    - endpoints
    snapshotVolumes: true
    ttl: "24h"  # 保留24小时
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  template:
    includedNamespaces:
    - production
    - staging
    excludedResources:
    - events
    - nodes
    snapshotVolumes: true
    ttl: "720h"  # 保留30天
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: weekly-backup
  namespace: velero
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨3点执行
  template:
    includedNamespaces:
    - "*"
    excludedNamespaces:
    - kube-system
    - velero
    snapshotVolumes: true
    ttl: "2160h"  # 保留90天
EOF

# ========== 差异备份配置 ==========
cat <<'EOF' > incremental-backup-config.sh
#!/bin/bash

echo "=== Incremental Backup Configuration ==="

# 创建增量备份策略
cat <<CONFIG | kubectl apply -f -
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: base-full-backup
  namespace: velero
spec:
  includedNamespaces:
  - production
  snapshotVolumes: true
  ttl: "168h"  # 保留一周
CONFIG

# 配置差异备份CronJob
cat <<CRONJOB | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: incremental-backup
  namespace: velero
spec:
  schedule: "*/30 * * * *"  # 每30分钟执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: velero/velero:v1.10.0
            command:
            - /velero
            - backup
            - create
            - incremental-$(date +%Y%m%d-%H%M%S)
            - --from-schedule=hourly-backup
            - --snapshot-volumes=true
            env:
            - name: VELERO_NAMESPACE
              value: velero
            - name: AWS_SHARED_CREDENTIALS_FILE
              value: /credentials/cloud
            volumeMounts:
            - name: cloud-credentials
              mountPath: /credentials
          volumes:
          - name: cloud-credentials
            secret:
              secretName: cloud-credentials
          restartPolicy: OnFailure
CRONJOB
EOF

chmod +x incremental-backup-config.sh
```

### 7.2 备份存储优化

```bash
# ========== 存储生命周期管理 ==========
cat <<'EOF' > storage-lifecycle-manager.sh
#!/bin/bash

echo "=== Storage Lifecycle Management ==="

# 清理过期备份
kubectl exec -n velero deploy/velero -- velero backup delete --older-than 720h --confirm

# 压缩旧备份
kubectl exec -n velero deploy/velero -- sh -c "
cd /backups
for backup in \$(find . -name '*.tar.gz' -mtime +30); do
    gzip -9 \$backup
done
"

# 验证存储使用
STORAGE_USAGE=$(kubectl exec -n velero deploy/velero -- df -h /backups | tail -1 | awk '{print $5}')
echo "Current storage usage: $STORAGE_USAGE"

# 存储容量规划
cat <<'PLANNER' > storage-planner.py
#!/usr/bin/env python3

import datetime
import sys

def calculate_storage_needs(daily_growth_gb=10, retention_days=90):
    """计算存储需求"""
    total_needed = daily_growth_gb * retention_days
    recommended_size = total_needed * 1.5  # 50%缓冲
    
    print(f"Daily growth: {daily_growth_gb} GB")
    print(f"Retention period: {retention_days} days")
    print(f"Total needed: {total_needed} GB")
    print(f"Recommended size: {recommended_size:.0f} GB")
    
    return recommended_size

if __name__ == "__main__":
    daily_growth = float(sys.argv[1]) if len(sys.argv) > 1 else 10
    retention = int(sys.argv[2]) if len(sys.argv) > 2 else 90
    
    calculate_storage_needs(daily_growth, retention)
PLANNER

python3 storage-planner.py 15 180  # 15GB/天增长，180天保留期
EOF

chmod +x storage-lifecycle-manager.sh

# ========== 跨区域备份策略 ==========
cat <<EOF > cross-region-backup.yaml
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: primary-region
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: velero-primary-backups
    prefix: backups
  config:
    region: us-west-2
---
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: secondary-region
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: velero-secondary-backups
    prefix: backups
  config:
    region: us-east-1
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cross-region-sync
  namespace: velero
spec:
  schedule: "0 4 * * *"  # 每天凌晨4点同步
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: sync
            image: amazon/aws-cli
            command:
            - /bin/bash
            - -c
            - |
              # 同步备份到备用区域
              aws s3 sync s3://velero-primary-backups/ s3://velero-secondary-backups/ \
                --exclude "*" \
                --include "*/latest/*" \
                --include "*/weekly/*"
            env:
            - name: AWS_DEFAULT_REGION
              value: us-west-2
          restartPolicy: OnFailure
EOF
```

---