---
title: Game Day 作战手册模板
description: Game Day（故障演练日）的标准 runbook 模板与执行 Checklist，从规划到复盘全流程
summary: 规划→基线→演练→验证→复盘五阶段模板 + 角色/通讯/中止条件/证据采集 Checklist
category: reliability
tags:
- slo
- sli
- reliability
- chaos-engineering
- game-day
- runbook
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

# Game Day 作战手册模板

> **核心原则**：Game Day 不是"看系统能不能扛"，而是"提前在白天、清醒、有准备的状态下发现真实事故才会暴露的弱点"。它的产物不是"我们演练过了"的截图，而是一份**可执行的问题清单 + 改进工单**。演练结束不等于完成——修复上线才算完成。

## Game Day 五阶段

```
规划(T-2w) ──▶ 基线(T-1w) ──▶ 演练(T) ──▶ 验证(T) ──▶ 复盘(T+2d)
   场景设计      基线指标采集   注入+观察    SLO 校验    blameless 复盘
```

## 元信息模板（文档头）

```markdown
# Game Day: <名称>
- 日期：2026-07-11 14:00–17:00 (UTC+8)
- 范围：<服务/集群>
- 类型：□ Pod 故障  □ 网络分区  □ 依赖失效  □ AZ 故障  □ 数据库故障
- Game Master：<姓名>
- 参演角色：IC/Ops/Comms/Scribe/Observer
- 风险等级：□ Staging  □ Prod 小流量  □ Prod 全量
- 中止条件（见下）
```

## 阶段一：规划（T-2 周）

### 场景定义 Checklist

- [ ] 用一句话描述稳态假设（"P99 < 200ms 且错误率 < 0.1%"）
- [ ] 列出注入变量（"随机杀 30% api pod，持续 5 分钟"）
- [ ] 定义成功/失败标准（可量化）
- [ ] 定义**中止条件**（红线，触发即停）

```
中止红线（任一触发立即停止实验并回滚）：
  - 错误率 > 5%（> 30s）
  - P99 延迟 > 2s（> 60s）
  - 客户投诉 > 3 起
  - Game Master 判定风险升级
```

- [ ] 回滚方案（命令清单，预演过）
- [ ] 通知干系人（客户/高管/支持团队）

## 阶段二：基线（T-1 周）

```bash
# 🟢 只读：采集基线指标快照
kubectl -n monitoring port-forward svc/prometheus 9090
./capture-baseline.sh --service api --window 30m > baseline.json
```

记录：正常 RPS、P50/P95/P99、错误率、CPU/内存水位、副本数。这些是演练对比的参照系。

## 阶段三：演练（T 日）

### 时间线模板

```
T-0:00  Game Master 宣读场景与中止条件，所有人确认就位
T+0:05  启动负载测试（k6，模拟峰值流量）
T+0:10  确认负载下稳态正常
T+0:15  🔴 注入故障（Chaos Mesh）
T+0:16  Observer 开始逐分钟记录指标
T+0:20  IC 判断：是否达预期？是否触发中止？
T+0:25  停止注入，观察恢复
T+0:30  验证系统自愈（无人工干预）
T+0:35  圆桌快速讨论：观察到了什么
T+0:40  进入下一场景 或 结束
```

### 角色分工

| 角色 | 职责 | 是否可演戏 |
|------|------|-----------|
| Game Master | 全权控制节奏、喊停 | 否（裁判） |
| IC | 现场指挥、go/no-go | 是（参演） |
| Ops | 执行命令 | 是 |
| Observer | 只记录不干预 | 否 |
| Comms | 对外公告 | 是 |

## 阶段四：验证（SLO 校验）

```bash
# 🟢 只读：对比基线与演练期指标
./compare-slo.sh --baseline baseline.json --during drill-window.json
```

输出：
```
[PASS] 错误率峰值 0.8% < 阈值 1%
[FAIL] P99 峰值 2.3s > 阈值 1s   ← 发现问题 #1
[PASS] 自愈时间 45s < 目标 90s
[PASS] 无人工干预
```

## 阶段五：复盘（T+2 天内，blameless）

模板见 [[可靠性/事后复盘/01-blameless-postmortem-template.md]]，产出：
1. 时间线（Scribe 记录逐分钟）
2. 发现的问题清单（每条带证据：指标截图 + 日志链接）
3. 改进工单（带 owner 与截止日期，进 sprint）
4. 可复用的检测/告警规则

## 常见陷阱

1. **没有中止条件**：红线不写清，出事就乱。每个场景必须有"立即停"的标准。
2. **演练目标定太满**：第一次 Game Day 就搞全量 AZ 故障，出事概率高、收益低。从 Pod 级别逐步升级。
3. **只测不修**：发现问题进备忘录就完了——必须开工单排期，下个 Game Day 验证修复。
4. **.prod 演练不通知客户**：除非有变更窗口豁免，否则务必状态页预告，避免被当真实事故响应。

## 相关

- [[可靠性/混沌工程/06-game-day-runbook-template.md|self]]
- [[可靠性/混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[可靠性/性能测试/02-chaos-load-integration.md|02 chaos load integration]]
- [[可靠性/事后复盘/01-blameless-postmortem-template.md|01 blameless postmortem template]]

<!-- risk-assessed -->
