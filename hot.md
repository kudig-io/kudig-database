---
title: Hot Cache
description: 'Cross-link complete orphan elimination: 9,121 links added, orphans 2,659 (65%) → 0 (0.0%), cohesion 0.00 → 1.0000'
category: general
tags:
- k8s
- etcd
- prometheus
- istio
- cilium
- flux
- falco
- networkpolicy
- crd
- operator
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hot Cache 是什么
- 如何 Hot Cache
trigger_keywords:
- Hot
- Cache
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

# Hot Cache

*A ~500-word semantic snapshot of recent activity. Updated after every major write operation.*

## Recent Activity
- [2026-05-21T23:08:00+08:00] CROSS_LINK complete orphan elimination: 9,121 links added/conversions across ~2,400 pages. Orphans: 2,659 (65%) → 0 (0.0%). Cohesion: 0.00 → 1.0000. All orphan pages now connected.
- Cross-link FINAL: 19937 links added/fixed, orphans at 160 (4.0%). Target <5% ACHIEVED.
- Cross-link deep-dive complete: 14612 links added/fixed, orphans at 255 (6.4%).- Cross-link final: 7045 links added, orphans down to 954 (24.0%). Domain: 25, Release-notes: 1.- Domain & release-notes cross-link pass: 4918 links added, orphans down to 1748 (43.9%).- [2026-05-21] WIKI_SYNTHESIZE 第二轮 — 扫描 578 页，创建 10 个新综合页：控制器模式×Deployment、CRD×可观测性、Pod生命周期×Secret、Operator×Pod生命周期、控制器×可观测性、Secret×存储、Cilium eBPF×可观测性、可观测性支柱×Prometheus、CI/CD×Secret、etcd×Operator。：Operator 模式 × 可观测性、Deployment × Secret 管理、Pod 生命周期 × 存储模型、CNI 插件 × NetworkPolicy、etcd × 可观测性。跳过 10 个候选对。- [2026-05-21] Cross-linked 4760 mentions across 969 pages (orphans remaining: 2752).
## Active Threads

- KUDIG-DATABASE wiki vault: 578 页面覆盖 3,273 个源文件（40 domains + 22 topics）。
- 新综合页面（15 个）：第一轮 5 个 + 第二轮 10 个（控制器×Deployment、CRD×可观测性、Pod×Secret、Operator×Pod、控制器×可观测性、Secret×存储、Cilium×监控、可观测性支柱×Prometheus、CI/CD×Secret、etcd×Operator）。
- 核心 K8s 概念：架构、控制平面、工作负载、网络、存储、安全、可观测性、弹性伸缩、Operator、HA、多租户。
- FTA 方法论：故障树分析、诊断执行引擎、症状向量匹配、Runbook 自动化、Agent 编排。
- K8S 培训体系完整：15 课基础概念、新人上手、On-Call 问答、故障决策树、两套 28 天培训。
- 版本演进索引：K8s 核心 + 33 个生态组件（Argo CD、Flux、Istio、Cilium、Prometheus、Falco、Trivy 等）的版本历史与升级指南。
- 最佳实践体系：基础设施（集群/网络/存储）、可观测性（监控/日志/追踪）、运维（部署/扩缩容/灾备）、安全（网络/Pod/密钥）。
- CNCF 生态覆盖：236 个 CNCF landscape 项目。

## Key Takeaways

- 578 个 wiki 页面构成 3,273 个源文件的精炼入口点。
- 5 个新 synthesis 页面填补了 Operator-可观测性、Deployment-Secret、Pod-存储、CNI-NetworkPolicy、etcd-监控的跨域知识空白。
- 中文输出已配置为默认语言，所有 wiki 操作统一中文。
- 版本演进系列页面填补了 K8s 生态组件版本历史的空白，为升级决策提供参考。
- 最佳实践体系覆盖基础设施、可观测性、运维操作、安全四大域。

## Flagged Contradictions

*None — vault freshly ingested with no conflicting sources.*
