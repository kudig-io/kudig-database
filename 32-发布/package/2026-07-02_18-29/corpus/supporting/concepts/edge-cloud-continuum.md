---
title: 边缘-云连续体的运维架构
description: '# 边缘-云连续体的运维架构'
summary: '# 边缘-云连续体的运维架构'
category: synthesis
tags:
- edge-computing
- cloud
- kubeedge
- multi-cluster
- iot
- prometheus
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 边缘-云连续体的运维架构 是什么
- 如何 边缘-云连续体的运维架构
trigger_keywords:
- 边缘-云连续体的运维架构
prerequisites:
- kubectl-basics
- prometheus-basics
relationships:
- target: '[[entities/kubeedge.md]]'
  type: related_to
- target: '[[domain-17-system-foundation/速查卡/networking.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 边缘-云连续体的运维架构

## 架构分层

```
中心云 (Cloud):
  → 控制平面、数据聚合、AI 训练
      ↓
区域节点 (Edge Cluster):
  → 数据预处理、区域调度
      ↓
边缘设备 (Edge Node):
  → 本地推理、实时响应
      ↓
终端设备 (Device):
  → 传感器、执行器
```

## [[entities/kubeedge.md|KubeEdge]] 核心能力

```
云边协同:
  - 云端控制，边缘自治
  - 断网时边缘继续运行
  - 网络恢复后状态同步

边缘特有挑战:
  - 资源受限（CPU/内存/存储）
  - 网络不稳定
  - 物理安全
```

## 运维策略

```
1. 边缘应用轻量化
   → 裁剪容器镜像
   → 使用静态 Pod

2. 数据同步策略
   → 关键数据本地缓存
   → 批量上传到云端

3. 监控降级
   → 边缘本地 Prometheus
   → 定期汇总到中心
```

## 相关 Domain

- domain-01-cluster-fundamentals/09-edge-computing/01-kubeedge-overview
- domain-03-[[domain-17-system-foundation/速查卡/networking.md|networking]]-traffic/07-edge-networking/01-edge-network-patterns
## Related

- [[domain-10-troubleshooting-diagnostics/高级排障/37-multi-cluster-management-troubleshooting.md|多集群管理故障排查]]
- [[domain-10-troubleshooting-diagnostics/高级排障/40-large-scale-cluster-operations.md|大规模集群运维]]


<!-- risk-assessed -->
