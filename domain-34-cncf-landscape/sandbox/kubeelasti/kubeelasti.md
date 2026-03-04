# KubeElastic

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubeelastic.io/ |
| **GitHub** | https://github.com/kubeelasti/kubeelastic |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KubeElastic 是一个 Kubernetes 原生的弹性伸缩和资源优化平台，专注于基于实时负载和成本的智能资源调整。它结合机器学习预测算法，自动调整 Pod 资源配额（VPA）和副本数（HPA），同时优化集群节点利用率，帮助用户在保证性能 SLO 的前提下降低云成本。

### 核心特性

- **智能 VPA**: 基于历史负载模式的 ML 预测，自动调整 Pod CPU/内存 requests
- **增强 HPA**: 支持多指标组合、预测性扩缩容和缩容冷却策略
- **成本感知调度**: 优先使用低成本节点（Spot/Preemptible）运行可容忍中断的工作负载
- **资源推荐**: 分析工作负载资源使用，提供优化建议
- **节点整合**: 自动整合低利用率节点，减少资源浪费
- **SLO 保障**: 在弹性伸缩过程中保障应用性能 SLO

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  KubeElastic Controller               │
│                                                       │
│  ┌──────────────┐ ┌───────────────┐ ┌─────────────┐  │
│  │ Metrics      │ │ ML Prediction │ │ Cost        │  │
│  │ Collector    │ │ Engine        │ │ Analyzer    │  │
│  │ (Prometheus) │ │ (Prophet/     │ │             │  │
│  │              │ │  ARIMA)       │ │             │  │
│  └──────┬───────┘ └───────┬───────┘ └──────┬──────┘  │
│         │                 │                 │         │
│  ┌──────▼─────────────────▼─────────────────▼──────┐ │
│  │           Elastic Policy Engine                  │ │
│  │  (VPA/HPA 决策 + 节点整合 + 成本优化)           │ │
│  └─────────────────────┬───────────────────────────┘ │
└────────────────────────┼─────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
   ┌─────▼─────┐  ┌──────▼──────┐ ┌─────▼──────┐
   │ Enhanced  │  │ Smart VPA   │ │ Node       │
   │ HPA       │  │ Controller  │ │ Consolidator│
   │ Controller│  │             │ │             │
   └─────┬─────┘  └──────┬──────┘ └──────┬─────┘
         │               │                │
   ┌─────▼─────┐  ┌──────▼──────┐ ┌──────▼─────┐
   │ Deployment │  │ Pod Resource│ │ Node       │
   │ Replicas   │  │ Requests    │ │ Scale Down │
   └───────────┘  └─────────────┘ └────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装
helm repo add kubeelastic https://kubeelastic.io/charts
helm install kubeelastic kubeelastic/kubeelastic \
  --namespace kubeelastic-system \
  --create-namespace \
  --set prometheus.url=http://prometheus:9090
```

### 创建 ElasticPolicy

```yaml
# elastic-policy.yaml
apiVersion: kubeelastic.io/v1alpha1
kind: ElasticPolicy
metadata:
  name: web-app-policy
  namespace: production
spec:
  target:
    apiVersion: apps/v1
    kind: Deployment
    name: web-frontend
  
  # VPA 配置
  vpa:
    enabled: true
    updatePolicy: Auto  # Auto, Initial, Off
    resourcePolicy:
      containerPolicies:
        - containerName: "*"
          minAllowed:
            cpu: 100m
            memory: 128Mi
          maxAllowed:
            cpu: 4
            memory: 8Gi
  
  # HPA 配置
  hpa:
    enabled: true
    minReplicas: 2
    maxReplicas: 20
    metrics:
      - type: Resource
        resource:
          name: cpu
          target:
            type: Utilization
            averageUtilization: 70
      - type: External
        external:
          metric:
            name: requests_per_second
          target:
            type: AverageValue
            averageValue: "1000"
    behavior:
      scaleDown:
        stabilizationWindowSeconds: 300
        policies:
          - type: Percent
            value: 10
            periodSeconds: 60
  
  # 预测配置
  prediction:
    enabled: true
    algorithm: prophet
    lookAheadMinutes: 15
```

### 成本感知调度

```yaml
# cost-aware-workload.yaml
apiVersion: kubeelastic.io/v1alpha1
kind: CostAwareWorkload
metadata:
  name: batch-job
spec:
  target:
    apiVersion: batch/v1
    kind: Job
    name: data-processing
  
  costPolicy:
    preferSpotInstances: true
    maxSpotPercentage: 80
    fallbackToOnDemand: true
    
  interruptionTolerance:
    enabled: true
    checkpointEnabled: true
    gracePeriodSeconds: 120
```

---

## 高级功能

### ML 预测配置

```yaml
apiVersion: kubeelastic.io/v1alpha1
kind: PredictionModel
metadata:
  name: traffic-predictor
spec:
  metrics:
    - name: http_requests_total
      query: 'sum(rate(http_requests_total{app="web"}[5m]))'
  
  algorithm: prophet
  prophet:
    seasonality:
      daily: true
      weekly: true
    changepoints: auto
    
  training:
    historyDays: 30
    retrainIntervalHours: 24
```

### 节点整合策略

```yaml
apiVersion: kubeelastic.io/v1alpha1
kind: NodeConsolidationPolicy
metadata:
  name: cluster-consolidation
spec:
  enabled: true
  
  # 触发条件
  triggers:
    nodeUtilizationThreshold: 40  # 节点利用率低于 40% 触发
    evaluationInterval: 10m
  
  # 排除节点
  nodeSelector:
    matchExpressions:
      - key: node-role.kubernetes.io/master
        operator: DoesNotExist
  
  # 安全约束
  constraints:
    maxPodsToEvictPerNode: 10
    podDisruptionBudgetRespect: true
    drainTimeout: 300s
```

### 资源推荐报告

```bash
# 获取资源优化建议
kubectl get resourcerecommendations -n production

# NAME            NAMESPACE    TARGET         SAVINGS
# web-frontend    production   Deployment     $120/month
# api-server      production   Deployment     $85/month

# 查看详细建议
kubectl describe resourcerecommendation web-frontend -n production
```

---

## 与其他方案对比

| 特性 | KubeElastic | Kubernetes VPA | KEDA | Goldilocks |
|:---|:---|:---|:---|:---|
| VPA | ML 增强 | 基础 | 不支持 | 推荐 |
| HPA | 预测性 | 需配合 | 增强 | 不支持 |
| 成本优化 | 内置 | 无 | 无 | 无 |
| 节点整合 | 自动 | 无 | 无 | 无 |
| ML 预测 | Prophet/ARIMA | 无 | 无 | 无 |
| Spot 支持 | 智能调度 | 无 | 无 | 无 |

---

## 最佳实践

1. **渐进启用**: 先以 Dry-run 模式观察推荐值，确认合理后再启用自动调整
2. **SLO 优先**: 配置合理的性能 SLO，避免激进缩容影响服务质量
3. **预测校准**: 定期检查预测准确性，调整模型参数
4. **Spot 容错**: 对使用 Spot 实例的工作负载配置 checkpoint 和重试策略
5. **监控告警**: 配置成本和资源利用率告警，跟踪优化效果

---

## 参考资源

- [KubeElastic 官方文档](https://kubeelastic.io/docs/)
- [KubeElastic GitHub](https://github.com/kubeelasti/kubeelastic)
- [Kubernetes VPA](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
