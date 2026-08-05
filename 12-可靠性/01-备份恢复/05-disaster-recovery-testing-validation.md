---
title: Disaster Recovery Testing and Validation — DR Drills, RTO/RPO Verification
description: K8s 灾难恢复验证 — DR 演练设计、RTO/RPO 验证、自动化恢复测试、数据一致性检查、恢复 Runbook
summary: 灾难恢复计划的验证与演练实践，确保恢复流程可执行且满足 RTO/RPO 目标
category: practice
tags:
- disaster-recovery
- dr-drill
- rto-rpo
- validation
- resilience
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: reliability
---
# 灾难恢复验证与演练实践

> 确保 DR 计划可执行、RTO/RPO 可达标的系统化验证方法。

## DR 验证框架

```
┌─────────────────────────────────────────────────────────┐
│  DR 验证层次                                             │
│                                                         │
│  L1: 备份完整性验证（自动化，每日）                       │
│  L2: 单服务恢复测试（自动化，每周）                       │
│  L3: 命名空间级恢复（半自动，每月）                       │
│  L4: 集群级 DR 切换（手动，每季度）                       │
│  L5: 全面 GameDay（手动，半年）                           │
└─────────────────────────────────────────────────────────┘
```

## RTO/RPO 目标定义

| 服务等级 | RTO | RPO | 验证频率 | 示例 |
|----------|-----|-----|----------|------|
| Tier-0（关键） | < 5 min | 0（同步复制） | 每周 | 支付、核心 API |
| Tier-1（重要） | < 30 min | < 5 min | 每月 | 订单、用户 |
| Tier-2（标准） | < 4 h | < 1 h | 每季度 | 内部工具 |
| Tier-3（低优先） | < 24 h | < 24 h | 半年 | 分析、报表 |

## 自动化备份验证

### 备份完整性检查（CronJob）

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-validation
  namespace: backup-system
spec:
  schedule: "0 6 * * *"  # 每天早 6 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: backup-validator
          containers:
            - name: validate
              image: registry.example.com/backup-validator:v1.0
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 备份验证 $(date) ==="
                  
                  # 1. 检查 Velero 备份状态
                  FAILED=$(velero backup get -n velero --output json | \
                    jq '[.items[] | select(.status.phase != "Completed")] | length')
                  if [ "$FAILED" -gt 0 ]; then
                    echo "ALERT: $FAILED 个备份未完成"
                    curl -X POST $ALERT_WEBHOOK -d '{"text":"备份验证失败"}'
                  fi
                  
                  # 2. 验证 etcd 快照可恢复
                  LATEST_SNAPSHOT=$(ls -t /backups/etcd/ | head -1)
                  etcdutl snapshot status /backups/etcd/$LATEST_SNAPSHOT --write-out=table
                  
                  # 3. 验证对象存储备份存在
                  aws s3 ls s3://k8s-backups/$(date -d yesterday +%Y%m%d)/ --recursive | wc -l
                  
                  # 4. 测试恢复到临时命名空间
                  velero restore create validation-$(date +%s) \
                    --from-backup $(velero backup get -n velero -o name | head -1) \
                    --namespace-mappings production:validation-tmp \
                    --wait --timeout 10m
                  
                  # 5. 验证恢复的资源
                  kubectl get all -n validation-tmp
                  kubectl delete ns validation-tmp --wait=false
                  
                  echo "=== 验证完成 ==="
          restartPolicy: OnFailure
```

### 数据库备份验证

```bash
#!/bin/bash
# validate-db-backup.sh — 验证数据库备份可恢复
BACKUP_FILE=$(ls -t /backups/postgres/ | head -1)
RESTORE_NS="db-validation-$(date +%s)"

echo "验证备份: $BACKUP_FILE"

# 创建临时命名空间
kubectl create ns $RESTORE_NS

# 恢复到临时实例
kubectl apply -n $RESTORE_NS -f - <<EOF
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: validation-pg
spec:
  instances: 1
  storage:
    size: 50Gi
  bootstrap:
    recovery:
      source: backup-source
  externalClusters:
    - name: backup-source
      barmanObjectStore:
        destinationPath: s3://pg-backups/production/
        s3Credentials:
          accessKeyId:
            name: backup-creds
            key: ACCESS_KEY_ID
          secretAccessKey:
            name: backup-creds
            key: SECRET_ACCESS_KEY
EOF

# 等待恢复完成
kubectl wait --for=jsonpath='{.status.readyInstances}'=1 \
  cluster/validation-pg -n $RESTORE_NS --timeout=15m

# 验证数据完整性
kubectl exec -n $RESTORE_NS validation-pg-1 -- \
  psql -U postgres -c "SELECT COUNT(*) FROM pg_stat_user_tables;"

# 清理
kubectl delete ns $RESTORE_NS --wait=false
echo "✅ 备份验证成功"
```

## DR 切换演练

### 演练脚本（集群级）

```bash
#!/bin/bash
# dr-drill.sh — 季度 DR 切换演练
set -e

DR_CLUSTER="dr-cluster"
PRIMARY_CLUSTER="primary-cluster"
TIMESTAMP=$(date +%Y%m%d-%H%M)

echo "=== DR 演练开始: $TIMESTAMP ==="

# Phase 1: 验证 DR 集群就绪
echo "--- Phase 1: DR 集群检查 ---"
kubectl --context=$DR_CLUSTER get nodes
kubectl --context=$DR_CLUSTER get pods -A --field-selector status.phase!=Running | wc -l

# Phase 2: 验证数据同步延迟
echo "--- Phase 2: 数据同步检查 ---"
LAG=$(kubectl --context=$DR_CLUSTER exec postgres-dr-1 -- \
  psql -U postgres -t -c "SELECT EXTRACT(EPOCH FROM (now() - pg_last_xact_replay_timestamp()));")
echo "复制延迟: ${LAG}s"
if (( $(echo "$LAG > 60" | bc -l) )); then
  echo "ERROR: 复制延迟过大，中止演练"
  exit 1
fi

# Phase 3: 模拟主集群不可用
echo "--- Phase 3: 模拟主集群故障 ---"
# 在 DNS 层面切换（不实际关闭主集群）
# Route53/CloudDNS 权重调整
aws route53 change-resource-record-sets \
  --hosted-zone-id Z1234567890 \
  --change-batch '{
    "Changes": [{
      "Action": "UPSERT",
      "ResourceRecordSet": {
        "Name": "api.example.com",
        "Type": "CNAME",
        "TTL": 60,
        "ResourceRecords": [{"Value": "dr-elb.example.com"}]
      }
    }]
  }'

# Phase 4: 验证 DR 集群服务
echo "--- Phase 4: 服务验证 ---"
sleep 60  # 等待 DNS 传播
curl -sf https://api.example.com/health || echo "FAIL: API 不可用"
curl -sf https://api.example.com/api/v1/status || echo "FAIL: 状态接口异常"

# Phase 5: 记录 RTO
echo "--- Phase 5: RTO 记录 ---"
echo "RTO: $(($(date +%s) - START_TIME))s"

# Phase 6: 回切
echo "--- Phase 6: 回切主集群 ---"
# 恢复 DNS
aws route53 change-resource-record-sets \
  --hosted-zone-id Z1234567890 \
  --change-batch '{
    "Changes": [{
      "Action": "UPSERT",
      "ResourceRecordSet": {
        "Name": "api.example.com",
        "Type": "CNAME",
        "TTL": 60,
        "ResourceRecords": [{"Value": "primary-elb.example.com"}]
      }
    }]
  }'

echo "=== DR 演练完成 ==="
```

## 恢复 Runbook 模板

```markdown
# [Tier-0] 生产集群完全不可用恢复

## 前提条件
- DR 集群已部署且数据同步 < 5min
- DNS 切换权限已确认
- 值班 SRE 已到位

## 恢复步骤

### 1. 确认故障（< 2 min）
- [ ] 确认主集群 API Server 不可达
- [ ] 确认非网络抖动（多源验证）
- [ ] 宣布 P0 事件

### 2. 决策切换（< 3 min）
- [ ] IC 确认切换决策
- [ ] 通知相关方（Slack/电话）
- [ ] 记录切换时间戳

### 3. 执行切换（< 5 min）
- [ ] DNS 切换到 DR 集群
- [ ] 验证 DR 集群服务正常
- [ ] 更新状态页

### 4. 验证恢复（< 10 min）
- [ ] 核心 API 健康检查通过
- [ ] 数据库读写正常
- [ ] 消息队列消费正常
- [ ] 监控告警正常

### 5. 后续处理
- [ ] 持续监控 DR 集群
- [ ] 排查主集群故障原因
- [ ] 修复后规划回切
- [ ] 48h 内完成复盘
```

## 验证度量

| 指标 | 目标 | 采集 |
|------|------|------|
| 备份成功率 | > 99.9% | Velero 状态 |
| 备份验证通过率 | 100% | 验证 CronJob |
| DR 演练 RTO | < 目标 RTO | 演练记录 |
| DR 演练 RPO | < 目标 RPO | 复制延迟 |
| 演练频率 | 按 Tier 定义 | 日历 |
| Runbook 覆盖率 | 100% Tier-0/1 | 文档审计 |

## 数据一致性验证

### 数据库一致性检查脚本

```bash
#!/bin/bash
# validate-data-consistency.sh — 验证主从数据一致性
set -euo pipefail

PRIMARY_DB="postgres-primary.database.svc"
STANDBY_DB="postgres-standby.database.svc"
DB_USER="postgres"
DB_NAME="production"

echo "=== 数据一致性验证 $(date) ==="

# 1. 表数量对比
PRIMARY_TABLES=$(psql -h $PRIMARY_DB -U $DB_USER -d $DB_NAME -t -c \
  "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = 'public';")
STANDBY_TABLES=$(psql -h $STANDBY_DB -U $DB_USER -d $DB_NAME -t -c \
  "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = 'public';")

echo "主库表数量: $PRIMARY_TABLES"
echo "从库表数量: $STANDBY_TABLES"

if [ "$PRIMARY_TABLES" != "$STANDBY_TABLES" ]; then
  echo "❌ 表数量不一致"
  exit 1
fi

# 2. 关键表行数对比
CRITICAL_TABLES=("users" "orders" "payments" "products")

for table in "${CRITICAL_TABLES[@]}"; do
  PRIMARY_COUNT=$(psql -h $PRIMARY_DB -U $DB_USER -d $DB_NAME -t -c \
    "SELECT COUNT(*) FROM $table;")
  STANDBY_COUNT=$(psql -h $STANDBY_DB -U $DB_USER -d $DB_NAME -t -c \
    "SELECT COUNT(*) FROM $table;")
  
  DIFF=$((PRIMARY_COUNT - STANDBY_COUNT))
  
  if [ "$DIFF" -eq 0 ]; then
    echo "✅ $table: 一致 ($PRIMARY_COUNT 行)"
  elif [ "$DIFF" -lt 10 ]; then
    echo "⚠️ $table: 轻微差异 (主库 $PRIMARY_COUNT, 从库 $STANDBY_COUNT)"
  else
    echo "❌ $table: 显著差异 (主库 $PRIMARY_COUNT, 从库 $STANDBY_COUNT)"
  fi
done

# 3. 校验和对比 (抽样)
echo "--- 校验和验证 ---"
PRIMARY_CHECKSUM=$(psql -h $PRIMARY_DB -U $DB_USER -d $DB_NAME -t -c \
  "SELECT md5(string_agg(id::text || updated_at::text, ',' ORDER BY id)) FROM users WHERE id % 100 = 0;")
STANDBY_CHECKSUM=$(psql -h $STANDBY_DB -U $DB_USER -d $DB_NAME -t -c \
  "SELECT md5(string_agg(id::text || updated_at::text, ',' ORDER BY id)) FROM users WHERE id % 100 = 0;")

if [ "$PRIMARY_CHECKSUM" == "$STANDBY_CHECKSUM" ]; then
  echo "✅ 抽样校验和一致"
else
  echo "❌ 抽样校验和不一致"
  echo "主库: $PRIMARY_CHECKSUM"
  echo "从库: $STANDBY_CHECKSUM"
fi

echo "=== 验证完成 ==="
```

### 对象存储一致性验证

```bash
#!/bin/bash
# validate-object-storage.sh — 验证对象存储备份一致性
set -euo pipefail

SOURCE_BUCKET="s3://prod-backups"
DR_BUCKET="s3://dr-backups"
DATE=$(date -d yesterday +%Y%m%d)

echo "=== 对象存储一致性验证 ==="

# 1. 文件数量对比
SOURCE_COUNT=$(aws s3 ls $SOURCE_BUCKET/$DATE/ --recursive | wc -l)
DR_COUNT=$(aws s3 ls $DR_BUCKET/$DATE/ --recursive | wc -l)

echo "源桶文件数: $SOURCE_COUNT"
echo "DR 桶文件数: $DR_COUNT"

# 2. 总大小对比
SOURCE_SIZE=$(aws s3 ls $SOURCE_BUCKET/$DATE/ --recursive --summarize | tail -1 | awk '{print $3}')
DR_SIZE=$(aws s3 ls $DR_BUCKET/$DATE/ --recursive --summarize | tail -1 | awk '{print $3}')

echo "源桶总大小: $SOURCE_SIZE bytes"
echo "DR 桶总大小: $DR_SIZE bytes"

# 3. 抽样校验和对比
SAMPLE_FILES=$(aws s3 ls $SOURCE_BUCKET/$DATE/ --recursive | head -10 | awk '{print $4}')

for file in $SAMPLE_FILES; do
  SOURCE_MD5=$(aws s3api head-object --bucket prod-backups --key $file | jq -r '.ETag')
  DR_MD5=$(aws s3api head-object --bucket dr-backups --key $file | jq -r '.ETag' 2>/dev/null || echo "missing")
  
  if [ "$SOURCE_MD5" == "$DR_MD5" ]; then
    echo "✅ $file: 一致"
  else
    echo "❌ $file: 不一致或缺失"
  fi
done

echo "=== 验证完成 ==="
```

## 演练自动化编排

### Argo Workflow 演练编排

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: dr-validation-workflow
  namespace: dr-system
spec:
  entrypoint: main
  templates:
    - name: main
      steps:
        - - name: pre-checks
            template: pre-checks
        - - name: backup-validation
            template: validate-backup
        - - name: restore-test
            template: test-restore
        - - name: data-consistency
            template: check-consistency
        - - name: failover-drill
            template: drill-failover
        - - name: generate-report
            template: report
        - - name: cleanup
            template: cleanup
    
    - name: pre-checks
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 前置检查 ==="
            # 检查 DR 集群健康
            kubectl --context=dr-cluster get nodes
            kubectl --context=dr-cluster get pods -A --field-selector status.phase!=Running
            
            # 检查备份状态
            velero backup get -n velero | grep -v Completed && exit 1
            
            # 检查复制延迟
            LAG=$(kubectl exec -n database postgres-standby-0 -- \
              psql -U postgres -t -c "SELECT EXTRACT(EPOCH FROM (now() - pg_last_xact_replay_timestamp()));")
            [ "$LAG" -lt 60 ] || exit 1
    
    - name: validate-backup
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 备份验证 ==="
            # 验证最新备份可恢复
            LATEST_BACKUP=$(velero backup get -n velero -o name | head -1)
            velero restore create validation-$(date +%s) \
              --from-backup $LATEST_BACKUP \
              --namespace-mappings production:validation-tmp \
              --wait --timeout 15m
    
    - name: test-restore
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 恢复测试 ==="
            # 验证恢复的资源
            kubectl get all -n validation-tmp
            
            # 运行冒烟测试
            kubectl run smoke-test --image=curlimages/curl --rm -i --restart=Never -- \
              curl -sf http://api.validation-tmp.svc:8080/health
    
    - name: check-consistency
      container:
        image: postgres:15
        command: [sh, -c]
        args:
          - |
            echo "=== 数据一致性检查 ==="
            ./validate-data-consistency.sh
    
    - name: drill-failover
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 故障转移演练 ==="
            # 执行 DNS 切换
            ./dr-drill.sh
    
    - name: report
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 生成报告 ==="
            ./generate-dr-report.sh
    
    - name: cleanup
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 清理 ==="
            kubectl delete ns validation-tmp --wait=false --ignore-not-found
```

### 定期演练 CronWorkflow

```yaml
apiVersion: argoproj.io/v1alpha1
kind: CronWorkflow
metadata:
  name: monthly-dr-validation
  namespace: dr-system
spec:
  schedule: "0 10 1 * *"  # 每月 1 号 10:00
  concurrencyPolicy: Forbid
  workflowSpec:
    entrypoint: main
    templates:
      - name: main
        steps:
          - - name: run-validation
              template: validation
          - - name: notify
              template: notify
    
    - name: validation
      container:
        image: dr-tools:latest
        command: [sh, -c]
        args:
          - |
            ./run-full-dr-validation.sh
    
    - name: notify
      container:
        image: curlimages/curl
        command: [sh, -c]
        args:
          - |
            curl -X POST $SLACK_WEBHOOK -d '{"text":"Monthly DR validation completed"}'
```

## 监控与告警

### PrometheusRule DR 验证告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-validation-alerts
  namespace: monitoring
spec:
  groups:
    - name: dr-validation.rules
      rules:
        # 备份验证失败
        - alert: BackupValidationFailed
          expr: |
            backup_validation_success == 0
          for: 0m
          labels:
            severity: critical
          annotations:
            summary: "备份验证失败，备份可能不可恢复"

        # 数据一致性检查失败
        - alert: DataConsistencyCheckFailed
          expr: |
            data_consistency_check_success == 0
          for: 0m
          labels:
            severity: critical
          annotations:
            summary: "数据一致性检查失败，主从数据可能不一致"

        # DR 演练逾期
        - alert: DRDrillOverdue
          expr: |
            time() - dr_drill_last_success_timestamp > 30 * 24 * 3600
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "DR 演练超过 30 天未执行"

        # 复制延迟过高
        - alert: ReplicationLagHigh
          expr: |
            pg_replication_lag_seconds > 60
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "数据库复制延迟超过 60s，RPO 风险"

        # 恢复时间超标
        - alert: RestoreTimeExceeded
          expr: |
            dr_restore_duration_seconds > 300
          for: 0m
          labels:
            severity: warning
          annotations:
            summary: "恢复时间超过 5 分钟，RTO 风险"
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "DR 验证概览",
    "panels": [
      {
        "title": "备份验证成功率",
        "type": "stat",
        "targets": [
          { "expr": "sum(rate(backup_validation_success_total[1d])) / sum(rate(backup_validation_attempt_total[1d])) * 100" }
        ]
      },
      {
        "title": "数据一致性检查",
        "type": "stat",
        "targets": [
          { "expr": "data_consistency_check_success" }
        ]
      },
      {
        "title": "RTO 趋势",
        "type": "graph",
        "targets": [
          { "expr": "dr_rto_seconds" }
        ]
      },
      {
        "title": "RPO 趋势",
        "type": "graph",
        "targets": [
          { "expr": "dr_rpo_seconds" }
        ]
      },
      {
        "title": "复制延迟",
        "type": "graph",
        "targets": [
          { "expr": "pg_replication_lag_seconds" }
        ]
      }
    ]
  }
}
```

## 持续改进

### 演练后复盘模板

```markdown
# DR 演练复盘报告

## 演练信息
- 演练日期: __________
- 演练类型: □ 备份验证 □ 恢复测试 □ 故障转移 □ 全面演练
- 参与人员: __________

## 演练结果
| 指标 | 目标 | 实际 | 状态 |
|-----|------|------|------|
| RTO | < 5 min | ___ min | □ 达标 □ 未达标 |
| RPO | < 60 s | ___ s | □ 达标 □ 未达标 |
| 数据一致性 | 100% | ___% | □ 达标 □ 未达标 |

## 发现的问题
1. __________
2. __________
3. __________

## 改进行动
| 行动 | 负责人 | 截止日期 | 状态 |
|-----|-------|---------|------|
| __________ | __________ | __________ | ☐ |
| __________ | __________ | __________ | ☐ |

## 下次演练计划
- 日期: __________
- 重点: __________
```

### 改进项跟踪

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: dr-improvements
  namespace: dr-system
data:
  improvements.yaml: |
    improvements:
      - id: DR-IMP-2026-001
        title: 优化 DNS TTL 加速切换
        owner: @platform-team
        due_date: 2026-08-15
        status: in_progress
        priority: high
        source: 演练发现
        
      - id: DR-IMP-2026-002
        title: 预热 DR 集群节点池
        owner: @sre-team
        due_date: 2026-08-31
        status: pending
        priority: medium
        source: RTO 超标分析
        
      - id: DR-IMP-2026-003
        title: 自动化数据一致性检查
        owner: @sre-team
        due_date: 2026-09-15
        status: pending
        priority: high
        source: 手动检查耗时过长
```

## Related

- [[12-可靠性/01-备份恢复/index.md|备份恢复]]
- [[12-可靠性/01-备份恢复/04-velero-enterprise-backup-restore.md|Velero 备份]]
- [[12-可靠性/05-事后复盘/04-incident-management-gameday-practice.md|GameDay 实践]]
