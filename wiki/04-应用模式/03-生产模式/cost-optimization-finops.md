---
title: 成本优化与 FinOps 生产模式
description: 生产级 K8s 成本治理：Right-sizing、Spot/抢占式策略、自动伸缩经济性与 Chargeback/Showback 实践
summary: 生产级 K8s 成本治理：Right-sizing、Spot/抢占式策略、自动伸缩经济性与 Chargeback/Showback 实践，含成本归因与优化清单。
category: application-patterns
tags:
- finops
- cost-optimization
- spot
- autoscaling
- chargeback
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- FinOps 实践者
estimated_read_time: 16min
intent_queries:
- K8s 成本优化怎么做
- 如何实现 FinOps Chargeback
trigger_keywords:
- FinOps
- 成本优化
- Spot
- right-sizing
- Chargeback
- 资源利用率
prerequisites:
- kubectl-basics
- resource-management
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可执行的运维命令。成本优化操作可能影响工作负载稳定性，执行前请在非生产验证。命令风险等级：🔴 高风险、🟡 中风险、🟢 低风险/只读。

# 成本优化与 FinOps 生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

Kubernetes 集群的资源利用率通常只有 30-50%，远低于虚拟机时代的 60-70%。根因是 requests 过度分配、缺少成本归因和缺少自动伸缩。本文涵盖四大成本优化杠杆：Right-sizing、Spot 节点、自动伸缩经济性和成本归因（Chargeback/Showback），目标是将集群利用率提升至 60%+ 同时保障 SLO。

---

## 1. 成本优化四杠杆

| 杠杆 | 节省幅度 | 风险 | 实施难度 | ROI |
|---|---|---|---|---|
| **Right-sizing** | 20-40% | 低（渐进式） | 低 | ⭐⭐⭐⭐⭐ |
| **Spot/抢占式节点** | 50-70%（节点成本） | 中（可中断） | 中 | ⭐⭐⭐⭐ |
| **自动伸缩 (HPA/Karpenter)** | 15-30% | 低 | 中 | ⭐⭐⭐⭐ |
| **架构优化（多租户/共享）** | 10-20% | 中 | 高 | ⭐⭐⭐ |

> 💡 **优先级建议**: 先做 Right-sizing（零风险高收益），再上 Spot（批量/无状态），然后自动伸缩，最后架构优化。

---

## 2. Right-sizing 实战

### 2.1 发现浪费

```bash
# 🟢 找出 requests 远超实际使用的 Pod（核心浪费源）
# 使用 Kubecost 或 OpenCost 的 right-sizing 建议
kubectl top pods -A --sort-by=memory | head -30

# 🟢 计算资源利用率（使用量 / requests）
kubectl get pods -A -o json | python3 -c "
import json,sys
pods=json.load(sys.stdin)['items']
for p in pods:
    ns=p['metadata']['namespace']
    name=p['metadata']['name']
    for c in p['spec'].get('containers',[]):
        r=c.get('resources',{}).get('requests',{})
        print(f'{ns}/{name}/{c[\"name\"]} req_cpu={r.get(\"cpu\",\"-\")} req_mem={r.get(\"memory\",\"-\")}')
" | head -20
```

### 2.2 分级 Right-sizing 策略

| 工作负载类型 | 策略 | 工具 |
|---|---|---|
| 核心在线服务 | VPA Off 模式采集建议 → 季度人工审核 | VPA + Kubecost |
| 批处理 Job | 依据历史 P95 使用量设 requests | Kubecost recommendations |
| 开发/测试环境 | 激进 right-sizing（P50 × 1.0） | VPA Initial 模式自动应用 |

### 2.3 Right-sizing 安全边界

```yaml
# Right-sizing 不是一刀切——设置安全下限
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: api-vpa
spec:
  updatePolicy:
    updateMode: "Off"
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        minAllowed:
          cpu: 100m          # 不低于 0.1 核
          memory: 128Mi      # 不低于 128MB
        maxAllowed:
          cpu: 4000m         # 不高于 4 核
          memory: 8Gi        # 不高于 8GB
        controlledResources: ["cpu", "memory"]
```

> ⚠️ **Right-sizing 红线**: 永远不要直接在生产应用 VPA Recreate 模式。先用 Off 模式观察 ≥ 7 天，确认建议值合理后再灰度调整。

---

## 3. Spot/抢占式节点经济性

### 3.1 混合节点池模型

```
生产集群节点池组成（典型配置）:
  ├── On-Demand 节点池 (30%): 核心服务、有状态工作负载
  │     └── 保证可用性，不接受 Spot 中断
  ├── Spot 节点池 (50%): 无状态服务、批处理、CI/CD
  │     └── 成本降低 60-70%，需容中断
  └── 节省计划/承诺用量 (20%): 长期基线负载
        └── 预留容量，享受折扣
```

### 3.2 Spot 中断处理

```yaml
# 部署 node-termination-handler 监听 Spot 中断通知
# AWS: aws-node-termination-handler
# 阿里云: ack-termination-handler
# 通用: kubernetes-sigs/aws-node-termination-handler (多云)

spec:
  priorityClassName: spot-batch-low    # Spot 工作负载低优先级
  containers:
    - name: worker
      # 应用必须支持：优雅终止 + 状态持久化 + 幂等重试
  tolerations:
    - key: "spot-instance"
      operator: "Exists"
```

### 3.3 Spot 成本核算

```bash
# 🟢 估算 Spot 节省（以阿里云为例）
# Spot 实例价格 ≈ 按量付费的 20-30%
# 例: 32核128G ecs.g6.4xlarge
#   按量: ¥12.48/小时  Spot: ¥3.12/小时  节省: 75%
```

> ⚠️ **Spot 适用边界**: 仅用于无状态、可重试、可中断的工作负载。数据库/StatefulSet/关键队列消费者**不可**用 Spot。

---

## 4. 自动伸缩经济性

### 4.1 HPA + Karpenter 组合

| 组件 | 作用 | 经济价值 |
|---|---|---|
| **HPA** | 按负载水平扩缩 Pod 副本数 | 低谷期自动缩容，节省 Pod 层资源 |
| **Karpenter** | 按 Pod 需求自动供应/回收节点 | 空闲节点秒级回收，无节点浪费 |
| **Cluster Autoscaler** | 传统节点扩缩容（较慢） | 适合固定节点池场景 |

### 4.2 Karpenter 节点供应策略

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot", "on-demand"]   # 优先 Spot
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["c", "m"]               # 通用计算型
        - key: karpenter.k8s.aws/instance-cpu
          operator: In
          values: ["4", "8", "16", "32"]
  limits:
    cpu: 1000
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized   # 低利用率节点合并
    consolidateAfter: 30s
```

> 💡 Karpenter 的 `WhenEmptyOrUnderutilized` 策略会自动迁移 Pod 并回收低利用率节点，相比 Cluster Autoscaler 额外节省 10-15% 节点成本。

### 4.3 缩容到零

```yaml
# 开发环境非工作时间缩容到零
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: dev-api
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: dev-api
  minReplicas: 0              # 允许缩到零
  maxReplicas: 3
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 50
```

> 🟡 缩容到零适用于开发/测试/预发环境，**不适用于生产**。冷启动延迟（镜像拉取 + 初始化）通常 30-90s，无法满足生产 SLO。

---

## 5. 成本归因 (Chargeback/Showback)

### 5.1 标签体系

成本归因的基础是统一的标签体系。每个工作负载必须标记归属：

```yaml
metadata:
  labels:
    app.kubernetes.io/name: payment-service
    app.kubernetes.io/part-of: order-platform
    finops.team: payments         # 团队归属
    finops.env: production        # 环境
    finops.cost-center: CC-1024   # 成本中心
```

### 5.2 成本分摊模型

| 分摊方式 | 说明 | 适用场景 |
|---|---|---|
| **直接归属** | Pod requests 直接计入团队 | 独占工作负载 |
| **按比例分摊** | 共享资源按 requests 比例分摊 | 共享 namespace / 节点 |
| **均摊** | 平台组件成本均摊到所有租户 | Ingress/监控/日志等共享服务 |

### 5.3 Kubecost / OpenCost 部署

```bash
# 🟢 部署 OpenCost（开源）监控成本
helm install opencost opencost/opencost \
  --namespace opencost --create-namespace \
  --set opencost.prometheus.external.url=http://prometheus:9090

# 🟢 按团队/namespace 查看成本
kubectl port-forward svc/opencost-ui 9090 -n opencost
# 访问 http://localhost:9090 → 按 namespace/label 聚合成本
```

---

## 6. 生产检查清单

| # | 检查项 | 验证方法 | 合格标准 |
|---|---|---|---|
| 1 | 集群 CPU 利用率 | Kubecost/OpenCost 大盘 | ≥ 60%（非生产可更高） |
| 2 | 集群内存利用率 | Kubecost/OpenCost 大盘 | ≥ 65% |
| 3 | 核心服务标签完整 | `kubectl get pods --show-labels` | finops.team/env/cost-center 齐全 |
| 4 | VPA 建议模式已部署 | `kubectl get vpa -A` | 核心服务有 Off 模式 |
| 5 | Spot 占比合理 | 节点池统计 | 无状态工作负载 Spot ≥ 40% |
| 6 | 月度成本报告已生成 | Kubecost 月报 | 按 team/namespace/env 聚合 |
| 7 | 空闲资源已回收 | Karpenter consolidation | 无 < 20% 利用率的常驻节点 |
| 8 | 开发环境缩容策略 | 检查 HPA minReplicas | 非生产 minReplicas=0（可缩到零） |

---

## 7. 排障速查

| 症状 | 可能根因 | 诊断 | 修复 |
|---|---|---|---|
| 成本突增 | 新增高 requests 工作负载 / 节点未回收 | Kubecost 日环比报告 | right-sizing + Karpenter consolidation |
| 集群利用率低（< 40%） | requests 过高 / 无自动缩容 | `kubectl top nodes` + HPA 检查 | right-sizing + 配 HPA |
| Spot 中断导致服务降级 | 核心服务误调度到 Spot | 检查 priorityClass + tolerations | 核心服务移回 On-Demand |
| 成本归因缺失 | 标签不完整 | 检查 Pod labels | 补全 finops.* 标签 + NetworkPolicy |

---

## 8. 跨域协作

- **资源 QoS 与 right-sizing 深入**: 见 [[resource-qos-rightsizing|资源 QoS 与 Right-sizing]]
- **调度与 Spot 策略**: 见 [[scheduling-topology-patterns|调度与拓扑分布模式]]
- **FinOps 成本治理 Runbook**: 见 `生产运维/01-finops/14-finops-cost-governance-runbook.md`
- **自动伸缩配置**: 见 `工作负载/00-core-workloads/21-hpa-vpa-autoscaling.md`


<!-- risk-assessed -->
