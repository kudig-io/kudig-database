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

## Related

- [[可靠性/备份恢复/index.md|备份恢复]]
- [[可靠性/备份恢复/05-velero-enterprise-backup-restore.md|Velero 备份]]
- [[可靠性/事后复盘/04-incident-management-gameday-practice.md|GameDay 实践]]
