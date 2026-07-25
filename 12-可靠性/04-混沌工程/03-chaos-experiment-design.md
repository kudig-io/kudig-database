---
title: 混沌实验设计与执行
description: '# 混沌实验设计与执行'
summary: '# 混沌实验设计与执行'
category: domain
tags:
- chaos-engineering
- experiment-design
- reliability
- redis
- mysql
- postgresql
- gateway
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌实验设计与执行 是什么
- 如何 混沌实验设计与执行
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌实验设计与执行
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 混沌实验设计与执行

## 实验设计流程

```
1. 定义稳态假设
   → "正常情况下，服务延迟 P99 < 200ms，错误率 < 0.1%"

2. 识别变量（要注入的问题）
   → 网络延迟增加 100ms

3. 定义成功标准
   → P99 < 400ms，错误率 < 1%

4. 设定中止条件
   → 错误率 > 5% 或 P99 > 2s

5. 执行实验
   → 使用 Chaos Mesh 注入问题

6. 监控和记录
   → 实时观察指标变化

7. 分析结果
   → 是否验证/否定假设？

8. 改进和迭代
   → 修复发现的问题
```

## 8 类经典实验模板

### 1. 依赖超时实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: dependency-timeout
spec:
  action: delay
  mode: one
  selector:
    labelSelectors:
      app: order-service
  delay:
    latency: "500ms"
    correlation: "100"
    jitter: "0ms"
  target:
    selector:
      labelSelectors:
        app: payment-service
    mode: all
  duration: "5m"
```

### 2. Pod 级联问题实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-cascade-failure
spec:
  action: pod-kill
  mode: fixed-percent
  value: "30"
  selector:
    labelSelectors:
      app: api-gateway
  duration: "30s"
```

### 3. CPU 饱和实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: StressChaos
metadata:
  name: cpu-saturation
spec:
  mode: all
  selector:
    labelSelectors:
      app: data-processor
  stressors:
    cpu:
      workers: 4
      load: 80
  duration: "5m"
```

### 4. 数据库连接池耗尽实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: db-connection-exhaust
spec:
  action: loss
  mode: all
  selector:
    labelSelectors:
      app: order-service
  loss:
    loss: "100"
  target:
    selector:
      labelSelectors:
        app: postgresql
    mode: all
  duration: "2m"
```

### 5. 网络分区实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-partition
spec:
  action: partition
  mode: all
  selector:
    labelSelectors:
      app: service-a
  direction: to
  target:
    selector:
      labelSelectors:
        app: service-b
    mode: all
  duration: "5m"
```

### 6. 证书过期实验

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 模拟证书即将过期
kubectl create secret tls expired-cert \
  --cert=expired.crt \
  --key=expired.key \
  -n default --dry-run=client -o yaml | kubectl apply -f -
```
### 7. 配置错误实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: HTTPChaos
metadata:
  name: config-error-simulate
spec:
  mode: all
  selector:
    labelSelectors:
      app: config-service
  target: Request
  port: 8080
  path: /api/v1/config
  method: GET
  abort: true
  duration: "3m"
```

### 8. 级联依赖问题实验

```yaml
# 同时注入多个问题
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata:
  name: cascade-failure
spec:
  templates:
    - name: kill-cache
      templateType: Schedule
      deadline: 2m
      schedule:
        schedule: "*/1m"
        concurrencyPolicy: Allow
        type: PodChaos
        podChaos:
          action: pod-kill
          mode: one
          selector:
            labelSelectors:
              app: redis
    - name: delay-db
      templateType: Schedule
      deadline: 5m
      schedule:
        schedule: "*/1m"
        concurrencyPolicy: Allow
        type: NetworkChaos
        networkChaos:
          action: delay
          mode: all
          selector:
            labelSelectors:
              app: mysql
          delay:
            latency: "2s"
```

## 实验安全清单

```
执行前检查:
□ 实验范围已限制（命名空间/标签选择器）
□ 中止条件已设定（自动或手动）
□ 相关团队已通知
□ 监控和告警已就绪
□ 回滚方案已准备
□ 实验时间避开高峰期
```

## 实验结果分析

### 结果分类

| 结果类型 | 含义 | 后续行动 |
|---------|------|----------|
| **假设验证** | 系统按预期处理故障 | 记录成功，提升实验强度 |
| **假设否定** | 系统未达预期，发现问题 | 开 Incident，修复后重测 |
| **实验无效** | 实验未实际注入故障 | 检查实验配置，重新执行 |
| **实验中止** | 触发中止条件，提前结束 | 分析中止原因，调整参数 |

### 结果分析脚本

```bash
#!/bin/bash
# 🟢 低风险：分析实验结果
set -euo pipefail

EXPERIMENT_NAME=${1:-"dependency-timeout"}
NAMESPACE=${2:-"production"}

echo "=== 实验结果分析: $EXPERIMENT_NAME ==="

# 获取实验状态
STATUS=$(kubectl get podchaos,networkchaos,stresschaos $EXPERIMENT_NAME -n $NAMESPACE -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")
START_TIME=$(kubectl get podchaos,networkchaos,stresschaos $EXPERIMENT_NAME -n $NAMESPACE -o jsonpath='{.status.startTime}' 2>/dev/null || echo "N/A")
END_TIME=$(kubectl get podchaos,networkchaos,stresschaos $EXPERIMENT_NAME -n $NAMESPACE -o jsonpath='{.status.endTime}' 2>/dev/null || echo "N/A")

# 获取指标数据
P99_BEFORE=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=histogram_quantile(0.99, sum by(le)(rate(http_request_duration_seconds_bucket{job="api"}[5m] offset 10m)))' \
  | jq -r '.data.result[0].value[1]')

P99_DURING=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=histogram_quantile(0.99, sum by(le)(rate(http_request_duration_seconds_bucket{job="api"}[5m])))' \
  | jq -r '.data.result[0].value[1]')

ERROR_RATE=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=sum(rate(http_requests_total{job="api",code=~"5.."}[5m]))/sum(rate(http_requests_total{job="api"}[5m]))' \
  | jq -r '.data.result[0].value[1]')

echo "实验状态: $STATUS"
echo "开始时间: $START_TIME"
echo "结束时间: $END_TIME"
echo ""
echo "=== 指标对比 ==="
echo "P99 延迟 (实验前): ${P99_BEFORE}s"
echo "P99 延迟 (实验中): ${P99_DURING}s"
echo "错误率: ${ERROR_RATE}"
echo ""

# 判断结果
if (( $(echo "$ERROR_RATE > 0.05" | bc -l) )); then
  echo "❌ 结果: 假设否定 - 错误率超过 5%"
  echo "建议: 开 Incident 调查，修复后重测"
elif (( $(echo "$P99_DURING > $P99_BEFORE * 2" | bc -l) )); then
  echo "⚠️ 结果: 部分验证 - 延迟显著增加但未超限"
  echo "建议: 优化超时配置，增加重试"
else
  echo "✅ 结果: 假设验证 - 系统按预期处理故障"
  echo "建议: 提升实验强度，扩大爆炸半径"
fi
```

## 实验报告模板

```markdown
# 混沌实验报告

## 实验信息

| 项目 | 内容 |
|-----|------|
| 实验名称 | dependency-timeout |
| 实验类型 | 网络延迟注入 |
| 目标服务 | order-service → payment-service |
| 执行时间 | 2026-07-21 14:00 - 14:30 |
| 执行人 | @sre-team |

## 稳态假设

> 在正常情况下，order-service 的 P99 延迟 < 200ms，错误率 < 0.1%。
> 当 payment-service 增加 500ms 延迟时，order-service 应通过超时和降级保持 P99 < 400ms，错误率 < 1%。

## 实验配置

```yaml
action: delay
latency: 500ms
duration: 5m
target: payment-service
```

## 实验结果

| 指标 | 实验前 | 实验中 | 阈值 | 结果 |
|-----|-------|-------|------|------|
| P99 延迟 | 180ms | 350ms | < 400ms | ✅ 通过 |
| 错误率 | 0.05% | 0.8% | < 1% | ✅ 通过 |
| RPS | 1200 | 1150 | > 1000 | ✅ 通过 |

## 发现的问题

1. 超时配置为 3s，过长，建议调整为 1s
2. 缺少熔断机制，连续失败时未快速失败

## 改进行动

| 行动 | 负责人 | 截止日期 | 状态 |
|-----|-------|---------|------|
| 调整超时配置为 1s | @dev-team | 7/25 | ☐ |
| 增加熔断器 | @dev-team | 7/31 | ☐ |
| 重新执行实验验证 | @sre-team | 8/1 | ☐ |

## 结论

✅ 假设验证：系统基本按预期处理故障，但存在优化空间。
```

## 实验优先级矩阵

| 实验类型 | 影响范围 | 实施难度 | 发现价值 | 优先级 |
|---------|---------|---------|---------|--------|
| Pod Kill | 低 | 低 | 高 | 🔴 P0 |
| 网络延迟 | 中 | 低 | 高 | 🔴 P0 |
| CPU 压力 | 中 | 低 | 中 | 🟡 P1 |
| 依赖超时 | 中 | 中 | 高 | 🔴 P0 |
| 网络分区 | 高 | 中 | 高 | 🟡 P1 |
| 磁盘 IO | 中 | 中 | 中 | 🟡 P1 |
| 数据库故障 | 高 | 高 | 高 | 🟡 P1 |
| AZ 故障 | 极高 | 高 | 极高 | 🟢 P2 |

### 实验路线图

```
第 1 个月 (基础):
  - Pod Kill (单副本)
  - 网络延迟 (100ms)
  - CPU 压力 (50%)

第 2 个月 (进阶):
  - Pod Kill (30% 副本)
  - 网络延迟 (500ms)
  - 依赖超时
  - 网络分区

第 3 个月 (高级):
  - 数据库故障转移
  - AZ 故障模拟
  - 级联故障实验
```

## 与 CI/CD 集成

### GitHub Actions 集成

```yaml
# .github/workflows/chaos-gate.yml
name: Chaos Gate
on:
  pull_request:
    branches: [main]

jobs:
  chaos-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Deploy to staging
        run: |
          argocd app sync api-staging
          kubectl rollout status deployment/api -n staging --timeout=5m
      
      - name: Run chaos experiment
        run: |
          kubectl apply -f chaos-experiments/pod-kill.yaml -n staging
          kubectl wait podchaos/api-pod-kill --for=condition=complete --timeout=5m -n staging
      
      - name: Verify SLO
        run: |
          ERROR_RATE=$(curl -sG "$PROM/api/v1/query" \
            --data-urlencode 'query=sum(rate(http_requests_total{job="api",code=~"5.."}[2m]))/sum(rate(http_requests_total{job="api"}[2m]))' \
            | jq -r '.data.result[0].value[1]')
          
          if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
            echo "::error::错误率 $ERROR_RATE 超过 1%"
            exit 1
          fi
      
      - name: Cleanup
        if: always()
        run: |
          kubectl delete podchaos api-pod-kill -n staging --ignore-not-found
```

### Argo Rollouts 集成

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: chaos-analysis
spec:
  args:
    - name: service-name
  metrics:
    - name: chaos-test
      interval: 5m
      count: 1
      provider:
        job:
          spec:
            template:
              spec:
                containers:
                  - name: chaos-test
                    image: chaos-runner:latest
                    command: [sh, -c]
                    args:
                      - |
                        kubectl apply -f chaos-experiments/pod-kill.yaml -n staging
                        kubectl wait podchaos/api-pod-kill --for=condition=complete --timeout=5m
                        
                        # 验证 SLO
                        verify-slo --service {{args.service-name}} --window 2m
                restartPolicy: Never
```

## 故障排查

### 实验未生效排查

```bash
# 🟢 低风险：检查实验状态
kubectl get podchaos,networkchaos,stresschaos -A

# 🟢 低风险：查看实验事件
kubectl describe podchaos <name> -n <namespace>

# 🟢 低风险：检查 Chaos Mesh 组件
kubectl get pods -n chaos-mesh
kubectl logs -n chaos-mesh -l app=chaos-mesh-controller-manager --tail=100

# 🟢 低风险：检查目标 Pod 标签
kubectl get pods -n <namespace> --show-labels | grep <app-label>
```

### 常见问题诊断

| 问题 | 可能原因 | 解决方案 |
|-----|---------|----------|
| 实验一直 Pending | RBAC 权限不足 | 检查 ServiceAccount 权限 |
| 实验未影响目标 | 标签选择器不匹配 | 验证 labelSelectors 配置 |
| 实验立即结束 | duration 设置过短 | 调整 duration 参数 |
| 实验无法停止 | finalizer 阻塞 | 手动删除 finalizer |
| 指标无变化 | 实验未实际注入 | 检查 Chaos Mesh 日志 |

## 相关

- [[12-可靠性/04-混沌工程/01-chaos-engineering-overview.md|01 chaos engineering overview]]
- deployment]]


<!-- risk-assessed -->
