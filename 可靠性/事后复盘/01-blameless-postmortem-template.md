---
title: 无责事后复盘模板
description: '| 14:35 | On-call 响应并开始排查 | 通过 PagerDuty |'
summary: '| 14:35 | On-call 响应并开始排查 | 通过 PagerDuty |'
category: domain
tags:
- postmortem
- sre
- incident-management
- reliability
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 无责事后复盘模板 是什么
- 如何 无责事后复盘模板
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 无责事后复盘模板
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 无责事后复盘模板

> **核心原则**: 事后复盘的目标是理解系统为何允许问题发生，而非追究谁犯了错误。每个人的决策在当时看来都是合理的。

## 模板结构

```markdown
# 事件复盘: [事件标题]

## 元信息
- 事件编号: INC-2026-001
- 日期: 2026-05-21
- 影响服务: order-service, payment-service
- 严重级别: P1 (严重)
- 持续时间: 23 分钟
- 复盘主持人: [SRE Lead]
- 参与者: [On-call, Dev Lead, QA Lead]

## 摘要 (Executive Summary)
[2-3 句话描述发生了什么、影响范围、持续时间]

## 事件时间线 (Timeline)

| 时间 | 事件 | 备注 |
|------|------|------|
| 14:32 | 告警触发: 订单服务错误率 > 5% | 自动告警 |
| 14:35 | On-call 响应并开始排查 | 通过 PagerDuty |
| 14:45 | 定位到数据库连接池耗尽 | 发现 max_connections = 100 |
| 14:50 | 临时扩容连接池 | 错误率开始下降 |
| 14:55 | 服务完全恢复 | 错误率 < 0.1% |

## 影响评估 (Impact Assessment)

- 受影响用户: ~12,000 人
- 失败订单: ~450 笔
- 收入影响: 约 ¥85,000
- 数据丢失: 无
- 合规影响: 无

## 根因分析 (Root Cause Analysis)

### 5 Whys

1. 为什么订单服务错误率升高?
   → 数据库连接池耗尽，新请求无法获取连接

2. 为什么连接池会耗尽?
   → 连接池配置 max_connections = 100，远低于实际需求

3. 为什么配置如此低?
   → 配置沿用开发环境默认值，未根据生产环境调整

4. 为什么生产环境未调整?
   → 上线检查清单缺少数据库连接池配置项

5. 为什么检查清单不完整?
   → 新服务上线流程未经过 SRE 评审

### 根因分类

- 直接原因: 数据库连接池配置不当
-  Contributing Factor: 缺乏生产环境配置审查流程
-  Contributing Factor: 连接池使用率监控缺失

## 经验教训 (Lessons Learned)

### 做得好的 (What Went Well)
- 告警及时，On-call 在 3 分钟内响应
- 快速定位根因并修复
- 客服团队及时发布状态更新

### 需要改进的 (What Went Wrong)
- 连接池配置未经审查
- 缺乏连接池使用率监控
- 上线流程缺少 SRE 把关

### 意外发现 (Where We Got Lucky)
- 问题发生在低峰期，影响用户较少
- 数据库本身未崩溃，只是连接拒绝

## 改进措施 (Action Items)

| 措施 | 负责人 | 截止日期 | 优先级 | 状态 |
|------|--------|---------|--------|------|
| 更新所有服务连接池配置 | @devops | 2026-05-28 | P0 | 待开始 |
| 添加连接池使用率监控 | @sre | 2026-05-25 | P0 | 待开始 |
| 更新上线检查清单 | @sre-lead | 2026-05-30 | P1 | 待开始 |
| SRE 评审所有新服务上线 | @sre-lead | 2026-06-01 | P1 | 待开始 |

## 无责声明 (Blameless Statement)

本复盘采用无责原则。所有参与者在当时都做出了基于可用信息的最佳决策。
问题的根源在于系统和流程，而非个人。

---
复盘完成日期: 2026-05-22
下次审查: 2026-06-22
```

## 无责文化的核心

```
❌ "张三配置错了连接池"
✅ "连接池配置流程缺少审查环节"

❌ "李四没有及时响应告警"
✅ "告警信息不够清晰，On-call 手册缺少该场景指导"

❌ "测试团队没有测出这个问题"
✅ "测试环境未模拟生产环境负载，缺乏压力测试"
```

## 复盘流程自动化

### 事件数据收集脚本

```bash
#!/bin/bash
# 🟢 低风险：事件数据收集脚本
set -euo pipefail

INCIDENT_ID=${1:?"Usage: $0 <incident-id>"}
OUTPUT_DIR="/tmp/postmortem-$INCIDENT_ID"
mkdir -p $OUTPUT_DIR

echo "=== 收集事件数据: $INCIDENT_ID ==="

# 1. 收集 Kubernetes 事件
echo "[1] 收集 K8s 事件..."
kubectl get events -A --sort-by='.lastTimestamp' > $OUTPUT_DIR/k8s-events.txt

# 2. 收集 Pod 状态
echo "[2] 收集 Pod 状态..."
kubectl get pods -A -o wide > $OUTPUT_DIR/pod-status.txt

# 3. 收集监控数据
echo "[3] 收集监控数据..."
# 从 Prometheus 查询关键指标
curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1h]))' > $OUTPUT_DIR/error-rate.json
curl -s 'http://prometheus:9090/api/v1/query?query=histogram_quantile(0.99,rate(http_request_duration_seconds_bucket[1h]))' > $OUTPUT_DIR/latency-p99.json

# 4. 收集告警历史
echo "[4] 收集告警历史..."
curl -s 'http://alertmanager:9093/api/v2/alerts?silenced=false&inhibited=false' > $OUTPUT_DIR/alerts.json

# 5. 收集日志
echo "[5] 收集相关日志..."
# 根据受影响服务收集日志

# 6. 生成时间线模板
echo "[6] 生成时间线模板..."
cat > $OUTPUT_DIR/timeline-template.md <<EOF
# 事件时间线: $INCIDENT_ID

| 时间 | 事件 | 操作人 | 备注 |
|-----|------|-------|------|
| | 告警触发 | 自动 | |
| | IC 宜布 SevX | @ic | |
| | 定位根因 | @ops | |
| | 执行修复 | @ops | |
| | 服务恢复 | 自动 | |
EOF

echo "=== 数据收集完成: $OUTPUT_DIR ==="
ls -la $OUTPUT_DIR
```

### 复盘文档自动生成

```bash
#!/bin/bash
# 🟢 低风险：复盘文档生成脚本
set -euo pipefail

INCIDENT_ID=${1:?"Usage: $0 <incident-id>"}
SEVERITY=${2:-Sev2}
DATE=$(date +%Y-%m-%d)

cat > /tmp/postmortem-$INCIDENT_ID.md <<EOF
# 事件复盘: $INCIDENT_ID

## 元信息
- 事件编号: $INCIDENT_ID
- 日期: $DATE
- 严重级别: $SEVERITY
- 复盘主持人: [SRE Lead]
- 参与者: [On-call, Dev Lead, QA Lead]

## 摘要 (Executive Summary)
[2-3 句话描述发生了什么、影响范围、持续时间]

## 事件时间线 (Timeline)

| 时间 | 事件 | 备注 |
|------|------|------|
| | 告警触发 | 自动告警 |
| | On-call 响应 | 通过 PagerDuty |
| | 定位根因 | |
| | 执行修复 | |
| | 服务恢复 | |

## 影响评估 (Impact Assessment)

- 受影响用户: 
- 失败请求: 
- 收入影响: 
- 数据丢失: 
- 合规影响: 

## 根因分析 (Root Cause Analysis)

### 5 Whys

1. 为什么服务异常?
   → 

2. 为什么?
   → 

3. 为什么?
   → 

4. 为什么?
   → 

5. 为什么?
   → 

### 根因分类

- 直接原因: 
- 贡献因素: 
- 贡献因素: 

## 经验教训 (Lessons Learned)

### 做得好的 (What Went Well)
- 

### 需要改进的 (What Went Wrong)
- 

### 意外发现 (Where We Got Lucky)
- 

## 改进措施 (Action Items)

| 措施 | 负责人 | 截止日期 | 优先级 | 状态 |
|------|--------|---------|--------|------|
| | | | P0 | 待开始 |
| | | | P1 | 待开始 |

## 无责声明 (Blameless Statement)

本复盘采用无责原则。所有参与者在当时都做出了基于可用信息的最佳决策。
问题的根源在于系统和流程，而非个人。

---
复盘完成日期: $DATE
下次审查: $(date -v+1m +%Y-%m-%d 2>/dev/null || date -d "+1 month" +%Y-%m-%d)
EOF

echo "复盘文档已生成: /tmp/postmortem-$INCIDENT_ID.md"
```

## 改进措施跟踪

### 跟踪看板配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: postmortem-actions
  namespace: sre
data:
  actions.yaml: |
    actions:
      - id: ACT-2026-001
        title: "更新所有服务连接池配置"
        owner: "@devops"
        due: "2026-05-28"
        priority: P0
        status: in_progress
        postmortem: INC-2026-001
        
      - id: ACT-2026-002
        title: "添加连接池使用率监控"
        owner: "@sre"
        due: "2026-05-25"
        priority: P0
        status: pending
        postmortem: INC-2026-001
        
      - id: ACT-2026-003
        title: "更新上线检查清单"
        owner: "@sre-lead"
        due: "2026-05-30"
        priority: P1
        status: pending
        postmortem: INC-2026-001
```

### 自动提醒 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: postmortem-action-reminder
  namespace: sre
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: reminder
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 复盘改进行动提醒 ==="
                  
                  # 检查逾期行动
                  OVERDUE=$(kubectl get configmap postmortem-actions -n sre -o yaml | \
                    yq '.data.actions' | \
                    yq '.actions[] | select(.due < now and .status != "done")')
                  
                  if [ -n "$OVERDUE" ]; then
                    echo "⚠️ 以下行动已逾期:"
                    echo "$OVERDUE"
                    # 发送 Slack 提醒
                    curl -X POST -H 'Content-type: application/json' \
                      --data '{"text":"⚠️ 复盘改进行动逾期提醒"}' \
                      $SLACK_WEBHOOK
                  fi
```

## 复盘质量检查清单

### 文档完整性检查

| 检查项 | 要求 | 状态 |
|-------|------|------|
| 元信息完整 | 编号/日期/级别/参与者 | ☐ |
| 摘要清晰 | 2-3 句话概括 | ☐ |
| 时间线详细 | 逐分钟记录 | ☐ |
| 影响量化 | 用户数/收入/SLA | ☐ |
| 根因深入 | 5 Whys 到底 | ☐ |
| 改进可执行 | 负责人+截止日期 | ☐ |
| 无责语言 | 无指责性表述 | ☐ |

### 语言检查

```bash
#!/bin/bash
# 🟢 低风险：复盘文档语言检查
set -euo pipefail

FILE=${1:?"Usage: $0 <postmortem-file>"}

echo "=== 复盘文档语言检查 ==="

# 检查指责性语言
BLAME_WORDS=("他的错" "她的错" "应该知道" "不应该" "忘记了" "忽略了")

for word in "${BLAME_WORDS[@]}"; do
  if grep -q "$word" "$FILE"; then
    echo "⚠️ 发现可能的指责性语言: '$word'"
    grep -n "$word" "$FILE"
  fi
done

# 检查被动语态（推荐）
if grep -qE "(被|由|所)" "$FILE"; then
  echo "✓ 使用了被动语态（符合无责原则）"
fi

echo "=== 检查完成 ==="
```

## 常见反模式

| 反模式 | 表现 | 正确做法 |
|-------|------|----------|
| **指责游戏** | "张三配置错了" | "配置流程缺少审查" |
| **浅尝辄止** | 5 Whys 只做到 2 层 | 深入到系统/流程层面 |
| **改进空洞** | "加强监控" | "添加连接池使用率 > 80% 告警" |
| **无后续** | 复盘完就结束 | 跟踪改进措施直到完成 |
| **选择性记忆** | 只记录技术细节 | 包含决策过程、沟通时间线 |
| **过度归因** | 单一根因 | 识别多个贡献因素 |

## 相关

- [[可靠性/事后复盘/02-postmortem-culture-guide.md|02 postmortem culture guide]]


<!-- risk-assessed -->
