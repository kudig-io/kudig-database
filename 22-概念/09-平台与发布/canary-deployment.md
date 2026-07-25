---
title: 金丝雀发布
summary: 金丝雀发布（Canary Deployment）是一种渐进式发布策略，通过将少量生产流量导入新版本，在真实环境中验证新版本的稳定性，再逐步扩大流量比例直至全量发布。
category: concepts
tags:
- core-concept
- 发布变更
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-07
last_updated: 2026-07
status: reviewed
---



# 金丝雀发布

金丝雀发布（Canary Deployment）是一种渐进式发布策略，通过将少量生产流量导入新版本，在真实环境中验证新版本的稳定性，再逐步扩大流量比例直至全量发布。

## 核心原理

金丝雀发布的名称源于历史上矿工使用金丝雀检测有毒气体的做法。其基本流程为：

1. 部署新版本，仅导入少量流量（如 1% 或 5%）
2. 观察新版本的关键指标
3. 若指标正常，逐步增加流量比例（10% → 25% → 50% → 100%）
4. 若指标异常，立即停止推广并回滚

这种方式将发布风险控制在最小范围内，是新版本在生产环境中验证的黄金标准。

## Kubernetes 实现方式

在 Kubernetes 生态中，金丝雀发布有多种实现路径：

### 原生方式

通过两个 Deployment（稳定版和金丝雀版）共享同一个 Service，手动调整副本数来控制流量比例：

- 稳定版 Deployment：9 个副本
- 金丝雀版 Deployment：1 个副本
- 总流量比例约为 9:1

这种方式简单直接，但粒度受限于 Pod 数量，且切换不够灵活。

### Ingress 方式

利用 Ingress Controller 的流量分割能力实现精细控制：

- **Nginx Ingress**：通过 `canary-by-header`、`canary-weight` 等 annotation 控制
- **ALB Ingress**：支持基于权重的路由规则，可直接设置百分比

Ingress 方式无需修改 Service 或 Deployment，仅需调整 Ingress 资源即可。

### Service Mesh 方式

通过 Istio 等 Service Mesh 实现最精细的流量控制：

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
spec:
  http:
    - route:
        - destination:
            host: my-app-stable
          weight: 90
        - destination:
            host: my-app-canary
          weight: 10
```

Service Mesh 支持基于 HTTP 头、Cookie、用户身份等维度的路由，灵活性最高。更多演进路线参见 [[service-mesh-evolution]]。

## 关键监控指标

金丝雀发布期间必须关注以下指标：

| 指标类别 | 具体指标 | 异常阈值参考 |
|----------|----------|-------------|
| 错误率 | HTTP 5xx 比例 | 较基线上升 > 0.1% |
| 延迟 | P99 响应时间 | 较基线上升 > 20% |
| 吞吐量 | QPS/TPS | 较基线下降 > 10% |
| 业务指标 | 订单成功率、转化率 | 较基线下降 > 5% |
| 资源使用 | CPU、内存、GC | 较基线上升 > 30% |

## 远程顾问指导要点

作为远程顾问，指导现场执行金丝雀发布时应注意：

- **设置初始比例**：建议从 1% 或 5% 开始，确保即使出现问题影响面也极小
- **选择流量特征**：优先将内部用户、非关键业务流量或特定地域流量导入金丝雀版本
- **定义明确的决策标准**：提前确定错误率、延迟等指标的红线，避免主观判断
- **设定观察时长**：每个阶段至少观察 15-30 分钟，确保覆盖正常业务周期
- **准备一键回滚**：金丝雀版本出现异常时，能够快速将流量比例归零或切回稳定版本
- **渐进推广节奏**：建议按 5% → 10% → 25% → 50% → 100% 的节奏推进，每个阶段充分验证

## 技术深度解析

### 流量分割的实现层级

金丝雀发布的流量控制可以在不同网络层级实现：

| 层级 | 机制 | 精度 | 适用场景 |
|------|------|------|---------|
| Pod 副本比例 | Deployment replicas 比例 | 粗（受 Pod 数限制） | 简单场景 |
| Service Mesh | Istio VirtualService weight | 精细（1% 级别） | 生产标准 |
| Ingress | Nginx canary-weight | 中等（5% 级别） | 非 Mesh 环境 |
| 网关层 | API Gateway 流量分割 | 精细 + 支持用户分群 | 高级路由需求 |

### Nginx Ingress 金丝雀

```yaml
# 金丝雀 Ingress（通过 annotation 控制）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-app-canary
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"        # 10% 流量
    # 或基于 header/Cookie 的定向金丝雀
    nginx.ingress.kubernetes.io/canary-by-header: "x-canary"
    nginx.ingress.kubernetes.io/canary-by-header-value: "true"
spec:
  rules:
  - http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: my-app-canary
            port:
              number: 80
```

### Argo Rollouts 自动化金丝雀

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  replicas: 10
  strategy:
    canary:
      canaryService: my-app-canary       # 金丝雀 Service
      stableService: my-app-stable       # 稳定版 Service
      trafficRouting:
        istio:
          virtualService:
            name: my-app-vs
            routes:
            - primary
      steps:
      - setWeight: 5
      - pause: {duration: 5m}
      - analysis:
          templates:
          - templateName: success-rate-check
      - setWeight: 25
      - pause: {duration: 10m}
      - analysis:
          templates:
          - templateName: success-rate-check
      - setWeight: 50
      - pause: {duration: 10m}
      - setWeight: 100
```

## 最佳实践

- **从极小比例开始**：建议从 1% 或 5% 开始——即使新版本有严重 bug，影响面也极小
- **定义自动化决策标准**：提前用 AnalysisTemplate 定义错误率、延迟等指标红线，避免人工判断延迟
- **选择有意义的流量子集**：优先将内部用户、灰度地域或非关键业务流量导入金丝雀——而非随机分配
- **每个阶段充分观察**：每阶段至少观察 15-30 分钟，覆盖一个完整业务周期（如包含高峰期）
- **准备一键回滚**：金丝雀异常时，能够通过将 canary weight 归零瞬间切回稳定版本

## 常见陷阱

- **金丝雀流量太少无统计意义**：如果金丝雀只占 1% 流量但总 QPS 很低（如 10 QPS），样本量不足以发现问题——需要确保金丝雀流量绝对值足够
- **Session 一致性问题**：用户请求在稳定版和金丝雀版之间随机切换，可能导致 session 不一致——使用基于 Cookie/header 的粘性路由
- **金丝雀与稳定版数据库 schema 冲突**：如果金丝雀版本修改了数据库 schema，可能与稳定版不兼容——需要遵循兼容性扩展策略

更多部署排错方法请参考 [[19-故障诊断/04-高级排障/05-workloads/02-deployment-troubleshooting.md|deployment-troubleshooting]]，其他部署策略参见 [[22-概念/09-平台与发布/blue-green-deployment.md|blue-green-deployment]]。


## 源码实现分析

### Argo Rollouts 金丝雀控制器

```go
// github.com/argoproj/argo-rollouts/rollout/canary.go
// Argo Rollouts 金丝雀发布核心
func (c *RolloutContext) reconcileCanary() error {
    // 1. 获取当前 step 配置
    // canary.steps: [{setWeight: 10}, {pause: {duration: 5m}}, {setWeight: 50}...]
    currentStep := rollout.Spec.Strategy.Canary.Steps[rollout.Status.CurrentStepIndex]
    
    // 2. 调整流量权重
    if currentStep.SetWeight != nil {
        // 修改 Service selector 或 Istio VirtualService weight
        c.setCanaryWeight(*currentStep.SetWeight)
        // stable Service → 90% 流量
        // canary Service → 10% 流量
    }
    
    // 3. 暂停等待（人工确认或定时）
    if currentStep.Pause != nil {
        // 等待 duration 或手动 promote
        return c.pauseRollout()
    }
    
    // 4. 分析指标（AnalysisRun）
    if currentStep.Analysis != nil {
        // 执行 Prometheus 查询，检查错误率/延迟
        // 失败则自动回滚
        c.runAnalysis()
    }
}
```

```
┌─────────────────────────────────────────────────────────┐
│     金丝雀发布流量控制                              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Step 1: setWeight 10%                                  │
│    stable (v1) ── 90% ──▶ [Pod-v1 x 9]                │
│    canary (v2) ── 10% ──▶ [Pod-v2 x 1]                │
│                                                         │
│  Step 2: pause 5m + analysis                            │
│    检查: error_rate < 1%, p99_latency < 200ms          │
│    失败 → 自动回滚 (abort)                             │
│                                                         │
│  Step 3: setWeight 50%                                  │
│    stable (v1) ── 50% ──▶ [Pod-v1 x 5]                │
│    canary (v2) ── 50% ──▶ [Pod-v2 x 5]                │
│                                                         │
│  Step 4: setWeight 100% (promote)                       │
│    canary (v2) ── 100% ─▶ [Pod-v2 x 10]               │
│    stable (v1) ── 删除                                  │
└─────────────────────────────────────────────────────────┘
```

### 生产配置：Argo Rollouts 金丝雀

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: api-server
spec:
  replicas: 10
  strategy:
    canary:
      canaryService: api-canary
      stableService: api-stable
      trafficRouting:
        istio:
          virtualService:
            name: api-vs
      steps:
      - setWeight: 10
      - pause: {duration: 5m}
      - analysis:
          templates:
          - templateName: success-rate
      - setWeight: 50
      - pause: {duration: 10m}
      - setWeight: 100
  selector:
    matchLabels: {app: api}
  template:
    metadata:
      labels: {app: api}
    spec:
      containers:
      - name: api
        image: api:v2.1.0
```

## 面试要点

1. **金丝雀发布与蓝绿发布的区别？**
   - 金丝雀：渐进式流量切换（10%→50%→100%），可回滚
   - 蓝绿：一次性全量切换，需要双倍资源
   - 金丝雀风险更低，但发布周期更长
   - 蓝绿切换更快，但回滚也是全量

2. **如何实现基于流量的金丝雀（而非副本数）？**
   - 副本数金丝雀：简单但不精确（10 Pod 中 1 个 = 10%）
   - 流量金丝雀：通过 Istio/Nginx/Gateway API 控制实际流量比例
   - Argo Rollouts 支持 Istio/Nginx/ALB/Gateway API 流量路由
   - 流量金丝雀更精确，但需要 Service Mesh 或 Ingress 支持

3. **金丝雀发布中的自动分析如何工作？**
   - AnalysisRun CRD 定义 Prometheus 查询
   - 检查指标：错误率、延迟 P99、业务指标
   - 失败自动回滚（abort），成功继续下一步
   - 支持 Webhook 通知和人工审批

4. **Flagger 与 Argo Rollouts 的对比？**
   - Flagger：轻量，专注金丝雀/蓝绿，与 Istio/Linkerd 集成
   - Argo Rollouts：功能更全，支持实验（Experiment）、A/B 测试
   - Flagger 配置更简单，Argo Rollouts 更灵活
   - 两者都是 CNCF 项目

## 参见

- [[kubernetes]] — core-concept 领域核心页面

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
