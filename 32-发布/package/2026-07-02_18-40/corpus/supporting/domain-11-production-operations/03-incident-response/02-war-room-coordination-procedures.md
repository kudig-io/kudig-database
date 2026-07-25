---
title: 战时指挥室协调流程
description: '定义 War Room 启动条件、角色分工、跨团队协调、信息同步与决策记录机制'
summary: '定义 War Room 启动条件、角色分工、跨团队协调、信息同步与决策记录机制'
category: production-operations
tags:
- production
- operations
- incident-response
- war-room
- coordination
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
- War Room 流程 是什么
- 如何 组织战时指挥室
- 如何 跨团队协调事件响应
trigger_keywords:
- war-room
- incident
- coordination
- escalation
- ic
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


# 战时指挥室协调流程

## 1. War Room 启动条件

### 1.1 自动启动

以下条件满足任意一条时，系统自动创建 War Room 频道：

```
自动触发规则:

规则 1: P0 事件创建时
  触发: PagerDuty P0 Incident 创建
  动作:
    - 创建 Slack 频道 #warroom-YYYYMMDD-HHMM-<简述>
    - 邀请 Primary + Secondary On-Call
    - 邀请 Manager On-Call
    - 频道 Topic 设置为 Incident ID
    - Pin 初始告警信息

规则 2: 多服务连锁故障
  触发: 5 分钟内 ≥ 3 个服务同时告警
  动作: 自动升级为 P0 并创建 War Room

规则 3: 手动请求
  触发: On-Call 工程师发送 /warroom 命令
  动作: 立即创建 War Room 并通知相关人员
```

### 1.2 手动启动

On-Call 工程师在以下场景应主动启动 War Room：

- 初步诊断发现影响面超出预期
- 需要多个团队协作排查
- 根因不明且影响持续扩大
- 修复操作需要多方确认

## 2. 角色分工

### 2.1 角色定义

```
┌─────────────────────────────────────────────────────┐
│                   War Room 角色                      │
├──────────┬──────────────────────────────────────────┤
│ IC       │ Incident Commander — 指挥官               │
│          │ 全局决策、资源调度、进度把控               │
├──────────┼──────────────────────────────────────────┤
│ Scribe   │ 记录员                                    │
│          │ 时间线记录、关键决策记录、状态更新         │
├──────────┼──────────────────────────────────────────┤
│ Comms    │ Communications — 对外沟通                 │
│          │ 客户通知、管理层汇报、StatusPage 更新     │
├──────────┼──────────────────────────────────────────┤
│ L2       │ Level 2 技术专家                          │
│          │ 深度排查、修复方案制定、操作执行           │
└──────────┴──────────────────────────────────────────┘
```

### 2.2 IC（Incident Commander）职责

```
IC 核心职责:

1. 全局掌控
   - 确认事件严重级别
   - 确定优先排查方向
   - 决定是否需要更多资源

2. 进度管控
   - 每 15 分钟触发状态更新
   - 确保各方向有明确 Owner
   - 识别阻塞点并推动解决

3. 决策权
   - 批准/否决修复操作
   - 决定是否需要回滚
   - 决定是否需要降级服务
   - 决定何时关闭事件

4. 资源调度
   - 请求额外 L2 支持
   - 协调跨团队资源
   - 决定是否需要厂商支持
```

### 2.3 Scribe 职责

```
Scribe 核心职责:

1. 时间线记录
   - 每个关键动作记录时间戳
   - 记录操作结果和观察到的变化
   - 格式: [HH:MM:SS] <执行人> <动作> <结果>

2. 状态同步
   - 每 15 分钟发布状态摘要
   - 更新 Incident Ticket
   - 更新 StatusPage（如需要）

3. 决策记录
   - 记录每个关键决策及其理由
   - 记录被否决的方案及原因
   - 为事后复盘保留完整上下文
```

### 2.4 Comms 职责

```
Comms 核心职责:

1. 内部沟通
   - 向管理层发送定时汇报
   - 通知受影响的内部团队
   - 协调客服团队准备话术

2. 外部沟通
   - 更新 StatusPage
   - 发布客户通知（初始/进展/恢复）
   - 准备 FAQ 和 Workaround 指引

3. 沟通节奏
   - P0: 每 30 分钟外部更新
   - P1: 每 1 小时外部更新
   - 关键里程碑立即更新
```

### 2.5 L2 技术专家职责

```
L2 核心职责:

1. 深度排查
   - 执行复杂诊断命令
   - 分析日志、指标、Trace
   - 定位根因

2. 方案制定
   - 制定修复方案（含风险评估）
   - 准备回滚方案
   - 向 IC 申请执行批准

3. 操作执行
   - 执行经批准的修复操作
   - 验证修复效果
   - 确认服务恢复正常
```

## 3. War Room 启动检查清单

```
War Room 启动 Checklist:

□ 创建专用 Slack 频道
□ 设置频道 Topic（Incident ID + 简述）
□ 确认 IC 到位
□ 确认 Scribe 到位
□ 确认至少 1 名 L2 到位
□ 如为 P0，确认 Comms 到位
□ Pin 初始告警和相关信息
□ 创建共享文档（Google Doc / Notion）
□ 确认所有参与者音视频正常
□ 开始录制（如需要）
```

## 4. 协作流程

### 4.1 标准协作时序

```
T+0min    事件触发，自动创建 War Room
  │
  ▼
T+5min    IC 确认角色分工，明确当前状态
  │       Scribe 开始记录时间线
  │
  ▼
T+10min   IC 组织快速评估
  │       - 影响范围确认
  │       - 初步根因假设
  │       - 分配排查方向
  │
  ▼
T+15min   第一次状态更新
  │       Scribe 发布摘要到频道
  │
  ▼
T+30min   每 15 分钟循环:
  │       - 各方向汇报进展
  │       - IC 调整策略
  │       - Scribe 更新记录
  │       - Comms 更新外部状态（如需要）
  │
  ▼
修复完成  IC 确认恢复
  │       - L2 验证指标正常
  │       - Scribe 记录恢复时间
  │       - Comms 发布恢复通知
  │
  ▼
关闭事件  IC 宣布关闭
          - Scribe 整理完整时间线
          - 安排事后复盘会议
```

### 4.2 并行排查模式

```
多方向并行排查:

当根因不明时，IC 应同时分配多个排查方向:

方向 A: 网络层排查
  Owner: L2-Network
  排查: DNS / Service Mesh / Ingress

方向 B: 应用层排查
  Owner: L2-App
  排查: Pod 状态 / 日志 / 配置

方向 C: 基础设施排查
  Owner: L2-Infra
  排查: 节点状态 / 存储 / 控制面

每 10 分钟各方向汇报:
  - 已排除什么
  - 发现了什么
  - 需要什么支持

IC 根据汇报决定聚焦哪个方向
```

## 5. 跨团队协调

### 5.1 协调触发条件

```
需要跨团队协调的场景:

场景 1: 根因在其他团队负责的系统
  动作: IC 通过 Escalation Chain 联系对方 On-Call
  要求: 提供明确的现象描述和已排查结论

场景 2: 修复需要多方操作
  动作: 制定操作序列，明确每步执行人
  要求: 每步操作前确认，操作后验证

场景 3: 影响范围跨多个业务线
  动作: Comms 通知所有相关业务方
  要求: 统一口径，避免信息不一致
```

### 5.2 外部团队接入流程

```
外部团队接入 War Room:

Step 1: IC 发起邀请
  - 说明事件背景（2-3 句话）
  - 说明需要什么帮助
  - 提供 War Room 频道链接

Step 2: 对方接入后
  - Scribe 提供当前状态摘要
  - IC 明确对方的职责范围
  - 分配排查/修复方向

Step 3: 协作期间
  - 对方参与状态汇报循环
  - 对方操作需经 IC 批准
  - Scribe 记录对方的所有操作
```

### 5.3 厂商协调

```
云厂商/第三方厂商协调:

何时联系:
  - 问题定位在厂商托管的服务
  - 需要厂商侧操作（如扩容配额）
  - 需要厂商提供诊断信息

联系方式:
  - 优先使用企业支持工单（P0 通道）
  - 同时使用 TAM（Technical Account Manager）直接联系
  - 在 War Room 中同步厂商反馈

信息传递:
  - 提供: 集群 ID、Region、时间范围、错误信息
  - 要求: ETA、Root Cause、Action Items
```

## 6. 信息同步机制

### 6.1 频道规范

```
Slack 频道结构:

#warsroom-YYYYMMDD-HHMM-<简述>
  │
  ├── 主频道: 实时讨论、决策、状态更新
  │
  ├── Thread 1: 时间线（仅 Scribe 发布）
  │
  ├── Thread 2: 操作记录（每次操作一个回复）
  │
  └── Thread 3: 相关链接（Dashboard、日志、文档）
```

### 6.2 状态更新模板

```markdown
## [HH:MM] 状态更新 #N

**事件**: <一句话描述>
**级别**: P0/P1
**持续时间**: X 分钟/小时
**当前状态**: 排查中 / 已定位 / 修复中 / 已恢复

**影响范围**:
- 受影响服务: <列表>
- 受影响用户比例: X%
- 业务损失: 评估中 / ¥X

**当前进展**:
- <方向 1>: <结论>
- <方向 2>: <结论>

**下一步**:
- <计划操作 1>
- <计划操作 2>

**需要支持**:
- <如有>
```

### 6.3 沟通工具矩阵

| 场景 | 工具 | 频率 |
|------|------|------|
| 实时讨论 | Slack 频道 | 持续 |
| 深度排查 | 音视频会议 | 持续 |
| 状态更新 | Slack Thread | 每 15 min |
| 管理层汇报 | Email / 飞书 | 每 30 min |
| 客户通知 | StatusPage | 每 30 min |
| 操作审批 | Slack 频道 + IC 确认 | 每次操作前 |

## 7. 决策记录

### 7.1 决策记录格式

```markdown
## 决策记录

### DEC-001: 执行数据库回滚
- **时间**: 14:32
- **决策人**: IC (engineer-a)
- **背景**: 确认根因为 DDL 变更导致索引失效
- **方案**: 回滚至变更前的数据库版本
- **风险**: 会丢失变更后的 2 分钟写入数据
- **备选方案**: 在线重建索引（预计 2 小时）
- **决策理由**: 数据丢失可控，在线重建时间不可接受
- **批准**: IC 批准，L2 确认可执行
```

### 7.2 关键决策点

```
必须记录的决策:

□ 严重级别确认/变更
□ 排查方向选择（排除其他方向的原因）
□ 修复方案选择（备选方案及比较）
□ 回滚决定
□ 服务降级决定
□ 客户沟通内容和时机
□ 事件关闭决定
```

## 8. War Room 关闭流程

### 8.1 关闭条件

```
关闭 Checklist:

□ 服务恢复正常运行 ≥ 15 分钟
□ 核心指标回到基线水平
□ 无残余告警
□ 影响范围已确认（无遗漏）
□ Scribe 完成时间线整理
□ Comms 已发送恢复通知
□ 事后复盘会议已安排（24-48 小时内）
□ Incident Ticket 已更新
```

### 8.2 关闭仪式

```
IC 宣布关闭:

"事件 [ID] 已恢复，War Room 关闭。
 恢复时间: [HH:MM]
 总持续时间: X 小时 Y 分钟
 事后复盘: [日期时间]
 感谢各位的协作。"

关闭后:
  - 将频道设为只读（保留记录）
  - Scribe 提交完整事件报告
  - IC 确认复盘会议邀请已发送
```

## 9. 工具与自动化

### 9.1 War Room Bot

```python
# warroom_bot.py — 自动化 War Room 管理
class WarRoomBot:
    def __init__(self, slack_client):
        self.slack = slack_client

    def create_warroom(self, incident):
        """创建 War Room 频道并邀请相关人员"""
        channel_name = f"warroom-{incident.timestamp}-{incident.slug}"
        channel = self.slack.create_channel(channel_name)

        # 邀请角色
        self.slack.invite(channel, incident.ic)
        self.slack.invite(channel, incident.secondary_oncall)
        if incident.severity == "P0":
            self.slack.invite(channel, incident.manager_oncall)
            self.slack.invite(channel, incident.comms)

        # 设置 Topic
        self.slack.set_topic(channel, f"[{incident.id}] {incident.summary}")

        # Pin 初始信息
        self.slack.pin_message(channel, incident.initial_alert)

        return channel

    def auto_status_reminder(self, channel, interval_minutes=15):
        """定时提醒 IC 发布状态更新"""
        pass
```

### 9.2 与 Kubernetes 事件集成

```yaml
# 自动 War Room 创建的 PrometheusRule
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: warroom-trigger
spec:
  groups:
    - name: warroom
      rules:
        - alert: CascadeFailureDetected
          expr: |
            count(ALERTS{alertstate="firing", severity="critical"}) >= 3
          for: 2m
          labels:
            action: create-warroom
          annotations:
            summary: "检测到连锁故障，建议创建 War Room"
```

---

*本文档定义 P0/P1 事件期间 War Room 的组织和协作规范。所有 On-Call 和 L2 人员应熟悉角色职责和协作流程。*


<!-- risk-assessed -->
