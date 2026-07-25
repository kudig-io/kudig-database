---
title: 错误预算策略模板
description: 组织级错误预算策略模板：角色定义、决策规则、评审节奏与例外流程
summary: 可直接套用的错误预算策略模板，定义谁在何时因预算状态做什么决策
category: reliability
tags:
- slo
- sli
- reliability
- error-budget
- policy
- governance
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

# 错误预算策略模板

> **核心原则**：错误预算策略是组织契约，不是技术文档。它定义"谁有权在预算紧张时按下暂停键"——如果没有这样的权力分配，SLO 就只是仪表盘上的装饰。把这份策略签进团队 OKR，让产品、研发、SRE 三方都认可，预算才能真正约束行为。

## 策略文档头

```markdown
# <组织> 错误预算策略 v1.0
- 生效日期: 2026-07-11
- 评审周期: 季度
- 拥有者: SRE 团队
- 审批人: CTO / 各产品线负责人
- 适用范围: 所有标记为"核心"的服务
```

## 1. SLO 与预算定义

每个核心服务必须登记以下字段：

| 字段 | 示例 |
|------|------|
| 服务名 | payment-api |
| SLO | 99.9% 可用性（30 天滚动） |
| SLI 定义 | `1 - 5xx率` |
| 错误预算 | 30 天允许 43.2 分钟不可用 |
| 责任团队 | 支付团队 |
| 升级 owner | 支付 TL + SRE on-call |

**规则**：没有登记 SLO 的服务不享受错误预算保护，也不受本策略约束——但默认按 99% 处理。

## 2. 预算状态与决策矩阵

```
错误预算剩余     研发行为            SRE 行为             产品行为
─────────────────────────────────────────────────────────
> 50%           正常迭代             正常发布              正常上功能
25–50%          正常 + 加监控        正常发布              功能上但谨慎
0–25%           仅修复 Bug           仅 Sev1 修复           暂停新功能
< 0%            全面冻结             全面冻结              优先可靠性
```

**冻结定义**：
- 不允许发布非 Sev1 变更
- 不允许进行有风险的容量调整
- 不允许破坏性混沌实验
- 自动化执行见 [[12-可靠性/06-SRE实践/05-error-budget-automation.md]]

## 3. 角色与权责

| 角色 | 权限 |
|------|------|
| **SRE** | 宣布"发布冻结"、强制可靠性项目排期 |
| **产品负责人** | 在预算耗尽时，可申请"功能例外"（需 SRE 同意） |
| **研发 TL** | 执行冻结，组织可靠性改进 |
| **CTO** | 仲裁 SRE 与产品的冲突 |

**冲突仲裁**：SRE 说冻结、产品说必须发——升级到 CTO，CTO 默认支持 SRE（可靠性优先），除非业务有书面合规/收入理由。

## 4. 例外（Break-Glass）流程

紧急情况下可绕过冻结，但每次例外必须：
1. 在发布前于 Slack `#release-override` 发声明
2. 自动开审计工单（Jira/Linear）
3. 24 小时内补复盘，说明为什么需要绕过
4. 累计例外超 3 次/季度 → 强制可靠性项目介入

```yaml
# 例外标签（CI/CD）
labels:
  override-reason: "Sev1 hotfix - payment 5xx spike"
  approver: "@sre-lead"
  ticket: "PAY-1234"
```

## 5. 评审节奏

- **周度**：SRE 与各产品 TL 过预算看板（15 分钟），关注快速燃烧的服务。
- **月度**：SLO 委员会评审 SLO 达成率、是否调整 SLO 目标、策略例外数。
- **季度**：CTO 评审整体可靠性、策略有效性、是否升级/降级服务等级。

**SLO 调整规则**：
- 连续 3 个月轻松达成 → 可考虑收紧（如 99.9% → 99.95%），释放更多迭代空间。
- 连续 2 个月未达成 → 不放松 SLO，而是投入可靠性改进；放松 SLO 是最后手段。

## 6. 自动化执行挂钩

策略需配合自动化才有效（详见 [[12-可靠性/06-SRE实践/05-error-budget-automation.md]]）：

- CI 流水线门控：预算 < 25% 自动阻断非 Sev1 PR。
- 准入控制：预算 < 0% 自动拒绝 Deployment 创建。
- 告警：预算 < 25% 自动通知产品 TL。

**没有自动化的策略 = 没有策略**。

## 7. 度量策略自身有效性

每季度回答：
1. 冻结次数 vs 实际执行冻结次数（差距大 = 策略没被遵守）
2. 例外次数趋势（上升 = SLO 设得太松或可靠性债在累积）
3. SLO 达成率（持续 100% = SLO 太松；持续 < 70% = SLO 太严或可靠性不足）

## 实施清单

- [ ] 核心服务清单与 SLO 已登记
- [ ] 决策矩阵已与产品/研发对齐并签字
- [ ] 冲突仲裁路径已明确（CTO）
- [ ] 例外流程已自动化（CI 标签 + 工单）
- [ ] 自动化门控已部署（CI + 准入）
- [ ] 评审日历已设置（周/月/季）
- [ ] 策略已纳入新员工 onboarding

## 错误预算计算实战

### 多服务预算计算

```python
#!/usr/bin/env python3
"""错误预算计算器 - 支持多服务多窗口"""

from dataclasses import dataclass
from datetime import datetime, timedelta

@dataclass
class SLODefinition:
    service: str
    target: float  # 如 0.999 = 99.9%
    window_days: int = 30

def calculate_error_budget(slo: SLODefinition, actual_availability: float):
    """计算错误预算状态"""
    budget_total = 1 - slo.target  # 总预算
    budget_used = 1 - actual_availability  # 已使用
    budget_remaining = budget_total - budget_used  # 剩余
    budget_percent = (budget_remaining / budget_total) * 100  # 剩余百分比
    
    # 计算允许停机时间
    window_minutes = slo.window_days * 24 * 60
    allowed_downtime = window_minutes * budget_total
    used_downtime = window_minutes * budget_used
    remaining_downtime = window_minutes * budget_remaining
    
    return {
        "service": slo.service,
        "target": f"{slo.target*100:.2f}%",
        "actual": f"{actual_availability*100:.2f}%",
        "budget_total_minutes": round(allowed_downtime, 1),
        "budget_used_minutes": round(used_downtime, 1),
        "budget_remaining_minutes": round(remaining_downtime, 1),
        "budget_remaining_percent": round(budget_percent, 1),
        "status": get_budget_status(budget_percent)
    }

def get_budget_status(percent: float) -> str:
    if percent > 50:
        return "🟢 正常"
    elif percent > 25:
        return "🟡 警告"
    elif percent > 0:
        return "🟠 紧张"
    else:
        return "🔴 耗尽"

# 示例使用
services = [
    SLODefinition("payment-api", 0.9999),
    SLODefinition("order-service", 0.999),
    SLODefinition("user-service", 0.999),
    SLODefinition("search-service", 0.995),
]

# 模拟实际可用性
actuals = [0.9998, 0.9985, 0.9992, 0.9970]

print("=== 错误预算报告 ===")
for slo, actual in zip(services, actuals):
    result = calculate_error_budget(slo, actual)
    print(f"\n{result['service']}:")
    print(f"  目标: {result['target']}, 实际: {result['actual']}")
    print(f"  预算剩余: {result['budget_remaining_minutes']}min ({result['budget_remaining_percent']}%)")
    print(f"  状态: {result['status']}")
```

### PromQL 预算计算

```promql
# 30 天滚动窗口错误预算剩余
1 - (
  sum(increase(http_requests_total{job="api-gateway", status=~"5.."}[30d]))
  /
  sum(increase(http_requests_total{job="api-gateway"}[30d]))
) / (1 - 0.999)

# 按服务分解的预算消耗
sum by (service) (
  increase(http_requests_total{status=~"5.."}[30d])
) / sum by (service) (
  increase(http_requests_total[30d])
)

# 预算消耗速度（每分钟）
sum(rate(http_requests_total{status=~"5.."}[1h]))
/
sum(rate(http_requests_total[1h]))
/
(1 - 0.999)
```

## 预算消耗归因分析

### 归因分类

| 归因类别 | 定义 | 示例 | 责任方 |
|---------|------|------|-------|
| **计划内变更** | 发布导致的错误 | 新版本 Bug、配置错误 | 研发团队 |
| **基础设施故障** | 底层设施问题 | 节点宕机、网络分区 | SRE/云平台 |
| **依赖服务故障** | 第三方服务问题 | 支付网关、短信服务 | 供应商管理 |
| **容量不足** | 资源耗尽 | CPU/内存/连接池满 | 容量规划 |
| **未知/未分类** | 无法确定原因 | 偶发错误 | 待分析 |

### 归因分析模板

```markdown
# 错误预算消耗归因报告

## 时间范围: 2026-07-01 ~ 2026-07-31

## 消耗汇总
- 总预算: 43.2 分钟
- 已消耗: 28.5 分钟 (66%)
- 剩余: 14.7 分钟 (34%)

## 归因分解
| 事件 | 时间 | 消耗 | 归因 | 责任方 | 改进措施 |
|-----|------|------|------|-------|----------|
| 发布 v2.3.1 | 07-05 | 12min | 计划内变更 | 研发 | 加强测试覆盖 |
| 节点故障 | 07-12 | 8min | 基础设施 | SRE | 增加节点冗余 |
| 支付网关超时 | 07-18 | 5.5min | 依赖服务 | 供应商 | 增加超时重试 |
| 内存泄漏 | 07-25 | 3min | 容量不足 | 研发 | 修复泄漏 |

## 趋势分析
- 计划内变更占比: 42% (目标 < 30%)
- 基础设施占比: 28% (目标 < 20%)
- 依赖服务占比: 19% (目标 < 25%)
- 容量不足占比: 11% (目标 < 15%)

## 改进行动
1. [ ] 研发: 加强发布前测试，减少变更引入故障
2. [ ] SRE: 评估节点冗余策略
3. [ ] 架构: 依赖服务降级方案
```

## 跨团队协作机制

### 预算共享与转移

```
场景: 服务 A 依赖服务 B

服务 A SLO: 99.9% (预算 43.2min)
服务 B SLO: 99.9% (预算 43.2min)

问题: B 故障 10min → A 也故障 10min
归因: B 消耗 A 的预算 10min

解决方案:
1. 依赖预算预留: A 预留 30% 预算给依赖故障
2. 依赖 SLO 约束: B 的 SLO 必须 ≥ A 的 SLO
3. 降级策略: B 故障时 A 可降级运行
```

### 跨团队预算协议

| 条款 | 内容 |
|-----|------|
| **依赖披露** | 服务必须披露关键依赖及其 SLO |
| **预算预留** | 为依赖故障预留 20-30% 预算 |
| **故障通知** | 依赖故障时 5min 内通知下游 |
| **联合复盘** | 跨服务故障必须联合复盘 |
| **SLO 对齐** | 上游 SLO 不得低于下游 SLO |

## 策略演进路线图

### 成熟度阶段

```
阶段 1: 手动管理 (Month 1-3)
  ├─ 手动计算错误预算
  ├─ 邮件/会议通知预算状态
  └─ 人工执行冻结决策

阶段 2: 半自动化 (Month 4-6)
  ├─ Prometheus 自动计算预算
  ├─ 自动发送预算告警
  └─ 手动确认冻结

阶段 3: 自动化门控 (Month 7-12)
  ├─ CI/CD 自动门控
  ├─ 准入控制自动拦截
  └─ 自动归因分析

阶段 4: 智能化 (Year 2+)
  ├─ 预测性预算告警
  ├─ 智能发布窗口建议
  └─ AI 辅助归因
```

### 各阶段关键交付物

| 阶段 | 交付物 | 验收标准 |
|-----|-------|----------|
| 1 | SLO 定义文档 | 核心服务 100% 覆盖 |
| 1 | 预算计算表格 | 每周更新 |
| 2 | Prometheus 告警规则 | Burn Rate 告警生效 |
| 2 | Grafana Dashboard | 预算可视化 |
| 3 | CI/CD 门控 | 自动阻断超预算发布 |
| 3 | 归因报告自动化 | 每周自动生成 |
| 4 | 预测模型 | 提前 7 天预警预算耗尽 |

## 策略模板 YAML 化

### 可执行策略定义

```yaml
apiVersion: policy.kudig.io/v1
kind: ErrorBudgetPolicy
metadata:
  name: organization-error-budget-policy
  namespace: governance
spec:
  version: "1.0"
  effectiveDate: "2026-07-11"
  reviewCycle: quarterly
  owner: sre-team
  approver: cto

  # 默认 SLO（未登记服务）
  defaultSLO:
    availability: 99.0
    window: 30d

  # 预算状态与行动
  budgetStates:
    - name: normal
      range: "> 50%"
      actions:
        development: normal
        release: normal
        product: normal

    - name: warning
      range: "25% - 50%"
      actions:
        development: normal_with_monitoring
        release: normal
        product: cautious

    - name: critical
      range: "0% - 25%"
      actions:
        development: bugfix_only
        release: sev1_only
        product: feature_freeze

    - name: exhausted
      range: "< 0%"
      actions:
        development: full_freeze
        release: full_freeze
        product: reliability_first

  # 例外流程
  exceptions:
    channel: "#release-override"
    requireApproval: true
    postMortemDeadline: 24h
    maxPerQuarter: 3

  # 自动化挂钩
  automation:
    ciGate:
      enabled: true
      threshold: 25%
    admissionControl:
      enabled: true
      threshold: 0%
    notifications:
      - type: slack
        channel: "#slo-alerts"
        threshold: 25%
      - type: email
        recipients: ["product-leads@company.com"]
        threshold: 25%
```

## 常见问题 FAQ

| 问题 | 解答 |
|-----|------|
| **Q: SLO 应该多严格？** | 从历史 P90 开始，逐步收紧。不要一开始就定 99.99%。 |
| **Q: 预算耗尽怎么办？** | 冻结非紧急变更，全力投入可靠性改进。不要放松 SLO。 |
| **Q: 产品坚持要发布怎么办？** | 升级到 CTO 仲裁。默认支持 SRE，除非有书面业务理由。 |
| **Q: 依赖服务故障算谁的？** | 归因给依赖服务，但下游应预留依赖预算并实现降级。 |
| **Q: 多久回顾一次 SLO？** | 月度回顾达成率，季度调整目标。连续 3 月轻松达标可收紧。 |
| **Q: 测试环境需要 SLO 吗？** | 不需要。SLO 只针对生产环境。测试环境用可用性目标即可。 |
| **Q: 多个 SLI 如何综合？** | 分别设定，取最严格的。可用性和延迟通常是独立的。 |

## 相关

- [[12-可靠性/06-SRE实践/05-error-budget-automation.md|05 error budget automation]]
- [[12-可靠性/06-SRE实践/02-release-gate-slo-based.md|02 release gate slo based]]
- [[12-可靠性/06-SRE实践/03-slo-sli-guide.md|03 slo sli guide]]
- [[12-可靠性/06-SRE实践/05-reliability-maturity-model.md|05 reliability maturity model]]

<!-- risk-assessed -->
