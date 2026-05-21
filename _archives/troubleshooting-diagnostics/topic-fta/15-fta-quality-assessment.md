---
title: 第十五章：FTA 质量评估与优化
description: '# 第十五章：FTA 质量评估与优化'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- grafana
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 第十五章：FTA 质量评估与优化 是什么
- 如何 第十五章：FTA 质量评估与优化
- 第十五章：FTA 质量评估与优化 根因分析
- 第十五章：FTA 质量评估与优化 故障树
trigger_keywords:
- 第十五章：FTA
- 质量评估与优化
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- monitoring-basics
---

# 第十五章：FTA 质量评估与优化

> **所属部分**: 第四部分 - FTA 系统工程实践  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第十四章：构建 FTA 系统的工程化方法](./14-fta-system-engineering.md)  
> **下一章**: [第十六章：团队能力建设](./16-team-capability-building.md)

---

## 15.1 核心质量指标

| 指标 | 定义 | 计算方式 | 目标值 | 数据来源 |
|------|------|---------|-------|---------|
| **覆盖率** | FTA 覆盖的故障模式比例 | (FTA 包含的故障 / 历史故障总数) x 100% | > 95% | 故障工单系统 |
| **诊断准确率** | FTA 正确定位根因的比例 | (正确诊断次数 / 总诊断次数) x 100% | > 90% | Agent 诊断日志 |
| **首次修复率** | 第一个修复方案成功的比例 | (首次修复成功 / 总修复次数) x 100% | > 80% | Agent 执行日志 |
| **底事件可观测性** | 有监控覆盖的底事件比例 | (有监控的 BE / 总 BE 数) x 100% | 100% | 监控系统 |
| **平均诊断深度** | 诊断遍历的平均层数 | Sum(诊断深度) / 诊断次数 | 2-4 层 | Agent 诊断日志 |
| **MTTD 改善** | 平均检测时间改善比例 | (旧MTTD - 新MTTD) / 旧MTTD x 100% | > 50% | 监控系统 |
| **MTTR 改善** | 平均修复时间改善比例 | (旧MTTR - 新MTTR) / 旧MTTR x 100% | > 60% | 工单系统 |

## 15.2 质量监控 Dashboard

```yaml
# Grafana Dashboard 配置 (关键面板)

panels:
  - title: "FTA 覆盖率趋势"
    type: graph
    query: |
      (count(fta_matched_incidents) / count(total_incidents)) * 100
    threshold:
      green: "> 95%"
      yellow: "85-95%"
      red: "< 85%"

  - title: "Agent 诊断准确率 (7天滚动)"
    type: stat
    query: |
      (count(agent_diagnosis_correct{window="7d"}) / 
       count(agent_diagnosis_total{window="7d"})) * 100
    threshold:
      green: "> 90%"
      yellow: "80-90%"
      red: "< 80%"

  - title: "各顶事件 MTTR 对比"
    type: bar
    query: |
      avg(incident_resolution_time) by (top_event)
    
  - title: "1阶最小割集数量"
    type: stat
    query: |
      count(fta_minimal_cut_set{order="1"})
    description: "单点故障数量，目标为 0"
    threshold:
      green: "0"
      red: "> 0"
```

## 15.3 持续优化方法

**A/B 测试**：

```
实验设计:
  组A (对照组): 传统 Runbook 处理
  组B (实验组): FTA + Agent 自动处理
  
  按轮值周期随机分配告警到两组
  
  对比指标:
  ┌──────────────┬──────────────┬──────────────┬──────────┐
  │ 指标          │ 组A (Runbook) │ 组B (FTA+Agent)│ 改善幅度 │
  ├──────────────┼──────────────┼──────────────┼──────────┤
  │ MTTD         │ 5.2 min      │ 0.8 min      │ -84.6%  │
  │ MTTR (P0)    │ 35 min       │ 8 min        │ -77.1%  │
  │ MTTR (P1)    │ 45 min       │ 15 min       │ -66.7%  │
  │ 误诊率       │ 12%          │ 5%           │ -58.3%  │
  │ 重复工单率    │ 18%          │ 4%           │ -77.8%  │
  │ SRE 介入率   │ 100%         │ 25%          │ -75.0%  │
  └──────────────┴──────────────┴──────────────┴──────────┘
```

**混沌工程验证**：

```yaml
# 定期混沌实验计划
chaos_experiment_schedule:
  weekly:
    - name: "FTA 路径验证 - Pod 级别"
      experiments:
        - pod-kill (随机 Pod)
        - [[entities/docker|container]]-oom (内存压力)
        - pod-cpu-stress (CPU 压力)
      validation: "Agent 是否在 5 分钟内正确诊断和修复"
      
  monthly:
    - name: "FTA 路径验证 - 节点级别"
      experiments:
        - node-drain (节点排水)
        - network-partition (网络分区)
        - disk-fill (磁盘填充)
      validation: "Agent 是否正确触发节点级别修复流程"
      
  quarterly:
    - name: "FTA 完整性审计"
      method: "将全部 63 个底事件逐一注入故障"
      validation: "FTA 路径覆盖率是否达到 95%"
      output: "FTA 更新清单"
```

---

> **导航**: [<< 上一章 - 构建 FTA 系统的工程化方法](./14-fta-system-engineering.md) | [下一章 - 团队能力建设 >>](./16-team-capability-building.md)
