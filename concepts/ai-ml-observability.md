---
title: AI/ML 工作负载的可观测性
description: '## GPU 监控'
summary: '## GPU 监控'
category: synthesis
tags:
- ai-ml
- observability
- gpu-monitoring
- mLOps
- metrics
- gpu
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI/ML 工作负载的可观测性 是什么
- 如何 AI/ML 工作负载的可观测性
trigger_keywords:
- AI
- ML
- 工作负载的可观测性
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
relationships:
- target: '[[skills/best-practices/best-practices/observability/monitoring.md]]'
  type: related_to
- target: '[[系统基础/知识字典/observability/observability.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI/ML 工作负载的可观测性

## GPU 监控

```
关键指标:
├── dcgm_gpu_utilization        GPU 计算利用率
├── dcgm_memory_used            GPU 显存使用
├── dcgm_temperature            GPU 温度
├── dcgm_power_usage            GPU 功耗
├── dcgm_pcie_traffic           PCIe 带宽
└── dcgm_xid_errors             GPU 错误
```

## 训练任务追踪

```
MLflow / WandB + Kubernetes:
  → Pod 标签关联实验 ID
  → 资源使用与实验结果关联
  → 自动发现资源浪费的训练任务
```

## 推理服务监控

```
模型服务 SLO:
  - P99 推理延迟 < 100ms
  - 吞吐量 > 1000 QPS
  - GPU 利用率 > 60%（避免浪费）
```

## 相关 Domain

- AI基础设施/03-gpu-scheduling/01-gpu-scheduling-management
- [[系统基础/知识字典/observability/observability.md|observability]]/02-metrics/02-[[skills/best-practices/best-practices/observability/monitoring.md|monitoring]]-metrics-system]]


<!-- risk-assessed -->
