---
title: K8s 问题分布与 MTTR 基准
description: '| **应用配置错误** | 35% | 45 分钟 | 中 | 高 — 最常见 |'
summary: '| **应用配置错误** | 35% | 45 分钟 | 中 | 高 — 最常见 |'
category: synthesis
tags:
- k8s
- reliability
- benchmarks
- mttr
- statistics
- etcd
- scheduler
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 问题分布与 MTTR 基准 是什么
- 如何 K8s 问题分布与 MTTR 基准
trigger_keywords:
- K8s
- 问题分布与
- MTTR
- 基准
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
relationships:
- target: '[[概念/etcd Operational Reference.md]]'
  type: uses
- target: '[[技能/fta-方法论/top-events-index/Kubernetes FTA Top Events Index.md]]'
  type: uses
- target: '[[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md]]'
  type: related_to
- target: '[[文档/indexes/INDEX.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 问题分布与 MTTR 基准

## 行业问题分布

基于行业生产数据，Kubernetes 问题按类别分布如下：

| 类别 | 占比 | MTTR | 诊断难度 | FTA 优先级 |
|------|------|------|---------|-----------|
| **应用配置错误** | 35% | 45 分钟 | 中 | 高 — 最常见 |
| **资源耗尽** | 22% | 30 分钟 | 中低 | 高 — 常见 |
| **网络问题** | 18% | 60 分钟 | 高 | 高 — 难诊断 |
| **控制平面** | 10% | 90 分钟 | 非常高 | 关键 — MTTR 最长 |
| **存储** | 8% | 75 分钟 | 高 | 中 — 不频繁但修复慢 |
| **安全/认证** | 5% | 40 分钟 | 中 | 中 |
| **其他** | 2% | 50 分钟 | 不确定 | 低 |

## 关键洞察

1. **应用配置错误占比 35%，排名第一**：FTA 应扩展与配置相关的底事件。大多数 YAML 错误、资源限制配置不当、ConfigMap/Secret 引用缺失都属于此类。

2. **控制平面 MTTR 最长，达 90 分钟**：虽然只占问题的 10%，但控制平面问题（etcd、API Server）需要最深层的 FTA 覆盖，因为它们的严重性和诊断复杂度最高。

3. **网络问题最难诊断**：仅占 18% 但 MTTR 达 60 分钟，表明诊断挑战巨大。FTA 需要为 DNS、CNI 和策略相关问题提供广泛的诊断分支。

4. **资源耗尽最适合自动化**：30 分钟 MTTR 且有明确的可观测信号（内存 > 95%、磁盘 > 90%），使其成为 [[技能/FTA-Driven Runbook Automation.md|FTA 驱动的 Runbook 自动化]] 的最佳候选。

## etcd FMEA 风险优先数

| 故障模式 | 严重度 (S) | 发生率 (O) | 检测度 (D) | RPN |
|---------|:---:|:---:|:---:|:---:|
| 磁盘空间耗尽 | 9 | 5 | 3 | **135** |
| 丢失法定人数 (Quorum) | 10 | 3 | 4 | **120** |
| 数据损坏 | 10 | 2 | 6 | **120** |
| 响应延迟高 | 7 | 6 | 2 | **84** |
| 版本不兼容 | 8 | 2 | 5 | **80** |
| 证书过期 | 9 | 4 | 2 | **72** |

> RPN = S × O × D。值 > 100 需要重点关注。

## 诊断时间分解

| 阶段 | 典型时间 | 优化手段 |
|------|---------|---------|
| 检测 | 1-5 分钟 | Prometheus 告警、症状向量匹配 |
| 诊断 | 5-60 分钟 | FTA 引导路径、自动化证据收集 |
| 修复 | 2-30 分钟 | 预审批 Runbook、自动修复动作 |
| 验证 | 2-10 分钟 | 自动化健康检查、SLO 验证 |

## 按业务影响的顶事件排名

| 顶事件 | 严重度 | 频率 | 业务影响 |
|--------|--------|------|---------|
| TE-1: 集群不可用 | P0 | 罕见 | 全部服务中断 |
| TE-2: 应用不可用 | P0 | 常见 | 收入影响 |
| TE-3: Pod 启动失败 | P1 | 常见 | 部署受阻 |
| TE-4: 网络异常 | P1 | 常见 | 部分服务降级 |
| TE-5: 存储问题 | P1 | 不常见 | 数据访问受阻 |
| TE-15: 灾备失败 | P0 | 罕见 | 业务连续性风险 |

## 容量规划基准

- etcd：SSD 上 10,000+ 写入/秒
- API Server：每实例 1,000+ 请求/秒
- kube-scheduler：100+ Pod/秒调度
- 典型集群：每集群 500-5,000 Pod
- 典型节点：每节点 50-110 Pod（取决于实例规格）

## 相关

- FTA Methodology and Core Principles.md|FTA 方法论与核心原则]]
- [[文档/indexes/INDEX.md|Index]]|Kubernetes FTA Top Events Index]].md|Kubernetes FTA 顶事件索引]]
- [[概念/etcd Operational Reference.md|etcd Operational Reference]].md|etcd 运维参考]]
- [[故障诊断/Production Troubleshooting Playbook.md|生产排障手册]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## Related

- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/dex.md|Dex (entities)]]


<!-- risk-assessed -->
