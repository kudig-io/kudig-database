---
title: 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
description: '# 工作负载管理'
summary: 'Init Containers → Main Containers → Sidecar Containers 执行顺序。'
category: reference
tags:
- k8s
- workloads
- pod
- scheduling
- hpa
- vpa
- autoscaling
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工作负载管理：Pod 生命周期、调度策略与弹性伸缩 是什么
- 如何 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
trigger_keywords:
- 工作负载管理：Pod
- 生命周期
- 调度策略与弹性伸缩
prerequisites:
- kubectl-basics
---



# 工作负载管理

## Pod 生命周期

Pod 状态流转：
```
Pending → Running → Succeeded
                  → Failed
                  → Unknown
```

Init Containers → Main Containers → Sidecar Containers 执行顺序。
探针机制：
- **livenessProbe**：存活检测，失败则重启容器
- **readinessProbe**：就绪检测，失败则从 Endpoint 摘除
- **startupProbe**：启动检测，避免慢启动容器被误杀

## 调度策略

- **nodeSelector**：简单标签匹配
- **nodeAffinity**：表达式式节点亲和性（required/preferred）
- **podAffinity/podAntiAffinity**：Pod 间亲和/反亲和
- **taints & tolerations**：节点排斥 + Pod 容忍
- **topologySpreadConstraints**：跨域均匀分布

## 资源管理

QoS 优先级（OOM 时驱逐顺序）：
1. **BestEffort**：未设置 requests/limits → 最先被驱逐
2. **Burstable**：requests < limits → 次优先被驱逐
3. **Guaranteed**：requests = limits → 最后被驱逐

## 弹性伸缩

| 组件 | 维度 | 触发条件 |
|------|------|----------|
| HPA | Pod 副本数 | CPU/Memory/自定义指标 |
| VPA | Pod 资源配置 | 历史使用量分析 |
| CA（Cluster Autoscaler） | 节点数 | Pending Pod 资源不足 |
| KEDA | 事件驱动 | 消息队列/外部事件 |

HPA 经典公式：`目标副本数 = ceil(当前副本数 × (当前指标值 / 目标指标值))`

---

> 来源：.zread/wiki/drafts/8-gong-zuo-fu-zai-guan-li-pod-sheng-ming-zhou-qi-diao-du-ce-lue-yu-dan-xing-shen-suo.md

## Related

- [[keda]] — KEDA

- [[domain-07-platform-engineering/topic-code-analysis/deployment-create/08-hpa-integration.md|Deployment 与 HPA 集成源码分析]]