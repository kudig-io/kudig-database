---
title: 混沌测试与负载测试集成
description: → 准备回滚方案
summary: → 准备回滚方案
category: domain
tags:
- chaos-engineering
- load-testing
- integration
- reliability
- prometheus
- grafana
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌测试与负载测试集成 是什么
- 如何 混沌测试与负载测试集成
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌测试与负载测试集成
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 混沌测试与负载测试集成

## GameDay 概念

**GameDay** = 有组织的、可控的生产环境实验，验证系统在真实负载下的韧性。

## GameDay 流程

```
1. 规划 (2 周前)
   → 确定实验场景
   → 设定成功/失败标准
   → 准备回滚方案

2. 基线测量 (1 周前)
   → 在无问题情况下运行负载测试
   → 记录正常性能指标

3. 执行 (GameDay)
   → 启动负载测试
   → 注入问题
   → 实时监控 SLO

4. 验证
   → SLO 是否达标？
   → 自动恢复是否生效？

5. 复盘
   → 记录发现
   → 制定改进措施
```

## 集成架构

```
┌─────────────┐     ┌─────────────┐
│  Load Test  │     │  Chaos Test │
│   (k6)      │     │ (Chaos Mesh)│
└──────┬──────┘     └──────┬──────┘
       │                   │
       └─────────┬─────────┘
                 ▼
        ┌─────────────────┐
        │  Target System  │
        │  (Production)   │
        └────────┬────────┘
                 ▼
        ┌─────────────────┐
        │   Observability │
        │ (Prometheus/    │
        │  Grafana/SLO)   │
        └─────────────────┘
```

## 完整 GameDay 执行手册

### 阶段 1: 规划 (T-14 天)

```markdown
# GameDay 规划文档

## 基本信息
- 日期: 2026-07-28
- 时间窗口: 02:00-06:00 (低峰期)
- 参与人员: SRE, 后端开发, 产品经理
- 审批状态: ☐ 待审批

## 实验场景
| 场景 | 目标服务 | 故障类型 | 预期影响 |
|-----|---------|---------|----------|
| 场景 1 | payment-api | Pod Kill (30%) | 短暂 5xx，自动恢复 |
| 场景 2 | order-service | 网络延迟 (200ms) | 延迟增加，不超时 |
| 场景 3 | inventory-db | 主从切换 | 短暂不可用，自动恢复 |

## 成功标准
- [ ] SLO 达标率 > 99%
- [ ] MTTD < 1 分钟
- [ ] MTTR < 5 分钟
- [ ] 无数据丢失

## 回滚方案
- 紧急停止: `kubectl delete podchaos --all -n production`
- 联系人: @sre-oncall
- 升级路径: SRE → 技术总监 → CTO
```

### 阶段 2: 基线测量 (T-7 天)

```bash
#!/bin/bash
# 🟢 低风险：基线性能测量
set -euo pipefail

echo "=== 基线性能测量 $(date) ==="

# 1. 运行负载测试（无故障）
k6 run --out json=baseline.json tests/load-test.js

# 2. 记录关键指标
echo "[1] 基线指标:"
P95=$(cat baseline.json | jq '.metrics.http_req_duration["p(95)"]')
ERROR_RATE=$(cat baseline.json | jq '.metrics.http_req_failed.rate')
RPS=$(cat baseline.json | jq '.metrics.http_reqs.rate')

echo "  P95 延迟: ${P95}ms"
echo "  错误率: ${ERROR_RATE}"
echo "  吞吐量: ${RPS} RPS"

# 3. 保存基线
cat > baseline-metrics.yaml <<EOF
baseline:
  timestamp: $(date -Iseconds)
  p95_latency_ms: $P95
  error_rate: $ERROR_RATE
  rps: $RPS
EOF

echo "=== 基线测量完成 ==="
```

### 阶段 3: 执行 (GameDay)

```yaml
# Argo Workflow 自动化 GameDay
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: gameday-execution
  namespace: chaos-automation
spec:
  entrypoint: gameday-steps
  templates:
    - name: gameday-steps
      steps:
        - - name: pre-check
            template: pre-flight-check
        - - name: start-load
            template: start-load-test
        - - name: inject-chaos
            template: inject-chaos
            arguments:
              parameters:
                - name: chaos-type
                  value: pod-kill
        - - name: observe
            template: observe-and-monitor
        - - name: stop-chaos
            template: stop-chaos
        - - name: verify-recovery
            template: verify-recovery
        - - name: generate-report
            template: generate-report

    - name: pre-flight-check
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 执行前检查 ==="
            # 检查目标服务健康
            kubectl get pods -n production -l app=payment-api
            # 检查监控正常
            curl -s http://prometheus:9090/-/healthy
            # 检查回滚方案就绪
            echo "✓ 检查完成"

    - name: start-load-test
      container:
        image: grafana/k6:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 启动负载测试 ==="
            k6 run --out json=/tmp/load-test.json /tests/load-test.js &
            echo $! > /tmp/k6-pid
            sleep 60  # 等待负载稳定

    - name: inject-chaos
      inputs:
        parameters:
          - name: chaos-type
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 注入混沌: {{inputs.parameters.chaos-type}} ==="
            kubectl apply -f /chaos/{{inputs.parameters.chaos-type}}.yaml
            echo "✓ 混沌已注入"

    - name: observe-and-monitor
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 观察期 (5 分钟) ==="
            for i in {1..10}; do
              echo "--- 检查点 $i ---"
              # 检查 SLO
              curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))'
              sleep 30
            done

    - name: stop-chaos
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 停止混沌 ==="
            kubectl delete podchaos --all -n production
            echo "✓ 混沌已停止"

    - name: verify-recovery
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            echo "=== 验证恢复 ==="
            sleep 60
            # 检查服务状态
            kubectl get pods -n production -l app=payment-api
            # 检查错误率恢复
            curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))'

    - name: generate-report
      container:
        image: python:3.11-slim
        command: [python, /scripts/generate_report.py]
```

### 阶段 4: 复盘 (T+1 天)

```markdown
# GameDay 复盘报告

## 执行摘要
- 日期: 2026-07-28
- 场景: Pod Kill (30%), 网络延迟 (200ms)
- 结果: ✅ 通过 / ⚠️ 部分通过 / ❌ 失败

## 指标对比
| 指标 | 基线 | 混沌期间 | 恢复后 | 目标 | 状态 |
|-----|------|---------|-------|------|------|
| P95 延迟 | 120ms | 450ms | 130ms | < 500ms | ✅ |
| 错误率 | 0.1% | 3.2% | 0.2% | < 5% | ✅ |
| MTTD | - | 45s | - | < 60s | ✅ |
| MTTR | - | 3m20s | - | < 5m | ✅ |

## 发现的问题
1. **问题**: 第 2 分钟出现 8% 错误率峰值
   - **根因**: HPA 扩容延迟 (60s)
   - **改进**: 调整 HPA 稳定窗口

2. **问题**: 部分客户端未配置重试
   - **根因**: SDK 版本过旧
   - **改进**: 升级客户端 SDK

## 改进行动
| 行动 | 负责人 | 截止日期 | 状态 |
|-----|-------|---------|------|
| 优化 HPA 扩容速度 | @sre | 08-05 | ☐ |
| 升级客户端 SDK | @backend | 08-10 | ☐ |
| 增加 PDB minAvailable | @sre | 08-01 | ☐ |
```

## 场景库

### 基础场景

| 场景 | 故障类型 | 目标 | 验证点 |
|-----|---------|------|-------|
| **Pod 弹性** | Pod Kill | Deployment | 自动重启、服务不中断 |
| **网络韧性** | 网络延迟 | Service | 超时配置、重试机制 |
| **依赖故障** | 依赖服务不可用 | 外部 API | 降级、熔断 |
| **资源压力** | CPU/内存压力 | Node | HPA、资源限制 |
| **存储故障** | PVC 不可用 | StatefulSet | 数据持久化、恢复 |

### 高级场景

| 场景 | 故障类型 | 目标 | 验证点 |
|-----|---------|------|-------|
| **AZ 故障** | 节点批量终止 | 跨 AZ 部署 | 拓扑分布、自动恢复 |
| **级联故障** | 多服务同时故障 | 微服务链路 | 熔断、降级、隔离 |
| **数据一致性** | 数据库主从切换 | StatefulSet | 数据不丢失、一致性 |
| **配置错误** | ConfigMap 错误 | 配置加载 | 回滚、告警 |
| **证书过期** | TLS 证书失效 | Ingress | 自动轮换、告警 |

## 持续混沌工程

### 自动化调度

```yaml
# 定期混沌实验 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: continuous-chaos
  namespace: chaos-automation
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨 3 点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
            - name: chaos-runner
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 持续混沌实验 $(date) ==="
                  
                  # 随机选择场景
                  SCENARIOS=("pod-kill" "network-delay" "cpu-stress")
                  SCENARIO=${SCENARIOS[$RANDOM % ${#SCENARIOS[@]}]}
                  echo "本次场景: $SCENARIO"
                  
                  # 应用实验
                  kubectl apply -f /chaos/$SCENARIO.yaml
                  
                  # 等待实验完成
                  sleep 300
                  
                  # 清理
                  kubectl delete -f /chaos/$SCENARIO.yaml
                  
                  # 生成报告
                  echo "实验完成: $SCENARIO"
```

### 混沌实验仪表盘

```
┌─────────────────────────────────────────────────────────────────┐
│  Chaos Engineering Dashboard                                    │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │
│  │ 本周实验    │  │ 成功率      │  │ 发现问题    │           │
│  │ 12 次       │  │ 92%         │  │ 3 个        │           │
│  └─────────────┘  └─────────────┘  └─────────────┘           │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ 实验历史 (30天)                                         │   │
│  │ [============================]                          │   │
│  └─────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│  ┌────────────────────────┐  ┌────────────────────────┐        │
│  │ MTTD 趋势              │  │ MTTR 趋势              │        │
│  │ [====================] │  │ [====================] │        │
│  └────────────────────────┘  └────────────────────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

## GameDay 检查清单

### 执行前

| 序号 | 检查项 | 状态 |
|-----|--------|------|
| 1 | 实验场景已确定 | ☐ |
| 2 | 成功/失败标准已定义 | ☐ |
| 3 | 回滚方案已准备并验证 | ☐ |
| 4 | 监控仪表盘已就绪 | ☐ |
| 5 | 基线性能已测量 | ☐ |
| 6 | 通知已发送给相关团队 | ☐ |
| 7 | 审批已获得 | ☐ |
| 8 | 备份已完成 | ☐ |

### 执行中

| 序号 | 检查项 | 状态 |
|-----|--------|------|
| 1 | 负载测试已启动 | ☐ |
| 2 | 混沌已按计划注入 | ☐ |
| 3 | 实时监控 SLO | ☐ |
| 4 | 记录关键时间点 | ☐ |
| 5 | 异常时立即停止 | ☐ |

### 执行后

| 序号 | 检查项 | 状态 |
|-----|--------|------|
| 1 | 混沌已完全清理 | ☐ |
| 2 | 服务已完全恢复 | ☐ |
| 3 | 指标已收集 | ☐ |
| 4 | 报告已生成 | ☐ |
| 5 | 复盘会议已安排 | ☐ |
| 6 | 改进行动已分配 | ☐ |

## 相关

- [[12-可靠性/04-混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[12-可靠性/07-性能测试/01-load-testing-methodology.md|01 load testing methodology]]


<!-- risk-assessed -->
