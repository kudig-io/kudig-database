---
title: Incident Management and GameDay Practice — Full Lifecycle
description: K8s 事件管理与 GameDay — 事件响应流程、严重等级定义、GameDay 演练设计、混沌注入、恢复验证、持续改进
summary: 生产事件全生命周期管理与 GameDay 演练实践，从预防到恢复到持续改进
category: practice
tags:
- incident-management
- gameday
- chaos-engineering
- sre
- resilience
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: reliability
---
# 事件管理与 GameDay 演练实践

> 生产事件全生命周期管理与定期 GameDay 演练的完整实践。

## 事件响应流程

```
检测 → 分级 → 响应 → 缓解 → 恢复 → 复盘 → 改进
 │       │       │       │       │       │       │
告警   P0-P3   IC/OL   止血   验证   无责   Action
触发   定义    角色    操作   确认   复盘   Items
```

## 严重等级定义

| 等级 | 定义 | 响应时间 | 恢复目标 | 通知范围 |
|------|------|----------|----------|----------|
| P0 (SEV-1) | 核心服务完全不可用/数据丢失 | 5 min | 30 min | VP + 全团队 |
| P1 (SEV-2) | 核心服务严重降级（>50% 用户受影响） | 15 min | 1 h | 总监 + 相关团队 |
| P2 (SEV-3) | 非核心服务不可用/核心轻微降级 | 30 min | 4 h | 团队 Lead |
| P3 (SEV-4) | 轻微问题/无用户影响 | 下一工作日 | 1 周 | 相关工程师 |

## 事件角色

| 角色 | 职责 | 人数 |
|------|------|------|
| Incident Commander (IC) | 协调全局、决策升级/回滚 | 1 |
| Operations Lead (OL) | 执行缓解操作 | 1-2 |
| Communications Lead (CL) | 内外部沟通、状态页更新 | 1 |
| Subject Matter Expert (SME) | 提供技术诊断 | 按需 |
| Scribe | 记录时间线 | 1 |

## 事件响应 Runbook 模板

```markdown
# [P0] API 网关 5xx 激增

## 快速诊断（< 5 min）
1. 确认影响范围
   - Grafana Dashboard: [链接]
   - 检查: `kubectl get pods -n ingress-nginx -o wide`
   - 检查: `kubectl top pods -n ingress-nginx`

2. 检查最近变更
   - ArgoCD: `argocd app list --selector team=platform`
   - 最近部署: `kubectl rollout history deployment/ingress-nginx -n ingress-nginx`

## 缓解操作（按优先级）
### 方案 A: 回滚最近部署
kubectl rollout undo deployment/ingress-nginx -n ingress-nginx
# 或 ArgoCD 回滚
argocd app rollback ingress-nginx

### 方案 B: 扩容
kubectl scale deployment/ingress-nginx -n ingress-nginx --replicas=10

### 方案 C: 切换流量到备用集群
kubectl apply -f failover/traffic-shift.yaml

## 升级条件
- 15 分钟内未缓解 → 升级 P0 + 通知 VP
- 数据一致性存疑 → 立即通知 DBA

## 恢复确认
- [ ] 5xx 率 < 0.1% 持续 10 分钟
- [ ] P99 延迟恢复正常
- [ ] 无新错误日志
- [ ] 状态页更新为 Resolved
```

## GameDay 演练设计

### 演练类型

| 类型 | 目标 | 频率 | 参与者 |
|------|------|------|--------|
| Tabletop（桌面推演） | 验证流程/沟通 | 月度 | 全团队 |
| Functional（功能演练） | 验证特定恢复能力 | 季度 | SRE + 开发 |
| Full-scale（全面演练） | 端到端灾难恢复 | 半年 | 全组织 |
| Chaos（混沌注入） | 发现未知弱点 | 持续 | SRE |

### GameDay 计划模板

```yaml
# GameDay 计划
gameday:
  name: "2026-Q3 数据库故障转移演练"
  date: "2026-09-15"
  duration: "4h"
  participants:
    - role: IC
      person: sre-lead
    - role: OL
      person: dba-oncall
    - role: CL
      person: eng-manager
    - role: Observer
      person: [dev-lead, qa-lead]
  
  objectives:
    - 验证 PostgreSQL 主从切换 RTO < 5min
    - 验证应用自动重连能力
    - 验证监控告警及时性（< 1min 检测）
    - 验证 Runbook 可执行性
  
  scenarios:
    - name: "主库 Pod 被驱逐"
      injection:
        tool: kubectl
        command: "kubectl delete pod postgres-0 -n database --grace-period=0"
      expected:
        detection_time: "< 30s"
        failover_time: "< 2min"
        data_loss: "0 transactions"
      success_criteria:
        - "Patroni 自动选举新主"
        - "应用 30s 内恢复连接"
        - "无数据丢失"
    
    - name: "整个 AZ 不可用"
      injection:
        tool: chaos-mesh
        manifest: |
          apiVersion: chaos-mesh.org/v1alpha1
          kind: NetworkChaos
          metadata:
            name: az-isolation
            namespace: database
          spec:
            action: partition
            mode: all
            selector:
              namespaces: ["database"]
              labelSelectors:
                topology.kubernetes.io/zone: "cn-east-1a"
            direction: both
            duration: "10m"
      expected:
        detection_time: "< 1min"
        recovery_time: "< 5min"
      success_criteria:
        - "跨 AZ 副本接管"
        - "服务降级但不中断"
  
  abort_criteria:
    - "真实用户受到影响"
    - "数据一致性风险"
    - "超出预定时间窗口 30 分钟"
  
  communication:
    slack_channel: "#gameday-2026q3"
    status_page: "https://status.example.com"
```

### Chaos Mesh 注入示例

```yaml
# Pod 故障注入
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: kill-api-pods
  namespace: production
spec:
  action: pod-kill
  mode: one  # 一次杀一个
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: api-server
  scheduler:
    cron: "@every 30m"
  duration: "10s"
---
# 网络延迟注入
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay
  namespace: production
spec:
  action: delay
  mode: all
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: payment-service
  delay:
    latency: "200ms"
    jitter: "50ms"
    correlation: "25"
  duration: "5m"
---
# DNS 故障注入
apiVersion: chaos-mesh.org/v1alpha1
kind: DNSChaos
metadata:
  name: dns-error
  namespace: production
spec:
  action: error
  mode: all
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: order-service
  patterns:
    - "payment-service.production.svc.cluster.local"
  duration: "2m"
```

## 复盘流程（Blameless Postmortem）

### 时间线模板

```markdown
# 事件复盘：[标题]

## 元数据
- 日期: 2026-07-20
- 严重等级: P1
- 持续时间: 47 分钟
- 影响: 约 15,000 用户无法下单
- IC: @sre-lead
- 作者: @engineer-x

## 摘要
[2-3 句话描述发生了什么、影响、根因]

## 时间线（UTC+8）
| 时间 | 事件 |
|------|------|
| 14:02 | 部署 order-service v2.3.1（含数据库迁移） |
| 14:05 | 告警触发：order-service 5xx > 5% |
| 14:07 | IC 确认 P1，开启事件频道 |
| 14:12 | 发现数据库连接池耗尽 |
| 14:18 | 定位根因：迁移锁表导致连接阻塞 |
| 14:25 | 决策：回滚部署 |
| 14:30 | 回滚完成，服务恢复 |
| 14:49 | 确认所有指标正常，关闭事件 |

## 根因分析（5 Whys）
1. 为什么用户无法下单？→ API 返回 500
2. 为什么 API 返回 500？→ 数据库连接超时
3. 为什么连接超时？→ 连接池被迁移锁阻塞
4. 为什么迁移锁阻塞连接？→ 迁移在大表上执行 ALTER TABLE
5. 为什么生产执行了锁表迁移？→ CI 未检测破坏性迁移

## 改进行动
| 行动 | 负责人 | 截止日期 | 优先级 |
|------|--------|----------|--------|
| CI 添加迁移锁检测 | @dba | 07-25 | P0 |
| 大表迁移使用 gh-ost | @dba | 08-01 | P0 |
| 部署前自动检查迁移 | @platform | 08-15 | P1 |
| 连接池监控告警 | @sre | 07-22 | P1 |
| Runbook 补充迁移回滚步骤 | @sre | 07-25 | P2 |

## 做得好的
- 告警 3 分钟内触发
- IC 快速决策回滚
- 团队 15 分钟内定位根因

## 待改进的
- 部署未等待迁移完成即切流量
- 缺少数据库连接池告警
- Runbook 无迁移相关步骤
```

## GameDay 度量

| 指标 | 目标 | 采集 |
|------|------|------|
| 检测时间 (MTTD) | < 1 min | 告警触发时间 |
| 响应时间 (MTTR-respond) | < 5 min | IC 确认时间 |
| 恢复时间 (MTTR-recover) | < 30 min | 服务恢复时间 |
| 演练覆盖率 | 核心服务 100%/季度 | GameDay 记录 |
| Action Item 完成率 | > 90% 在截止日前 | 项目管理工具 |
| 重复事件率 | < 5% | 事件分类统计 |

## 事件通讯模板

### 内部通知模板

```markdown
## 🚨 [P0] 事件通知 - {service_name}

**状态**: 🔴 进行中 / 🟡 监控中 / 🟢 已解决
**开始时间**: 2026-07-21 14:05 UTC+8
**影响范围**: 约 15,000 用户无法下单
**IC**: @sre-lead

### 当前状况
- 订单服务 5xx 错误率 > 50%
- 数据库连接池耗尽

### 已采取行动
- 14:07 IC 确认 P0，开启事件频道
- 14:12 定位根因：数据库迁移锁表
- 14:25 决策：回滚部署

### 下一步
- 验证回滚完成
- 确认服务恢复
- 30 分钟内发布更新

### 沟通节奏
- 每 15 分钟更新一次
- 下次更新: 14:45
```

### 外部状态页模板

```markdown
## 服务状态更新

**服务**: 订单服务
**状态**: 部分中断 (Partial Outage)
**开始时间**: 2026-07-21 14:05 UTC+8

### 影响
部分用户可能无法完成下单操作。我们的团队正在积极处理此问题。

### 更新时间线
- **14:05** - 我们检测到订单服务异常
- **14:10** - 已确认问题并启动应急响应
- **14:30** - 已实施修复措施，服务正在恢复
- **14:49** - 服务已完全恢复

### 后续
我们将在 48 小时内发布详细的事后分析报告。
```

## 改进项跟踪

### 改进项 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: incident-improvements
  namespace: monitoring
data:
  improvements.yaml: |
    improvements:
      - id: INC-2026-001-A1
        incident: INC-2026-001
        title: CI 添加迁移锁检测
        owner: @dba
        due_date: 2026-07-25
        status: completed
        priority: P0
        
      - id: INC-2026-001-A2
        incident: INC-2026-001
        title: 大表迁移使用 gh-ost
        owner: @dba
        due_date: 2026-08-01
        status: in_progress
        priority: P0
        
      - id: INC-2026-001-A3
        incident: INC-2026-001
        title: 连接池监控告警
        owner: @sre
        due_date: 2026-07-22
        status: completed
        priority: P1
```

### 改进项跟踪 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: improvement-tracker
  namespace: monitoring
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: tracker
              image: improvement-tracker:latest
              command: [sh, -c]
              args:
                - |
                  # 检查逾期改进项
                  OVERDUE=$(kubectl get configmap incident-improvements -n monitoring -o yaml | \
                    yq '.data."improvements.yaml"' | \
                    yq '.improvements[] | select(.due_date < "'$(date +%Y-%m-%d)'" and .status != "completed")')
                  
                  if [ -n "$OVERDUE" ]; then
                    echo "发现逾期改进项:"
                    echo "$OVERDUE"
                    # 发送提醒
                    curl -X POST $SLACK_WEBHOOK -d '{"text":"⚠️ 发现逾期改进项，请相关负责人跟进"}'
                  fi
          restartPolicy: OnFailure
```

## 事件指标仪表板

### Grafana Dashboard 配置

```json
{
  "dashboard": {
    "title": "事件管理概览",
    "panels": [
      {
        "title": "MTTD 趋势",
        "type": "graph",
        "targets": [
          { "expr": "avg(incident_detection_time_seconds) / 60" }
        ]
      },
      {
        "title": "MTTR 趋势",
        "type": "graph",
        "targets": [
          { "expr": "avg(incident_resolution_time_seconds) / 60" }
        ]
      },
      {
        "title": "事件数量 (按等级)",
        "type": "bargauge",
        "targets": [
          { "expr": "count by (severity) (incidents_total)" }
        ]
      },
      {
        "title": "改进项完成率",
        "type": "stat",
        "targets": [
          { "expr": "sum(incident_improvements_completed) / sum(incident_improvements_total) * 100" }
        ]
      }
    ]
  }
}
```

## Related

- [[12-可靠性/05-事后复盘/index.md|事后复盘]]
- [[12-可靠性/04-混沌工程/index.md|混沌工程]]
- [[12-可靠性/06-SRE实践/index.md|SRE 实践]]
