---
title: 分级响应矩阵与严重级别定义
description: '定义 P0-P3 严重级别、响应 SLA、On-Call 轮值规则及自动升级触发条件'
summary: '定义 P0-P3 严重级别、响应 SLA、On-Call 轮值规则及自动升级触发条件'
category: production-operations
tags:
- production
- operations
- incident-response
- escalation
- sla
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 分级响应矩阵 是什么
- 如何 定义严重级别
- 如何 设置 On-Call 轮值
trigger_keywords:
- escalation
- severity
- P0
- P1
- incident
- on-call
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 分级响应矩阵与严重级别定义

## 1. 严重级别总览

| 级别 | 名称 | 影响范围 | 业务损失 | 典型场景 |
|------|------|---------|---------|---------|
| **P0** | 紧急/Critical | 核心业务完全中断 | 直接营收损失 > ¥100万/小时 | 全站不可用、数据丢失、安全事件 |
| **P1** | 严重/High | 核心业务严重降级 | 直接营收损失 ¥10-100万/小时 | 支付链路异常、主库不可用 |
| **P2** | 一般/Medium | 非核心功能异常 | 间接损失、用户投诉上升 | 搜索服务降级、后台任务积压 |
| **P3** | 低/Limited | 体验瑕疵或预警 | 无直接损失 | 日志延迟、监控告警抖动 |

## 2. P0 — 紧急响应

### 2.1 判定标准

满足以下**任意一条**即为 P0：

- 生产环境核心服务完全不可用（HTTP 5xx > 50%）
- 数据丢失或数据一致性受损
- 安全事件（数据泄露、未授权访问）
- 影响范围 > 50% 用户
- 合规/法律风险事件

### 2.2 场景示例

```
场景 A: 全站 502
  现象: 所有 API 返回 502 Bad Gateway
  影响: 100% 用户无法访问
  根因: Ingress Controller Pod 全部 OOMKilled
  级别: P0

场景 B: 数据库主库切换失败
  现象: MySQL 主库宕机后自动切换未生效
  影响: 所有写操作阻塞，读操作超时
  根因: Patroni etcd 集群脑裂
  级别: P0

场景 C: 用户数据泄露
  现象: 安全团队发现未授权 API 可批量导出用户 PII
  影响: 潜在数百万用户数据暴露
  根因: RBAC 配置错误 + API 鉴权绕过
  级别: P0
```

### 2.3 响应要求

| 项目 | 要求 |
|------|------|
| 首次响应 | ≤ 5 分钟 |
| War Room 启动 | ≤ 15 分钟 |
| 状态更新频率 | 每 15 分钟 |
| 管理层通知 | ≤ 30 分钟 |
| 恢复目标 | ≤ 1 小时（RTO） |
| 升级至 VP/CTO | ≤ 1 小时未恢复 |

## 3. P1 — 严重响应

### 3.1 判定标准

满足以下**任意一条**即为 P1：

- 核心业务功能严重降级（成功率 < 90%）
- 影响范围 10%-50% 用户
- 主要数据通路部分中断
- SLA 即将被突破（Error Budget 剩余 < 10%）

### 3.2 场景示例

```
场景 A: 支付成功率下降
  现象: 支付成功率从 99.5% 降至 85%
  影响: 约 15% 用户支付失败
  根因: 第三方支付通道限流
  级别: P1

场景 B: 主库只读
  现象: MySQL 主库进入只读模式
  影响: 所有写操作失败，读操作正常
  根因: 磁盘空间不足触发保护
  级别: P1

场景 C: 认证服务延迟飙升
  环象: OAuth2 Token 签发 P99 从 50ms 升至 5s
  影响: 部分用户登录超时
  根因: Redis Cluster 主从切换
  级别: P1
```

### 3.3 响应要求

| 项目 | 要求 |
|------|------|
| 首次响应 | ≤ 15 分钟 |
| 状态更新频率 | 每 30 分钟 |
| 恢复目标 | ≤ 4 小时 |
| 升级至 P0 | 1 小时未缓解或影响扩大 |

## 4. P2 — 一般响应

### 4.1 判定标准

- 非核心功能不可用或严重降级
- 影响范围 < 10% 用户
- 存在变通方案（Workaround）
- 非生产环境严重问题

### 4.2 场景示例

```
场景 A: 搜索服务降级
  现象: Elasticsearch 集群 Yellow，搜索延迟 2x
  影响: 搜索结果延迟，但可返回
  根因: 2 个数据节点磁盘满
  级别: P2

场景 B: 定时任务积压
  现象: Celery Worker 队列积压 > 10万
  影响: 报表生成延迟、通知发送延迟
  根因: Worker Pod 被 OOMKilled 后未自动恢复
  级别: P2

场景 C: CI/CD Pipeline 中断
  现象: GitLab Runner 无法调度新 Job
  影响: 开发团队无法部署
  根因: Runner 节点 cert 过期
  级别: P2
```

### 4.3 响应要求

| 项目 | 要求 |
|------|------|
| 首次响应 | ≤ 1 小时 |
| 状态更新频率 | 每日 |
| 恢复目标 | ≤ 24 小时 |
| 升级至 P1 | 影响范围扩大或超过 SLA |

## 5. P3 — 低优先级

### 5.1 判定标准

- 体验瑕疵，不影响核心功能
- 预警性告警（尚未造成实际影响）
- 非生产环境一般问题

### 5.2 场景示例

```
# 🟢 低风险：只读/信息收集，通常无副作用
场景 A: 日志采集延迟
  现象: Fluentd 采集延迟 > 30 分钟
  影响: 日志检索不实时，不影响业务
  级别: P3

场景 B: 监控告警抖动
  现象: CPU 告警在阈值附近反复触发/恢复
  影响: On-Call 噪音
  级别: P3

场景 C: Staging 环境部署失败
  现象: Staging 环境 Helm Chart 版本冲突
  影响: 仅影响测试
  级别: P3
```
### 5.3 响应要求

| 项目 | 要求 |
|------|------|
| 首次响应 | ≤ 4 小时（工作时间） |
| 状态更新频率 | 按需 |
| 恢复目标 | ≤ 1 周 |
| 升级至 P2 | 用户投诉增加 |

## 6. On-Call 轮值规则

### 6.1 轮值模式

```
Primary On-Call:   负责首次响应 + 初步诊断
Secondary On-Call: Primary 15 分钟未响应时接管
Manager On-Call:   P0 事件升级 + 跨团队协调

轮值周期: 每周轮换（周一 10:00 交接）
备份机制: 每个角色至少 2 人可选
```

### 6.2 On-Call 职责

```
值班期间:
  - 保持通讯工具在线（Slack/电话）
  - 响应告警并完成初步分诊
  - P0/P1: 15 分钟内开始处理
  - 需要升级时及时通知 Secondary

交接要求:
  - 更新 On-Call Handoff 文档
  - 说明当前未关闭事件
  - 标注已知问题和临时措施
```

### 6.3 轮值排班表格式

```yaml
# oncall-schedule.yaml
week_rotation:
  - week: "2026-W28"
    primary: "engineer-a"
    secondary: "engineer-b"
    manager: "manager-x"
  - week: "2026-W29"
    primary: "engineer-c"
    secondary: "engineer-d"
    manager: "manager-y"

escalation_chain:
  - level: 1
    target: "primary"
    wait: "0m"
  - level: 2
    target: "secondary"
    wait: "5m"
  - level: 3
    target: "manager"
    wait: "15m"
  - level: 4
    target: "vp-engineering"
    wait: "60m"
```

## 7. 自动升级触发条件

### 7.1 时间驱动升级

```
升级规则:

P0 事件:
  5 min  无响应  → 升级至 Secondary
  15 min 无响应  → 升级至 Manager + 自动创建 War Room
  30 min 未缓解  → 通知 VP Engineering
  60 min 未恢复  → 通知 CTO + 启动客户沟通

P1 事件:
  15 min 无响应  → 升级至 Secondary
  60 min 未缓解  → 升级至 Manager
  4 hr   未恢复  → 提升为 P0

P2 事件:
  4 hr   无响应  → 升级至 Team Lead
  24 hr  未处理  → 提升为 P1
```

### 7.2 影响驱动升级

```
自动升级触发器:

监控规则:
  - alert: AutoEscalateP1toP0
    expr: |
      increase(http_requests_total{status=~"5.."}[5m])
      / increase(http_requests_total[5m]) > 0.5
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "5xx 错误率超过 50%，自动升级至 P0"

  - alert: AutoEscalateSLO
    expr: |
      1 - (
        sum(rate(http_requests_total{status!~"5.."}[30m]))
        / sum(rate(http_requests_total[30m]))
      ) > slo_target * 0.1
    for: 10m
    labels:
      severity: high
    annotations:
      summary: "Error Budget 剩余不足 10%，升级至 P1"
```

### 7.3 重复事件升级

```
同一服务 24 小时内:
  3 次 P2 事件 → 自动升级为 P1
  2 次 P1 事件 → 自动升级为 P0 + 启动稳定性专项

同一根因 7 天内:
  重复出现 3 次 → 创建 Problem Ticket
  自动关联历史事件
```

## 8. 事件响应流程图

```
事件触发
    │
    ▼
自动分诊（告警标签 + 影响面评估）
    │
    ├── P0 ──→ 立即通知 Primary + 创建 Incident
    │              │
    │              ├── 5min 无响应 → Secondary
    │              ├── War Room 启动
    │              └── 每 15min 状态更新
    │
    ├── P1 ──→ 通知 Primary + 创建 Incident
    │              │
    │              ├── 15min 无响应 → Secondary
    │              └── 每 30min 状态更新
    │
    ├── P2 ──→ 创建 Ticket + 通知 On-Call
    │              │
    │              └── 每日状态更新
    │
    └── P3 ──→ 创建 Ticket + 工作时间处理
                   │
                   └── 按需更新
```

## 9. 严重级别调整规则

### 9.1 上调条件

- 影响范围扩大（用户比例上升）
- 业务损失加剧
- 修复时间超出预期
- 出现新的关联故障

### 9.2 下调条件

- 影响范围缩小且稳定
- 存在有效 Workaround
- 用户可自行恢复

### 9.3 调整审批

| 原级别 → 目标级别 | 审批人 |
|-------------------|--------|
| P3 → P2 | On-Call 工程师自行判断 |
| P2 → P1 | Team Lead |
| P1 → P0 | Manager On-Call |
| 任意下调 | 当前事件 IC + 1 名 L2 确认 |

## 10. 工具集成

### 10.1 PagerDuty 配置

```yaml
# pagerduty-escalation.yaml
escalation_policy:
  name: "K8s Production Escalation"
  num_loops: 3
  escalation_rules:
    - escalation_delay_in_minutes: 0
      targets:
        - type: "user_reference"
          id: "primary-oncall"
    - escalation_delay_in_minutes: 5
      targets:
        - type: "user_reference"
          id: "secondary-oncall"
    - escalation_delay_in_minutes: 15
      targets:
        - type: "schedule_reference"
          id: "manager-oncall-schedule"
```

### 10.2 与 Kubernetes 集成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 自动创建 Incident 的 Kubernetes Event Watcher
kubectl get events --all-namespaces --watch-only \
  --field-selector reason=FailedScheduling,reason=FailedMount \
  | while read line; do
    echo "$line" | python3 scripts/create-incident.py
  done
```
---

*本文档定义事件响应的分级标准和升级机制。所有 On-Call 人员必须熟记各级别判定标准和响应时限。*


<!-- risk-assessed -->
