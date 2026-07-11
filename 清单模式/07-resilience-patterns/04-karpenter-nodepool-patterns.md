---
title: Karpenter NodePool 模式
description: Karpenter NodePool/NodeClass 配置模式与最佳实践
summary: Karpenter NodePool 配置实现按需节点弹性伸缩，包括实例类型选择、污点容忍和成本优化
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- karpenter
- autoscaling
- nodepool
- aws
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- Karpenter NodePool 配置
- Karpenter 弹性伸缩
- Karpenter vs Cluster Autoscaler
trigger_keywords:
- karpenter
- nodepool
- nodeclass
- autoscaling
- ec2nodeclass
prerequisites:
- k8s-node-basics
- aws-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Karpenter NodePool 模式

## 1. Karpenter vs Cluster Autoscaler

| 特性 | Cluster Autoscaler | Karpenter |
|------|-------------------|-----------|
| 调度方式 | 节点组预设 | 直接根据 Pod 需求选择实例 |
| 速度 | 分钟级 | 秒级 |
| 灵活性 | 固定实例类型 | 动态选择最便宜合适的实例 |
| 多架构 | 复杂 | 原生支持 arm64/amd64 |
| Spot 支持 | 节点组级别 | 原生灵活 |

## 2. NodePool 配置

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: default
spec:
  template:
    metadata:
      labels:
        nodepool: default
    spec:
      requirements:
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64", "arm64"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand", "spot"]
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["m6i.large", "m6i.xlarge", "m6i.2xlarge",
                   "m6g.large", "m6g.xlarge", "m6g.2xlarge",
                   "c6i.large", "c6i.xlarge", "c6g.large"]
        - key: karpenter.k8s.aws/instance-family
          operator: In
          values: ["m6i", "m6g", "c6i", "c6g"]
        - key: karpenter.k8s.aws/instance-cpu
          operator: In
          values: ["2", "4", "8"]
      nodeClassRef:
        group: karpenter.k8s.aws
        kind: EC2NodeClass
        name: default
      taints:
        - key: spot-instance
          value: "true"
          effect: NoSchedule
  limits:
    cpu: 1000
    memory: 4000Gi
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30s
    budgets:
      - nodes: "10%"           # 同时最多中断 10% 节点
```

## 3. EC2NodeClass 配置

```yaml
apiVersion: karpenter.k8s.aws/v1
kind: EC2NodeClass
metadata:
  name: default
spec:
  amiFamily: AL2023
  amiSelectorTerms:
    - alias: al2023@latest
  subnetSelectorTerms:
    - tags:
        Name: "*private*"
  securityGroupSelectorTerms:
    - tags:
        Name: "*eks-cluster-sg*"
  role: KarpenterNodeRole-my-cluster
  blockDeviceMappings:
    - deviceName: /dev/xvda
      ebs:
        volumeSize: 100Gi
        volumeType: gp3
        iops: 3000
        throughput: 125
        deleteOnTermination: true
  userData: |
    #!/bin/bash
    echo "Custom bootstrap script"
  tags:
    Team: platform
    Environment: production
```

## 4. GPU 专用 NodePool

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: gpu-nodepool
spec:
  template:
    metadata:
      labels:
        accelerator: nvidia
        nodepool: gpu
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-gpu-manufacturer
          operator: In
          values: ["nvidia"]
        - key: karpenter.k8s.aws/instance-gpu-name
          operator: In
          values: ["a100", "h100", "l4", "t4"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]   # GPU 不用 Spot
        - key: karpenter.k8s.aws/instance-cpu
          operator: In
          values: ["32", "48", "64", "96"]
      nodeClassRef:
        group: karpenter.k8s.aws
        kind: EC2NodeClass
        name: gpu-nodeclass
      taints:
        - key: nvidia.com/gpu
          effect: NoSchedule
  limits:
    nvidia.com/gpu: 50
```

## 5. Spot 优化 NodePool

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: spot-nodepool
spec:
  template:
    metadata:
      labels:
        nodepool: spot
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot"]
        - key: karpenter.k8s.aws/instance-family
          operator: In
          values: ["m5", "m5a", "m6i", "m6a", "c5", "c6i"]
        - key: karpenter.k8s.aws/instance-cpu
          operator: In
          values: ["2", "4", "8", "16"]
      taints:
        - key: spot-instance
          value: "true"
          effect: NoSchedule
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30s
```

## 6. 应用使用 Karpenter NodePool

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: batch-processor
spec:
  template:
    spec:
      nodeSelector:
        nodepool: spot
      tolerations:
        - key: spot-instance
          operator: Equal
          value: "true"
          effect: NoSchedule
      containers:
        - name: processor
          image: registry.example.com/processor:v1.0.0
          resources:
            requests:
              cpu: "2"
              memory: 4Gi
```

## 7. 成本优化策略

### 7.1 合并 NodePool（减少碎片）

```yaml
# 一个 NodePool 覆盖多种工作负载
requirements:
  - key: karpenter.k8s.aws/instance-family
    operator: In
    values: ["m6i", "m6g", "c6i", "c6g", "r6i", "r6g"]  # 6 个族
  - key: karpenter.k8s.aws/instance-cpu
    operator: In
    values: ["2", "4", "8", "16", "32"]                  # 5 种 CPU 规格
```

### 7.2 Consolidation（整合）

```yaml
disruption:
  consolidationPolicy: WhenEmptyOrUnderutilized
  # WhenEmpty: 仅删除空节点
  # WhenEmptyOrUnderutilized: 替换低利用率节点（更激进省钱）
  consolidateAfter: 30s
  budgets:
    - nodes: "20%"               # 默认允许 20% 中断
    - schedule: "0 9 * * Mon-Fri"
      nodes: "0"                 # 工作时间不允许中断
```

## 8. 监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: karpenter-monitor
  namespace: kube-system
spec:
  endpoints:
    - port: http-metrics
      path: /metrics
      interval: 30s
```

关键指标：

| 指标 | 说明 |
|------|------|
| `karpenter_pods_pending` | Pending Pod 数 |
| `karpenter_nodes_created_total` | 创建的节点总数 |
| `karpenter_nodes_terminated_total` | 终止的节点总数 |
| `karpenter_nodes_total` | 当前节点总数 |
| `karpenter_allocation_duration_seconds` | 分配耗时 |

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| GPU/Spot/OnDemand 分开 NodePool | 便于管理 |
| 使用 Consolidation | 自动整合低利用率节点 |
| 设置 limits | 防止成本失控 |
| 配合 PDB | 保护应用可用性 |
| 监控 Pending Pod | 确认 Karpenter 正常工作 |
| 使用 arm64 降低成本 | Graviton 实例便宜 20% |

## 10. 调试

```bash
# 🟢 低风险：Karpenter 调试
# 查看 NodePool 状态
kubectl get nodepool

# 查看 Provisioned 节点
kubectl get nodes -l karpenter.sh/nodepool

# 查看 Karpenter 日志
kubectl logs -n kube-system -l app.kubernetes.io/name=karpenter

# 检查 Pending Pod 原因
kubectl get pod <pending-pod> -o wide
kubectl describe pod <pending-pod>
```

## Related

- [[清单模式/07-resilience-patterns/02-hpa-advanced-patterns|HPA 高级模式]]
- [[清单模式/07-resilience-patterns/01-pdb-patterns|PDB 模式]]

## See Also

- [Karpenter 文档](https://karpenter.sh/)
- [Karpenter 最佳实践](https://aws.github.io/aws-eks-best-practices/karpenter/)

<!-- risk-assessed -->
