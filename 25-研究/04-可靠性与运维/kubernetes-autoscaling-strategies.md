---
title: Kubernetes 自动伸缩策略
summary: 深入研究 K8s 多层级自动伸缩体系（HPA/VPA/KEDA/Cluster Autoscaler/Karpenter），对比各方案的适用场景、性能特征与生产配置。
category: research
tags:
- research
- autoscaling
- hpa
- vpa
- karpenter
- keda
tier: supporting
created: '2026-07-21'
updated: '2026-07-21'
last_updated: '2026-07-21'
status: done
---

# Kubernetes 自动伸缩策略

## 研究背景

Kubernetes 自动伸缩是应对流量波动、优化资源利用率的核心能力。随着云原生应用复杂度提升，单一 HPA 已无法满足需求，多层级伸缩体系成为生产标配。

## 核心问题

1. K8s 多层级伸缩体系（Pod/Node/Cluster）如何协同工作？
2. HPA v2、VPA、KEDA、Karpenter 各自的适用场景和限制？
3. 如何避免伸缩抖动（flapping）和冷启动问题？
4. GPU 工作负载的弹性伸缩有什么特殊考量？

## 调研发现

### 发现一：多层级伸缩体系

| 层级 | 组件 | 伸缩对象 | 触发条件 |
|------|------|----------|----------|
| Pod 水平 | HPA v2 | 副本数 | CPU/内存/自定义指标 |
| Pod 垂直 | VPA | requests/limits | 历史资源使用 |
| 事件驱动 | KEDA | 副本数 (0→N) | 队列深度/外部指标 |
| 节点 | Cluster Autoscaler | 节点数 | Pending Pod |
| 节点 (新一代) | Karpenter | 节点数+规格 | Pending Pod + 约束 |

### 发现二：HPA vs KEDA vs Karpenter 对比

| 维度 | HPA v2 | KEDA | Karpenter |
|------|--------|------|-----------|
| 伸缩对象 | Pod 副本 | Pod 副本 (含 0) | 节点 |
| 指标来源 | metrics-server / custom | 60+ Scaler | Pending Pod |
| 缩容到零 | 不支持 | 支持 | N/A |
| 响应速度 | 15-30s | 10-30s | 30-90s |
| 节点规格选择 | 无 | 无 | 智能选择 |
| 适用场景 | 无状态服务 | 事件驱动/批处理 | 节点资源管理 |

### 发现三：防抖动策略

| 策略 | 配置 | 效果 |
|------|------|------|
| 稳定窗口 | stabilizationWindowSeconds: 300 | 避免频繁缩容 |
| 伸缩速率限制 | maxScaleUp/Down | 限制单次变化幅度 |
| 冷却期 | --scale-down-delay-after-add | 新节点加入后等待 |
| PDB 保护 | minAvailable / maxUnavailable | 保证最小可用 |
| 预热 Pod | 保持最小副本 > 0 | 避免冷启动 |

## 落地方案

### 推荐组合

```
无状态 Web 服务:  HPA (CPU/自定义指标) + Karpenter (节点)
事件驱动任务:    KEDA (队列深度) + Karpenter (节点)
GPU 推理服务:    HPA (QPS/GPU利用率) + 预留节点池
批处理 Job:     KEDA (缩容到零) + Spot 实例
有状态服务:     固定副本 + VPA (资源调优)
```

## 参考资源

- [Kubernetes Autoscaling](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)
- [KEDA](https://keda.sh/)
- [Karpenter](https://karpenter.sh/)
- [VPA](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)

## Related Tags

- [[27-标签/k8s|k8s]]
- [[27-标签/production|production]]
- [[27-标签/best-practices|best-practices]]
