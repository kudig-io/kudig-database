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

模板见 [[12-可靠性/05-事后复盘/01-blameless-postmortem-template.md]]，产出：
1. 时间线（Scribe 记录逐分钟）
2. 发现的问题清单（每条带证据：指标截图 + 日志链接）
3. 改进工单（带 owner 与截止日期，进 sprint）
4. 可复用的检测/告警规则

## 常见陷阱

1. **没有中止条件**：红线不写清，出事就乱。每个场景必须有"立即停"的标准。
2. **演练目标定太满**：第一次 Game Day 就搞全量 AZ 故障，出事概率高、收益低。从 Pod 级别逐步升级。
3. **只测不修**：发现问题进备忘录就完了——必须开工单排期，下个 Game Day 验证修复。
4. **.prod 演练不通知客户**：除非有变更窗口豁免，否则务必状态页预告，避免被当真实事故响应。

## 场景库详解

### 基础场景（入门级）

| 场景 | 注入方式 | 预期结果 | 验证点 |
|-----|---------|---------|--------|
| Pod 随机杀死 | Chaos Mesh PodChaos | 自动重启，无感知 | 重启时间 < 30s |
| CPU 压力 | StressChaos | HPA 扩容 | 扩容触发 < 2min |
| 内存压力 | StressChaos | 无 OOM | 内存使用 < 80% |
| 网络延迟 | NetworkChaos | 延迟增加但可用 | P99 < 1s |
| DNS 故障 | DNSChaos | 服务发现降级 | 缓存生效 |

### 中级场景

| 场景 | 注入方式 | 预期结果 | 验证点 |
|-----|---------|---------|--------|
| 数据库主从切换 | 手动/脚本 | 自动重连 | 中断 < 30s |
| 缓存失效 | 清空 Redis | 降级到 DB | 无 5xx |
| 依赖服务超时 | NetworkChaos | 熔断生效 | 快速失败 |
| 磁盘 IO 压力 | IOChaos | 限流生效 | 无数据损坏 |
| 节点 NotReady | 停止 kubelet | Pod 迁移 | 迁移 < 5min |

### 高级场景

| 场景 | 注入方式 | 预期结果 | 验证点 |
|-----|---------|---------|--------|
| AZ 故障 | 多节点同时故障 | 跨 AZ 切换 | 服务不中断 |
| 区域故障 | 多 AZ 同时故障 | 跨区域切换 | RTO < 15min |
| 数据损坏 | 注入错误数据 | 检测并告警 | 告警 < 5min |
| 级联故障 | 多服务同时故障 | 降级保核心 | 核心可用 |

## 自动化编排

### Argo Workflow Game Day 编排

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: game-day-automation
  namespace: chaos
spec:
  entrypoint: game-day
  templates:
    - name: game-day
      steps:
        - - name: capture-baseline
            template: baseline
        - - name: start-load
            template: load-test
        - - name: inject-chaos
            template: chaos
        - - name: observe
            template: observe
            arguments:
              parameters:
                - name: duration
                  value: "300s"
        - - name: stop-chaos
            template: cleanup
        - - name: verify-recovery
            template: verify
        - - name: generate-report
            template: report

    - name: baseline
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 采集基线 ==="
            kubectl get pods -n production -o wide > /tmp/baseline-pods.txt
            curl -s 'http://prometheus:9090/api/v1/query?query=histogram_quantile(0.99,rate(http_request_duration_seconds_bucket[5m]))' > /tmp/baseline-latency.json

    - name: load-test
      container:
        image: grafana/k6:latest
        command: [k6, run, /scripts/load.js]
        volumeMounts:
          - name: scripts
            mountPath: /scripts

    - name: chaos
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 注入故障 ==="
            kubectl apply -f /chaos/experiment.yaml

    - name: observe
      inputs:
        parameters:
          - name: duration
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 观察期: {{inputs.parameters.duration}} ==="
            sleep {{inputs.parameters.duration}}

    - name: cleanup
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 清理故障 ==="
            kubectl delete -f /chaos/experiment.yaml --ignore-not-found

    - name: verify
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 验证恢复 ==="
            kubectl get pods -n production -o wide > /tmp/recovery-pods.txt
            # 对比基线与恢复后状态

    - name: report
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 生成报告 ==="
            cat > /tmp/game-day-report.md <<EOF
            # Game Day 报告
            - 日期: $(date)
            - 场景: $SCENARIO
            - 结果: $RESULT
            EOF
```

## 证据采集

### 指标采集脚本

```bash
#!/bin/bash
# 🟢 低风险：Game Day 证据采集脚本
set -euo pipefail

GAME_DAY_ID=${1:?"Usage: $0 <game-day-id>"}
OUTPUT_DIR="/tmp/game-day-$GAME_DAY_ID"
mkdir -p $OUTPUT_DIR

echo "=== Game Day 证据采集: $GAME_DAY_ID ==="

# 1. 系统状态快照
echo "[1] 系统状态..."
kubectl get nodes -o wide > $OUTPUT_DIR/nodes.txt
kubectl get pods -n production -o wide > $OUTPUT_DIR/pods.txt
kubectl get events -n production --sort-by='.lastTimestamp' > $OUTPUT_DIR/events.txt

# 2. 监控指标
echo "[2] 监控指标..."
# 错误率
curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))' > $OUTPUT_DIR/error-rate.json
# P99 延迟
curl -s 'http://prometheus:9090/api/v1/query?query=histogram_quantile(0.99,rate(http_request_duration_seconds_bucket[5m]))' > $OUTPUT_DIR/latency-p99.json
# CPU/内存
curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(container_cpu_usage_seconds_total[5m]))' > $OUTPUT_DIR/cpu.json
curl -s 'http://prometheus:9090/api/v1/query?query=sum(container_memory_working_set_bytes)' > $OUTPUT_DIR/memory.json

# 3. Grafana 截图
echo "[3] Grafana 截图..."
# 使用 Grafana API 或手动截图

# 4. 日志
echo "[4] 关键日志..."
kubectl logs -n production -l app=api --tail=1000 > $OUTPUT_DIR/api-logs.txt

echo "=== 采集完成: $OUTPUT_DIR ==="
ls -la $OUTPUT_DIR
```

### 证据清单

| 证据类型 | 内容 | 保留期限 |
|---------|------|----------|
| 系统快照 | 节点/Pod/事件状态 | 90 天 |
| 监控指标 | 错误率/延迟/资源 | 90 天 |
| Grafana 截图 | 关键面板截图 | 90 天 |
| 日志 | 相关服务日志 | 30 天 |
| 演练记录 | 时间线/操作记录 | 永久 |
| 复盘文档 | 问题/改进措施 | 永久 |

## 演练后跟踪

### 问题跟踪看板

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: game-day-issues
  namespace: chaos
data:
  issues.yaml: |
    game_day: GD-2026-07-11
    issues:
      - id: GD-001
        title: "P99 延迟超标"
        severity: high
        owner: "@sre-team"
        due: "2026-07-25"
        status: in_progress
        
      - id: GD-002
        title: "熔断阈值过高"
        severity: medium
        owner: "@dev-team"
        due: "2026-07-20"
        status: pending
```

### 跟踪 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: game-day-issue-tracker
  namespace: chaos
spec:
  schedule: "0 9 * * 1"  # 每周一 9:00
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: tracker
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== Game Day 问题跟踪 ==="
                  
                  # 检查逾期问题
                  ISSUES=$(kubectl get configmap game-day-issues -n chaos -o yaml | \
                    yq '.data.issues' | \
                    yq '.issues[] | select(.due < now and .status != "done")')
                  
                  if [ -n "$ISSUES" ]; then
                    echo "⚠️ 以下问题已逾期:"
                    echo "$ISSUES"
                    # 发送提醒
                  fi
```

## 成熟度评估

### Game Day 成熟度模型

| 级别 | 特征 | 频率 | 范围 |
|-----|------|------|------|
| **1. 无演练** | 从未进行过 Game Day | - | - |
| **2. 初级** | 偶尔进行，无标准流程 | 季度 | Staging |
| **3. 规范化** | 有模板、有流程 | 月度 | Staging + Prod 小流量 |
| **4. 自动化** | 自动化编排、持续演练 | 双周 | Prod |
| **5. 持续** | 集成到 CI/CD、常态化 | 每周 | 全环境 |

### 评估问卷

| 问题 | 1分 | 3分 | 5分 |
|-----|-----|-----|-----|
| 演练频率 | 从不 | 季度 | 月度+ |
| 场景覆盖 | 单一 | 3-5 个 | 10+ |
| 自动化程度 | 手动 | 半自动 | 全自动 |
| 问题跟踪 | 无 | 手动 | 自动 |
| 团队参与 | 少数人 | 部分团队 | 全团队 |

## 相关

- [[12-可靠性/04-混沌工程/06-game-day-runbook-template.md|self]]
- [[12-可靠性/04-混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[12-可靠性/07-性能测试/02-chaos-load-integration.md|02 chaos load integration]]
- [[12-可靠性/05-事后复盘/01-blameless-postmortem-template.md|01 blameless postmortem template]]

<!-- risk-assessed -->
