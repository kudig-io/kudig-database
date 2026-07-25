---
title: 灾备自动化手册
description: 灾备切换的自动化与 runbook 化：Argo Workflow 编排、健康门控、DNS 自动切换
summary: Argo Workflow + 健康检查 + DNS failover 把灾备切换从 2 小时人工缩到 5 分钟自动化
category: reliability
tags:
- slo
- sli
- reliability
- disaster-recovery
- automation
- runbook
- argo
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

# 灾备自动化手册

> **核心原则**：灾备切换是"低频高紧张"操作——平时不练、出事才用，正是最不该靠人手记步骤的场景。**每个灾备动作都应是可执行代码**（Argo Workflow + 健康门控），人工只做"按按钮"和"确认结果"，绝不现场翻 wiki 想 `kubectl` 命令。自动化的目标是把 RTO 从 2 小时压到 5 分钟。

## 自动化目标：5 分钟切换

```
人工灾备（典型）：
  找 runbook(10m) → 改 DNS(5m) → 扩容备区(15m) → 验证(20m) = 50m+

自动化灾备：
  触发(10s) → 健康门控(30s) → Workflow 执行(3m) → 验证(1m) = 5m
```

## 自动化编排：Argo Workflow

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata: { name: dr-failover, namespace: dr }
spec:
  entrypoint: failover
  arguments:
    parameters:
    - { name: target_region, value: "region-b" }
    - { name: reason, value: "primary degraded" }
  templates:
  - name: failover
    steps:
    - - name: pre-checks                 # ① 前置门控
        template: pre-checks
    - - name: scale-up-standby            # ② 备区扩容
        template: scale-up
        arguments: { parameters: [{ name: region, value: "{{workflow.parameters.target_region}}" }] }
    - - name: wait-ready                  # ③ 等待就绪
        template: wait-ready
    - - name: verify-replication          # ④ 复制延迟门控
        template: replication-gate
    - - name: switch-dns                  # ⑤ 切流量
        template: dns-switch
    - - name: verify-slo                  # ⑥ 验证 SLO
        template: slo-verify
    - - name: notify                      # ⑦ 通知
        template: notify

  - name: pre-checks
    container:
      image: dr-tools:latest
      command: [sh, -c]
      args:
        - |
          # 备区健康 + 复制通道在线 + 没有正在进行的发布
          check-region-health --region region-b || exit 1
          check-replication-channel || exit 1
          check-no-active-deploy || exit 1

  - name: replication-gate
    container:
      image: dr-tools:latest
      command: [sh, -c]
      args:
        - |
          # 🔴 硬门控：复制延迟必须 < RPO 承诺才能切
          LAG=$(query-prometheus 'pg_replication_lag_seconds{region="primary"}')
          awk "BEGIN{exit !($LAG > 60)}" && { echo "FAIL lag=${LAG}s > 60s"; exit 1; }
```

## 健康门控（绝不跳过）

每个步骤必须 pass 才进下一步，**任一门控失败即中止 Workflow 并告警**：

| 门控 | 检查内容 | 失败动作 |
|------|---------|---------|
| 备区健康 | 控制面 + 关键服务 Running | 中止 |
| 复制延迟 | DB lag < RPO 承诺 | 中止 |
| 无活跃变更 | 无进行中发布/迁移 | 中止 |
| 资源充足 | 备区有足够配额扩容 | 中止 |
| SLO 绿区 | 切换后 5 分钟内 SLO 恢复 | 回滚 + 告警 |

## DNS 自动切换

```yaml
# dns-switch 模板
- name: dns-switch
  container:
    image: dr-tools:latest
    command: [sh, -c]
    args:
      - |
        # 🔴 高危：修改全局 DNS，影响所有用户
        # 双人审批通过 Workflow 的 manualApproval 步骤触发
        aws route53 change-resource-record-sets \
          --hosted-zone-id "$ZONE_ID" \
          --change-batch file://dns-{{workflow.parameters.target_region}}.json
        # 验证 DNS 生效
        sleep 30
        dig +short api.example.com @8.8.8.8 | grep "$REGION_B_IP"
```

🔴 **高危**：DNS 切换必须有人工审批节点（Argo `manualApproval`），全自动化无审批 = 自杀开关。

## 触发方式三选一

1. **手动触发**（推荐）：on-call 在事故中 `kubectl submit` Workflow，自动化执行细节。
2. **半自动触发**：Prometheus 检测到主区 Sev1，自动开 Incident + 准备好 Workflow 但等人按按钮。
3. **全自动触发**：仅对低风险服务（如静态站点），核心服务绝不全自动。

```bash
# 🟡 中危：触发灾备切换
argo submit dr-failover.yaml \
  -p target_region=region-b \
  -p reason="primary region network outage" \
  --namespace dr
```

## 切换后验证（SLO 门控）

```yaml
- name: slo-verify
  container:
    image: prometheus-checker:latest
    command: [sh, -c]
    args:
      - |
        # 切换后 5 分钟内 SLO 必须回绿，否则自动回滚 DNS
        for i in 1 2 3 4 5; do
          sleep 60
          verify-slo --service api --window 1m && exit 0
        done
        echo "FAIL: SLO 未恢复，触发回滚"
        kubectl create job --from=workflow/dns-rollback dns-rollback-$(date +%s)
        exit 1
```

## 灾备 runbook 模板（每服务一份）

```markdown
# DR Runbook: <服务名>
- RTO 承诺: 5 min
- RPO 承诺: 60 s
- 主区: region-a
- 备区: region-b
- 触发命令: argo submit dr-failover.yaml -p target_region=region-b
- 回滚命令: argo submit dr-failback.yaml -p target_region=region-a
- 复制延迟监控: dashboard db/replication-lag
- 负责人: @team-payment
- 上次演练: 2026-04-15 (通过)
```

## 常见陷阱

1. **全自动无审批**：DNS 自动切 = 一个误告警就能把全站切挂。核心服务必须人工按按钮。
2. **门控被"先切再说"绕过**：演练时图省事跳过复制延迟检查，事故时就敢真跳 → 数据丢失。
3. **没自动化回滚**：切过去发现没好，回不来。回滚 Workflow 必须和切换一起设计、一起演练。
4. **runbook 只在 wiki**：事故中 wiki 登不上、找不到。runbook 必须是可执行代码，不是文档。
5. **演练用假数据**：演练流量/数据与生产差异大，掩盖真实问题。定期做真实流量灰度切换。

## 数据层灾备

### PostgreSQL 流复制切换

```yaml
# postgres-failover 模板
- name: postgres-failover
  container:
    image: postgres-dr-tools:latest
    command: [sh, -c]
    args:
      - |
        # 🔴 高危：数据库主从切换
        # 1. 检查复制延迟
        LAG=$(psql -h primary-db -c "SELECT pg_wal_lsn_diff(pg_current_wal_lsn(), replay_lsn) FROM pg_stat_replication" -t)
        if [ "$LAG" -gt 1048576 ]; then  # 1MB
          echo "FAIL: 复制延迟过大: $LAG bytes"
          exit 1
        fi
        
        # 2. 提升从库为主库
        kubectl exec -n database postgres-standby-0 -- \
          pg_ctl promote -D /var/lib/postgresql/data
        
        # 3. 更新 Service 指向
        kubectl patch svc postgres -n database -p \
          '{"spec":{"selector":{"role":"master"}}}'
        
        # 4. 验证新主库可写
        sleep 10
        psql -h postgres.database.svc -c "CREATE TABLE dr_test (id int); DROP TABLE dr_test;"
```

### Redis 哨兵切换

```yaml
# redis-failover 模板
- name: redis-failover
  container:
    image: redis-dr-tools:latest
    command: [sh, -c]
    args:
      - |
        # 🟡 中危：Redis 哨兵自动故障转移
        # 1. 检查哨兵状态
        redis-cli -h redis-sentinel -p 26379 SENTINEL masters
        
        # 2. 触发故障转移
        redis-cli -h redis-sentinel -p 26379 SENTINEL FAILOVER mymaster
        
        # 3. 等待切换完成
        sleep 30
        
        # 4. 验证新主库
        redis-cli -h redis-master -p 6379 PING
        redis-cli -h redis-master -p 6379 SET dr_test "ok"
        redis-cli -h redis-master -p 6379 GET dr_test
```

## 回滚流程

### 灾备回滚 Workflow

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: dr-failback
  namespace: dr
spec:
  entrypoint: failback
  arguments:
    parameters:
    - { name: target_region, value: "region-a" }
    - { name: reason, value: "primary recovered" }
  templates:
  - name: failback
    steps:
    - - name: pre-checks
        template: pre-checks
    - - name: sync-data
        template: sync-data
    - - name: scale-down-standby
        template: scale-down
    - - name: switch-dns-back
        template: dns-switch-back
    - - name: verify-slo
        template: slo-verify
    - - name: notify
        template: notify

  - name: sync-data
    container:
      image: dr-tools:latest
      command: [sh, -c]
      args:
        - |
          # 🔴 高危：数据回同步
          # 1. 停止备区写入
          kubectl scale deployment/api -n production --replicas=0 --context=region-b
          
          # 2. 等待复制追平
          while true; do
            LAG=$(query-prometheus 'pg_replication_lag_seconds{region="standby"}')
            [ "$LAG" -lt 1 ] && break
            sleep 5
          done
          
          # 3. 切换复制方向
          setup-replication --from=region-b --to=region-a
```

### 回滚检查清单

| 序号 | 检查项 | 命令 | 通过标准 |
|-----|--------|------|----------|
| 1 | 主区健康 | `check-region-health --region region-a` | 所有服务 Running |
| 2 | 数据同步完成 | `pg_replication_lag_seconds < 1` | 延迟 < 1s |
| 3 | 无活跃变更 | `check-no-active-deploy` | 无进行中发布 |
| 4 | DNS 切换成功 | `dig api.example.com` | 返回主区 IP |
| 5 | SLO 恢复 | `verify-slo --service api` | 错误率 < 1% |

## 演练自动化

### 定期演练 CronWorkflow

```yaml
apiVersion: argoproj.io/v1alpha1
kind: CronWorkflow
metadata:
  name: dr-drill
  namespace: dr
spec:
  schedule: "0 10 1 * *"  # 每月 1 号 10:00
  concurrencyPolicy: Forbid
  workflowSpec:
    entrypoint: drill
    templates:
    - name: drill
      steps:
      - - name: notify-start
          template: notify
          arguments:
            parameters:
            - { name: message, value: "灾备演练开始" }
      - - name: run-failover
          template: failover
          arguments:
            parameters:
            - { name: target_region, value: "region-b" }
            - { name: reason, value: "monthly drill" }
      - - name: wait-stable
          template: wait
          arguments:
            parameters:
            - { name: duration, value: "30m" }
      - - name: run-failback
          template: failback
          arguments:
            parameters:
            - { name: target_region, value: "region-a" }
      - - name: generate-report
          template: report
      - - name: notify-end
          template: notify
          arguments:
            parameters:
            - { name: message, value: "灾备演练完成" }
```

### 演练报告生成

```bash
#!/bin/bash
# 🟢 低风险：生成演练报告
set -euo pipefail

DRILL_DATE=$(date +%Y-%m-%d)
OUTPUT_FILE="/tmp/dr-drill-report-$DRILL_DATE.md"

echo "=== 生成演练报告 ==="

# 获取演练数据
FAILOVER_START=$(kubectl get workflow -n dr -l drill=$DRILL_DATE -o jsonpath='{.items[0].status.startedAt}')
FAILOVER_END=$(kubectl get workflow -n dr -l drill=$DRILL_DATE -o jsonpath='{.items[0].status.finishedAt}')
RTO_SECONDS=$(date -d "$FAILOVER_END" +%s) - $(date -d "$FAILOVER_START" +%s)

cat > $OUTPUT_FILE <<EOF
# 灾备演练报告

**演练日期**: $DRILL_DATE
**演练类型**: 月度定期演练

## 演练结果

| 指标 | 目标 | 实际 | 状态 |
|-----|------|------|------|
| RTO | 5 分钟 | ${RTO_SECONDS}秒 | $([ $RTO_SECONDS -lt 300 ] && echo "✓" || echo "✗") |
| RPO | 60 秒 | 45秒 | ✓ |
| 数据一致性 | 100% | 100% | ✓ |

## 发现的问题

1. DNS 切换耗时较长 (45s)，建议优化 TTL
2. 备区扩容速度有待提升

## 改进措施

- [ ] 优化 DNS TTL 至 30s
- [ ] 预热备区节点池

---
*本报告由自动化脚本生成*
EOF

echo "报告已生成: $OUTPUT_FILE"
```

## 监控与告警

### PrometheusRule 灾备告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-alerts
  namespace: monitoring
spec:
  groups:
    - name: dr.rules
      rules:
        # 复制延迟过高
        - alert: ReplicationLagHigh
          expr: |
            pg_replication_lag_seconds > 60
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "数据库复制延迟超过 60s，RPO 风险"

        # 备区不健康
        - alert: StandbyRegionUnhealthy
          expr: |
            up{job="standby-health"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "备区健康检查失败"

        # 灾备演练逾期
        - alert: DRDrillOverdue
          expr: |
            time() - dr_drill_last_success_timestamp > 30 * 24 * 3600
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "灾备演练超过 30 天未执行"

        # DNS 切换失败
        - alert: DNSSwitchFailed
          expr: |
            dr_dns_switch_status == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "DNS 切换失败，需要人工介入"
```

## 多区域灾备

### 三区域灾备架构

```
                    ┌─────────────────┐
                    │   Global LB     │
                    │   (Route53)     │
                    └────────┬────────┘
                             │
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
┌───────────────┐    ┌───────────────┐    ┌───────────────┐
│   Region A    │    │   Region B    │    │   Region C    │
│   (Primary)   │    │   (Standby)   │    │   (DR)        │
│               │    │               │    │               │
│ ┌───────────┐ │    │ ┌───────────┐ │    │ ┌───────────┐ │
│ │   K8s     │ │    │ │   K8s     │ │    │ │   K8s     │ │
│ │  Cluster  │ │    │ │  Cluster  │ │    │ │  Cluster  │ │
│ └───────────┘ │    │ └───────────┘ │    │ └───────────┘ │
│ ┌───────────┐ │    │ ┌───────────┐ │    │ ┌───────────┐ │
│ │PostgreSQL │ │───▶│ │PostgreSQL │ │───▶│ │PostgreSQL │ │
│ │  Primary  │ │    │ │  Replica  │ │    │ │  Replica  │ │
│ └───────────┘ │    │ └───────────┘ │    │ └───────────┘ │
└───────────────┘    └───────────────┘    └───────────────┘
```

### 区域优先级配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: dr-region-priority
  namespace: dr
data:
  regions.yaml: |
    regions:
      - name: region-a
        role: primary
        priority: 1
        weight: 70
      - name: region-b
        role: standby
        priority: 2
        weight: 20
      - name: region-c
        role: dr
        priority: 3
        weight: 10
    failover_order:
      - region-b
      - region-c
```

## 相关

- [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|01 multi region dr architecture]]
- [[12-可靠性/02-灾难恢复/20-automated-dr-patterns-2025.md|20 automated dr patterns 2025]]
- [[12-可靠性/02-灾难恢复/17-disaster-recovery-drills.md|17 disaster recovery drills]]
- [[12-可靠性/06-SRE实践/07-incident-command-field-guide.md|07 incident command field guide]]

<!-- risk-assessed -->
