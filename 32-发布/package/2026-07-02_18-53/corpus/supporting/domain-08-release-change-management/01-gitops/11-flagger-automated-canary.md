---
title: Flagger 自动化 Canary 发布
description: 'Flagger 自动化 Canary 分析与流量切换：Istio/Linkerd/Nginx/Gateway API 集成、自定义指标与告警完整指南'
summary: 'Flagger 自动化 Canary 分析与流量切换：Istio/Linkerd/Nginx/Gateway API 集成、自定义指标与告警完整指南'
category: release-change-management
tags:
- flagger
- canary
- istio
- linkerd
- progressive-delivery
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Flagger 自动化 Canary 是什么
- 如何配置 Flagger Canary 分析
- Flagger Istio 集成怎么做
trigger_keywords:
- flagger
- canary
- istio
- traffic-management
- progressive-delivery
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Flagger 自动化 Canary 发布

## 1. 架构概述

Flagger 是 Flux 生态的渐进式交付工具，通过自动化 Canary 分析和流量切换降低发布风险。它监控 Prometheus 指标，根据自定义指标自动推进或回滚发布，并支持多种服务网格和 Ingress 控制器。

### 1.1 工作流程

```
镜像更新 → Flagger 检测 → 创建 Canary 副本
    → 逐步提升流量权重
    → 每轮分析 Prometheus 指标
    → 指标达标 → 继续提升
    → 指标异常 → 自动回滚
    → 全量切换 → 缩容旧版本
```

### 1.2 核心组件

```
┌──────────────────────────────────────────────┐
│              Flagger Controller               │
│  ┌───────────┐  ┌───────────┐  ┌──────────┐ │
│  │ Canary    │  │ Metric    │  │ Alert    │ │
│  │ Reconciler│  │ Analyzer  │  │ Provider │ │
│  └─────┬─────┘  └─────┬─────┘  └────┬─────┘ │
│        │              │              │        │
│        ▼              ▼              ▼        │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │ Istio/   │  │Prometheus│  │ Slack/    │  │
│  │ Linkerd/ │  │ Datadog  │  │ MS Teams/ │  │
│  │ Nginx    │  │ CloudWatch│ │ Discord   │  │
│  └──────────┘  └──────────┘  └───────────┘  │
└──────────────────────────────────────────────┘
```

## 2. 安装配置

### 2.1 通过 Helm 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add flagger https://flagger.app
helm repo update

# 安装 Flagger（以 Istio 为例）
helm upgrade -i flagger flagger/flagger \
  --namespace flagger-system \
  --create-namespace \
  --set meshProvider=istio \
  --set metricsServer=http://prometheus.istio-system:9090 \
  --set prometheus.install=false

# 安装 Grafana Dashboard（可选）
helm upgrade -i flagger-grafana flagger/grafana \
  --namespace flagger-system \
  --set url=http://prometheus.istio-system:9090
```
### 2.2 支持的流量提供者

| 提供者 | meshProvider 值 | 流量管理方式 |
|--------|-----------------|-------------|
| Istio | `istio` | VirtualService 权重 |
| Linkerd | `linkerd` | TrafficSplit CRD |
| Nginx Ingress | `nginx` | Ingress 注解权重 |
| Gateway API | `gatewayapi` | HTTPRoute 权重 |
| Contour | `contour` | HTTPProxy 权重 |
| Traefik | `traefik` | TraefikService 权重 |
| AWS App Mesh | `appmesh` | VirtualRouter 权重 |

## 3. Canary 分析配置

### 3.1 Istio Canary 完整示例

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
  namespace: production
spec:
  # 目标工作负载
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  # 自动回滚的 HPA（可选）
  autoscalerRef:
    apiVersion: autoscaling/v2
    kind: HorizontalPodAutoscaler
    name: my-app
  # Istio 流量管理
  service:
    port: 80
    targetPort: 8080
    # Istio 特有配置
    trafficPolicy:
      tls:
        mode: ISTIO_MUTUAL
    # HTTP 请求重试
    retries:
      attempts: 3
      perTryTimeout: 1s
      retryOn: "gateway-error,connect-failure,refused-stream"
  # 分析配置
  analysis:
    # Canary 提升间隔
    interval: 1m
    # 最大失败次数（超过则回滚）
    threshold: 5
    # 最大流量权重
    maxWeight: 50
    # 每次提升的权重步长
    stepWeight: 10
    # 自定义指标
    metrics:
    - name: request-success-rate
      # 阈值：成功率 ≥ 99%
      thresholdRange:
        min: 99
      interval: 1m
    - name: request-duration
      # 阈值：P99 延迟 < 500ms
      thresholdRange:
        max: 500
      interval: 1m
    # Webhook（发布前后钩子）
    webhooks:
    - name: acceptance-test
      type: pre-rollout
      url: http://flagger-loadtester.flagger-system/
      timeout: 30s
      metadata:
        type: bash
        cmd: "curl -sd 'test' http://my-app-canary.production/api/validate"
    - name: load-test
      type: rollout
      url: http://flagger-loadtester.flagger-system/
      timeout: 5s
      metadata:
        cmd: "hey -z 1m -q 10 -c 2 http://my-app-canary.production/"
  # 进度截止时间
  progressDeadlineSeconds: 600
```

### 3.2 Linkerd Canary 配置

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  service:
    port: 80
    targetPort: 8080
  analysis:
    interval: 1m
    threshold: 3
    maxWeight: 50
    stepWeight: 10
    metrics:
    - name: request-success-rate
      thresholdRange:
        min: 99
      interval: 1m
    - name: request-duration
      thresholdRange:
        max: 300
      interval: 1m
    webhooks:
    - name: load-test
      type: rollout
      url: http://flagger-loadtester.flagger-system/
      metadata:
        cmd: "hey -z 1m -q 10 -c 2 http://my-app-canary.production:80/"
```

### 3.3 Nginx Ingress Canary 配置

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  ingressRef:
    apiVersion: networking.k8s.io/v1
    kind: Ingress
    name: my-app
  service:
    port: 80
    targetPort: 8080
  analysis:
    interval: 1m
    threshold: 5
    maxWeight: 50
    stepWeight: 10
    metrics:
    - name: request-success-rate
      thresholdRange:
        min: 99
    - name: request-duration
      thresholdRange:
        max: 500
    webhooks:
    - name: acceptance-test
      type: pre-rollout
      url: http://flagger-loadtester.flagger-system/
      metadata:
        cmd: "curl -sd 'test' http://my-app-canary/api/validate"
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-app
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "0"
spec:
  ingressClassName: nginx
  rules:
  - host: my-app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: my-app-primary
            port:
              number: 80
```

### 3.4 Gateway API Canary 配置

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  service:
    port: 80
    targetPort: 8080
  analysis:
    interval: 1m
    threshold: 5
    maxWeight: 50
    stepWeight: 10
    metrics:
    - name: request-success-rate
      thresholdRange:
        min: 99
    webhooks:
    - name: load-test
      type: rollout
      url: http://flagger-loadtester.flagger-system/
      metadata:
        cmd: "hey -z 1m -q 10 -c 2 http://my-app-canary.production/"
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: my-app
spec:
  parentRefs:
  - name: my-gateway
  rules:
  - backendRefs:
    - name: my-app-primary
      port: 80
      weight: 100
    - name: my-app-canary
      port: 80
      weight: 0
```

## 4. 自定义指标

### 4.1 MetricTemplate CRD

```yaml
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: apdex-score
spec:
  provider:
    type: prometheus
    address: http://prometheus.monitoring:9090
  query: |
    sum(rate(http_request_duration_seconds_bucket{
      namespace="{{ namespace }}",
      le="0.25"
    }[5m])) by (service)
    /
    sum(rate(http_request_duration_seconds_count{
      namespace="{{ namespace }}"
    }[5m])) by (service)
    +
    sum(rate(http_request_duration_seconds_bucket{
      namespace="{{ namespace }}",
      le="1"
    }[5m])) by (service)
    /
    sum(rate(http_request_duration_seconds_count{
      namespace="{{ namespace }}"
    }[5m])) by (service)
    / 2
```

### 4.2 使用自定义指标

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
spec:
  analysis:
    metrics:
    - name: apdex-score
      templateRef:
        name: apdex-score
      thresholdRange:
        min: 0.8
      interval: 1m
    - name: error-rate
      # 使用内置 Prometheus 查询
      thresholdRange:
        max: 0.01
      interval: 30s
```

### 4.3 多数据源指标

```yaml
# Datadog 指标模板
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: datadog-latency
spec:
  provider:
    type: datadog
    address: https://api.datadoghq.com
    secretRef:
      name: datadog-credentials
    # 地区（可选）
    region: us-east-1
  query: |
    sum:trace.http.request.duration{
      kube_namespace:{{ namespace }},
      kube_service:{{ target }}-primary
    }.p99()
---
# CloudWatch 指标模板
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: cloudwatch-5xx
spec:
  provider:
    type: cloudwatch
    region: us-west-2
    secretRef:
      name: aws-credentials
  query: |
    period: 60
    metric: HTTPCode_Target_5XX_Count
    namespace: AWS/ApplicationELB
    statistic: Sum
    dimensions:
    - TargetGroup: my-target-group
```

## 5. Webhook 钩子

### 5.1 Webhook 类型

| 类型 | 执行时机 | 用途 |
|------|---------|------|
| `pre-rollout` | Canary 创建前 | 依赖检查、预验证 |
| `rollout` | 流量切换期间 | 负载测试、自动化验证 |
| `post-rollout` | 全量发布后 | 通知、集成测试 |
| `confirm-promotion` | 提升前确认 | 人工审批门禁 |
| `rollback` | 回滚时 | 清理、通知 |

### 5.2 完整 Webhook 示例

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
spec:
  analysis:
    webhooks:
    # 发布前验证
    - name: smoke-test
      type: pre-rollout
      url: http://flagger-loadtester.flagger-system/
      timeout: 30s
      metadata:
        type: bash
        cmd: |
          curl -sf http://my-app-canary.production/healthz && \
          curl -sf http://my-app-canary.production/api/v1/status | jq -e '.ready == true'
    # 负载测试
    - name: load-test
      type: rollout
      url: http://flagger-loadtester.flagger-system/
      timeout: 5s
      metadata:
        cmd: "hey -z 2m -q 20 -c 5 -o latency http://my-app-canary.production/api/v1/endpoint"
    # 人工审批（设置为阻塞）
    - name: manual-approval
      type: confirm-promotion
      url: http://flagger-loadtester.flagger-system/gate/approve
      # 超时后自动回滚
    # 发布后通知
    - name: slack-notify
      type: post-rollout
      url: http://flagger-loadtester.flagger-system/
      metadata:
        type: cmd
        cmd: "curl -X POST https://hooks.slack.com/services/xxx -d '{\"text\":\"发布完成\"}'"
```

### 5.3 Gate 人工审批

```yaml
# 阻塞等待审批
webhooks:
- name: approval-gate
  type: confirm-promotion
  url: http://flagger-loadtester.flagger-system/gate/check
  metadata:
    # Gate 名称
    gate: my-app-production

# 手动放行（kubectl 命令）
# kubectl -n flagger-system exec -it deploy/flagger-loadtester -- \
#   curl -X POST http://localhost:8080/gate/approve/my-app-production

# 拒绝发布
# kubectl -n flagger-system exec -it deploy/flagger-loadtester -- \
#   curl -X POST http://localhost:8080/gate/reject/my-app-production
```

## 6. 告警集成

### 6.1 AlertProvider CRD

```yaml
# Slack 告警
apiVersion: flagger.app/v1beta1
kind: AlertProvider
metadata:
  name: slack
  namespace: flagger-system
spec:
  type: slack
  address: https://hooks.slack.com/services/T00/B00/xxx
  secretRef:
    name: slack-webhook-url
  # 代理（可选）
  proxy: http://proxy.internal:3128
---
# Microsoft Teams
apiVersion: flagger.app/v1beta1
kind: AlertProvider
metadata:
  name: teams
  namespace: flagger-system
spec:
  type: msteams
  address: https://outlook.office.com/webhook/xxx
---
# Discord
apiVersion: flagger.app/v1beta1
kind: AlertProvider
metadata:
  name: discord
  namespace: flagger-system
spec:
  type: discord
  address: https://discord.com/api/webhooks/xxx
```

### 6.2 在 Canary 中配置告警

```yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: my-app
spec:
  analysis:
    # 引用告警提供者
    alerts:
    - name: slack-critical
      severity: error
      providerRef:
        name: slack
        namespace: flagger-system
    - name: teams-info
      severity: info
      providerRef:
        name: teams
        namespace: flagger-system
```

## 7. 运维命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Canary 状态
kubectl get canary -n production
kubectl describe canary my-app -n production

# 查看事件
kubectl get events -n production --field-selector involvedObject.name=my-app

# 手动推进（跳过分析）
kubectl annotate canary my-app \
  -n production \
  flagger.app/skip-analysis=true

# 强制回滚
kubectl annotate canary my-app \
  -n production \
  flagger.app/abort=true

# 查看发布历史
kubectl get canary my-app -n production -o jsonpath='{.status.lastPromotedAt}'

# 调试日志
kubectl logs -n flagger-system deploy/flagger -f --tail=100
```
## 8. 与 Argo Rollouts 对比

| 特性 | Flagger | Argo Rollouts |
|------|---------|---------------|
| 生态 | Flux | ArgoCD |
| 流量管理 | 原生集成多网格 | 通过插件 |
| 分析引擎 | 内置 + Webhook | AnalysisTemplate CRD |
| 学习曲线 | 较低 | 中等 |
| 企业特性 | Flux 生态优势 | Argo 生态优势 |
| 推荐场景 | Flux 用户、多网格 | ArgoCD 用户 |

## Related

- [[domain-08-release-change-management/GitOps/09-argo-rollouts-progressive-delivery|Argo Rollouts 渐进式交付]]
- [[domain-08-release-change-management/变更管理/02-canary-release-strategy|Canary 发布策略]]

## See Also

- [Flagger 官方文档](https://docs.flagger.app/)
- [Flagger MetricTemplate 参考](https://docs.flagger.app/usage/metrics)


<!-- risk-assessed -->
