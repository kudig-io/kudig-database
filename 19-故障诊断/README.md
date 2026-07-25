---
title: Troubleshooting & Diagnostics
description: 故障诊断知识域 — FTA 故障树、FEBM 方法论、资源/基础设施/高级排障、JVM 调优、多故障场景
summary: 故障诊断知识域入口，涵盖 FTA 故障树分析、FEBM 法医鉴定循证方法论、Pod/节点/网络/存储排障、JVM 调优、工具套件、技能体系
category: domain
tags:
- troubleshooting
- diagnostics
- fta
- febm
- runbook
- incident-response
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: advanced
audience:
- SRE
- 所有工程师
- 平台团队
estimated_read_time: 10min
---
# 故障诊断 Troubleshooting

> FTA 故障树、FEBM 方法论、资源/基础设施/高级排障、JVM 调优、多故障场景与工具套件。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[19-故障诊断/06-FTA故障树/README.md\|FTA故障树/]] | FTA | 36 个故障树、Mermaid 图、JSON 工作流 |
| [[19-故障诊断/07-FEBM方法论/README.md\|FEBM方法论/]] | FEBM | 法医鉴定循证、证据链、根因分析 |
| [[19-故障诊断/01-核心排障/README.md\|核心排障/]] | 核心 | Pod/Deployment/Service 常见故障 |
| [[19-故障诊断/02-资源排障/README.md\|资源排障/]] | 资源 | CPU/内存/磁盘/配额问题 |
| [[19-故障诊断/03-基础设施排障/README.md\|基础设施排障/]] | 基础设施 | 节点/网络/存储/DNS 故障 |
| [[19-故障诊断/04-高级排障/README.md\|高级排障/]] | 高级 | 控制平面/etcd/证书/准入控制 |
| [[19-故障诊断/09-多故障场景/README.md\|多故障场景/]] | 多故障 | 级联故障、雪崩、复合问题 |
| [[19-故障诊断/05-JVM调优/README.md\|JVM调优/]] | JVM | GC 调优、内存泄漏、线程分析 |
| [[19-故障诊断/11-工具/README.md\|工具/]] | 工具 | kubectl-debug、stern、k9s、诊断脚本 |
| [[19-故障诊断/08-技能体系/README.md\|技能体系/]] | 技能 | 故障诊断技能树、培训路径 |
| [[19-故障诊断/10-QA语料/README.md\|QA语料/]] | QA | 问答语料、知识库训练数据 |

## 跨域导航

- [[15-AI基础设施/README.md|AI基础设施]]
- [[16-专项技术/README.md|专项技术]]
- [[18-云厂商/README.md|云厂商]]
- [[11-发布变更/README.md|发布变更]]
- [[09-可观测性/README.md|可观测性]]
- [[12-可靠性/README.md|可靠性]]
- [[06-存储/README.md|存储]]
- [[08-安全/README.md|安全]]
- [[14-容器运行时/README.md|容器运行时]]
- [[02-工作负载/README.md|工作负载]]
- [[10-平台工程/README.md|平台工程]]
- [[04-应用模式/README.md|应用模式]]
- [[07-数据库中间件/README.md|数据库中间件]]
- [[03-清单模式/README.md|清单模式]]
- [[13-生产运维/README.md|生产运维]]
- [[21-生态参考/README.md|生态参考]]
- [[17-系统基础/README.md|系统基础]]
- [[05-网络/README.md|网络]]
- [[01-集群基础/README.md|集群基础]]

## 故障诊断方法论

### FEBM 法医鉴定循证方法

| 阶段 | 核心活动 | 输出物 |
|------|----------|--------|
| 证据收集 | 日志、事件、指标、拓扑快照 | 证据链清单 |
| 假设形成 | 基于证据提出可能根因 | 假设列表（按概率排序） |
| 假设验证 | 逐一验证/排除假设 | 确认根因 |
| 修复执行 | 最小影响修复 + 验证 | 修复报告 |
| 复盘归档 | 时间线、根因链、预防措施 | 事故报告 |

### 快速决策树

```
故障发生
├─ 服务完全不可用？
│   ├─ 是 → P0 立即响应，检查控制平面 + 节点状态
│   └─ 否 → 影响范围评估
│       ├─ >50% 用户 → P0
│       ├─ 10-50% → P1，30min 内响应
│       └─ <10% → P2，计划内处理
├─ 是否由变更引起？
│   ├─ 是 → 立即回滚变更
│   └─ 否 → 进入 FEBM 流程
└─ 是否级联故障？
    ├─ 是 → 切断级联源（熔断/限流/隔离）
    └─ 否 → 单点故障排查
```

## 严重级别分类

| 级别 | 定义 | 响应时间 | 升级路径 |
|------|------|----------|----------|
| P0 | 核心业务完全不可用，数据丢失风险 | 5min | 值班→主管→CTO |
| P1 | 部分服务降级，有 workaround | 30min | 值班→团队 Lead |
| P2 | 非关键路径，影响可控 | 4h | 工单跟踪 |
| P3 | 潜在风险，无当前影响 | 下一迭代 | Backlog |

## 常用诊断命令速查

``` bash
# 🟢 集群健康总览
kubectl get nodes && kubectl get pods -A --field-selector=status.phase!=Running
kubectl top nodes && kubectl top pods -A --sort-by=cpu | head -20

# 🟢 事件查看（最近 1 小时）
kubectl get events -A --sort-by='.lastTimestamp' | tail -50

# 🟢 控制平面状态
kubectl get pods -n kube-system -l tier=control-plane
kubectl -n kube-system logs -l component=kube-apiserver --tail=20
```
