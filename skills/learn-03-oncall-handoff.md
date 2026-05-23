---
title: 'Day 3: 值班交接 SOP'
description: '## 概述'
category: skills
tags:
- k8s
- learn
- quick-start
- prometheus
- grafana
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 'Day 3: 值班交接 SOP 是什么'
- '如何 Day 3: 值班交接 SOP'
trigger_keywords:
- Day
- '3:'
- 值班交接
- SOP
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

trigger_keywords:
- Day
- '3:'
- 值班交接
- SOP
- learn  role: contributor---
# Day 3: 值班交接 SOP

> **适用对象**: oncall 值班工程师 | **版本**: K8s 1.28-1.33

---

## 概述

值班交接是保障系统持续稳定运行的关键环节。良好的交接可以确保问题不遗漏、上下文不丢失、响应不延迟。本文档定义了标准化的值班交接流程、交班人和接班人的职责清单、紧急交接的处理方式，以及值班期间的行为规范。

---

## 1. 交接流程

```
接班人登录值班系统
  → 交班人介绍当前情况 (10-15 分钟)
  → 共同检查系统状态 (5-10 分钟)
  → 确认交接文档完整
  → 接班人签字确认
  → 交班人离岗
```

### 1.1 交接时间

| 类型 | 时间 | 说明 |
|------|------|------|
| 日常交接 | 9:00 / 18:00 | 早班/晚班，提前 15 分钟到岗 |
| 周末交接 | 9:00 | 周五 → 周六 → 周日 → 周一 |
| 紧急交接 | 随时 | 紧急情况立即交接，不限时间 |
| 节假日交接 | 9:00 | 提前确认排班表 |

### 1.2 交接检查清单

```bash
echo "========== 值班交接系统检查 =========="
echo "时间: $(date)"
echo ""

echo "--- 1. 集群健康检查 ---"
kubectl get nodes | grep -v Ready && echo "[异常] 有节点不健康" || echo "[正常] 所有节点 Ready"
echo ""

echo "--- 2. Pod 状态检查 ---"
ABNORMAL=$(kubectl get pods -A | grep -v Running | grep -v Completed | grep -v NAMESPACE | wc -l)
echo "异常 Pod 数量: $ABNORMAL"
[ "$ABNORMAL" -gt 0 ] && kubectl get pods -A | grep -v Running | grep -v Completed
echo ""

echo "--- 3. 事件检查 (最近 1 小时) ---"
kubectl get events -A --sort-by='.lastTimestamp' | tail -20
echo ""

echo "--- 4. 告警状态 ---"
echo "请检查 Prometheus/Grafana 告警面板"
echo "重点: critical 级别是否有 firing 告警"
echo ""

echo "--- 5. 进行中的维护任务 ---"
kubectl get pods -n kube-system | grep -E "upgrade|drain|migrate" && echo "[注意] 有维护任务进行中" || echo "[正常] 无维护任务"
echo ""

echo "--- 6. 未完成工单 ---"
echo "请检查工单系统中的 P0/P1 工单"
echo ""

echo "========== 检查完毕 =========="
```

---

## 2. 交班人职责

### 2.1 交接前准备（下班前 30 分钟）

```bash
# 1. 整理当前处理的工单状态
kubectl get events -A --sort-by='.lastTimestamp' | tail -50 > /tmp/handover-events.txt

# 2. 检查是否有进行中的维护任务
kubectl get pods -n kube-system | grep -E "upgrade|drain|migrate"

# 3. 整理告警状态
# 查看 Prometheus AlertManager，确认无 firing 的 critical 告警
# 如有，记录在交接文档中

# 4. 整理需要跟进的问题
kubectl get pods -A | grep -v Running | grep -v Completed > /tmp/handover-pods.txt

# 5. 导出当前系统状态快照
kubectl get nodes -o wide > /tmp/handover-nodes.txt
kubectl get pods -A -o wide > /tmp/handover-all-pods.txt
kubectl top nodes > /tmp/handover-top.txt 2>/dev/null

# 6. 确认下个班次的 oncall 联系人
```

### 2.2 交接内容清单

```markdown
## 值班交接文档

**交班人**: [名字]
**接班人**: [名字]
**交接时间**: [时间]
**班次**: [早班/晚班/周末]

---

### 系统状态
- [ ] 集群健康状态: [正常/异常 - 说明]
- [ ] 关键服务状态: [正常/异常 - 说明]
- [ ] 告警状态: [无告警/有告警 - 列出]
- [ ] 节点资源使用: [正常/偏高 - 说明]

### 进行中的工单

| 工单ID | 描述 | 状态 | 需跟进 | 优先级 |
|--------|------|------|--------|--------|
| INC-001 | node-03 磁盘使用率高 | 处理中 | 清理日志 | P2 |
| INC-002 | nginx-ingress 升级 | 已完成 | 验证 | P3 |

### 近期变更

| 时间 | 变更内容 | 操作人 | 影响 |
|------|---------|--------|------|
| 10:00 | 升级 nginx-ingress 到 1.10 | 张三 | 无影响 |
| 14:00 | 新增 monitoring NS | 李四 | 无影响 |

### 待处理事项
1. [ ] [P2] node-03 日志清理，磁盘使用率 85%
2. [ ] [P3] 验证 nginx-ingress 升级后功能正常
3. [ ] [ ] [其他事项]

### 已知问题
- **问题 1**: node-03 磁盘使用率高，已清理部分日志，需要联系应用团队排查日志输出量
- **问题 2**: monitoring NS 的 Pod 偶尔重启，原因待查

### 升级联系方式
| 角色 | 联系人 | 手机号 | 职责 |
|------|--------|--------|------|
| SRE 值班 | 张三 | 138xxxx | 主值班 |
| SRE 备值 | 李四 | 139xxxx | 备份 |
| 网络团队 | 王五 | 137xxxx | 网络问题 |
| 安全团队 | 赵六 | 136xxxx | 安全事件 |
| 研发团队 | - | 群组 | 业务问题 |
```

---

## 3. 接班人职责

### 3.1 接班后检查

```bash
echo "========== 接班检查 =========="

# 1. 确认集群健康
echo "--- 集群节点 ---"
kubectl get nodes | grep -v Ready && echo "[警告] 有节点不健康" || echo "[正常]"

# 2. 检查异常 Pod
echo "--- 异常 Pod ---"
kubectl get pods -A | grep -v Running | grep -v Completed | grep -v NAMESPACE

# 3. 检查事件
echo "--- 最近事件 ---"
kubectl get events -A --sort-by='.lastTimestamp' | tail -30

# 4. 检查告警平台
echo "--- 告警状态 ---"
echo "请登录 Prometheus/Grafana 确认无 critical 告警"

# 5. 查看工单系统
echo "--- 工单系统 ---"
echo "请确认无未处理的 P0/P1 工单"

# 6. 确认通信畅通
echo "--- 通信确认 ---"
echo "请测试钉钉/飞书/邮件通知是否正常"

echo "========== 检查完毕 =========="
```

### 3.2 接班确认

```
1. 完成系统检查，确认状态与交班文档一致
2. 确认所有工单和待处理事项已了解
3. 确认联系方式和升级路径已知晓
4. 在交接文档上签名确认
5. 交班人可以离岗
```

---

## 4. 紧急交接场景

### 4.1 突发问题交接

```bash
# 交班人突然无法继续值班时

# 1. 紧急交接清单
echo "========== 紧急交接 =========="
echo "时间: $(date)"
echo ""

echo "--- 当前问题状态 ---"
echo "问题描述: [简要描述]"
echo "影响范围: [受影响的服务/用户]"
echo "已执行措施: [已做的操作]"
echo "待执行措施: [接下来要做的操作]"
echo ""

echo "--- 系统快照 ---"
kubectl get nodes > /tmp/emergency-nodes.txt
kubectl get pods -A > /tmp/emergency-pods.txt
kubectl get events -A --sort-by='.lastTimestamp' | tail -100 > /tmp/emergency-events.txt
kubectl top nodes > /tmp/emergency-top.txt 2>/dev/null

echo "快照已保存到 /tmp/emergency-*"
echo ""
echo "========== 紧急交接完毕 =========="

# 2. 发送交接消息
# 发送钉钉/飞书消息给接班人和 SRE 值班
```

### 4.2 交接消息模板

```markdown
【紧急交接】
时间：[时间]
交班人：[名字]
接班人：[名字]

当前状态：
- 问题描述：[描述]
- 影响范围：[范围]
- 已执行措施：[措施]
- 当前状态：[稳定/恶化/恢复中]

待处理：
1. [ ] 任务 1
2. [ ] 任务 2

系统快照：
- 节点: /tmp/emergency-nodes.txt
- Pod: /tmp/emergency-pods.txt
- 事件: /tmp/emergency-events.txt

关键联系人：
- SRE 值班: [手机号]
- 网络团队: [手机号]
```

### 4.3 问题中交接注意事项

```
1. 故障处理中的交接，必须当面或电话交接
2. 不能仅通过文字消息交接
3. 接班人必须确认理解当前问题状态
4. 交接后交班人保持电话畅通 30 分钟
5. 故障恢复后需复盘交接过程
```

---

## 5. 值班纪律

### 5.1 值班期间要求

| 要求 | 说明 | 违反后果 |
|------|------|---------|
| 响应时间 | 告警 5 分钟内响应 | 警告/记录 |
| 在岗要求 | 不离开值班岗位 (临时离开需通报) | 警告/记录 |
| 专注要求 | 不处理与工作无关的事项 | 警告/记录 |
| 记录要求 | 及时更新工单状态 | 警告/记录 |
| 通信要求 | 保持手机畅通、IM 在线 | 警告/记录 |

### 5.2 值班记录模板

```markdown
## 值班日志

**日期**: YYYY-MM-DD
**值班人**: [名字]
**班次**: [早班/晚班]

### 09:00 接班
- 系统状态: 正常
- 接班自: [前一位值班人]

### 10:00 巡检
- 节点状态: 全部 Ready
- 异常 Pod: 无
- 告警: 无

### 11:00 处理工单 INC-001
- 问题: node-03 磁盘使用率高
- 处理: 清理 /var/log 日志文件
- 结果: 磁盘使用率从 85% 降至 60%

### 14:00 变更: nginx-ingress 升级
- 操作: 升级 nginx-ingress 到 1.10
- 结果: 成功，无影响

### 18:00 交班
- 系统状态: 正常
- 交班给: [下一位值班人]
```

### 5.3 交接纪律

| 规则 | 说明 |
|------|------|
| 准时交接 | 不擅自离岗，提前 15 分钟到岗 |
| 详细交接 | 交接内容要详细、清晰、无遗漏 |
| 确认机制 | 接班人未确认前，交班人不得离岗 |
| 紧急增援 | 紧急情况下可要求临时增援 |
| 交接文档 | 每次交接必须有文档记录 |

---

## 6. 常见问题处理

### Q: 接班后系统异常怎么办？

```bash
# 1. 立即检查
kubectl get nodes
kubectl get pods -A | grep -v Running | grep -v Completed
kubectl get events -A --sort-by='.lastTimestamp' | tail -30

# 2. 评估影响
# - 仅影响内部系统: 按 P2/P3 处理
# - 影响外部用户: 按 P0/P1 处理

# 3. 联系交班人了解情况
# 4. 如交班人无法联系，联系 SRE 值班
```

### Q: 交班人拖延交接怎么办？

- 联系 SRE 主管协调
- 如交班人有紧急情况，记录延迟原因
- 不影响系统监控，继续履行值班职责

### Q: 值班期间收到多个告警如何处理？

```
1. 按 P0 > P1 > P2 > P3 优先级处理
2. 同一问题的多个告警合并处理
3. P0/P1 立即响应，通知相关人员
4. P2/P3 记录后按计划处理
5. 无法同时处理时，请求增援
```

### Q: 忘记交接某个重要事项怎么办？

- 立即通过电话/IM 通知接班人
- 补充更新交接文档
- 如已造成影响，主动承担责任并参与复盘

---

## 7. 值班工具清单

| 工具 | 用途 | 位置 |
|------|------|------|
| kubectl | 集群操作 | 已安装在值班机器 |
| aliyun CLI | ACK API 调用 | 已安装 |
| Grafana | 监控面板 | 浏览器访问 |
| AlertManager | 告警管理 | 浏览器访问 |
| 工单系统 | 工单管理 | 浏览器访问 |
| 钉钉/飞书 | 即时通信 | 手机 + 电脑 |
| 交接文档模板 | 交接记录 | 共享文档 |

---

```yaml
---  - "值班交接怎么做"
  - "oncall交接内容有哪些"
  - "紧急情况怎么交接"
  - "交接检查清单"
  - "接班检查流程"  - "值班交接"
  - "oncall交接"
  - "紧急交接"
  - "SOP流程"
  - "交班检查"
  - "接班验证"
  - "shift-handoff"
  - "SRE值班"  - sre工程师
  - ops工程师
  - 值班人员
related_domains:
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start/01-day-one-checklist
  - domain-11-production-operations/topic-learn/quick-start/02-first-ticket-guide
  - P1-5-oncall-quick-reference-card
id: QUICKSTART-DAY3
topic: onboarding
type: sop
tags: [onboarding, oncall, handoff, shift, sre, ops-engineer, k8s-1.28-1.33]
---
```

## Related

- [[skills/learn-07-namespace-resource-quota|learn-07-namespace-resource-quota]] — 第七课：Namespace 与资源隔离
- [[skills/learn-15-scheduling-basics|learn-15-scheduling-basics]] — 第15课：调度与亲和性
- [[skills/learn-inner-training|learn-inner-training]] — [[Kubernetes|Kubernetes]]es 培训：Inner Training|Kubernetes 培训：Inner Training]]
- [[skills/learn-lecturer-persona|learn-lecturer-persona]] — K8S 讲师角色设定与场景规范
- [[prometheus]] — Prometheus
