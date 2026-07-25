---
title: "Feature Flag 与渐进式暴露"
description: "Feature Flag 管理平台与渐进式暴露策略：Unleash/LaunchDarkly 集成、渐进式发布、A/B 测试、Flag 生命周期管理与 K8s 实践"
summary: "全面覆盖 Feature Flag 在 Kubernetes 环境中的生产实践，包括 Flag 管理平台选型（Unleash/LaunchDarkly/Flagsmith）、渐进式暴露策略设计、A/B 测试框架、Flag 生命周期治理、与 Argo Rollouts 集成以及技术债务管理"
category: 发布变更
tags:
- feature-flags
- progressive-exposure
- unleash
- launchdarkly
- ab-testing
- flag-lifecycle
- release-strategy
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 中如何部署和管理 Feature Flag 平台"
- "Feature Flag 如何实现渐进式发布和 A/B 测试"
- "Feature Flag 技术债务如何治理"
trigger_keywords:
- feature-flag
- 渐进式暴露
- unleash
- launchdarkly
- a/b-testing
- flag-lifecycle
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

# Feature Flag 与渐进式暴露

## 概述

Feature Flag（特性开关）是现代软件交付的核心基础设施，它将代码部署（Deployment）与功能发布（Release）解耦，使团队能够在不重新部署的情况下控制功能的可见性和行为。在 Kubernetes 环境中，Feature Flag 与渐进式交付（Progressive Delivery）深度结合，实现从内部测试 → 小流量验证 → 全量发布的平滑过渡。

本文覆盖 Feature Flag 的完整生产实践：平台选型与部署、渐进式暴露策略设计、A/B 测试框架、Flag 生命周期管理以及与 [[11-发布变更/01-GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts]] 的集成模式。

## 核心概念

### Feature Flag 分类体系

| Flag 类型 | 生命周期 | 变更频率 | 受众 | 示例 |
|-----------|---------|---------|------|------|
| Release Flag（发布开关） | 短期（天-周） | 高 | 内部 → 全量 | 新功能灰度发布 |
| Experiment Flag（实验开关） | 中期（周-月） | 中 | A/B 分组用户 | UI 布局对比测试 |
| Ops Flag（运维开关） | 长期（月-年） | 低 | 运维团队 | 降级开关、限流阈值 |
| Permission Flag（权限开关） | 永久 | 极低 | 特定用户/租户 | 企业版功能、Beta 邀请 |

### 渐进式暴露阶段模型

```
┌─────────────────────────────────────────────────────────────────┐
│                  渐进式暴露阶段                                    │
│                                                                   │
│  Stage 1        Stage 2        Stage 3        Stage 4            │
│  内部验证        小流量          扩大范围        全量发布            │
│  ┌──────┐      ┌──────┐      ┌──────┐      ┌──────┐            │
│  │ 1%   │─────▶│ 5%   │─────▶│ 25%  │─────▶│ 100% │            │
│  │内部用户│      │Beta用户│      │随机用户│      │所有用户│            │
│  └──────┘      └──────┘      └──────┘      └──────┘            │
│      │              │              │              │               │
│      ▼              ▼              ▼              ▼               │
│  功能验证        性能验证        业务验证        清理 Flag          │
│  错误率监控      SLO 检查       转化率分析      代码简化            │
│  日志分析        用户反馈       回归测试        文档更新            │
│                                                                   │
│  每阶段 Gate：                                                    │
│  • 错误率 < 基线 * 1.1                                           │
│  • P99 延迟 < SLO 阈值                                           │
│  • 无 P0/P1 告警                                                 │
│  • 观察期 ≥ 24h（Stage 2+）                                      │
└─────────────────────────────────────────────────────────────────┘
```

### Flag 管理平台对比

| 维度 | Unleash (开源) | LaunchDarkly (SaaS) | Flagsmith (开源+SaaS) |
|------|---------------|--------------------|--------------------|
| 部署模式 | 自托管 K8s | SaaS / 私有化 | 自托管 / SaaS |
| SDK 支持 | 15+ 语言 | 20+ 语言 | 15+ 语言 |
| 渐进式发布 | 基础策略 | 高级规则引擎 | 分段 + 百分比 |
| A/B 测试 | 需集成 | 内置 | 基础支持 |
| 审计日志 | 基础 | 完整 | 完整 |
| 数据主权 | 完全自控 | 依赖 SaaS | 可选 |
| 成本 | 免费（自托管） | $$$$ | 免费（自托管） |
| 适用场景 | 成本敏感、数据合规 | 企业级、快速上手 | 平衡方案 |

## 生产部署/实现

### Unleash 平台 K8s 部署

Unleash 是开源 Feature Flag 平台，适合对数据主权有要求的企业：

```yaml
# 🟡 中风险：部署新服务，需确认资源和网络策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: unleash-server
  namespace: feature-flags
  labels:
    app.kubernetes.io/name: unleash
    app.kubernetes.io/component: server
spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: unleash
      app.kubernetes.io/component: server
  template:
    metadata:
      labels:
        app.kubernetes.io/name: unleash
        app.kubernetes.io/component: server
    spec:
      containers:
      - name: unleash
        image: unleashorg/unleash-server:5.12.0
        ports:
        - containerPort: 4242
          name: http
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: unleash-db-secret
              key: url
        - name: DATABASE_SSL
          value: "false"
        - name: LOG_LEVEL
          value: "warn"
        - name: METRICS_RATE_LIMITING
          value: "60"
        - name: SERVER_METRICS_ENABLE
          value: "true"
        resources:
          requests:
            cpu: 250m
            memory: 512Mi
          limits:
            cpu: "1"
            memory: 1Gi
        livenessProbe:
          httpGet:
            path: /health
            port: 4242
          initialDelaySeconds: 15
        readinessProbe:
          httpGet:
            path: /health
            port: 4242
          initialDelaySeconds: 5
        volumeMounts:
        - name: config
          mountPath: /unleash/config
      volumes:
      - name: config
        configMap:
          name: unleash-config
---
apiVersion: v1
kind: Service
metadata:
  name: unleash-server
  namespace: feature-flags
spec:
  selector:
    app.kubernetes.io/name: unleash
    app.kubernetes.io/component: server
  ports:
  - port: 4242
    targetPort: 4242
    name: http
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: unleash-server-hpa
  namespace: feature-flags
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: unleash-server
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

### 应用侧 SDK 集成模式

应用通过 Unleash SDK 或 Sidecar 模式获取 Flag 状态：

```yaml
# 🟢 低风险：应用配置变更，不影响集群状态
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
  namespace: production
spec:
  replicas: 5
  selector:
    matchLabels:
      app: payment-service
  template:
    metadata:
      labels:
        app: payment-service
    spec:
      containers:
      - name: payment-service
        image: registry.internal/payment-service:v2.5.0
        env:
        # Feature Flag SDK 配置
        - name: UNLEASH_API_URL
          value: "http://unleash-server.feature-flags.svc:4242/api"
        - name: UNLEASH_API_TOKEN
          valueFrom:
            secretKeyRef:
              name: unleash-client-token
              key: token
        - name: UNLEASH_APP_NAME
          value: "payment-service"
        - name: UNLEASH_ENVIRONMENT
          value: "production"
        # Flag 评估缓存（减少 API 调用）
        - name: UNLEASH_REFRESH_INTERVAL
          value: "15"
        - name: UNLEASH_METRICS_INTERVAL
          value: "60"
        # 降级策略：Flag 服务不可用时的默认行为
        - name: UNLEASH_DISABLE_METRICS
          value: "false"
        - name: FEATURE_FLAG_FALLBACK
          value: "disabled"
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "2"
            memory: 2Gi
```

### 与 Argo Rollouts 集成的渐进式发布

将 Feature Flag 与 [[11-发布变更/01-GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts]] 结合，实现基于 Flag 的渐进式发布：

```yaml
# 🟡 中风险：Rollout 配置变更影响发布行为
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: checkout-service
  namespace: production
spec:
  replicas: 10
  strategy:
    canary:
      steps:
      # Stage 1: 内部验证（通过 Feature Flag 控制）
      - setWeight: 5
      - analysis:
          templates:
          - templateName: internal-validation
          args:
          - name: feature-flag
            value: "new-checkout-flow"
      - pause:
          duration: 2h
      # Stage 2: 小流量验证
      - setWeight: 15
      - analysis:
          templates:
          - templateName: slo-validation
      - pause:
          duration: 24h
      # Stage 3: 扩大范围
      - setWeight: 50
      - analysis:
          templates:
          - templateName: slo-validation
      - pause:
          duration: 24h
      # Stage 4: 全量
      - setWeight: 100
      canaryMetadata:
        labels:
          release-stage: canary
      stableMetadata:
        labels:
          release-stage: stable
  selector:
    matchLabels:
      app: checkout-service
  template:
    metadata:
      labels:
        app: checkout-service
    spec:
      containers:
      - name: checkout-service
        image: registry.internal/checkout-service:v3.2.0
        env:
        - name: UNLEASH_API_URL
          value: "http://unleash-server.feature-flags.svc:4242/api"
        - name: UNLEASH_API_TOKEN
          valueFrom:
            secretKeyRef:
              name: unleash-client-token
              key: token
---
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: slo-validation
  namespace: production
spec:
  args:
  - name: service-name
    value: checkout-service
  metrics:
  - name: error-rate
    interval: 5m
    successCondition: result[0] < 0.01
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus-server.monitoring.svc:9090
        query: |
          sum(rate(http_requests_total{service="{{args.service-name}}",code=~"5.."}[5m]))
          /
          sum(rate(http_requests_total{service="{{args.service-name}}"}[5m]))
  - name: latency-p99
    interval: 5m
    successCondition: result[0] < 500
    failureLimit: 3
    provider:
      prometheus:
        address: http://prometheus-server.monitoring.svc:9090
        query: |
          histogram_quantile(0.99,
            sum(rate(http_request_duration_seconds_bucket{service="{{args.service-name}}"}[5m])) by (le)
          ) * 1000
```

### A/B 测试配置

```yaml
# 🟢 低风险：Feature Flag 策略配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ab-test-config
  namespace: feature-flags
data:
  experiment-config.yaml: |
    experiments:
    - name: checkout-redesign-2026q3
      description: "新版结账流程 A/B 测试"
      flag_key: new-checkout-flow
      status: running
      start_date: "2026-07-20"
      end_date: "2026-08-20"
      variants:
      - name: control
        weight: 50
        description: "现有结账流程"
      - name: treatment
        weight: 50
        description: "新版一步结账"
      targeting:
        # 排除内部用户（避免污染实验数据）
        exclude:
        - attribute: user_type
          operator: equals
          value: internal
        # 按用户 ID 哈希分组（确保一致性）
        stickiness: user_id
      metrics:
        primary:
        - name: conversion_rate
          type: conversion
          minimum_detectable_effect: 0.02
        secondary:
        - name: avg_order_value
          type: continuous
        - name: page_load_time
          type: continuous
          guard: true  # 护栏指标：不能恶化
      guardrails:
        max_error_rate_increase: 0.005
        max_latency_p99_increase_ms: 100
```

## 运维操作

### Flag 状态管理

```bash
# 🟢 低风险：只读查询
# 查看所有 Flag 状态（通过 Unleash API）
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features | \
  jq '.features[] | {name, enabled, type, createdAt}'

# 查看特定 Flag 的详细配置
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features/new-checkout-flow | \
  jq '{name, enabled, strategies, variants}'

# 查看 Flag 使用指标（哪些应用在使用）
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/client-metrics | \
  jq '.applications[] | {appName, instances, strategies}'
```

### 紧急 Flag 切换

```bash
# 🔴 高风险：紧急关闭 Flag 会立即影响所有用户
# 紧急关闭有问题的 Feature Flag（Kill Switch）
curl -X POST -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features/problematic-feature/off \
  -d '{"environment": "production"}'

# 验证 Flag 已关闭
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features/problematic-feature | \
  jq '.enabled'

# 批量关闭某项目的所有实验性 Flag
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features?tag=experiment:checkout | \
  jq -r '.features[].name' | while read flag; do
    curl -X POST -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
      http://unleash-server.feature-flags.svc:4242/api/admin/features/$flag/off
  done
```

### Flag 生命周期审计

```bash
# 🟢 低风险：只读审计
# 查找超过 30 天未修改的 Release Flag（可能是遗留 Flag）
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features | \
  jq '[.features[] | select(.type == "release") |
    select(.createdAt < "2026-06-19") |
    {name, createdAt, enabled}]'

# 统计各类型 Flag 数量
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/features | \
  jq '.features | group_by(.type) | map({type: .[0].type, count: length})'

# 查找从未被评估的 Flag（死 Flag）
curl -s -H "Authorization: $UNLEASH_ADMIN_TOKEN" \
  http://unleash-server.feature-flags.svc:4242/api/admin/client-metrics | \
  jq '[.applications[].seenToggles[] | select(.lastSeen == null)]'
```

## 故障排查

### Flag 服务不可用时的降级

```bash
# 🟢 低风险：只读诊断
# 检查 Unleash 服务健康状态
kubectl get pods -n feature-flags -l app.kubernetes.io/name=unleash
kubectl exec -n feature-flags deployment/unleash-server -- \
  wget -qO- http://localhost:4242/health

# 检查应用侧 SDK 连接状态
kubectl logs -n production deployment/payment-service --tail=50 | \
  grep -i "unleash\|feature.flag\|fallback"

# 验证降级策略是否生效（Flag 服务不可用时应用应使用默认值）
kubectl exec -n production deployment/payment-service -- \
  wget -qO- http://localhost:8080/internal/feature-flags/status
```

### Flag 评估不一致

```bash
# 🟢 低风险：只读诊断
# 检查各实例的 Flag 缓存是否同步
for pod in $(kubectl get pods -n production -l app=payment-service -o name); do
  echo "=== $pod ==="
  kubectl exec -n production $pod -- \
    wget -qO- http://localhost:8080/internal/feature-flags/new-checkout-flow
done

# 检查 Unleash 服务端是否有配置分发延迟
kubectl logs -n feature-flags deployment/unleash-server --tail=100 | \
  grep -i "sync\|poll\|stale"
```

### 渐进式发布卡住

```bash
# 🟢 低风险：只读诊断
# 检查 Argo Rollout 状态
kubectl argo rollouts get rollout checkout-service -n production

# 查看 Analysis Run 结果
kubectl get analysisrun -n production -l rollout=checkout-service --sort-by='.metadata.creationTimestamp'
kubectl describe analysisrun -n production <latest-analysis-run>

# 检查是否因 SLO 违规导致暂停
kubectl argo rollouts get rollout checkout-service -n production -o json | \
  jq '.status.conditions'
```

## 最佳实践

### Flag 生命周期管理

1. **创建规范**：每个 Flag 必须有明确的 Owner、预期生命周期和清理计划。Release Flag 创建时自动设置 30 天清理提醒。

2. **命名规范**：`{team}-{feature}-{purpose}`，例如 `checkout-oneclick-release`、`payment-3ds-ops`。

3. **清理流程**：
   - Release Flag 全量后 7 天内必须清理代码
   - 实验 Flag 结束后 3 天内归档
   - 每季度执行 Flag 审计，清理死 Flag

4. **代码清理自动化**：通过 CI 检测已全量的 Flag，自动创建清理 PR。

### 与发布流程集成

Feature Flag 应与 [[11-发布变更/04-变更管理/02-canary-release-strategy.md|金丝雀发布策略]] 和 [[11-发布变更/01-GitOps/11-flagger-automated-canary.md|Flagger 自动化金丝雀]] 深度集成：
- 部署阶段：代码部署但 Flag 关闭（Dark Launch）
- 验证阶段：逐步开启 Flag（渐进式暴露）
- 稳定阶段：Flag 全量，安排代码清理
- 回滚阶段：关闭 Flag 即刻回滚（无需重新部署）

### 安全与合规

- Flag API Token 使用最小权限原则（只读 Token 用于 SDK，管理 Token 仅限 CI/CD）
- 审计日志保留 90 天，记录所有 Flag 变更操作
- 敏感 Flag（如定价策略）需要双人审批才能变更
- 与 [[11-发布变更/04-变更管理/01-change-window-and-approval.md|变更窗口与审批]] 流程集成

## Related

- [[11-发布变更/04-变更管理/02-canary-release-strategy.md|金丝雀发布策略]]
- [[11-发布变更/01-GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts 渐进式交付]]
- [[11-发布变更/01-GitOps/11-flagger-automated-canary.md|Flagger 自动化金丝雀]]
- [[11-发布变更/04-变更管理/01-change-window-and-approval.md|变更窗口与审批]]
- [[11-发布变更/04-变更管理/03-change-rollback-playbook.md|变更回滚手册]]
- [[11-发布变更/01-GitOps/07-gitops-security-compliance.md|GitOps 安全合规]]
- [[12-可靠性/06-SRE实践/02-release-gate-slo-based.md|基于 SLO 的发布门控]]
