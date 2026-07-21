---
title: Argo Rollouts Deep Dive
description: Argo Rollouts 生产级深度指南 — 架构原理、策略配置、流量管理、指标分析、生产实践
summary: Argo Rollouts 完整生产指南，涵盖 Rollout CRD 架构、金丝雀/蓝绿策略、Istio/Nginx/ALB 流量管理、Prometheus 指标分析、自动回滚、生产案例
tags:
- argo-rollouts
- progressive-delivery
- canary
- blue-green
- kubernetes
difficulty: advanced
domain: 发布变更
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# Argo Rollouts 生产级深度指南

## 1. 架构与核心概念

### 1.1 为什么需要 Argo Rollouts

原生 Kubernetes Deployment 仅支持 RollingUpdate 和 Recreate 两种策略，缺乏：
- **渐进式流量控制**：无法按百分比逐步转移流量
- **自动化指标分析**：无法基于业务指标自动判断发布质量
- **高级部署模式**：不支持蓝绿、金丝雀、A/B 测试

Argo Rollouts 通过自定义 `Rollout` CRD 替代 Deployment，提供完整的渐进式交付能力。

### 1.2 核心组件架构

```
┌─────────────────────────────────────────────────────────┐
│                    Argo Rollouts Controller              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐ │
│  │  Rollout    │  │  Analysis   │  │   Experiment    │ │
│  │  Reconciler │  │  Runner     │  │   Controller    │ │
│  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘ │
│         │                │                   │          │
│  ┌──────▼──────────────────▼───────────────────▼──────┐ │
│  │              Traffic Router Plugins                 │ │
│  │  Istio │ Nginx │ ALB │ SMI │ Traefik │ Ambassador │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 1.3 核心 CRD

| CRD | 用途 | 关键字段 |
|-----|------|----------|
| Rollout | 替代 Deployment | strategy.canary / strategy.blueGreen |
| AnalysisTemplate | 定义指标分析模板 | metrics[].provider / metrics[].successCondition |
| AnalysisRun | 分析模板的执行实例 | status / measurements |
| Experiment | 临时运行多个版本 | templates[] / duration |

## 2. 金丝雀策略深度配置

### 2.1 基础金丝雀配置

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  replicas: 10
  strategy:
    canary:
      # 流量步骤定义
      steps:
        - setWeight: 10        # 10% 流量到金丝雀
        - pause: { duration: 5m }  # 暂停 5 分钟观察
        - setWeight: 30
        - pause: { duration: 5m }
        - setWeight: 60
        - pause: { duration: 5m }
        - setWeight: 100       # 完全切换
      # 金丝雀 Service（可选，用于独立路由）
      canaryService: my-app-canary
      stableService: my-app-stable
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
        - name: my-app
          image: my-app:v2.0
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
```

### 2.2 带自动化分析的金丝雀

```yaml
spec:
  strategy:
    canary:
      steps:
        - setWeight: 10
        - analysis:
            templates:
              - templateName: success-rate-check
            args:
              - name: service-name
                value: my-app-canary
        - setWeight: 50
        - analysis:
            templates:
              - templateName: success-rate-check
              - templateName: latency-check
        - setWeight: 100
```

### 2.3 蓝绿策略配置

```yaml
spec:
  strategy:
    blueGreen:
      activeService: my-app-active
      previewService: my-app-preview
      autoPromotionEnabled: false  # 手动确认切换
      prePromotionAnalysis:
        templates:
          - templateName: smoke-test
      postPromotionAnalysis:
        templates:
          - templateName: success-rate-check
      abortScaleDownDelaySeconds: 30
```

## 3. 流量管理集成

### 3.1 Istio 流量管理

```yaml
spec:
  strategy:
    canary:
      trafficRouting:
        istio:
          virtualService:
            name: my-app-vs
            routes:
              - primary
          destinationRule:
            name: my-app-dr
            canarySubsetName: canary
            stableSubsetName: stable
      steps:
        - setWeight: 10
        - pause: { duration: 5m }
```

对应的 VirtualService：
```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: my-app-vs
spec:
  hosts:
    - my-app
  http:
    - name: primary
      route:
        - destination:
            host: my-app-stable
            subset: stable
          weight: 100
        - destination:
            host: my-app-canary
            subset: canary
          weight: 0
```

### 3.2 Nginx Ingress 流量管理

```yaml
spec:
  strategy:
    canary:
      trafficRouting:
        nginx:
          stableIngress: my-app-ingress
          additionalIngressAnnotations:
            canary-by-header: X-Canary
            canary-by-header-value: "true"
```

### 3.3 AWS ALB 流量管理

```yaml
spec:
  strategy:
    canary:
      trafficRouting:
        alb:
          ingress: my-app-ingress
          servicePort: 80
          rootService: my-app-root
```

## 4. AnalysisTemplate 指标分析

### 4.1 Prometheus 成功率检查

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate-check
spec:
  args:
    - name: service-name
  metrics:
    - name: success-rate
      interval: 60s
      count: 5  # 连续检查 5 次
      successCondition: "result[0] >= 0.99"  # 成功率 >= 99%
      failureLimit: 2  # 允许 2 次失败
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(http_requests_total{
              service="{{args.service-name}}",
              status=~"2.."
            }[5m])) /
            sum(rate(http_requests_total{
              service="{{args.service-name}}"
            }[5m]))
```

### 4.2 延迟检查

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: latency-check
spec:
  metrics:
    - name: latency-p99
      interval: 60s
      count: 3
      successCondition: "result[0] <= 500"  # P99 <= 500ms
      failureLimit: 1
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            histogram_quantile(0.99,
              sum(rate(http_request_duration_seconds_bucket{
                service="my-app-canary"
              }[5m])) by (le)
            ) * 1000
```

### 4.3 多指标组合分析

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: comprehensive-check
spec:
  metrics:
    - name: success-rate
      successCondition: "result[0] >= 0.995"
      failureLimit: 1
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(http_requests_total{status=~"2.."}[5m])) /
            sum(rate(http_requests_total[5m]))
    - name: error-rate
      successCondition: "result[0] <= 0.01"
      failureLimit: 1
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(http_requests_total{status=~"5.."}[5m])) /
            sum(rate(http_requests_total[5m]))
    - name: pod-restarts
      successCondition: "result[0] == 0"
      failureLimit: 0
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(kube_pod_container_status_restarts_total{
              pod=~"my-app-canary.*"
            }) -
            sum(kube_pod_container_status_restarts_total{
              pod=~"my-app-canary.*"
            } offset 5m)
```

## 5. 生产最佳实践

### 5.1 发布流程设计

```
代码提交 → CI 构建 → 镜像推送 → ArgoCD 同步 → Rollout 触发
                                              ↓
                                    金丝雀 10% + 分析
                                              ↓
                                    金丝雀 50% + 分析
                                              ↓
                                    全量发布 / 自动回滚
```

### 5.2 关键配置建议

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| steps 数量 | 3-5 步 | 太少风险大，太多发布慢 |
| pause duration | 3-10m | 给指标足够时间反映问题 |
| analysis interval | 60s | 太短指标不稳定 |
| analysis count | 3-5 | 连续多次确认 |
| failureLimit | 1-2 | 允许偶发抖动 |

### 5.3 回滚策略

```yaml
spec:
  strategy:
    canary:
      # 自动回滚条件
      abortScaleDownDelaySeconds: 30
      # 分析失败自动回滚
      analysis:
        unsuccessfulRunLimit: 3
```

手动回滚命令：
```bash
# 中止当前发布
kubectl argo rollouts abort my-app

# 回滚到上一版本
kubectl argo rollouts undo my-app

# 重试失败的发布
kubectl argo rollouts retry my-app
```

### 5.4 监控与告警

```yaml
# PrometheusRule for Rollout 告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: argo-rollouts-alerts
spec:
  groups:
    - name: argo-rollouts
      rules:
        - alert: RolloutDegraded
          expr: argo_rollouts_rollout_phase{phase="Degraded"} == 1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Rollout {{ $labels.name }} 处于降级状态"
        - alert: AnalysisRunFailed
          expr: argo_rollouts_analysis_run_status{status="Failed"} == 1
          labels:
            severity: warning
          annotations:
            summary: "AnalysisRun {{ $labels.name }} 失败"
```

## 6. 故障排查

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Rollout 卡在 Progressing | 分析失败 | 检查 AnalysisRun 日志 |
| 流量未切换 | TrafficRouting 配置错误 | 检查 VirtualService/Ingress |
| 金丝雀 Pod 未创建 | 资源不足 | 检查节点资源 |
| 分析指标为空 | Prometheus 查询错误 | 手动执行 PromQL 验证 |

### 6.2 诊断命令

```bash
# 查看 Rollout 状态
kubectl argo rollouts get rollout my-app --watch

# 查看分析运行
kubectl get analysisrun -l rollout-name=my-app

# 查看实验
kubectl get experiment -l rollout-name=my-app

# 查看控制器日志
kubectl logs -n argo-rollouts -l app.kubernetes.io/name=argo-rollouts
```

## Related

- [[发布变更/Progressive-Delivery/index.md|Progressive Delivery 索引]]
- [[发布变更/Progressive-Delivery/02-canary-analysis-patterns.md|金丝雀分析模式]]
- [[发布变更/GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts 基础]]
- [[可观测性/指标/index.md|指标监控]]
