---
title: GitOps 渐进式发布
description: Argo Rollouts 和 Flux Notification 实现金丝雀、蓝绿发布
summary: 使用 Argo Rollouts 实现 GitOps 驱动的金丝雀/蓝绿发布，结合分析指标自动推进或回滚
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- argocd
- argo-rollouts
- progressive-delivery
- canary
- blue-green
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- 开发工程师
estimated_read_time: 14min
intent_queries:
- GitOps 金丝雀发布
- Argo Rollouts 配置
- 蓝绿部署 GitOps
trigger_keywords:
- argo-rollouts
- canary
- blue-green
- progressive-delivery
- analysis
prerequisites:
- gitops-basics
- deployment-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GitOps 渐进式发布

## 1. 渐进式发布类型

| 策略 | 原理 | 适用场景 |
|------|------|----------|
| **金丝雀** | 逐步增加流量比例 | 需要验证指标 |
| **蓝绿** | 两套环境瞬时切换 | 需要快速回滚 |
| **滚动** | 逐个替换 Pod | 标准更新 |

## 2. Argo Rollouts — 金丝雀发布

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: frontend
  namespace: production
spec:
  replicas: 10
  strategy:
    canary:
      canaryService: frontend-canary    # 金丝雀 Service
      stableService: frontend-stable     # 稳定 Service
      trafficRouting:
        nginx:
          stableIngress: frontend-ingress  # 主 Ingress
      steps:
        - setWeight: 10                   # 10% 流量到金丝雀
        - pause: { duration: 5m }         # 等待 5 分钟
        - analysis:                       # 运行分析
            templates:
              - templateName: success-rate
        - setWeight: 30                   # 分析通过，增加到 30%
        - pause: { duration: 5m }
        - setWeight: 50
        - pause: { duration: 10m }
        - setWeight: 100                  # 全量切换
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      containers:
        - name: frontend
          image: registry.example.com/frontend:v2.0.0
          ports:
            - containerPort: 8080
```

## 3. AnalysisTemplate — 自动验证

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
  namespace: production
spec:
  args:
    - name: service-name
      value: frontend
  metrics:
    - name: success-rate
      interval: 1m
      successCondition: result[0] >= 0.95
      failureLimit: 3                     # 连续 3 次失败则中止
      provider:
        prometheus:
          address: http://prometheus.monitoring:9090
          query: |
            sum(rate(http_requests_total{service="{{args.service-name}}",code!~"5.."}[2m]))
            /
            sum(rate(http_requests_total{service="{{args.service-name}}"}[2m]))
    - name: latency-p99
      interval: 1m
      successCondition: result[0] < 500   # P99 延迟 < 500ms
      failureLimit: 2
      provider:
        prometheus:
          address: http://prometheus.monitoring:9090
          query: |
            histogram_quantile(0.99,
              sum(rate(http_request_duration_ms_bucket{service="{{args.service-name}}"}[2m])) by (le)
            )
```

## 4. 蓝绿发布

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: backend
  namespace: production
spec:
  replicas: 5
  strategy:
    blueGreen:
      activeService: backend          # 当前活跃 Service（蓝/绿）
      previewService: backend-preview # 预览 Service
      autoPromotionEnabled: false     # 手动确认提升
      scaleDownDelaySeconds: 600      # 旧版本保留 10 分钟（用于回滚）
      prePromotionAnalysis:
        templates:
          - templateName: pre-promotion-checks
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
        - name: backend
          image: registry.example.com/backend:v3.0.0
```

## 5. Nginx Ingress 流量分割

```yaml
# 两个 Service 分别指向稳定版和金丝雀版
apiVersion: v1
kind: Service
metadata:
  name: frontend-stable
  namespace: production
spec:
  selector:
    app: frontend
  ports:
    - port: 80
      targetPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: frontend-canary
  namespace: production
spec:
  selector:
    app: frontend
  ports:
    - port: 80
      targetPort: 8080
---
# Argo Rollouts 通过 Ingress 注解动态调整权重
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: frontend-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"  # Rollouts 动态更新
spec:
  rules:
    - host: app.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: frontend-canary
                port:
                  number: 80
```

## 6. 手动确认（Promotion）

```bash
# 🟡 中风险：发布控制操作
# 手动提升金丝雀到下一阶段
kubectl argo rollouts promote frontend -n production

# 手动回滚
kubectl argo rollouts abort frontend -n production

# 查看发布状态
kubectl argo rollouts get rollout frontend -n production --watch

# 重启发布（从最新版本开始）
kubectl argo rollouts restart frontend -n production
```

## 7. GitOps 集成

将 Rollout 资源纳入 Git 管理，ArgoCD 自动同步：

```yaml
# argocd/app.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: frontend
  namespace: argocd
spec:
  source:
    repoURL: https://github.com/example/manifests
    path: apps/production/frontend
  syncPolicy:
    automated:
      prune: true
      selfHeal: false  # 发布过程中不自动覆盖
```

> ⚠️ **关键**：发布进行中时 `selfHeal` 应为 `false`，避免 ArgoCD 覆盖 Rollout 的中间状态。

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 设置 `failureLimit` | 分析失败自动回滚 |
| 使用 `maxSurge` 控制资源 | 避免发布期间 Pod 数量翻倍 |
| 发布窗口管理 | 与变更冻结期配合 |
| 监控关键指标 | 错误率、延迟、资源使用 |
| 保留旧版本 | `scaleDownDelaySeconds` 留够回滚时间 |
| 发布前分析 | `prePromotionAnalysis` 在流量切入前验证 |

## Related

- [[清单模式/04-gitops-patterns/01-argocd-app-of-apps|App-of-Apps 模式]]
- [[清单模式/07-resilience-patterns/05-health-probe-patterns|健康探针设计]]

## See Also

- [Argo Rollouts 文档](https://argo-rollouts.readthedocs.io/)
- [渐进式发布最佳实践](https://argoproj.github.io/argo-rollouts/features/canary/)

<!-- risk-assessed -->
