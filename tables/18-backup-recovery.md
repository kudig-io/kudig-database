# 18 - 备份与恢复表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd](https://kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd/)

## 备份策略概览

| 备份类型 | 备份内容 | 工具 | RPO | RTO | 适用场景 |
|---------|---------|------|-----|-----|---------|
| **etcd快照** | 集群状态数据 | etcdctl | 分钟级 | 10-30分钟 | 集群级灾难恢复 |
| **资源导出** | YAML资源定义 | kubectl/Velero | 分钟级 | 依赖资源 | 资源迁移 |
| **PV备份** | 持久化数据 | Velero+CSI快照 | 小时级 | 依赖数据量 | 数据保护 |
| **应用级备份** | 应用数据 | 应用工具 | 应用定义 | 应用定义 | 数据库等 |
| **GitOps** | 配置代码 | Git | 实时 | 分钟级 | 配置恢复 |

## etcd备份

| 方法 | 命令 | 优点 | 缺点 | 版本要求 |
|-----|------|------|------|---------|
| **snapshot save** | `etcdctl snapshot save` | 官方支持，完整备份 | 需要停机验证 | etcd 3.x |
| **定时快照** | CronJob调用etcdctl | 自动化 | 需配置存储 | etcd 3.x |
| **云快照** | 云盘快照 | 块级一致性 | 云厂商依赖 | - |

```bash
# etcd备份脚本
#!/bin/bash
BACKUP_DIR=/backup/etcd
DATE=$(date +%Y%m%d_%H%M%S)
ENDPOINTS="https://127.0.0.1:2379"
CACERT="/etc/kubernetes/pki/etcd/ca.crt"
CERT="/etc/kubernetes/pki/etcd/healthcheck-client.crt"
KEY="/etc/kubernetes/pki/etcd/healthcheck-client.key"

# 创建快照
ETCDCTL_API=3 etcdctl \
  --endpoints=$ENDPOINTS \
  --cacert=$CACERT \
  --cert=$CERT \
  --key=$KEY \
  snapshot save $BACKUP_DIR/snapshot_$DATE.db

# 验证快照
ETCDCTL_API=3 etcdctl snapshot status $BACKUP_DIR/snapshot_$DATE.db --write-out=table

# 保留最近7天
find $BACKUP_DIR -name "snapshot_*.db" -mtime +7 -delete

# 上传到OSS(可选)
ossutil cp $BACKUP_DIR/snapshot_$DATE.db oss://bucket/etcd-backup/
```

## etcd恢复

```bash
# 停止apiserver
systemctl stop kube-apiserver

# 恢复etcd数据
ETCDCTL_API=3 etcdctl snapshot restore snapshot.db \
  --data-dir=/var/lib/etcd-new \
  --name=etcd-0 \
  --initial-cluster=etcd-0=https://10.0.0.1:2380 \
  --initial-cluster-token=etcd-cluster-1 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380

# 替换数据目录
mv /var/lib/etcd /var/lib/etcd-old
mv /var/lib/etcd-new /var/lib/etcd
chown -R etcd:etcd /var/lib/etcd

# 启动服务
systemctl start etcd
systemctl start kube-apiserver

# 验证
kubectl get nodes
kubectl get pods -A
```

## Velero备份方案

| 功能 | 说明 | 版本支持 |
|-----|------|---------|
| **资源备份** | K8S资源YAML | v1.0+ |
| **PV快照** | CSI快照集成 | v1.2+ |
| **调度备份** | 定时备份 | v1.0+ |
| **跨集群恢复** | 迁移场景 | v1.0+ |
| **Restic备份** | 文件级PV备份 | v1.5+ |
| **CSI快照** | 原生快照 | v1.4+ |

```yaml
# Velero安装(Helm)
helm repo add vmware-tanzu https://vmware-tanzu.github.io/helm-charts
helm install velero vmware-tanzu/velero \
  --namespace velero \
  --create-namespace \
  --set configuration.provider=alibabacloud \
  --set configuration.backupStorageLocation.bucket=velero-bucket \
  --set configuration.backupStorageLocation.config.region=cn-hangzhou \
  --set snapshotsEnabled=true \
  --set deployRestic=true
```

```bash
# Velero备份命令
# 备份命名空间
velero backup create ns-backup --include-namespaces=production

# 备份带PV
velero backup create full-backup --include-namespaces=production --snapshot-volumes

# 定时备份
velero schedule create daily-backup --schedule="0 2 * * *" --include-namespaces=production

# 查看备份
velero backup get
velero backup describe <backup-name>
velero backup logs <backup-name>

# 恢复
velero restore create --from-backup <backup-name>
velero restore create --from-backup <backup-name> --include-namespaces=production
```

## CSI快照备份

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: disk-snapshot
driver: diskplugin.csi.alibabacloud.com
deletionPolicy: Retain
parameters:
  forceDelete: "false"

---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: data-snapshot
spec:
  volumeSnapshotClassName: disk-snapshot
  source:
    persistentVolumeClaimName: data-pvc

---
# 从快照恢复PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc-restored
spec:
  storageClassName: alicloud-disk-essd
  dataSource:
    name: data-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

## 数据库备份策略

| 数据库 | 备份工具 | K8S集成方式 | 建议RPO |
|-------|---------|------------|--------|
| **MySQL** | mysqldump/xtrabackup | CronJob+PVC | 1小时 |
| **PostgreSQL** | pg_dump/pg_basebackup | CronJob+PVC | 1小时 |
| **MongoDB** | mongodump | CronJob+PVC | 1小时 |
| **Redis** | RDB/AOF | PV快照 | 实时(AOF) |
| **Elasticsearch** | Snapshot API | CronJob | 1小时 |

```yaml
# MySQL备份CronJob示例
apiVersion: batch/v1
kind: CronJob
metadata:
  name: mysql-backup
spec:
  schedule: "0 */6 * * *"  # 每6小时
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: mysql:8.0
            command:
            - /bin/bash
            - -c
            - |
              mysqldump -h mysql -u root -p$MYSQL_ROOT_PASSWORD --all-databases > /backup/dump_$(date +%Y%m%d_%H%M%S).sql
              # 保留7天
              find /backup -name "dump_*.sql" -mtime +7 -delete
            env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: password
            volumeMounts:
            - name: backup-volume
              mountPath: /backup
          restartPolicy: OnFailure
          volumes:
          - name: backup-volume
            persistentVolumeClaim:
              claimName: mysql-backup-pvc
```

## 灾难恢复计划

| 灾难级别 | 场景 | 恢复策略 | RTO目标 |
|---------|------|---------|--------|
| **Level 1** | Pod/Deployment失败 | K8S自动恢复 | 秒级 |
| **Level 2** | 节点故障 | Pod重调度 | 分钟级 |
| **Level 3** | 控制平面故障 | etcd恢复/HA切换 | 10-30分钟 |
| **Level 4** | 集群全损 | 从备份重建 | 1-4小时 |
| **Level 5** | 数据中心故障 | 跨地域恢复 | 1-24小时 |

## ACK备份方案

| 功能 | 服务 | 配置方式 |
|-----|------|---------|
| **集群备份** | ACK控制台 | 自动/手动 |
| **etcd备份** | 托管自动 | Pro版自动 |
| **应用备份** | Velero+OSS | 组件安装 |
| **云盘快照** | 快照服务 | 控制台/API |
| **跨地域备份** | OSS跨区复制 | OSS配置 |

## 恢复测试清单

| 测试项 | 频率 | 验证方法 | 负责人 |
|-------|------|---------|-------|
| **etcd恢复** | 季度 | 测试集群恢复 | SRE |
| **Velero恢复** | 月度 | 恢复到测试NS | SRE |
| **数据库恢复** | 月度 | 恢复到测试实例 | DBA |
| **全集群恢复** | 半年 | DR演练 | Team |
| **跨地域恢复** | 年度 | DR演练 | Team |

# 18 - 备份与恢复表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd](https://kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd/)

(保持原有内容基础上补充)

## 灾难恢复演练计划

### 演练场景清单

| 场景 | 频率 | 目标RTO | 验证项 | 负责人 |
|-----|------|--------|-------|-------|
| **单Pod故障** | 每周 | 30秒 | 自动重启 | 自动化 |
| **单节点故障** | 每月 | 5分钟 | Pod重调度 | SRE |
| **etcd恢复** | 季度 | 30分钟 | 集群状态完整 | SRE Lead |
| **整个命名空间恢复** | 季度 | 1小时 | 应用恢复 | 应用团队 |
| **跨地域切换** | 半年 | 4小时 | 全量切换 | 全员 |
| **全集群重建** | 年度 | 8小时 | 完整恢复 | 全员 |

### 备份验证自动化

```yaml
# 定期验证备份CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-validation
  namespace: backup
spec:
  schedule: "0 3 * * 1"  # 每周一凌晨3点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: validator
            image: backup-validator:latest
            env:
            - name: BACKUP_LOCATION
              value: "oss://backup-bucket/velero/"
            command:
            - /bin/bash
            - -c
            - |
              # 1. 检查最近备份时间
              LATEST=$(velero backup get -o json | jq -r '.items[0].metadata.creationTimestamp')
              AGE=$(($(date +%s) - $(date -d $LATEST +%s)))
              if [ $AGE -gt 86400 ]; then
                echo "ERROR: Latest backup is older than 24h"
                exit 1
              fi
              
              # 2. 验证备份完整性
              velero backup describe <latest-backup> --details | grep -q "Phase: Completed"
              
              # 3. 尝试恢复到测试命名空间
              velero restore create test-restore-$(date +%s) \
                --from-backup <latest-backup> \
                --namespace-mappings production:backup-test
              
              # 4. 验证恢复成功
              kubectl wait --for=condition=Ready pod -n backup-test --all --timeout=300s
              
              # 5. 清理测试命名空间
              kubectl delete namespace backup-test
              
              echo "Backup validation successful"
          restartPolicy: OnFailure
```

---

**备份原则**: 3-2-1规则(3份备份，2种介质，1份异地)，定期验证，自动化备份，文档化恢复流程

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)