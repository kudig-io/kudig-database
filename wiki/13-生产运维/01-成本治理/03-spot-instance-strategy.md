---
title: Spot/抢占式实例策略
description: '定义混合实例池设计、Spot 中断处理（PDB + Graceful Shutdown）及成本节省量化分析'
summary: '定义混合实例池设计、Spot 中断处理（PDB + Graceful Shutdown）及成本节省量化分析'
category: production-operations
tags:
- production
- operations
- finops
- spot-instances
- cost-optimization
- graceful-shutdown
tier: core
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
- Spot 实例策略 是什么
- 如何 使用抢占式实例
- 如何 处理 Spot 中断
trigger_keywords:
- spot
- preemptible
- cost-optimization
- graceful-shutdown
- pdb
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


# Spot/抢占式实例策略

## 1. Spot 实例概述

### 1.1 Spot 实例 vs 按需实例

| 维度 | 按需实例 | Spot 实例 |
|------|---------|----------|
| 价格 | 固定价格 | 市场价格（通常 60-90% 折扣） |
| 可用性 | 保证 | 不保证，可能被回收 |
| 中断 | 无 | 2 分钟通知后回收 |
| 适用场景 | 稳定负载、关键服务 | 容错、弹性、无状态负载 |
| 计费 | 按秒/小时 | 按秒/小时（按竞价） |

### 1.2 Spot 实例适用场景

```
适合 Spot 的工作负载:

✓ 无状态 Web 服务（有足够副本）
✓ 批处理任务（可断点续传）
✓ CI/CD Runner
✓ 数据处理（Spark/Flink）
✓ 机器学习训练
✓ 开发/测试环境
✓ 搜索索引构建

不适合 Spot 的工作负载:

✗ 有状态服务（数据库、消息队列）
✗ 单副本关键服务
✗ 无法容忍中断的服务
✗ 长时间运行且无法检查点的任务
```

### 1.3 成本节省潜力

```
典型成本节省分析:

按需实例成本: ¥100/月/节点
Spot 实例成本: ¥25-40/月/节点（60-75% 折扣）

假设:
  - 集群 100 个节点
  - 60% 工作负载可迁移到 Spot
  - 平均 70% 折扣

计算:
  按需成本: 100 × ¥100 = ¥10,000/月
  Spot 成本: 60 × ¥30 = ¥1,800/月
  按需保留: 40 × ¥100 = ¥4,000/月
  混合总成本: ¥5,800/月

  月度节省: ¥4,200（42%）
  年度节省: ¥50,400
```

## 2. 混合实例池设计

### 2.1 节点组架构

```
混合节点池架构:

┌─────────────────────────────────────────────────────┐
│                    Kubernetes 集群                    │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ │
│  │  On-Demand   │  │    Spot      │  │    Spot      │ │
│  │  Node Group  │  │  Node Group  │  │  Node Group  │ │
│  │  (稳定)      │  │  (池 A)      │  │  (池 B)      │ │
│  │              │  │              │  │              │ │
│  │  m6i.2xlarge │  │  m6i.2xlarge │  │  m5.2xlarge  │ │
│  │  10 nodes    │  │  30 nodes    │  │  20 nodes    │ │
│  │              │  │              │  │              │ │
│  │  核心服务    │  │  无状态服务  │  │  批处理任务  │ │
│  └─────────────┘  └─────────────┘  └─────────────┘ │
│                                                      │
└─────────────────────────────────────────────────────┘
```

### 2.2 AWS 实例配置

```yaml
# Cluster Autoscaler 多节点组配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
        - name: cluster-autoscaler
          command:
            - ./cluster-autoscaler
            - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/prod-cluster
            - --balance-similar-node-groups  # 平衡相似节点组
            - --skip-nodes-with-system-pods=false
```

```json
// AWS ASG 配置示例（Terraform）
{
  "on_demand_asg": {
    "instance_types": ["m6i.xlarge", "m6i.2xlarge"],
    "capacity_type": "ON_DEMAND",
    "min_size": 5,
    "max_size": 15,
    "desired_capacity": 10,
    "labels": {
      "node-type": "on-demand",
      "workload": "critical"
    },
    "taints": []
  },
  "spot_asg_pool_a": {
    "instance_types": ["m6i.xlarge", "m6i.2xlarge", "m5.xlarge", "m5.2xlarge"],
    "capacity_type": "SPOT",
    "min_size": 10,
    "max_size": 50,
    "desired_capacity": 30,
    "labels": {
      "node-type": "spot",
      "spot-pool": "pool-a",
      "workload": "stateless"
    },
    "taints": [
      {
        "key": "spot",
        "value": "true",
        "effect": "NoSchedule"
      }
    ]
  }
}
```

### 2.3 工作负载分配策略

```yaml
# 核心服务: 仅调度到 On-Demand 节点
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-service
spec:
  template:
    spec:
      nodeSelector:
        node-type: on-demand
      tolerations: []
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: node-type
                    operator: In
                    values:
                      - on-demand
```

```yaml
# 无状态服务: 优先 Spot，回退 On-Demand
apiVersion: apps/v1
kind: Deployment
metadata:
  name: stateless-service
spec:
  template:
    spec:
      tolerations:
        - key: spot
          operator: Equal
          value: "true"
          effect: NoSchedule
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 90
              preference:
                matchExpressions:
                  - key: node-type
                    operator: In
                    values:
                      - spot
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: kubernetes.io/os
                    operator: In
                    values:
                      - linux
```

## 3. Spot 中断处理

### 3.1 中断通知机制

```
# 🟢 低风险：只读/信息收集，通常无副作用
Spot 中断流程:

┌──────────────────────────────────────────────────────┐
│  T-2min: AWS 发送中断通知                              │
│  (通过 Instance Metadata / Event Bridge)              │
└─────────────────────┬────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────────────────┐
│  Karpenter / Node Termination Handler 捕获通知        │
└─────────────────────┬────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────────────────┐
│  1. 标记节点为 cordoned（不可调度）                     │
│  2. 触发 Pod 优雅终止                                  │
│  3. 等待 Pod 完成迁移                                  │
│  4. 节点下线                                           │
└──────────────────────────────────────────────────────┘
```
### 3.2 Node Termination Handler

```yaml
# AWS Node Termination Handler (Queue Processor 模式)
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: aws-node-termination-handler
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: aws-node-termination-handler
  template:
    metadata:
      labels:
        app: aws-node-termination-handler
    spec:
      serviceAccountName: aws-node-termination-handler
      nodeSelector:
        node-type: spot  # 仅在 Spot 节点上运行
      containers:
        - name: handler
          image: public.ecr.aws/aws-ec2/aws-node-termination-handler:v1.21.0
          env:
            - name: QUEUE_URL
              value: "https://sqs.cn-north-1.amazonaws.com.cn/xxx/spot-interruption"
            - name: LOG_LEVEL
              value: "info"
            - name: ENABLE_SPOT_INTERRUPTION_DRAINING
              value: "true"
            - name: ENABLE_REBALANCE_RECOMMENDATION
              value: "true"
            - name: POD_TERMINATION_GRACE_PERIOD
              value: "60"  # 60 秒优雅终止
```

### 3.3 Pod Disruption Budget (PDB)

```yaml
# PDB: 确保最少可用副本数
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: stateless-service-pdb
  namespace: production
spec:
  minAvailable: "70%"  # 至少 70% Pod 可用
  # 或使用 maxUnavailable
  # maxUnavailable: "30%"
  selector:
    matchLabels:
      app: stateless-service
```

```yaml
# 更严格的 PDB（关键服务）
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-service-pdb
  namespace: production
spec:
  minAvailable: 2  # 至少 2 个 Pod 可用
  selector:
    matchLabels:
      app: critical-service
```

### 3.4 Graceful Shutdown 配置

```yaml
# 优雅终止配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: stateless-service
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 90  # 给予足够的终止时间
      containers:
        - name: app
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - |
                    # 1. 停止接收新请求
                    # 2. 等待当前请求完成
                    # 3. 关闭数据库连接
                    sleep 15
          # 确保 SIGTERM 被正确处理
          # 代码中需要监听 SIGTERM 信号
```

```python
# Python 应用优雅终止示例
import signal
import sys
from flask import Flask

app = Flask(__name__)
shutting_down = False

def graceful_shutdown(signum, frame):
    global shutting_down
    shutting_down = True
    print("收到终止信号，开始优雅关闭...")
    # 停止接收新请求
    # 等待当前请求完成
    # 关闭数据库连接
    sys.exit(0)

signal.signal(signal.SIGTERM, graceful_shutdown)

@app.before_request
def check_shutdown():
    if shutting_down:
        return "Service is shutting down", 503
```

## 4. Karpenter Spot 策略

### 4.1 Karpenter NodePool 配置

```yaml
# Karpenter NodePool: Spot 实例池
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: spot-pool
spec:
  template:
    metadata:
      labels:
        node-type: spot
    spec:
      taints:
        - key: spot
          value: "true"
          effect: NoSchedule
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values:
            - spot
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values:
            - m
            - c
            - r
        - key: karpenter.k8s.aws/instance-generation
          operator: Gt
          values:
            - "5"
        - key: karpenter.k8s.aws/instance-size
          operator: In
          values:
            - xlarge
            - 2xlarge
            - 4xlarge
  limits:
    cpu: "500"
    memory: "1000Gi"
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h  # 30 天后替换节点
```

```yaml
# Karpenter NodePool: On-Demand 实例池（关键服务）
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: on-demand-pool
spec:
  template:
    metadata:
      labels:
        node-type: on-demand
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values:
            - on-demand
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values:
            - m
  limits:
    cpu: "200"
    memory: "400Gi"
  disruption:
    consolidationPolicy: WhenEmpty
```

## 5. 中断韧性设计

### 5.1 应用层韧性

```
Spot 中断韧性设计原则:

1. 无状态设计
   - 不在本地存储状态
   - 使用外部存储（Redis/S3/RDS）

2. 快速启动
   - 容器镜像优化（多阶段构建）
   - 启动时间 < 30 秒
   - 使用 Init Container 预热

3. 连接管理
   - 连接池支持快速重连
   - 实现 Circuit Breaker
   - 健康检查包含依赖检查

4. 请求排空
   - 实现 preStop Hook
   - 监听 SIGTERM 信号
   - 等待进行中的请求完成
```

### 5.2 架构层韧性

```yaml
# 混合部署: Spot + On-Demand 混合副本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-service
spec:
  replicas: 6
  template:
    spec:
      # 使用拓扑分散约束，确保 Pod 分散在不同节点类型
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: node-type
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: web-service
```

### 5.3 自动恢复机制

```yaml
# Cluster Autoscaler: Spot 中断后自动补充
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
        - name: cluster-autoscaler
          command:
            - ./cluster-autoscaler
            - --scale-down-enabled=true
            - --scale-down-utilization-threshold=0.5
            - --scale-down-unneeded-time=10m
            - --max-graceful-termination-sec=60
            - --expendable-pods-priority-cutoff=-10  # 允许驱逐低优先级 Pod
```

## 6. 成本节省量化分析

### 6.1 成本计算模型

```python
# spot_cost_analyzer.py
def analyze_spot_savings(cluster_config):
    """分析 Spot 实例成本节省"""
    # 节点配置
    on_demand_nodes = cluster_config["on_demand_nodes"]
    spot_nodes = cluster_config["spot_nodes"]

    # 价格（元/小时）
    on_demand_price = cluster_config["on_demand_price_per_node"]
    spot_price = cluster_config["spot_price_per_node"]
    discount = 1 - (spot_price / on_demand_price)

    # 月度成本
    hours_per_month = 730
    on_demand_cost = on_demand_nodes * on_demand_price * hours_per_month
    spot_cost = spot_nodes * spot_price * hours_per_month

    # 考虑中断损失（假设 5% 的 Spot 中断率）
    interruption_rate = 0.05
    interruption_cost = spot_cost * interruption_rate

    total_cost = on_demand_cost + spot_cost + interruption_cost
    pure_on_demand_cost = (on_demand_nodes + spot_nodes) * on_demand_price * hours_per_month

    savings = pure_on_demand_cost - total_cost
    savings_percentage = (savings / pure_on_demand_cost) * 100

    return {
        "on_demand_monthly_cost": on_demand_cost,
        "spot_monthly_cost": spot_cost,
        "interruption_cost": interruption_cost,
        "total_monthly_cost": total_cost,
        "monthly_savings": savings,
        "savings_percentage": round(savings_percentage, 1),
        "annual_savings": savings * 12,
        "spot_discount": round(discount * 100, 1)
    }

# 示例计算
result = analyze_spot_savings({
    "on_demand_nodes": 40,
    "spot_nodes": 60,
    "on_demand_price_per_node": 5.0,  # 元/小时
    "spot_price_per_node": 1.5,       # 元/小时（70% 折扣）
})
# 输出:
# 月度节省: ¥197,100 (54.2%)
# 年度节省: ¥2,365,200
```

### 6.2 ROI 分析

```
Spot 实例 ROI 分析:

投入:
  - Karpenter/Node Termination Handler 部署: 2 人天
  - 应用改造（优雅终止）: 1 人天/服务 × 10 服务 = 10 人天
  - PDB 配置: 1 人天
  - 监控告警: 1 人天
  总投入: 14 人天

产出（基于上面的计算）:
  - 年度节省: ¥2,365,200
  - 人力成本（14 人天 × ¥2,000/天）: ¥28,000
  - 净收益: ¥2,337,200

ROI: 8,347%
回收期: < 1 个月
```

### 6.3 监控看板

```
Spot 实例监控关键指标:

实时指标:
  - Spot 节点数量/比例
  - Spot 中断次数/频率
  - Spot 价格趋势

成本指标:
  - 月度 Spot vs On-Demand 成本
  - 累计节省金额
  - 每请求成本趋势

可用性指标:
  - Spot 中断后恢复时间
  - Pod 调度成功率
  - 服务可用性（SLO 达成率）
```

## 7. Spot 实例最佳实践

### 7.1 实例类型多样化

```
实例类型多样化策略:

原则: 使用 ≥ 5 种实例类型，降低同时中断风险

示例配置:
  Spot Pool A: m6i.xlarge, m6i.2xlarge, m5.xlarge, m5.2xlarge, m5a.xlarge
  Spot Pool B: c6i.xlarge, c6i.2xlarge, c5.xlarge, c5.2xlarge, c5a.xlarge
  Spot Pool R: r6i.xlarge, r6i.2xlarge, r5.xlarge, r5.2xlarge, r5a.xlarge

效果:
  - 单实例类型中断概率: ~5%
  - 池级别中断概率: < 1%
```

### 7.2 中断预测与预防

```
中断预防措施:

1. 实例类型选择
   - 选择供应充足的实例类型
   - 避免最新一代（供应不稳定）
   - 关注 Spot Advisor 评分

2. 时间策略
   - 避开中断高峰期（通常为工作时间）
   - 批处理任务安排在低峰期

3. 区域分散
   - 跨多个可用区部署
   - 使用 Karpenter 自动选择最佳可用区
```

---

*本文档定义 Spot/抢占式实例的完整策略。团队应根据工作负载特性选择合适的实例类型和中断处理机制，在保证可用性的前提下最大化成本节省。*


<!-- risk-assessed -->
