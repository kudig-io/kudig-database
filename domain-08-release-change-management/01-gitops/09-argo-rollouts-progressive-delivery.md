---
title: Argo Rollouts 渐进式交付
description: 'Argo Rollouts 渐进式交付策略：Canary、Blue-Green、Experiment 与 Analysis Template 完整实战指南'
summary: 'Argo Rollouts 渐进式交付策略：Canary、Blue-Green、Experiment 与 Analysis Template 完整实战指南'
category: release-change-management
tags:
- argo-rollouts
- canary
- blue-green
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
- Argo Rollouts 渐进式交付 是什么
- 如何配置 Argo Rollouts Canary 策略
- Argo Rollouts Blue-Green 部署怎么做
trigger_keywords:
- argo-rollouts
- canary
- blue-green
- progressive-delivery
- analysis-template
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


# Argo Rollouts 渐进式交付

## 1. 架构概述

Argo Rollouts 是 Kubernetes 原生的渐进式交付控制器，通过声明式方式管理应用发布策略。它扩展了 Deployment 的能力，支持 Canary、Blue-Green 和 Experiment 三种发布模式，并通过 Analysis Template 实现自动化质量门禁。

### 1.1 核心组件

```
┌─────────────────────────────────────────────────┐
│                 Argo Rollouts Controller         │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ Rollout  │  │ Analysis │  │  Experiment  │  │
│  │ Reconciler│ │ Run Ctrl │  │  Controller  │  │
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘  │
│       │              │               │           │
│       ▼              ▼               ▼           │
│  ┌─────────┐  ┌───────────┐  ┌────────────┐    │
│  │ ReplicaSet│ │ Prometheus│  │  A/B Pod   │    │
│  │ 管理     │  │ Job/Web   │  │  Groups    │    │
│  └─────────┘  └───────────┘  └────────────┘    │
└─────────────────────────────────────────────────┘
```

### 1.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 kubectl 安装
kubectl create namespace argo-rollouts
kubectl apply -n argo-rollouts \
  -f https://github.com/argoproj/argo-rollouts/releases/latest/download/install.yaml

# 通过 Helm 安装（推荐生产环境）
helm repo add argo https://argoproj.github.io/argo-helm
helm install argo-rollouts argo/argo-rollouts \
  --namespace argo-rollouts \
  --create-namespace \
  --set dashboard.enabled=true \
  --set controller.metrics.enabled=true

# 验证安装
kubectl get pods -n argo-rollouts
kubectl argo rollouts version
```
### 1.3 Rollout CRD 基本结构

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  replicas: 5
  revisionHistoryLimit: 3
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
        image: my-app:v1
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
  strategy:
    canary:          # 或 blueGreen
      steps:
      - setWeight: 20
      - pause: { duration: 5m }
```

## 2. Canary 策略

### 2.1 基本 Canary 配置

Canary 策略通过逐步将流量导向新版本来降低发布风险。每个步骤可以设置权重、暂停时间和自动化分析。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app-canary
spec:
  replicas: 10
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
        image: my-app:v2
  strategy:
    canary:
      # Canary 副本数限制
      maxSurge: "25%"
      maxUnavailable: 0
      # 步骤式发布
      steps:
      # Step 1: 暂停 2 分钟，手动验证
      - pause: { duration: 2m }
      # Step 2: 将 20% 流量切到 Canary
      - setWeight: 20
      # Step 3: 运行自动化分析
      - analysis:
          templates:
          - templateName: success-rate
          args:
          - name: service-name
            value: my-app
      # Step 4: 暂停等待人工确认
      - pause: { duration: 5m }
      # Step 5: 提升到 50%
      - setWeight: 50
      # Step 6: 再次分析
      - analysis:
          templates:
          - templateName: latency-check
      # Step 7: 暂停确认
      - pause: { duration: 5m }
      # Step 8: 全量发布
      - setWeight: 100
```

### 2.2 带流量管理的 Canary

与 Istio VirtualService 集成实现精确流量控制：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app-canary
spec:
  replicas: 5
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
      annotations:
        sidecar.istio.io/inject: "true"
    spec:
      containers:
      - name: my-app
        image: my-app:v2
  strategy:
    canary:
      canaryService: my-app-canary
      stableService: my-app-stable
      trafficRouting:
        istio:
          virtualService:
            name: my-app-vsvc
            routes:
            - primary
      steps:
      - setWeight: 10
      - pause: { duration: 3m }
      - analysis:
          templates:
          - templateName: canary-analysis
      - setWeight: 30
      - pause: { duration: 5m }
      - setWeight: 60
      - pause: { duration: 5m }
      - setWeight: 100
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: my-app-vsvc
spec:
  hosts:
  - my-app
  http:
  - route:
    - destination:
        host: my-app-stable
      weight: 100
    - destination:
        host: my-app-canary
      weight: 0
```

### 2.3 Canary 回滚策略

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app-canary
spec:
  strategy:
    canary:
      # 自动回滚条件
      analysis:
        templates:
        - templateName: rollback-check
        startingStep: 1
        args:
        - name: service-name
          value: my-app
      # 限流：每秒最多创建 1 个 Pod
      canaryMetadata:
        labels:
          role: canary
      stableMetadata:
        labels:
          role: stable
      # 反亲和性：Canary Pod 分散在不同节点
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  role: canary
              topologyKey: kubernetes.io/hostname
```

## 3. Blue-Green 策略

### 3.1 基本 Blue-Green 配置

Blue-Green 策略维护两个完整的环境，通过 Service 切换实现零停机发布。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app-bg
spec:
  replicas: 5
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
        image: my-app:v2
  strategy:
    blueGreen:
      # 活动和预发布 Service
      activeService: my-app-active
      previewService: my-app-preview
      # 预发布验证时间
      prePromotionAnalysis:
        templates:
        - templateName: preview-check
        args:
        - name: service-name
            value: my-app-preview
      # 自动回滚延迟
      autoPromotionSeconds: 600
      # 缩容旧版本延迟（保留快速回滚能力）
      scaleDownDelaySeconds: 300
      # 缩容旧版本的延迟修订数
      scaleDownDelayRevisionLimit: 2
```

### 3.2 Blue-Green 预发布验证

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app-bg
spec:
  strategy:
    blueGreen:
      activeService: my-app-active
      previewService: my-app-preview
      # 预发布阶段运行分析
      prePromotionAnalysis:
        templates:
        - templateName: e2e-smoke-test
          clusterScope: true
        args:
        - name: preview-url
          valueFrom:
            podTemplateHashValue: Latest
      # 发布后分析
      postPromotionAnalysis:
        templates:
        - templateName: production-health-check
        args:
        - name: service-name
          value: my-app-active
---
# 预发布 Service（仅内部访问）
apiVersion: v1
kind: Service
metadata:
  name: my-app-preview
spec:
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080
  type: ClusterIP
---
# 活动 Service（生产流量）
apiVersion: v1
kind: Service
metadata:
  name: my-app-active
spec:
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer
```

## 4. Experiment（A/B 测试）

### 4.1 Experiment CRD

Experiment 允许同时运行多个 Pod 组，用于对比测试不同版本的性能和行为。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Experiment
metadata:
  name: my-app-ab-test
spec:
  # 持续时间
  duration: 30m
  # 并行运行的模板
  templates:
  - name: baseline
    replicas: 2
    selector:
      matchLabels:
        app: my-app
        variant: baseline
    template:
      metadata:
        labels:
          app: my-app
          variant: baseline
      spec:
        containers:
        - name: my-app
          image: my-app:v1
  - name: candidate
    replicas: 2
    selector:
      matchLabels:
        app: my-app
        variant: candidate
    template:
      metadata:
        labels:
          app: my-app
          variant: candidate
      spec:
        containers:
        - name: my-app
          image: my-app:v2
  # 分析任务
  analyses:
  - name: compare-performance
    templateName: ab-test-analysis
    args:
    - name: baseline-service
      value: my-app-baseline
    - name: candidate-service
      value: my-app-candidate
```

### 4.2 在 Rollout 中使用 Experiment

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  strategy:
    canary:
      steps:
      # 第一步：运行 A/B 测试
      - experiment:
          duration: 20m
          templates:
          - name: canary
            specRef: canary
            weight: 20
          analyses:
          - name: canary-experiment
            templateName: canary-ab-analysis
            args:
            - name: canary-hash
              valueFrom:
                podTemplateHashValue: Latest
      # 实验通过后继续发布
      - setWeight: 50
      - pause: { duration: 5m }
      - setWeight: 100
```

## 5. Analysis Template

### 5.1 Prometheus 指标分析

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  args:
  - name: service-name
  metrics:
  - name: success-rate
    # 必须满足的条件
    successCondition: result[0] >= 0.99
    # 失败条件（立即终止）
    failureCondition: result[0] < 0.95
    # 采集间隔和超时
    interval: 30s
    count: 10
    # 最大失败次数
    failureLimit: 2
    provider:
      prometheus:
        address: http://prometheus.monitoring:9090
        query: |
          sum(rate(http_requests_total{
            service="{{args.service-name}}",
            status=~"2.."
          }[5m]))
          /
          sum(rate(http_requests_total{
            service="{{args.service-name}}"
          }[5m]))
---
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: latency-check
spec:
  args:
  - name: service-name
  metrics:
  - name: p99-latency
    successCondition: result[0] <= 500
    failureCondition: result[0] > 2000
    interval: 30s
    count: 5
    provider:
      prometheus:
        address: http://prometheus.monitoring:9090
        query: |
          histogram_quantile(0.99,
            sum(rate(http_request_duration_ms_bucket{
              service="{{args.service-name}}"
            }[5m])) by (le)
          )
  - name: error-rate
    successCondition: result[0] < 0.01
    failureCondition: result[0] > 0.05
    interval: 1m
    count: 3
    provider:
      prometheus:
        address: http://prometheus.monitoring:9090
        query: |
          sum(rate(http_requests_total{
            service="{{args.service-name}}",
            status=~"5.."
          }[5m]))
          /
          sum(rate(http_requests_total{
            service="{{args.service-name}}"
          }[5m]))
```

### 5.2 Job 分析

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: e2e-smoke-test
spec:
  args:
  - name: service-url
  metrics:
  - name: e2e-test
    # Job 提供成功/失败信号
    provider:
      job:
        spec:
          backoffLimit: 1
          template:
            spec:
              containers:
              - name: e2e-test
                image: my-test-runner:latest
                command: ["/bin/sh", "-c"]
                args:
                - |
                  set -e
                  # 等待服务就绪
                  for i in $(seq 1 30); do
                    if curl -sf "{{args.service-url}}/health"; then
                      break
                    fi
                    sleep 2
                  done
                  # 运行冒烟测试
                  curl -sf "{{args.service-url}}/api/v1/status" | jq -e '.status == "ok"'
              restartPolicy: Never
```

### 5.3 Web 分析（外部 HTTP 回调）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: external-verification
spec:
  args:
  - name: service-name
  metrics:
  - name: external-check
    provider:
      web:
        # 外部 HTTP 端点
        url: "https://monitoring.internal/api/v1/check?service={{args.service-name}}"
        method: POST
        headers:
        - name: Authorization
          value: "Bearer ${ANALYSIS_TOKEN}"
        - name: Content-Type
          value: "application/json"
        body: |
          {
            "service": "{{args.service-name}}",
            "check_type": "canary"
          }
        # JSONPath 提取结果
        jsonPath: "{$.result}"
        timeoutSeconds: 30
        insecure: false
```

### 5.4 AnalysisRun 参数传递

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisRun
metadata:
  name: manual-analysis-run
spec:
  analysisTemplateRef:
    name: success-rate
  args:
  - name: service-name
    value: my-app
  # 动态参数：从 Pod Template Hash 获取
  - name: pod-hash
    valueFrom:
      podTemplateHashValue: Latest
```

## 6. 与 ArgoCD 集成

### 6.1 GitOps 工作流

```yaml
# ArgoCD Application 配置
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/my-org/my-app-config.git
    targetRevision: HEAD
    path: overlays/production
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
  # Rollout 忽略镜像字段（由 CI 更新）
  ignoreDifferences:
  - group: argoproj.io
    kind: Rollout
    jsonPointers:
    - /spec/template/spec/containers/0/image
```

### 6.2 镜像更新自动化

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# CI Pipeline 中更新镜像
kubectl argo rollouts set image my-app \
  my-app=my-app:v2.1.0 \
  -n production

# 或通过 kubectl patch
kubectl patch rollout my-app \
  --type json \
  -p '[{"op":"replace","path":"/spec/template/spec/containers/0/image","value":"my-app:v2.1.0"}]' \
  -n production

# 查看发布状态
kubectl argo rollouts status my-app -n production
kubectl argo rollouts get rollout my-app -n production --watch
```
## 7. 生产运维命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Rollout 状态
kubectl argo rollouts get rollout my-app -n production

# 手动推进（跳过暂停）
kubectl argo rollouts promote my-app -n production

# 手动回滚
kubectl argo rollouts abort my-app -n production

# 重试失败的 AnalysisRun
kubectl argo rollouts retry analysisrun my-app-analysis-xxxxx -n production

# 清理旧 ReplicaSet
kubectl argo rollouts restart my-app -n production

# Dashboard（本地端口转发）
kubectl port-forward svc/argo-rollouts-dashboard -n argo-rollouts 3100:3100
```
## 8. 生产最佳实践

| 实践 | 建议 |
|------|------|
| Analysis 频率 | 间隔 30s-1m，采集 5-10 次 |
| 失败阈值 | failureLimit ≥ 2，避免单次抖动误判 |
| 回滚延迟 | Blue-Green scaleDownDelaySeconds ≥ 300 |
| 资源预算 | Canary maxSurge ≤ 25%，控制额外资源开销 |
| 监控覆盖 | 至少包含成功率 + 延迟 + 错误率三个指标 |
| 渐进步骤 | 生产建议 4-6 步，权重递增 |

## Related

- [[domain-08-release-change-management/01-gitops/01-argo-cd-enterprise-gitops|ArgoCD 企业级 GitOps]]
- [[domain-08-release-change-management/03-change-management/02-canary-release-strategy|Canary 发布策略]]

## See Also

- [Argo Rollouts 官方文档](https://argo-rollouts.readthedocs.io/)
- [Analysis Template 参考](https://argo-rollouts.readthedocs.io/en/stable/analysis/)


<!-- risk-assessed -->
