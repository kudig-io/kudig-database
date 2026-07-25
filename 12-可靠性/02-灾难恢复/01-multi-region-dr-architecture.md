---
title: 多区域灾备架构
description: 多区域灾备架构模式：主备、双活、引导灯三种模式与 RTO/RPO 目标及流量切换方案
summary: Active-Passive / Active-Active / Pilot-Light 三模式对比 + 全局流量切换 + 数据同步策略
category: reliability
tags:
- slo
- sli
- reliability
- disaster-recovery
- multi-region
- architecture
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 多区域灾备架构

> **核心原则**：多区域灾备不是"再建一个集群放着"，而是**明确承诺一个可量化的 RTO/RPO，并围绕这个承诺设计数据同步、流量切换、回切的完整闭环**。没演练过的灾备架构 = 没有灾备。RTO/RPO 写进文档没用，写进定期 Game Day 验证才有用。

## 三种架构模式对比

```
模式1: Active-Passive（主备）
   Region-A(主,写读写)  ──异步复制──▶  Region-B(备,待命)
   流量100%→A   A挂→切B

模式2: Active-Active（双活）
   Region-A(写读写) ◀──双向复制──▶ Region-B(写读写)
   流量50/50   任一挂→另一扛全部

模式3: Pilot-Light（引导灯）
   Region-A(全量)  ──异步复制──▶  Region-B(最小核心运行)
   B平时只跑核心/数据热   A挂→B扩容接管
```

| 模式 | RTO | RPO | 成本 | 复杂度 | 适用 |
|------|-----|-----|------|--------|------|
| Active-Passive | 分钟–小时 | 秒 | 高（全量冷备） | 中 | 合规要求高 |
| Active-Active | 秒 | ~0 | 最高（双全量） | 高 | 全球用户、零容忍 |
| Pilot-Light | 分钟 | 秒 | 中（最小热备） | 中 | 性价比首选 |

## 数据同步策略（灾备成败核心）

```
┌─────────────┐                     ┌─────────────┐
│ Region-A    │   1. 异步流          │ Region-B    │
│  无状态服务  │ ◀──────────────────▶ │  无状态服务  │
│  有状态DB    │   2. DB 复制          │  有状态DB    │
│  缓存        │   3. 缓存重建(不复制)  │  缓存        │
└─────────────┘                     └─────────────┘
```

1. **应用层无状态**：会话放 Redis/外部存储，Pod 可在任一区域拉起。
2. **DB 异步复制**：主从复制（PostgreSQL streaming、MySQL binlog、MongoDB replica set）。RPO 取决于复制延迟，**必须监控复制延迟指标**。
3. **缓存不跨区复制**：成本高且无必要，灾备区缓存冷启动，用预热脚本填关键数据。

⚠️ **冲突处理**：Active-Active 下双写需应用层做 CRDT 或最后写入胜出（LWW），数据库原生双活支持有限。

## 全局流量切换

```yaml
# AWS Route 53 / Cloudflare / GSLB 配置（示意）
Type: Failover
Primary: Region-A (health check: GET /health, 10s interval)
Secondary: Region-B
FailoverPolicy:
  - Primary 连续 3 次健康检查失败 → 自动切 Secondary
  - DNS TTL: 60s（短的 TTL 才能快速切换）
```

🔴 **高危**：手动切流必须双人确认。误切流到未就绪的备区会造成全站不可用。

```bash
# 🔴 高危：手动 DNS 切换（生产事故级操作）
# 切换前 Checklist：
#   [ ] 备区健康检查通过
#   [ ] 复制延迟 < RPO 承诺
#   [ ] 备区缓存已预热
#   [ ] 通知客户与支持团队
aws route53 change-resource-record-sets \
  --hosted-zone-id Z123 \
  --change-batch '{"Changes":[{"Action":"UPSERT","ResourceRecordSet":{...Region-B...}}]}'
```

## 回切（Failback，最易出错）

切换到备区容易，**回切到主区**才是难点——备区在故障期间产生了新数据，主区需要先追上：

```
T+0   主区故障恢复
T+1   反向建立复制：备区(主) → 主区(从)
T+2   等待主区数据追平（监控 lag=0）
T+3   选窗口切回（低峰期）
T+4   DNS 切回主区
T+5   观察 30 分钟稳定
```

跳过数据追平就回切 = 数据丢失。回切必须有"复制延迟=0"的硬门控。

## Kubernetes 多区域实现

```yaml
# Karmada / Cluster API 管理多集群（示意）
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata: { name: api-spread }
spec:
  resourceSelectors: [{ apiVersion: apps/v1, kind: Deployment, name: api }]
  placement:
    clusterAffinity: { clusterNames: [region-a, region-b] }
    replicaScheduling:
      replicaSchedulingType: Divided
      replicaDivisionPreference: Weighted
      weightPreference:
        staticWeightList:
        - targetCluster: { clusterNames: [region-a] }
          weight: 1
        - targetCluster: { clusterNames: [region-b] }
          weight: 1
```

## RTO/RPO 承诺矩阵（参考）

| 服务等级 | RTO | RPO | 架构建议 |
|---------|-----|-----|---------|
| 核心（支付） | < 1 min | ~0 | Active-Active |
| 重要（下单） | < 15 min | < 1 min | Pilot-Light |
| 一般（浏览） | < 1 hour | < 5 min | Active-Passive |
| 内部工具 | < 4 hour | < 1 hour | 备份恢复 |

## 演练铁律

1. **每季度全链路切换演练**：从 DNS 切换到真实流量接管，不能只"看一眼备区在不在"。
2. **演练产出 = 工单**：每次演练发现的问题必须开工单，下季度验证修复。
3. **轮换演练区域**：这次切到 B，下次切到 A，避免"备区永远是备区"的隐性腐烂。
4. **故障注入演练**：见 [[12-可靠性/02-灾难恢复/03-enterprise-disaster-recovery-chaos-engineering.md]]，用混沌工程模拟区域级故障。

## 常见陷阱

1. **RPO 承诺 vs 复制延迟不监控**：承诺 RPO=1分钟，但复制延迟实际 5 分钟，事故时数据丢 5 倍。
2. **备区配置漂移**：主区天天改，备区半年没动，切过去发现配置不兼容。用 GitOps 统一管理两边配置。
3. **只测切换不测回切**：回切才是真正考验数据一致性的环节。
4. **DNS TTL 太长**：TTL=1小时意味着切换后 1 小时内仍有流量去旧区，等于没切。

## 灾备自动化编排

### 切换流程自动化

```yaml
# Argo Workflow 灾备切换编排
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: dr-failover
  namespace: dr-automation
spec:
  entrypoint: failover-steps
  templates:
    - name: failover-steps
      steps:
        - - name: verify-dr-health
            template: check-dr-region
        - - name: scale-up-dr
            template: scale-dr-workloads
        - - name: switch-dns
            template: dns-failover
        - - name: verify-traffic
            template: verify-traffic-flow
        - - name: notify-stakeholders
            template: send-notification

    - name: check-dr-region
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 检查备区健康状态 ==="
            # 检查备区集群状态
            kubectl --context=region-b get nodes
            # 检查关键服务就绪状态
            kubectl --context=region-b get pods -n production -l tier=critical
            # 检查数据库复制延迟
            kubectl --context=region-b exec -n database deploy/postgres -- \
              psql -c "SELECT now() - pg_last_xact_replay_timestamp() AS replication_lag;"

    - name: scale-dr-workloads
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 扩容备区工作负载 ==="
            # 扩容无状态服务
            kubectl --context=region-b scale deploy/api-gateway -n production --replicas=10
            kubectl --context=region-b scale deploy/payment-service -n production --replicas=6
            # 等待 Pod 就绪
            kubectl --context=region-b rollout status deploy/api-gateway -n production --timeout=300s

    - name: dns-failover
      container:
        image: amazon/aws-cli:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 执行 DNS 切换 ==="
            # 更新 Route53 故障转移记录
            aws route53 change-resource-record-sets \
              --hosted-zone-id $HOSTED_ZONE_ID \
              --change-batch '{
                "Changes": [{
                  "Action": "UPSERT",
                  "ResourceRecordSet": {
                    "Name": "api.example.com",
                    "Type": "A",
                    "SetIdentifier": "primary",
                    "Failover": "PRIMARY",
                    "HealthCheckId": "'$DR_HEALTH_CHECK_ID'",
                    "AliasTarget": {
                      "HostedZoneId": "'$REGION_B_ZONE_ID'",
                      "DNSName": "'$REGION_B_LB_DNS'",
                      "EvaluateTargetHealth": true
                    }
                  }
                }]
              }'

    - name: verify-traffic-flow
      container:
        image: curlimages/curl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 验证流量切换 ==="
            sleep 60  # 等待 DNS 传播
            # 检查流量是否到达备区
            curl -s https://api.example.com/health | jq .
            # 检查错误率
            curl -s "http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~'5..'}[1m]))"

    - name: send-notification
      container:
        image: curlimages/curl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 发送通知 ==="
            curl -X POST -H 'Content-type: application/json' \
              --data '{"text":"🚨 灾备切换已执行，流量已切换到备区"}' \
              $SLACK_WEBHOOK_URL
```

### 回切自动化

```bash
#!/bin/bash
# 🔴 高风险：灾备回切脚本
set -euo pipefail

echo "=== 灾备回切流程 ==="

# 1. 检查主区健康
echo "[1] 检查主区健康状态..."
kubectl --context=region-a get nodes
kubectl --context=region-a get pods -n production

# 2. 建立反向复制
echo "[2] 建立反向复制（备区→主区）..."
kubectl --context=region-a exec -n database deploy/postgres -- \
  psql -c "SELECT pg_start_backup('failback', true);"
# 配置主区为备区的从库

# 3. 等待数据追平
echo "[3] 等待数据追平..."
while true; do
  LAG=$(kubectl --context=region-a exec -n database deploy/postgres -- \
    psql -t -c "SELECT EXTRACT(EPOCH FROM (now() - pg_last_xact_replay_timestamp()))::int;")
  if [ "$LAG" -eq 0 ]; then
    echo "✓ 数据已追平"
    break
  fi
  echo "  复制延迟: ${LAG}s"
  sleep 10
done

# 4. 执行 DNS 回切
echo "[4] 执行 DNS 回切..."
# 更新 Route53 记录

# 5. 验证
echo "[5] 验证流量..."
sleep 60
curl -s https://api.example.com/health

echo "=== 回切完成 ==="
```

## 数据一致性保障

### 复制延迟监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-replication-alerts
  namespace: monitoring
spec:
  groups:
    - name: dr.replication.rules
      rules:
        - alert: ReplicationLagHigh
          expr: |
            pg_replication_lag_seconds > 60
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "数据库复制延迟超过 60s，RPO 风险"
            description: "当前复制延迟: {{ $value }}s"

        - alert: ReplicationBroken
          expr: |
            pg_replication_is_replica == 0 and pg_replication_lag_seconds == -1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "数据库复制中断"

        - alert: DRRegionUnhealthy
          expr: |
            kube_deployment_status_replicas_available{cluster="region-b"} 
            < kube_deployment_status_replicas{cluster="region-b"} * 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "备区工作负载可用副本不足 80%"
```

### 数据一致性检查 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: dr-consistency-check
  namespace: dr-automation
spec:
  schedule: "0 */6 * * *"  # 每 6 小时
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: checker
              image: postgres:16
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 数据一致性检查 $(date) ==="
                  
                  # 1. 检查复制延迟
                  echo "[1] 复制延迟:"
                  PGPASSWORD=$DB_PASSWORD psql -h $PRIMARY_DB -U repl -c \
                    "SELECT client_addr, state, sent_lsn, replay_lsn, 
                     pg_wal_lsn_diff(sent_lsn, replay_lsn) AS lag_bytes
                     FROM pg_stat_replication;"
                  
                  # 2. 检查关键表行数一致性
                  echo "[2] 关键表行数对比:"
                  PRIMARY_COUNT=$(PGPASSWORD=$DB_PASSWORD psql -h $PRIMARY_DB -U app -t -c \
                    "SELECT COUNT(*) FROM orders WHERE created_at > now() - interval '1 hour';")
                  DR_COUNT=$(PGPASSWORD=$DB_PASSWORD psql -h $DR_DB -U app -t -c \
                    "SELECT COUNT(*) FROM orders WHERE created_at > now() - interval '1 hour';")
                  echo "  主区: $PRIMARY_COUNT, 备区: $DR_COUNT"
                  
                  # 3. 检查点数据校验
                  echo "[3] 校验和对比:"
                  PRIMARY_CHECKSUM=$(PGPASSWORD=$DB_PASSWORD psql -h $PRIMARY_DB -U app -t -c \
                    "SELECT md5(string_agg(id::text || amount::text, ',' ORDER BY id)) FROM orders WHERE created_at > now() - interval '1 hour';")
                  DR_CHECKSUM=$(PGPASSWORD=$DB_PASSWORD psql -h $DR_DB -U app -t -c \
                    "SELECT md5(string_agg(id::text || amount::text, ',' ORDER BY id)) FROM orders WHERE created_at > now() - interval '1 hour';")
                  
                  if [ "$PRIMARY_CHECKSUM" = "$DR_CHECKSUM" ]; then
                    echo "  ✓ 数据一致"
                  else
                    echo "  ✗ 数据不一致！主区: $PRIMARY_CHECKSUM, 备区: $DR_CHECKSUM"
                  fi
                  
                  echo "=== 检查完成 ==="
```

## 灾备演练自动化

### Game Day 自动化脚本

```bash
#!/bin/bash
# 🟡 中风险：灾备演练自动化脚本
set -euo pipefail

DRILL_TYPE=${1:-"full"}  # full | dns-only | app-only

echo "=== 灾备演练开始: $DRILL_TYPE ==="
echo "时间: $(date)"

# 1. 演练前检查
echo "[1] 演练前检查..."
# 检查备区健康
kubectl --context=region-b get nodes -o wide
kubectl --context=region-b get pods -n production --field-selector=status.phase!=Running

# 检查复制延迟
LAG=$(kubectl --context=region-b exec -n database deploy/postgres -- \
  psql -t -c "SELECT EXTRACT(EPOCH FROM (now() - pg_last_xact_replay_timestamp()))::int;")
echo "  复制延迟: ${LAG}s"
if [ "$LAG" -gt 60 ]; then
  echo "❌ 复制延迟过高，中止演练"
  exit 1
fi

# 2. 记录基线指标
echo "[2] 记录基线指标..."
BASELINE_ERROR_RATE=$(curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[5m]))/sum(rate(http_requests_total[5m]))' | jq -r '.data.result[0].value[1]')
echo "  基线错误率: $BASELINE_ERROR_RATE"

# 3. 执行切换
echo "[3] 执行切换..."
if [ "$DRILL_TYPE" = "full" ] || [ "$DRILL_TYPE" = "dns-only" ]; then
  # DNS 切换
  echo "  执行 DNS 切换..."
fi

if [ "$DRILL_TYPE" = "full" ] || [ "$DRILL_TYPE" = "app-only" ]; then
  # 应用扩容
  echo "  扩容备区应用..."
  kubectl --context=region-b scale deploy/api-gateway -n production --replicas=10
fi

# 4. 验证
echo "[4] 验证切换结果..."
sleep 60
CURRENT_ERROR_RATE=$(curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[5m]))/sum(rate(http_requests_total[5m]))' | jq -r '.data.result[0].value[1]')
echo "  当前错误率: $CURRENT_ERROR_RATE"

# 5. 生成报告
echo "[5] 生成演练报告..."
cat > /tmp/dr-drill-report.md <<EOF
# 灾备演练报告
- 日期: $(date)
- 类型: $DRILL_TYPE
- 基线错误率: $BASELINE_ERROR_RATE
- 切换后错误率: $CURRENT_ERROR_RATE
- 复制延迟: ${LAG}s
- 状态: $([ "$CURRENT_ERROR_RATE" = "$BASELINE_ERROR_RATE" ] && echo "✅ 成功" || echo "⚠️ 需关注")
EOF

echo "=== 演练完成 ==="
cat /tmp/dr-drill-report.md
```

## 成本优化策略

### 灾备成本构成

| 成本项 | Active-Passive | Active-Active | Pilot-Light | 优化建议 |
|-------|---------------|---------------|-------------|----------|
| 计算资源 | 100% 冷备 | 100% 双活 | 20% 热备 | Pilot-Light 性价比最高 |
| 存储复制 | 异步复制 | 双向复制 | 异步复制 | 压缩+增量备份 |
| 网络流量 | 复制流量 | 双向流量 | 复制流量 | 专线 vs 公网权衡 |
| 许可费用 | 双倍 | 双倍 | 最小 | 使用开源组件 |

### 成本优化配置

```yaml
# 备区资源缩减配置（Pilot-Light 模式）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: production
  annotations:
    # 备区默认最小副本
    dr.region-b/replicas: "2"
    # 切换时扩容目标
    dr.region-b/failover-replicas: "10"
spec:
  replicas: 2  # 平时最小运行
  template:
    spec:
      containers:
        - name: api
          resources:
            requests:
              cpu: "500m"  # 备区降低请求
              memory: 1Gi
            limits:
              cpu: "2"
              memory: 4Gi
---
# 使用 Spot/抢占式实例（备区无状态服务）
apiVersion: v1
kind: Node
metadata:
  labels:
    topology.kubernetes.io/zone: region-b
    node.kubernetes.io/lifecycle: spot
spec:
  taints:
    - key: node.kubernetes.io/lifecycle
      value: spot
      effect: PreferNoSchedule
```

## 灾备检查清单

### 架构就绪检查

| 序号 | 检查项 | 验证方法 | 通过标准 |
|-----|--------|---------|----------|
| 1 | RTO/RPO 已明确定义 | 检查文档 | 有量化目标 |
| 2 | 数据复制已配置 | 检查复制状态 | 延迟 < RPO |
| 3 | DNS 切换已配置 | 检查健康检查 | TTL ≤ 60s |
| 4 | 备区工作负载已部署 | 检查 Deployment | 最小副本运行 |
| 5 | 缓存预热脚本已准备 | 检查脚本 | 可执行 |
| 6 | 监控告警已配置 | 检查 PrometheusRule | 复制延迟告警 |
| 7 | 切换流程已自动化 | 检查 Workflow | 可一键执行 |
| 8 | 回切流程已验证 | 检查文档+演练 | 有数据追平步骤 |
| 9 | 演练已定期执行 | 检查演练记录 | 季度演练 |
| 10 | 成本已优化 | 检查资源使用 | 备区资源缩减 |

## 相关

- [[12-可靠性/02-灾难恢复/02-dr-automation-playbook.md|02 dr automation playbook]]
- [[12-可靠性/02-灾难恢复/18-cross-region-disaster-recovery.md|18 cross region disaster recovery]]
- [[12-可靠性/02-灾难恢复/03-enterprise-disaster-recovery-chaos-engineering.md|03 enterprise dr chaos]]

<!-- risk-assessed -->
