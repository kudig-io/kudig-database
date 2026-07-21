---
title: Flagger Progressive Delivery and Automated Canary Releases
description: K8s 渐进式交付 — Flagger 自动化金丝雀、A/B 测试、蓝绿部署、指标分析、Webhook 集成、多网格支持
summary: 使用 Flagger 实现 Kubernetes 上的自动化渐进式交付与金丝雀发布
category: practice
tags:
- flagger
- progressive-delivery
- canary
- ab-testing
- deployment
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: release
---
# Flagger 渐进式交付生产实践

> 自动化金丝雀发布、A/B 测试与蓝绿部署的完整实践。

## Flagger vs Argo Rollouts 对比

| 特性 | Flagger | Argo Rollouts |
|------|---------|---------------|
| 部署模型 | 独立 Deployment + Service | Rollout CRD 替代 Deployment |
| 网格集成 | Istio/Linkerd/App Mesh/Gateway API | Istio/Nginx/ALB/Traefik |
| 分析引擎 | 内置 + Webhook | AnalysisTemplate + Provider |
| A/B 测试 | ✅ 基于 Header/Cookie | ✅ 基于 Header |
| 蓝绿部署 | ✅ | ✅ |
| 实验（Experiment） | ❌ | ✅ |
| 学习曲线 | 低（不改 Deployment） | 中（需替换为 Rollout） |
| 适用场景 | 已有 Service Mesh | 无 Mesh / Ingress 场景 |

## Flagger 安装

```bash
# 安装 Flagger（Istio 集成）
helm repo add flagger https://flagger.app
helm install flagger flagger/flagger \
  --namespace istio-system \
  --set meshProvider=istio \
  --set metricsServer=http://prometheus.monitoring:9090 \
  --set slack.url=https://hooks.slack.com/services/xxx \
  --set slack.channel=deployments \
  --set slack.user=Flagger

# 或 Gateway API 模式（无需 Mesh）
helm install flagger flagger/flagger \
  --namespace flagger-system --create-namespace \
  --set meshProvider=gatewayapi:v1 \
  --set metricsServer=http://prometheus.monitoring:9090
```

## 金丝雀发布（Canary）

### Canary CRD 定义

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: api-server
  namespace: production
spec:
  provider: istio
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  autoscalerRef:
    apiVersion: keda.sh/v1alpha1
    kind: ScaledObject
    name: api-server-scaler
  service:
    port: 8080
    targetPort: 8080
    gateways:
      - production-gateway
    hosts:
      - api.example.com
    trafficPolicy:
      tls:
        mode: ISTIO_MUTUAL
  analysis:
    # 发布节奏
    interval: 1m
    threshold: 5          # 最多失败 5 次后回滚
    maxWeight: 50         # 最大流量 50%
    stepWeight: 10        # 每步增加 10%
    stepWeightPromotion: 10
    # 指标分析
    metrics:
      - name: request-success-rate
        templateRef:
          name: success-rate
          namespace: flagger-system
        thresholdRange:
          min: 99.5
        interval: 1m
      - name: request-duration
        templateRef:
          name: latency
          namespace: flagger-system
        thresholdRange:
          max: 500  # P99 < 500ms
        interval: 30s
      - name: error-rate
        templateRef:
          name: error-rate
          namespace: flagger-system
        thresholdRange:
          max: 1  # 错误率 < 1%
        interval: 1m
    # Webhook 验证
    webhooks:
      - name: smoke-test
        type: pre-rollout
        url: http://flagger-loadtester.production/
        timeout: 30s
        metadata:
          type: bash
          cmd: "curl -sf http://api-server-canary.production:8080/health"
      - name: load-test
        type: rollout
        url: http://flagger-loadtester.production/
        timeout: 60s
        metadata:
          type: cmd
          cmd: "hey -z 1m -q 10 -c 2 http://api-server-canary.production:8080/api/v1/status"
      - name: integration-test
        type: post-rollout
        url: http://test-runner.production/run
        timeout: 300s
        metadata:
          suite: integration
          version: "{{ .Metadata.version }}"
```

### MetricTemplate（自定义指标）

```yaml
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: success-rate
  namespace: flagger-system
spec:
  provider:
    type: prometheus
    address: http://prometheus.monitoring:9090
  query: |
    100 - sum(
      rate(istio_requests_total{
        reporter="destination",
        destination_workload_namespace="{{ namespace }}",
        destination_workload=~"{{ target }}",
        response_code!~"5.*"
      }[{{ interval }}])
    )
    /
    sum(
      rate(istio_requests_total{
        reporter="destination",
        destination_workload_namespace="{{ namespace }}",
        destination_workload=~"{{ target }}"
      }[{{ interval }}])
    ) * 100
---
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: latency
  namespace: flagger-system
spec:
  provider:
    type: prometheus
    address: http://prometheus.monitoring:9090
  query: |
    histogram_quantile(0.99,
      sum(
        rate(istio_request_duration_milliseconds_bucket{
          reporter="destination",
          destination_workload_namespace="{{ namespace }}",
          destination_workload=~"{{ target }}"
        }[{{ interval }}])
      ) by (le)
    )
```

## A/B 测试

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: web-frontend
  namespace: production
spec:
  provider: istio
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-frontend
  service:
    port: 80
    targetPort: 3000
  analysis:
    interval: 1m
    threshold: 5
    # A/B 测试：基于 Cookie 分流
    match:
      - headers:
          x-canary:
            exact: "true"
      - headers:
          cookie:
            regex: "^(.*?;)?(canary=always)(;.*)?$"
    iterations: 10  # 10 轮迭代后全量
    metrics:
      - name: request-success-rate
        thresholdRange:
          min: 99
        interval: 1m
      - name: conversion-rate
        templateRef:
          name: business-conversion
        thresholdRange:
          min: 2.5  # 转化率不低于 2.5%
        interval: 5m
    webhooks:
      - name: load-test
        type: rollout
        url: http://flagger-loadtester.production/
        metadata:
          type: cmd
          cmd: "hey -z 2m -q 5 -c 2 -H 'x-canary: true' http://web-frontend.production/"
```

## 蓝绿部署

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: payment-service
  namespace: production
spec:
  provider: istio
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: payment-service
  service:
    port: 8443
    targetPort: 8443
  analysis:
    # 蓝绿：iterations 控制测试轮数，然后一次性切换
    iterations: 5
    interval: 1m
    metrics:
      - name: request-success-rate
        thresholdRange:
          min: 99.9
        interval: 1m
    webhooks:
      - name: smoke-test
        type: pre-rollout
        url: http://flagger-loadtester.production/
        metadata:
          type: bash
          cmd: "curl -sf https://payment-service-canary.production:8443/health"
      - name: integration-test
        type: rollout
        url: http://test-runner.production/run
        metadata:
          suite: payment-integration
```

## 发布流程可视化

```
触发（Deployment 镜像更新）
    │
    ▼
┌─────────────┐     失败 → 回滚
│ 初始化       │────────────────→ 恢复 primary
│ (Canary 0%) │
└──────┬──────┘
       ▼
┌─────────────┐     指标异常 → 回滚
│ 步进 10%    │────────────────→ 恢复 primary
│ 分析指标    │
└──────┬──────┘
       ▼
┌─────────────┐     指标异常 → 回滚
│ 步进 20%    │────────────────→ 恢复 primary
│ 分析指标    │
└──────┬──────┘
       ▼
      ...
       ▼
┌─────────────┐
│ 50% 稳定    │
│ 最终确认    │
└──────┬──────┘
       ▼
┌─────────────┐
│ 提升 100%   │
│ 清理 Canary │
└─────────────┘
```

## 监控与告警

```bash
# 查看 Canary 状态
kubectl get canaries -A
kubectl describe canary api-server -n production

# 查看发布事件
kubectl get events -n production --field-selector reason=Synced
kubectl logs -n istio-system deploy/flagger --tail=100

# 手动触发回滚
kubectl annotate canary api-server -n production \
  flagger.app/rollback="true" --overwrite

# 暂停发布
kubectl annotate canary api-server -n production \
  flagger.app/suspend="true" --overwrite
```

### Prometheus 监控指标

```promql
# 金丝雀流量权重
flagger_canary_weight{namespace="production", name="api-server"}

# 发布状态（0=等待, 1=进行中, 2=成功）
flagger_canary_status{namespace="production", name="api-server"}

# 发布持续时间
flagger_canary_duration_seconds{namespace="production"}
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 设置合理 threshold | 生产环境 3-5 次失败即回滚 |
| 多指标组合 | 成功率 + 延迟 + 业务指标 |
| 负载测试 Webhook | 每步施加压力验证性能 |
| 渐进时间窗口 | 避免在高峰期发布 |
| Slack/钉钉通知 | 实时感知发布状态 |
| 配合 PDB | 确保回滚时可用性 |
| 镜像不可变 | 回滚 = 切回旧镜像，非重新构建 |

## Related

- [[发布变更/Progressive-Delivery/index.md|Progressive Delivery]]
- [[发布变更/Progressive-Delivery/01-argo-rollouts-deep-dive.md|Argo Rollouts]]
- [[发布变更/部署方案/index.md|部署方案]]
