---
title: 混沌实验自动化
description: 在 CI/CD 流水线与定时任务中自动化运行混沌实验，含假设自动校验与失败门控
summary: Argo Workflow + Chaos Mesh + Prometheus 自动校验稳态假设，实验失败即阻断发布
category: reliability
tags:
- slo
- sli
- reliability
- chaos-engineering
- ci-cd
- automation
- argo
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

# 混沌实验自动化

> **核心原则**：混沌实验不是"手动跑一次截图"，而是**与单元测试同等级的自动化质量门**。手动实验发现一次问题，自动化实验防止它回归一千次。把稳态假设写进流水线，让它每次发布都自动追问"我的韧性承诺还成立吗？"

## 自动化三层架构

```
┌──────────────────────────────────────────────┐
│  触发层   CI 流水线 / 定时 Cron / 发布事件      │
└──────────────────┬───────────────────────────┘
                   ▼
┌──────────────────────────────────────────────┐
│  执行层   Argo Workflow → Chaos Mesh CR       │
│           注入故障 + 采集稳态指标              │
└──────────────────┬───────────────────────────┘
                   ▼
┌──────────────────────────────────────────────┐
│  门控层   Prometheus 查询假设 → pass/fail      │
│           fail 阻断发布 / 开 Incident          │
└──────────────────────────────────────────────┘
```

## 1. 实验定义（稳态假设先行）

```yaml
# chaos-experiments/api-pod-kill.yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata: { name: api-resilience, namespace: staging }
spec:
  entrypoint: main
  templates:
    - name: main
      templateType: Serial
      children:
        - baseline-check   # 先确认稳态
        - inject-fault     # 再注入
        - hypothesis-check # 再验证
        - cleanup
    - name: baseline-check
      templateType: Task
      task:
        container:
          image: prometheus-checker:latest
          command: ["/bin/sh", "-c"]
          args: ["verify-slo --window 5m --exit-on-breach"]
    - name: inject-fault
      templateType: NetworkChaos
      networkChaos:
        selector: { namespaces: [staging], labelSelectors: {"app":"api"} }
        action: delay
        mode: all
        delay: { latency: "200ms" }
        duration: "120s"
    - name: hypothesis-check
      templateType: Task
      task:
        container:
          image: prometheus-checker:latest
          command: ["/bin/sh", "-c"]
          args:
            - |
              # 稳态假设：P99 仍 < 800ms，错误率仍 < 1%
              verify-slo \
                --p99-latency 800ms \
                --error-rate 0.01 \
                --window 2m
```

## 2. 假设校验脚本（PromQL）

```bash
#!/bin/bash
# verify-slo.sh — 用 PromQL 校验稳态假设，失败 exit 1
set -euo pipefail

LATENCY=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=histogram_quantile(0.99, sum by(le)(rate(http_request_duration_seconds_bucket{job="api"}[2m])))' \
  | jq -r '.data.result[0].value[1]')

ERRRATE=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=sum(rate(http_requests_total{job="api",code=~"5.."}[2m]))/sum(rate(http_requests_total{job="api"}[2m]))' \
  | jq -r '.data.result[0].value[1]')

awk "BEGIN{exit !($LATENCY > 0.8)}" && { echo "FAIL P99=${LATENCY}s > 0.8s"; exit 1; }
awk "BEGIN{exit !($ERRRATE > 0.01)}" && { echo "FAIL err=${ERRRATE} > 0.01"; exit 1; }
echo "PASS p99=${LATENCY}s err=${ERRRATE}"
```

## 3. CI 流水线门控

```yaml
# .github/workflows/chaos-gate.yml
name: Chaos Gate
on: { pull_request: { branches: [main] } }
jobs:
  chaos:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Deploy to staging
        run: argocd app sync api-staging
      - name: Run chaos experiment
        run: |
          kubectl apply -f chaos-experiments/api-pod-kill.yaml -n staging
          kubectl wait workflow/api-resilience --for=condition=complete --timeout=10m -n staging
      - name: Read verdict
        run: |
          STATUS=$(kubectl get workflow api-resilience -n staging -o jsonpath='{.status.conditions[?(@.type=="Complete")].reason}')
          [ "$STATUS" = "Succeeded" ] || { echo "::error::Chaos experiment failed"; exit 1; }
```

## 4. 定时回归实验（Cron）

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: Schedule
metadata: { name: nightly-resilience, namespace: staging }
spec:
  schedule: "0 2 * * *"        # 每晚 2 点
  historyLimit: 7
  workflow:                     # 引用上面的 Workflow
    entrypoint: main
    templates: [...]
```

定时实验的失败应自动开 Jira 工单并 Slack 通知，**不阻断发布**（与 CI 门控区分）。

## 实验纳入"黄金套件"

| 实验类型 | 频率 | 门控方式 |
|---------|------|---------|
| Pod 删除 | 每次发布 | CI 阻断 |
| 网络延迟 | 每次发布 | CI 阻断 |
| 依赖超时 | 每晚 | 工单 |
| 节点宕机 | 每周 | 工单 + 邮件 |
| AZ 故障 | 每月 | Game Day 手动 |

## 失败处理自动化

```
实验失败
   │
   ├─ CI 模式 → 阻断 PR + 评论实验报告 + @owner
   └─ Cron 模式 → 自动开 Incident(Sev3) + 采集现场指标快照
```

## 常见陷阱

1. **假设太宽松**：稳态阈值定成"不崩就行"等于没实验。阈值必须与 SLO 一致。
2. **没 baseline**：注入前不验证稳态，会误把已有问题归咎于实验。
3. **实验在隔离环境跑**：生产配置漂移会让 staging 实验失去意义，定期在 prod 小流量演练。
4. **失败后不修复**：实验发现的问题进 backlog 就消失了——必须像生产 bug 一样排期。

## 相关

- [[可靠性/混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[可靠性/混沌工程/07-blast-radius-control.md|07 blast radius control]]
- [[可靠性/性能测试/02-chaos-load-integration.md|02 chaos load integration]]

<!-- risk-assessed -->
