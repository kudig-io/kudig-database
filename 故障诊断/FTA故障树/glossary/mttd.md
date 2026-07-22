---
title: 平均检测时间
description: MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。...
summary: MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。...
category: fta
tags:
- fta
- troubleshooting
- reliability
- mttd
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 平均检测时间 是什么
- MTTD (Mean Time To Detect) 详解
trigger_keywords:
- 平均检测时间
- MTTD (Mean Time To Detect)
- fta
prerequisites:
- troubleshooting-methodology
---



# 平均检测时间

> **英文名**: MTTD (Mean Time To Detect)

## 概述

MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。

## 核心概念/原理

### 影响因素
- 监控覆盖度：未监控的组件故障无法被检测。
- 告警灵敏度：阈值过高导致延迟检测。
- 检测手段：主动探测 vs 被动告警。

## 关键机制或特性

通过完善监控覆盖、优化告警阈值和引入主动健康检查可以缩短 MTTD。

## 使用场景与最佳实践

在 K8s 中，使用 Prometheus 告警、Liveness/Readiness Probe 和 SLO 监控来缩短 MTTD。

## K8s 中降低 MTTD 的实践

| 策略 | 工具 | 效果 |
|------|------|------|
| 指标告警 | Prometheus + Alertmanager | 秒级发现 |
| 日志异常检测 | Loki + LogQL | 分钟级发现 |
| 事件监控 | K8s Events + Falco | 实时检测 |
| SLO 监控 | Sloth + Prometheus | 用户视角 |
| 合成监控 | Blackbox Exporter | 外部视角 |

## MTTD 优化示例

```
优化前:
  用户报告服务不可用 → 人工检查 → 发现故障
  MTTD = 30min (用户发现)

优化后:
  Prometheus 告警 (error_rate > 5% for 1min)
  → Alertmanager → PagerDuty → 值班人员
  MTTD = 2min (自动发现)

关键告警规则:
  - PodCrashLooping: kube_pod_container_status_waiting_reason
  - NodeNotReady: kube_node_status_condition
  - HighErrorRate: rate(http_requests_total{code=~"5.."}[5m])
  - EtcdHighLatency: histogram_quantile(0.99, etcd_disk_wal_fsync_duration)
```

## 面试要点

1. **MTTD 在 MTTR 中的位置？**
   - MTTR = MTTD + MTTA + MTTF + MTTR(repair)
   - MTTD 是第一步，发现越快修复越快

2. **如何降低 K8s 环境的 MTTD？**
   - 完善的告警规则（覆盖关键路径）
   - 多层监控（指标/日志/事件/追踪）
   - SLO 监控（用户视角）

3. **告警疲劳如何影响 MTTD？**
   - 过多告警导致真正问题被忽略
   - 需要告警分级和抑制规则
   - 只告警可操作的问题

## 参考链接

- [MTTD (Mean Time To Detect)]()

## Related

- [[故障诊断/FTA故障树/appendix-a-glossary.md|FTA 术语表]]
