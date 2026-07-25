---
title: 变更冻结策略
description: '定义冻结期间、例外审批流程（Emergency Change）、冻结前检查清单及冻结期间允许的操作类型'
summary: '定义冻结期间、例外审批流程（Emergency Change）、冻结前检查清单及冻结期间允许的操作类型'
category: production-operations
tags:
- production
- operations
- change-management
- freeze
- emergency-change
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
- 变更冻结策略 是什么
- 如何 实施变更冻结
- 如何 处理紧急变更
trigger_keywords:
- freeze
- change-management
- emergency
- blackout
- deployment
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


# 变更冻结策略

## 1. 变更冻结概述

### 1.1 为什么需要变更冻结

```
变更冻结的目的:

1. 稳定性保障
   - 减少故障风险
   - 确保关键业务时段稳定运行
   - 给 On-Call 团队休息时间

2. 风险控制
   - 避免多变更叠加导致复杂故障
   - 确保有足够人力处理突发问题
   - 降低回滚复杂度

3. 合规要求
   - 满足审计要求
   - 遵循行业最佳实践
   - 支持业务连续性计划
```

### 1.2 典型冻结场景

```
计划冻结场景:

1. 节假日冻结
   - 春节: 7-10 天
   - 国庆: 7 天
   - 其他法定假日: 1-3 天

2. 业务高峰期冻结
   - 电商大促（618、双11、双12）
   - 年终结算
   - 新产品发布后 48 小时

3. 重大活动保障
   - 公司重要发布会
   - 大型线上活动
   - 合规审计期间

4. 系统维护窗口
   - 数据库迁移
   - 网络割接
   - 核心系统升级
```

## 2. 冻结期间定义

### 2.1 冻结级别

| 级别 | 名称 | 限制范围 | 审批要求 | 适用场景 |
|------|------|---------|---------|---------|
| **Level 1** | 完全冻结 | 所有变更 | VP+ 审批 | 重大节假日、核心系统故障 |
| **Level 2** | 严格冻结 | 仅允许紧急修复 | Director 审批 | 业务高峰期、审计期间 |
| **Level 3** | 受限冻结 | 低风险变更可执行 | Manager 审批 | 一般节假日、常规保障 |

### 2.2 冻结日历

```yaml
# freeze-calendar.yaml
freeze_calendar:
  - name: "2026 春节冻结"
    start: "2026-01-26T00:00:00+08:00"
    end: "2026-02-02T23:59:59+08:00"
    level: 1
    approver: "vp-engineering"
    notification:
      - "2026-01-19"  # 提前 7 天通知
      - "2026-01-23"  # 提前 3 天提醒
    affected_services: "all"

  - name: "2026 年中大促冻结"
    start: "2026-06-17T18:00:00+08:00"
    end: "2026-06-19T06:00:00+08:00"
    level: 2
    approver: "director-platform"
    notification:
      - "2026-06-10"
      - "2026-06-16"
    affected_services:
      - "order-service"
      - "payment-service"
      - "inventory-service"
      - "api-gateway"

  - name: "2026 双 11 冻结"
    start: "2026-11-10T12:00:00+08:00"
    end: "2026-11-12T12:00:00+08:00"
    level: 2
    approver: "director-platform"
    affected_services: "all"
```

### 2.3 冻结状态自动化检查

```python
# freeze_checker.py
from datetime import datetime, timezone, timedelta
import yaml

class FreezeChecker:
    def __init__(self, calendar_file="freeze-calendar.yaml"):
        with open(calendar_file) as f:
            self.calendar = yaml.safe_load(f)["freeze_calendar"]

    def is_frozen(self, service=None):
        """检查当前是否处于冻结期"""
        now = datetime.now(timezone(timedelta(hours=8)))

        for freeze in self.calendar:
            start = datetime.fromisoformat(freeze["start"])
            end = datetime.fromisoformat(freeze["end"])

            if start <= now <= end:
                if freeze.get("affected_services") == "all":
                    return {
                        "frozen": True,
                        "level": freeze["level"],
                        "name": freeze["name"],
                        "approver": freeze["approver"]
                    }
                elif service and service in freeze.get("affected_services", []):
                    return {
                        "frozen": True,
                        "level": freeze["level"],
                        "name": freeze["name"],
                        "approver": freeze["approver"]
                    }

        return {"frozen": False}

    def get_upcoming_freezes(self, days=30):
        """获取即将到来的冻结期"""
        now = datetime.now(timezone(timedelta(hours=8)))
        future = now + timedelta(days=days)

        upcoming = []
        for freeze in self.calendar:
            start = datetime.fromisoformat(freeze["start"])
            if now < start < future:
                upcoming.append(freeze)

        return upcoming
```

## 3. 冻结期间允许的操作

### 3.1 Level 1（完全冻结）允许的操作

```
Level 1 — 完全冻结:

允许:
  □ 安全补丁（Critical CVE 修复）
  □ P0 事件修复
  □ 数据备份操作
  □ 监控告警处理
  □ 文档更新（非代码）

禁止:
  ✗ 任何代码变更
  ✗ 配置变更
  ✗ 基础设施变更
  ✗ 依赖升级
  ✗ 数据库 Schema 变更
  ✗ 网络策略变更
```

### 3.2 Level 2（严格冻结）允许的操作

```
Level 2 — 严格冻结:

允许:
  □ Level 1 所有允许项
  □ P1 事件修复（需审批）
  □ 功能开关关闭（Feature Flag Off）
  □ 资源扩容（Horizontal Scaling）
  □ 日志级别调整
  □ 缓存刷新

需审批:
  △ Bug 修复（P2 及以上）
  △ 配置调优
  △ 监控规则调整

禁止:
  ✗ 新功能发布
  ✗ 架构变更
  ✗ 数据库 Schema 变更
  ✗ 依赖升级
```

### 3.3 Level 3（受限冻结）允许的操作

```
Level 3 — 受限冻结:

允许:
  □ Level 2 所有允许项
  □ 低风险 Bug 修复
  □ 配置变更（经审批）
  □ 文档和工具更新
  □ 开发/测试环境变更

需审批:
  △ 生产环境代码变更
  △ 基础设施小版本升级

禁止:
  ✗ 重大架构变更
  ✗ 数据库 Schema 变更
  ✗ 生产环境网络变更
```

## 4. 冻结前检查清单

### 4.1 冻结前 7 天

```
冻结前 7 天 Checklist:

通知与协调:
  □ 发布冻结通知（邮件 + Slack）
  □ 确认各团队已知悉
  □ 确认 On-Call 排班覆盖冻结期
  □ 与业务方确认无重大发布计划

变更清理:
  □ 排查未完成的变更
  □ 确认进行中的变更可安全暂停
  □ 清理临时性的 Feature Flag
  □ 确认无紧急依赖升级

监控与告警:
  □ 检查监控系统正常运行
  □ 确认告警通道畅通
  □ 检查 Dashboard 数据正常
  □ 确认日志采集正常
```

### 4.2 冻结前 3 天

```
冻结前 3 天 Checklist:

稳定性检查:
  □ 运行全量健康检查
  □ 确认所有服务状态正常
  □ 检查资源使用率（确保有余量）
  □ 验证备份完整性

变更状态:
  □ 所有计划变更已暂停或完成
  □ Git 分支保护已启用（如需要）
  □ CI/CD Pipeline 已标记冻结状态
  □ 变更审批系统已更新冻结规则

应急准备:
  □ 应急联系人列表已更新
  □ 应急操作手册已准备
  □ 回滚方案已验证
  □ 升级流程已确认
```

### 4.3 冻结前 1 天

```
冻结前 1 天 Checklist:

最终确认:
  □ 再次通知所有相关人员
  □ 确认 On-Call 交接完成
  □ 验证告警接收正常
  □ 确认应急工具可用

系统状态:
  □ 最终健康检查通过
  □ 资源充足（CPU/Memory/Disk）
  □ 网络连通性正常
  □ 证书有效期检查（冻结期内不会过期）
```

## 5. 紧急变更（Emergency Change）

### 5.1 紧急变更定义

```
紧急变更判定标准:

必须满足以下条件之一:

1. P0 事件修复
   - 生产环境完全不可用
   - 数据丢失或泄露
   - 安全漏洞（CVSS ≥ 9.0）

2. 合规/法律要求
   - 监管机构要求
   - 法律诉讼相关
   - 数据保护法规要求

3. 财务风险
   - 直接财务损失持续中
   - 影响核心交易链路
```

### 5.2 紧急变更审批流程

```
紧急变更审批流程:

Step 1: 发起申请
  - 提交 Emergency Change Request
  - 说明变更原因和紧急程度
  - 提供影响评估和回滚方案

Step 2: 快速审批
  Level 1 冻结:
    审批人: VP Engineering
    响应时间: ≤ 30 分钟

  Level 2 冻结:
    审批人: Director / Manager On-Call
    响应时间: ≤ 15 分钟

  Level 3 冻结:
    审批人: Manager On-Call
    响应时间: ≤ 15 分钟

Step 3: 执行变更
  - 至少 2 人执行（一人操作，一人验证）
  - 全程记录操作步骤
  - 准备好回滚方案

Step 4: 验证与关闭
  - 验证变更效果
  - 确认无副作用
  - 更新 Emergency Change 记录
  - 安排事后复盘
```

### 5.3 紧急变更申请模板

```markdown
# Emergency Change Request

## 基本信息
- **申请时间**: YYYY-MM-DD HH:MM UTC+8
- **申请人**: <姓名>
- **当前冻结级别**: Level 1/2/3
- **关联事件**: INC-YYYYMMDD-NNN

## 变更描述
- **变更内容**: <具体描述>
- **影响范围**: <受影响服务/组件>
- **变更原因**: <为什么必须立即执行>

## 风险评估
- **风险等级**: 高/中/低
- **潜在副作用**: <描述>
- **回滚方案**: <详细步骤>
- **回滚时间**: <预计时间>

## 审批
- **审批人**: <姓名>
- **审批时间**: YYYY-MM-DD HH:MM
- **审批结果**: 批准 / 拒绝
- **审批意见**: <如有>

## 执行记录
- **执行人**: <姓名>
- **执行时间**: YYYY-MM-DD HH:MM - HH:MM
- **执行步骤**: <详细记录>
- **执行结果**: 成功 / 失败 / 部分成功
- **验证结果**: <描述>

## 事后复盘
- **复盘日期**: YYYY-MM-DD
- **改进项**: <如有>
```

## 6. 冻结期间运营

### 6.1 On-Call 安排

```
冻结期 On-Call 安排:

排班要求:
  - Primary + Secondary 双人值班
  - 每班次 8 小时（可缩短为 4 小时）
  - 确保每个班次有明确交接

升级链路:
  Level 1 冻结:
    Primary → Secondary → Manager → Director → VP

  Level 2/3 冻结:
    Primary → Secondary → Manager

联络方式:
  - 主要: Slack #oncall-freeze
  - 紧急: 电话
  - 备份: 飞书/微信
```

### 6.2 监控加强

```
冻结期监控加强措施:

告警阈值调整:
  - 降低告警阈值（更敏感）
  - 增加告警通知人
  - 启用短信/电话告警

巡检频率:
  - 核心服务: 每 2 小时人工巡检
  - 资源使用率: 每 4 小时检查
  - 证书/密钥有效期: 每天检查

Dashboard 监控:
  - 全屏显示核心 Dashboard
  - 配置异常自动高亮
  - 历史趋势对比
```

### 6.3 冻结期日志

```
冻结期运营日志:

格式:
  [YYYY-MM-DD HH:MM] <事件类型> <描述> <处理人>

事件类型:
  - [HEALTH] 健康检查
  - [ALERT] 告警处理
  - [INCIDENT] 事件处理
  - [CHANGE] 紧急变更
  - [HANDOVER] 班次交接

示例:
  [2026-01-28 10:00] [HEALTH] 全量健康检查通过 - engineer-a
  [2026-01-28 14:30] [ALERT] CPU 告警，已确认为正常波动 - engineer-b
  [2026-01-28 18:00] [HANDOVER] 班次交接完成，无未关闭事件 - engineer-a → engineer-c
```

## 7. 冻结解除

### 7.1 解除条件

```
冻结解除 Checklist:

□ 冻结期已结束
□ 无未关闭的紧急变更
□ On-Call 排班恢复正常
□ 告警阈值恢复原值
□ 通知所有团队冻结解除
□ 更新冻结日历（标记完成）
```

### 7.2 解除后变更恢复

```
冻结解除后变更恢复:

优先级排序:
  1. 冻结期间积压的 P0/P1 修复
  2. 安全补丁
  3. 重要 Bug 修复
  4. 常规功能发布

恢复节奏:
  Day 1: 仅处理积压的紧急修复
  Day 2: 开始处理重要变更
  Day 3: 恢复正常发布节奏

注意事项:
  - 不要一次性发布所有积压变更
  - 分批发布，每批间隔 ≥ 2 小时
  - 加强发布后的监控
```

## 8. 工具集成

### 8.1 CI/CD Pipeline 集成

```yaml
# GitLab CI: 冻结期检查
stages:
  - freeze-check
  - build
  - test
  - deploy

check-freeze:
  stage: freeze-check
  script:
    - |
      python3 scripts/freeze_checker.py
      if [ $? -ne 0 ]; then
        echo "当前处于变更冻结期，部署被阻止"
        echo "如需紧急变更，请提交 Emergency Change Request"
        exit 1
      fi
  rules:
    - if: '$CI_COMMIT_BRANCH == "main"'
```

### 8.2 Kubernetes Admission Webhook

```python
# freeze_admission_webhook.py
from flask import Flask, request, jsonify
from freeze_checker import FreezeChecker

app = Flask(__name__)
checker = FreezeChecker()

@app.route('/validate', methods=['POST'])
def validate():
    admission_review = request.get_json()
    uid = admission_review["request"]["uid"]
    namespace = admission_review["request"]["namespace"]
    resource = admission_review["request"]["resource"]["resource"]
    name = admission_review["request"]["object"]["metadata"]["name"]

    # 检查是否处于冻结期
    freeze_status = checker.is_frozen()

    if freeze_status["frozen"] and freeze_status["level"] == 1:
        # Level 1 冻结：阻止所有变更
        return jsonify({
            "apiVersion": "admission.k8s.io/v1",
            "kind": "AdmissionReview",
            "response": {
                "uid": uid,
                "allowed": False,
                "status": {
                    "message": f"变更冻结中（{freeze_status['name']}）。如需紧急变更，请提交 Emergency Change Request。"
                }
            }
        })

    return jsonify({
        "apiVersion": "admission.k8s.io/v1",
        "kind": "AdmissionReview",
        "response": {
            "uid": uid,
            "allowed": True
        }
    })
```

### 8.3 Slack Bot 集成

```python
# freeze_slack_bot.py
from slack_bolt import App
from freeze_checker import FreezeChecker

app = App(token="xoxb-your-token")
checker = FreezeChecker()

@app.command("/freeze-status")
def handle_freeze_status(ack, say):
    ack()
    status = checker.is_frozen()

    if status["frozen"]:
        say(f"""
🧊 *当前处于变更冻结期*

冻结名称: {status['name']}
冻结级别: Level {status['level']}
审批人: {status['approver']}

如需紧急变更，请提交 Emergency Change Request。
        """)
    else:
        upcoming = checker.get_upcoming_freezes(days=7)
        if upcoming:
            next_freeze = upcoming[0]
            say(f"""
✅ 当前无变更冻结

⚠️ 即将到来的冻结:
- {next_freeze['name']}
- 开始: {next_freeze['start']}
- 级别: Level {next_freeze['level']}
            """)
        else:
            say("✅ 当前无变更冻结，未来 7 天也无计划冻结。")
```

## 9. 冻结期最佳实践

### 9.1 冻结前发布策略

```
冻结前发布窗口:

原则:
  - 冻结前 48 小时完成所有非紧急变更
  - 冻结前 24 小时仅处理稳定性修复
  - 冻结当天不进行任何变更

冻结前发布检查:
  □ 变更已通过所有测试
  □ 变更已在 Staging 验证
  □ 监控已就位
  □ 回滚方案已准备
  □ On-Call 已知悉变更内容
```

### 9.2 冻结期间开发

```
冻结期间开发工作安排:

可以进行:
  □ 代码编写和本地测试
  □ Code Review
  □ 文档编写
  □ 技术方案设计
  □ 开发/测试环境变更
  □ 性能测试（非生产）

暂停进行:
  ✗ 生产环境部署
  ✗ 基础设施变更
  ✗ 数据库迁移
```

### 9.3 冻结复盘

```
冻结期复盘（冻结解除后 3 天内）:

复盘内容:
  - 冻结期间发生的事件
  - 紧急变更执行情况
  - 冻结策略有效性评估
  - 改进建议

改进建议方向:
  - 冻结时长是否合理
  - 冻结级别是否适当
  - 审批流程是否高效
  - 监控覆盖是否充分
```

---

*本文档定义变更冻结的完整策略和执行流程。所有团队成员应熟悉冻结期间的规则和紧急变更流程，确保关键业务时段的稳定性。*


<!-- risk-assessed -->
