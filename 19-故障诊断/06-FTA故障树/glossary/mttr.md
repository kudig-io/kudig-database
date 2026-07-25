---
title: 平均修复时间
description: MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR
  越短，系统恢复能...
summary: MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR 越短，系统恢复能...
category: fta
tags:
- fta
- troubleshooting
- reliability
- mttr
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
- 平均修复时间 是什么
- MTTR (Mean Time To Repair) 详解
trigger_keywords:
- 平均修复时间
- MTTR (Mean Time To Repair)
- fta
prerequisites:
- troubleshooting-methodology
---



# 平均修复时间

> **英文名**: MTTR (Mean Time To Repair)

## 概述

MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR 越短，系统恢复能力越强。

## 核心概念/原理

### 计算公式
MTTR = 总修复时间 / 故障次数
MTTR = MTTD + 诊断时间 + 修复时间 + 验证时间

## 关键机制或特性

降低 MTTR 是运维工程的核心目标。通过自动化诊断（AI Agent）、预定义修复方案（Runbook）和自动修复可以显著降低 MTTR。

## 使用场景与最佳实践

在 K8s 中，MTTR 包括：发现告警→定位根因→执行修复→验证恢复的总时间。

## K8s 场景 MTTR 分解

```
MTTR = MTTD + MTTA + MTTF + MTTR(repair)

K8s Pod CrashLoopBackOff 示例:
  MTTD (发现): 30s   ← Prometheus 告警触发
  MTTA (响应): 5min  ← 值班人员确认
  MTTF (定位): 15min ← 查看日志/事件定位根因
  MTTR (修复): 10min ← 修复配置/回滚部署
  总计: ~30min

优化策略:
  MTTD: 告警规则优化、SLO 监控
  MTTA: 自动升级、ChatOps 通知
  MTTF: Runbook 自动化、日志聚合
  MTTR: GitOps 回滚、自愈控制器
```

## K8s 各组件 MTTR 基准

| 故障类型 | 典型 MTTR | 优化后 MTTR |
|---------|----------|------------|
| Pod CrashLoop | 30min | 5min (自愈) |
| 节点 NotReady | 15min | 3min (自动替换) |
| etcd 单节点故障 | 20min | 5min (自动恢复) |
| 证书过期 | 60min | 10min (自动轮换) |
| 网络分区 | 45min | 15min (多路径) |

## 降低 MTTR 的实践

1. **自动化发现**: Prometheus + Alertmanager 告警，减少 MTTD
2. **Runbook 自动化**: 将排查步骤编码为脚本，减少 MTTF
3. **GitOps 回滚**: `git revert` + ArgoCD 自动同步，减少 MTTR
4. **混沌工程**: 定期演练故障场景，提高团队响应速度
5. **可观测性**: 日志/指标/追踪三支柱，加速根因定位

## 面试要点

1. **MTTR 的组成部分？**
   - MTTD（发现）+ MTTA（响应）+ MTTF（定位）+ MTTR（修复）
   - 每个环节都有优化空间

2. **如何降低 K8s 环境的 MTTR？**
   - 自动化告警和升级（减少 MTTD/MTTA）
   - 标准化 Runbook（减少 MTTF）
   - GitOps + 自动回滚（减少 MTTR）

3. **MTTR 与 SLO 的关系？**
   - SLO = 1 - (MTTR / MTBF)
   - 降低 MTTR 直接提高可用性
   - 99.9% 可用性 = 年停机 < 8.76h

## 参考链接

- [MTTR (Mean Time To Repair)]()

## Related

- [[19-故障诊断/06-FTA故障树/appendix-a-glossary.md|FTA 术语表]]
