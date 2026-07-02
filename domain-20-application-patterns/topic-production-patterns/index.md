---
title: topic-production-patterns 索引
description: domain-20 应用生产模式索引：Pod 可用性、资源 QoS、调度分布、状态应用、渐进交付、应用排障
summary: domain-20 应用生产模式索引：Pod 可用性、资源 QoS、调度分布、状态应用、渐进交付、应用排障。
category: application-patterns
tags:
- index
- production-patterns
- application-patterns
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 3min
intent_queries:
- 应用生产模式有哪些
- topic-production-patterns 目录
trigger_keywords:
- 生产模式
- 索引
- application-patterns
prerequisites:
- kubectl-basics
---

# topic-production-patterns 索引

本目录收录面向生产环境的应用级运维模式，聚焦"如何让应用在 K8s 上稳定、高效、可观测地运行"。每个文件均为可落地的生产参考，含检查清单、YAML 模板和排障速查。

| 文件 | 主题 | 核心内容 |
|---|---|---|
| [[pod-availability-lifecycle|Pod 可用性生产模式]] | 探针 / PDB / 优雅终止 | 四种探针职责、PDB 配置、preStop sleep、零停机滚动更新 |
| [[resource-qos-rightsizing|资源 QoS 与 Right-sizing]] | requests/limits / VPA / QoS 等级 | 三级 QoS 机制、CPU vs 内存差异、VPA 建议模式、Right-sizing 四步法 |
| [[scheduling-topology-patterns|调度与拓扑分布模式]] | topologySpread / 亲和性 / Spot / Descheduler | 跨 AZ 分布、podAntiAffinity 性能陷阱、Spot 混合部署、再平衡策略 |
| [[stateful-app-patterns|Stateful 应用生产模式]] | StatefulSet / PVC 快照 / 有序升级 | 稳定标识、三层备份、partition 金丝雀、主从 switchover |
| [[progressive-delivery-patterns|渐进式交付模式]] | Canary / 蓝绿 / 特性开关 / Argo Rollouts | 自动分析门控、自动回滚、蓝绿切换、发布安全清单 |
| [[cost-optimization-finops|成本优化与 FinOps]] | Right-sizing / Spot / 自动伸缩 / Chargeback | 四大成本杠杆、VPA 安全边界、Spot 混合池、标签体系与成本归因 |
| [[multi-cluster-dr-patterns|多集群与灾备模式]] | Active-Active/Passive / Velero / RTO/RPO | 拓扑选型、故障切换 Runbook、跨集群恢复、灾备演练清单 |
| [[application-security-hardening|应用安全加固模式]] | PSS/PSA / NetworkPolicy / mTLS / 镜像签名 | 三级安全标准、零信任网络、签名准入、渐进式推进策略 |
| [[application-runbooks|应用排障 Runbook]] | CrashLoopBackOff / OOM / ImagePull / 5xx | 五大高频故障诊断决策树、修复命令、重启根因频率 |

## 与其他目录的关系

- **垂直行业架构**: `topic-application-architecture/` — 按行业的参考架构（电商、金融、IoT 等）
- **微服务设计模式**: `sub-patterns/` — 微服务分解、CQRS、Saga、Sidecar 等设计模式
- **生产就绪指南**: `99-production-readiness-operations-guide.md` — 域级生产就绪总览


<!-- risk-assessed -->
