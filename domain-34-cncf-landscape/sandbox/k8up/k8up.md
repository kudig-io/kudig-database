# K8up

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://k8up.io/ |
| **GitHub** | https://github.com/k8up-io/k8up |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

K8up 是一个 Kubernetes 备份 Operator，基于 Restic 实现 PersistentVolume 的自动化备份。它通过 CRD 声明式管理备份、恢复、归档和清理策略，支持将备份存储到 S3、GCS、Azure Blob 等对象存储后端。

### 核心特性

- **声明式备份**: 通过 CRD 定义备份计划，全自动执行
- **Restic 后端**: 使用 Restic 进行去重、加密的增量备份
- **多存储支持**: S3, GCS, Azure Blob, Backblaze B2, SFTP 等
- **Pre/Post 备份命令**: 支持在备份前后执行自定义命令（如数据库转储）
- **定时调度**: Cron 表达式定义备份频率
- **自动清理 (Prune)**: 基于保留策略自动清理过期备份
- **监控集成**: Prometheus 指标导出

---

## 快速开始

### 安装

```bash
helm repo add k8up-io https://k8up-io.github.io/k8up
helm install k8up k8up-io/k8up \
  --namespace k8up-system \
  --create-namespace
```

### 配置备份仓库

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: backup-repo
type: Opaque
stringData:
  password: "restic-repo-password"
---
apiVersion: v1
kind: Secret
metadata:
  name: backup-s3-credentials
type: Opaque
stringData:
  username: "AWS_ACCESS_KEY_ID"
  password: "AWS_SECRET_ACCESS_KEY"
```

### 创建定时备份

```yaml
apiVersion: k8up.io/v1
kind: Schedule
metadata:
  name: daily-backup
spec:
  backend:
    repoPasswordSecretRef:
      name: backup-repo
      key: password
    s3:
      endpoint: https://s3.amazonaws.com
      bucket: my-k8s-backups
      accessKeyIDSecretRef:
        name: backup-s3-credentials
        key: username
      secretAccessKeySecretRef:
        name: backup-s3-credentials
        key: password
  backup:
    schedule: "0 2 * * *"  # 每天凌晨 2 点
    failedJobsHistoryLimit: 5
    successfulJobsHistoryLimit: 3
  check:
    schedule: "0 4 * * 0"  # 每周日凌晨 4 点检查完整性
  prune:
    schedule: "0 5 * * 0"  # 每周日凌晨 5 点清理
    retention:
      keepDaily: 7
      keepWeekly: 4
      keepMonthly: 12
```

### Pre/Post 备份钩子

```yaml
# Pod 注解定义备份命令
apiVersion: v1
kind: Pod
metadata:
  name: postgres
  annotations:
    k8up.io/backupcommand: "pg_dumpall -U postgres"
    k8up.io/file-extension: ".sql"
spec:
  containers:
    - name: postgres
      image: postgres:16
      volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
```

### 恢复

```yaml
apiVersion: k8up.io/v1
kind: Restore
metadata:
  name: restore-data
spec:
  backend:
    repoPasswordSecretRef:
      name: backup-repo
      key: password
    s3:
      endpoint: https://s3.amazonaws.com
      bucket: my-k8s-backups
      accessKeyIDSecretRef:
        name: backup-s3-credentials
        key: username
      secretAccessKeySecretRef:
        name: backup-s3-credentials
        key: password
  restoreMethod:
    folder:
      claimName: restored-data-pvc
  snapshot: "latest"  # 或指定 snapshot ID
```

---

## 监控

| 指标 | 说明 |
|:---|:---|
| `k8up_backup_success_total` | 成功备份次数 |
| `k8up_backup_failure_total` | 失败备份次数 |
| `k8up_backup_duration_seconds` | 备份耗时 |
| `k8up_restore_success_total` | 成功恢复次数 |

---

## 最佳实践

1. **定期测试恢复**: 不要只测试备份，定期验证恢复流程
2. **数据库钩子**: 使用 backupcommand 注解执行数据库一致性转储
3. **保留策略**: 根据合规要求配置合理的保留策略
4. **加密**: 使用强密码保护 Restic 仓库，密码存储在 Kubernetes Secret 中
5. **完整性检查**: 定期运行 Check 验证备份数据完整性
6. **监控告警**: 基于 `k8up_backup_failure_total` 配置告警

---

## 参考资源

- [K8up 官方文档](https://k8up.io/k8up/)
- [K8up GitHub](https://github.com/k8up-io/k8up)
- [Restic 文档](https://restic.net/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
