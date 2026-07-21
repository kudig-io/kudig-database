---
title: 事件指挥现场手册
description: Incident Commander 现场作战手册，覆盖严重度分级、角色分工、通讯节奏与升级路径
summary: Sev1–Sev4 分级 + IC/Comms/Ops/Scribe 四角色 + 固定通讯节奏的现场指挥 Checklist
category: reliability
tags:
- slo
- sli
- reliability
- incident-management
- incident-command
- oncall
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

# 事件指挥现场手册

> **IC 第一法则**：你的工作不是修 bug，是**让正确的人做正确的事，并让信息流通**。在事故里，一个冷静的 IC 比十个冲进终端的工程师更值钱。修系统是 Ops 角色的事，IC 永远站在白板前而不是终端前。

## 严重度分级

| Sev | 定义 | 响应时限 | 升级 | 通讯频率 |
|-----|------|---------|------|---------|
| **Sev1** | 核心服务全断 / 数据丢失 | 5 分钟 | 自动通知 VP+CTO | 每 15 分钟 |
| **Sev2** | 核心服务降级 / 非核心全断 | 15 分钟 | 通知 on-call manager | 每 30 分钟 |
| **Sev3** | 局部降级 / 有 workaround | 1 小时 | 工作时间升级 | 按需 |
| **Sev4** | 单点故障 / 无用户影响 | 4 小时 | 无 | 事后通报 |

**判定口诀**：用户付不了钱 = Sev1；用户用得难受 = Sev2；内部难受 = Sev3；只有你难受 = Sev4。

## 四角色分工

```
        ┌──────────────────────────┐
        │   Incident Commander     │
        │   (指挥 + 决策)           │
        └────────────┬─────────────┘
          ┌──────────┼──────────┐
          ▼          ▼          ▼
     ┌─────────┐ ┌─────────┐ ┌─────────┐
     │  Comms  │ │  Ops    │ │ Scribe  │
     │ 对外沟通 │ │ 修系统  │ │ 记录    │
     └─────────┘ └─────────┘ └─────────┘
```

- **IC**：不碰终端。维护任务板、做 go/no-go 决策、决定升级时机。
- **Comms**：写状态页、发客户邮件、运营 Slack 公告频道。禁止在指挥频道与客户来回切。
- **Ops**：执行命令、滚动回滚、扩缩容。每个高危及操作前向 IC 复述。
- **Scribe**：在事件文档里逐条记录"时间—动作—结果"，事后是复盘的唯一事实来源。

## 现场作战 Checklist

### T+0 启动（前 5 分钟）

- [ ] 在 `#incident-<id>` 开指挥频道，所有人加入
- [ ] 宣读："我是 IC，现在 SevX，目标优先级：止血 > 根因"
- [ ] 指派 Comms / Ops / Scribe
- [ ] 打开共享事件文档，记录影响面与开始时间
- [ ] **不要**这时候开始查日志——先止血

### 通讯节奏（"The 15-minute cadence"）

每 15 分钟（Sev1）/ 30 分钟（Sev2）发布一次状态更新，**模板**：

```
[+15min] 状态更新 #N
- 当前影响：___ 用户 / ___ 区域
- 已知信息：___
- 正在执行：___
- 下一次更新：+15min（绝不超时沉默）
- 是否需要升级：是/否
```

**铁律**：宁可说"还在查"也绝不沉默。沉默是用户恐慌的源头。

### 止血优先级阶梯

1. **回滚**（< 5 分钟内可做的最高 ROI 动作）
2. **扩容**（扛住流量）
3. **切流量**（蓝绿/金丝雀回退、DNS 切换）
4. **重启**（清状态）
5. **降级**（关非核心功能保核心）

每一步问 IC："这一步会让情况变糟吗？" 不确定就别做。

### 高危及操作协议

🔴 **高危操作三问**（Ops 执行前必须向 IC 复述）：
1. 这个命令做什么？
2. 影响哪些用户/数据？
3. 失败了怎么回滚？

IC 回复 "go" 才执行。Scribe 记录命令与结果。

## 升级与降级

**升级触发**：
- 15 分钟内无进展
- 影响面扩大（Sev3→Sev2）
- 需要 IC 没有的权限/知识
- 客户公开投诉爆发

**降级/解除**（IC 独占决策）：
- 所有 SLO 恢复绿区且稳定 30 分钟
- 验证无残留影响
- 宣布解除 → 转入复盘（48 小时内完成 blameless postmortem）

## 通讯渠道矩阵

| 受众 | 渠道 | 责任人 |
|------|------|--------|
| 指挥 | `#incident-<id>` Slack | IC |
| 全员 | `#status` 公告 | Comms |
| 客户 | 状态页 status.example.com | Comms |
| 高管 | 电话/短信群 | IC |
| 外部 | PR/法务（数据事件） | IC→Comms |

## 解除后 24 小时内

- [ ] 发送事件总结邮件
- [ ] 创建 postmortem 文档（blameless，见 [[可靠性/事后复盘/01-blameless-postmortem-template.md]]）
- [ ] 排期 48 小时内复盘会
- [ ] 把所有日志/截图归档（保留 90 天）

## IC 决策框架

### 决策矩阵

| 情况 | IC 决策 | 依据 |
|-----|---------|------|
| 影响面扩大 | 升级 Sev | 用户影响优先 |
| 15 分钟无进展 | 升级/换人 | 避免隧道视野 |
| 多个修复方案 | 选最快止血 | 止血 > 根因 |
| 不确定影响 | 不执行 | 安全第一 |
| 需要外部支持 | 立即升级 | 不要独自承担 |

### 止血决策树

```
事故发生
    │
    ▼
[能否 5 分钟内回滚?]
    │
    ├── 是 → 执行回滚 → 验证 → 结束
    │
    └── 否 → [能否扩容缓解?]
                │
                ├── 是 → 执行扩容 → 观察 → 继续排查
                │
                └── 否 → [能否切流量?]
                            │
                            ├── 是 → 执行切流 → 验证 → 继续排查
                            │
                            └── 否 → [能否降级?]
                                        │
                                        ├── 是 → 关闭非核心功能 → 保核心
                                        │
                                        └── 否 → 升级 + 专家支持
```

### IC 常见错误

| 错误 | 后果 | 正确做法 |
|-----|------|----------|
| 自己冲进终端修 bug | 失去全局视野 | 指派 Ops，自己指挥 |
| 沉默不更新 | 用户恐慌、高管焦虑 | 每 15 分钟更新 |
| 追求根因不止血 | 影响扩大 | 先止血，后根因 |
| 不敢升级 | 错过最佳时机 | 宁可误升级 |
| 同时处理多个问题 | 资源分散 | 聚焦最高优先级 |

## 常见事故场景快速响应

### 场景 1: 数据库连接池耗尽

```
症状: 大量 5xx，日志显示 "connection pool exhausted"

IC 指令:
1. Ops: 检查数据库连接数
   kubectl exec -n database sts/postgres-0 -- psql -c "SELECT count(*) FROM pg_stat_activity;"

2. Ops: 检查慢查询
   kubectl exec -n database sts/postgres-0 -- psql -c "SELECT query, state, query_start FROM pg_stat_activity WHERE state != 'idle' ORDER BY query_start;"

3. 决策: 是否有慢查询?
   - 是 → 终止慢查询: SELECT pg_terminate_backend(<pid>);
   - 否 → 扩容应用副本 + 检查连接池配置

4. 止血: 重启部分应用 Pod 释放连接
   kubectl rollout restart deployment/api -n production
```

### 场景 2: 内存泄漏导致 OOM

```
症状: Pod 频繁 OOMKilled，重启循环

IC 指令:
1. Ops: 确认 OOM 模式
   kubectl describe pod <pod> -n production | grep -A5 "Last State"

2. Ops: 检查内存使用趋势
   # Grafana: container_memory_working_set_bytes

3. 决策: 是否所有 Pod 都 OOM?
   - 是 → 可能是代码问题，回滚到上一版本
   - 部分 → 可能是流量不均，调整负载均衡

4. 止血: 临时提高内存限制
   kubectl patch deployment/api -n production -p '{"spec":{"template":{"spec":{"containers":[{"name":"api","resources":{"limits":{"memory":"2Gi"}}}]}}}}'
```

### 场景 3: 证书过期

```
症状: TLS 握手失败，用户无法访问

IC 指令:
1. Ops: 检查证书状态
   kubectl get secret tls-cert -n production -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates

2. 决策: 是否已过期?
   - 是 → 立即更新证书
   - 即将过期 → 计划更新

3. 止血: 更新证书
   # 使用 cert-manager
   kubectl delete certificaterequest <cr-name> -n production
   # 或手动更新
   kubectl create secret tls tls-cert --cert=new.crt --key=new.key -n production --dry-run=client -o yaml | kubectl apply -f -

4. 重启受影响的服务
   kubectl rollout restart deployment/api -n production
```

## 事故指挥工具链

### 推荐工具

| 功能 | 工具 | 用途 |
|-----|------|------|
| 指挥频道 | Slack/PagerDuty | 实时沟通 |
| 事件文档 | Notion/Confluence | 时间线记录 |
| 状态页 | Statuspage/Status.io | 客户通知 |
| 监控 | Grafana/Datadog | 指标观察 |
| 日志 | Loki/ELK | 日志查询 |
| 追踪 | Jaeger/Tempo | 链路追踪 |
| 告警 | PagerDuty/OpsGenie | 告警路由 |

### 事件文档模板

```markdown
# 事件 #<ID>: <标题>

## 元信息
- Sev: <1-4>
- IC: <姓名>
- 开始时间: <时间>
- 状态: 🔴 进行中 / 🟡 观察中 / 🟢 已解除

## 影响面
- 受影响服务: ___
- 受影响用户: ___
- 受影响区域: ___

## 时间线
| 时间 | 事件 | 操作人 |
|-----|------|-------|
| | | |

## 当前状态
- 正在执行: ___
- 下一步: ___
- 需要支持: ___

## 状态更新记录
### 更新 #1 (+15min)
- 当前影响: ___
- 已知信息: ___
- 正在执行: ___
```

## IC 培训与认证

### 培训路径

| 阶段 | 内容 | 时长 | 考核 |
|-----|------|------|------|
| **初级** | 角色职责、通讯节奏、工具使用 | 4h | 模拟演练 |
| **中级** | 决策框架、升级判断、多团队协作 | 8h | 实战观摩 |
| **高级** | 复杂事故、跨部门协调、媒体应对 | 16h | 独立指挥 |

### 模拟演练脚本

```
场景: 支付服务 5xx 激增

T+0:00  告警触发，IC 接手
T+0:02  IC: "我是 IC，现在 Sev2，目标：止血优先"
T+0:03  IC: "@Comms 开状态页，@Ops 查监控，@Scribe 开文档"
T+0:05  Ops: "错误率 15%，集中在支付服务"
T+0:07  IC: "能否回滚?"
T+0:08  Ops: "可以，5 分钟内完成"
T+0:09  IC: "执行回滚"
T+0:14  Ops: "回滚完成"
T+0:15  IC: "@Comms 更新状态，@Ops 继续观察"
T+0:20  Ops: "错误率降到 0.5%"
T+0:30  IC: "稳定 15 分钟，降级到 Sev3"
T+0:45  IC: "解除，安排复盘"
```

## 事故指标与复盘

### 关键指标

| 指标 | 定义 | 目标 |
|-----|------|------|
| **MTTD** | 平均检测时间 | < 5 分钟 |
| **MTTA** | 平均响应时间 | < 10 分钟 |
| **MTTR** | 平均恢复时间 | < 60 分钟 |
| **重复率** | 相同问题再次发生 | < 5% |
| **升级率** | 需要升级的比例 | < 20% |

### 事故统计脚本

```bash
#!/bin/bash
# 🟢 低风险：事故统计脚本
set -euo pipefail

echo "=== 事故统计 $(date +%Y-%m) ==="

# 从事件管理系统获取数据
# 这里用示例数据

cat <<EOF
本月事故统计:
- Sev1: 1 起
- Sev2: 3 起
- Sev3: 8 起
- Sev4: 15 起

关键指标:
- MTTD: 3.2 分钟 (目标 < 5) ✓
- MTTA: 8.5 分钟 (目标 < 10) ✓
- MTTR: 45 分钟 (目标 < 60) ✓
- 重复率: 8% (目标 < 5) ✗

待改进:
- 重复率超标，需要加强改进措施跟踪
EOF
```

## 相关

- [[可靠性/SRE实践/03-incident-command-system.md|03 incident command system]]
- [[可靠性/事后复盘/01-blameless-postmortem-template.md|01 blameless postmortem template]]
- [[可靠性/灾难恢复/02-dr-automation-playbook.md|02 dr automation playbook]]

<!-- risk-assessed -->
