---
title: '[2026-07-20] [P1] Velero 备份失败导致无法恢复'
summary: '[2026-07-20] [P1] Velero 备份失败导致无法恢复：09:10，运维人员在清理旧配置时误执行：'
category: case-study
tags:
- production
- incident
- reliability
- velero
- backup
- disaster-recovery
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-07-20'
severity: P1
mttr: 50min
status: resolved
last_updated: 2026-05-23
---



# [2026-07-20] Velero 备份 Job 因内存不足失败，删除操作后无法恢复关键 ConfigMap

## 工单信息
- **工单编号**: INC-2026-0720-016
- **发现时间**: 2026-07-20 09:10 UTC
- **恢复时间**: 2026-07-20 10:00 UTC
- **影响范围**: `prod-config` namespace 的关键 ConfigMap
- **业务影响**: 误删的 `app-config` ConfigMap 无法从 Velero 备份恢复，业务配置丢失 50 分钟

## 问题现象
09:10，运维人员在清理旧配置时误执行：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl delete configmap app-config -n prod-config
```

该 ConfigMap 包含核心数据库连接串和 API 密钥。删除后立即发现应用报错：
```
Error: configmap "app-config" not found
```

运维人员尝试从 Velero 备份恢复：
```bash
velero restore create --from-backup daily-backup-20260719
```

但恢复失败，Velero 日志显示：
```
error: backup daily-backup-20260719 not found
```

## 诊断过程

**09:15** — 检查 Velero 备份状态：
```bash
velero backup get
# NAME                    STATUS      ERRORS   WARNINGS
# daily-backup-20260719   Failed      1        0
# daily-backup-20260718   Completed   0        0
```

**09:17** — 查看失败原因：
```bash
velero backup logs daily-backup-20260719 | tail -n 20
# time="2026-07-19T02:00:15Z" level=error 
#   msg="backup failed" error="error executing custom action: 
#   rpc error: code = Unknown desc = error getting volume snapshotter: 
#   error getting volume snapshotter for provider aws: 
#   error getting cloud provider credentials: 
#   RequestLimitExceeded: Request limit exceeded."
```

**09:19** — 进一步排查：
```bash
kubectl get pods -n velero
# NAME                    READY   STATUS      RESTARTS
# velero-xxx              1/1     Running     0
# restic-xxx              0/1     OOMKilled   3

kubectl logs -n velero restic-xxx --previous | tail -n 10
# time="2026-07-19T02:00:10Z" level=info msg="Processing item" 
#   backup=velero/daily-backup-20260719 ...
# time="2026-07-19T02:00:12Z" level=error msg="Out of memory"
```

**09:21** — 发现根本原因：
- Restic DaemonSet 的 memory limit 为 512Mi
- 07-18 新部署的日志收集器在每个节点产生了大量小文件
- Restic 遍历文件系统时内存耗尽，备份 Job 失败
- 备份 Partial Failed，但未触发告警（告警仅检查 `STATUS == Failed`，未检查 `WARNINGS > 0`）

**09:23** — 检查 07-18 的备份是否可用：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
velero backup get daily-backup-20260718
# NAME                    STATUS      ERRORS   WARNINGS
# daily-backup-20260718   Completed   0        0

# 尝试恢复
cat <<'EOF' | kubectl apply -f -
apiVersion: velero.io/v1
kind: Restore
metadata:
  name: restore-configmap-20260720
  namespace: velero
spec:
  backupName: daily-backup-20260718
  includedNamespaces:
  - prod-config
  includedResources:
  - configmaps
EOF
```

**09:30** — 恢复成功：
```bash
kubectl get configmap app-config -n prod-config
# NAME        DATA   AGE
# app-config  5      2m
```

但 07-18 至 07-20 期间的配置变更丢失。

## 根因
1. 日志收集器产生大量小文件 → Restic 备份时内存超限 → 07-19 备份失败
2. 告警配置不完整：仅检查 `STATUS == Failed`，未检查 `WARNINGS > 0` 或 Partial Failed
3. 误删 ConfigMap 后，最新备份不可用，只能回退到 07-18 的备份，丢失 2 天配置变更

## 修复动作

**09:35** — 恢复最新配置变更：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 从 Git 仓库恢复 07-19 和 07-20 的配置变更
git log --oneline --since="2026-07-18" -- configs/prod-config/app-config.yaml
# abc1234 feat: update db connection string
# def5678 fix: add new API key

git show abc1234:configs/prod-config/app-config.yaml | kubectl apply -f -
git show def5678:configs/prod-config/app-config.yaml | kubectl apply -f -
```

**09:45** — 提升 Restic 内存：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch daemonset restic -n velero --type='merge' -p '
{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "restic",
          "resources": {
            "limits": {"cpu": "2", "memory": "4Gi"},
            "requests": {"cpu": "500m", "memory": "1Gi"}
          }
        }]
      }
    }
  }
}'
```

**09:50** — 手动触发一次备份验证：
```bash
velero backup create test-backup-20260720
velero backup get test-backup-20260720
# NAME                    STATUS      ERRORS   WARNINGS
# test-backup-20260720    Completed   0        0
```

## 验证
- 09:55 — 应用配置全部恢复，服务正常启动
- 10:00 — 业务验证通过，API 密钥和数据库连接正常

## 复盘
- **直接原因**: Velero Restic OOM → 备份失败 → 误删后无法从最新备份恢复
- **根本原因**: 
  1. Restic 内存配置过低，未评估文件增长
  2. 备份告警不完整，未覆盖 Partial Failed
- **改进措施**:
  1. Velero 备份告警：`(status == Failed) OR (warnings > 0) OR (errors > 0)`
  2. 将 ConfigMap/Secret 等关键配置纳入 GitOps，变更即备份
  3. Restic 内存 limit 提升至 4Gi，并添加文件数量监控
  4. 每月执行一次恢复演练，验证备份可用性
  5. 禁止直接 `kubectl delete` 关键 ConfigMap，必须通过 GitOps 删除
- **相关 Skill**: [[k8s-disaster-recovery-guide]]
- **相关 FTA**: [[backup-restore-fta]]
