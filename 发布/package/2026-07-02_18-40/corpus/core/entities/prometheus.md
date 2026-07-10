---
title: Prometheus (entities)
description: '## 概述'
summary: 'description: ''## 项目概述'''
category: entities
tags:
- k8s
- cncf
- observability
- prometheus
- grafana
- crd
- operator
- rag
- kubelet
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Prometheus 是什么
- 如何 Prometheus
trigger_keywords:
- Prometheus
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Prometheus

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **多维数据模型**: 基于指标名和键值对标签的时序数据
- **PromQL**: 强大灵活的查询语言
- **拉取模式**: 主动从目标拉取指标数据
- **服务发现**: 自动发现监控目标
- **告警管理**: 灵活的告警规则和通知
- **可视化**: 内置表达式浏览器，集成 Grafana

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 配置数据持久化存储
- 使用联邦集群处理大规模环境
- 合理设置数据保留期限
- 配置告警规则和通知渠道
- 控制标签基数，避免高基数标签
- 合理配置采集间隔

## 架构定位

在 CNCF 生态中，prometheus 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]
- [[concepts/observability-pillars.md|observability-pillars]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[zot]] — zot
- [[openfga]] — OpenFGA
- [[headlamp]] — Headlamp
- [[entities/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 10-monitoring-metrics-prometheus
- 99-prometheus-enterprise-guide
- 01-prometheus-enterprise-monitoring
- prometheus
- 02-prometheus-promql-advanced
- 03-prometheus-ha-deployment
- RELEASE-NOTES-0.12
- RELEASE-NOTES-2.32
- RELEASE-NOTES-2.22
- RELEASE-NOTES-2.47
- RELEASE-NOTES-2.16
- RELEASE-NOTES-2.36
- RELEASE-NOTES-2.53
- RELEASE-NOTES-0.16
- RELEASE-NOTES-2.12
- RELEASE-NOTES-2.43
- RELEASE-NOTES-2.26
- RELEASE-NOTES-2.37
- RELEASE-NOTES-2.52
- RELEASE-NOTES-0.17
- RELEASE-NOTES-2.13
- RELEASE-NOTES-2.42
- RELEASE-NOTES-2.27
- RELEASE-NOTES-0.13
- RELEASE-NOTES-1.8
- RELEASE-NOTES-2.33
- RELEASE-NOTES-2.23
- RELEASE-NOTES-2.46
- RELEASE-NOTES-2.17
- RELEASE-NOTES-2.4
- RELEASE-NOTES-0.18
- RELEASE-NOTES-1.3
- RELEASE-NOTES-2.38
- RELEASE-NOTES-2.28
- RELEASE-NOTES-3.5
- RELEASE-NOTES-1.7
- RELEASE-NOTES-2.0
- RELEASE-NOTES-2.18
- RELEASE-NOTES-2.49
- RELEASE-NOTES-3.1
- RELEASE-NOTES-1.6
- RELEASE-NOTES-2.1
- RELEASE-NOTES-2.19
- RELEASE-NOTES-2.48
- RELEASE-NOTES-3.0
- RELEASE-NOTES-2.5
- RELEASE-NOTES-0.19
- RELEASE-NOTES-1.2
- RELEASE-NOTES-2.39
- RELEASE-NOTES-2.29
- RELEASE-NOTES-3.4
- RELEASE-NOTES-1.5
- RELEASE-NOTES-2.2
- RELEASE-NOTES-3.3
- RELEASE-NOTES-2.6
- RELEASE-NOTES-1.1
- RELEASE-NOTES-3.7
- RELEASE-NOTES-3.10
- RELEASE-NOTES-2.7
- RELEASE-NOTES-1.0
- RELEASE-NOTES-3.6
- RELEASE-NOTES-3.11
- RELEASE-NOTES-1.4
- RELEASE-NOTES-2.3
- RELEASE-NOTES-3.2
- RELEASE-NOTES-0.20
- RELEASE-NOTES-2.34
- RELEASE-NOTES-2.8
- RELEASE-NOTES-2.51
- RELEASE-NOTES-0.14
- RELEASE-NOTES-2.41
- RELEASE-NOTES-2.10
- RELEASE-NOTES-3.9
- RELEASE-NOTES-2.24
- RELEASE-NOTES-2.55
- RELEASE-NOTES-2.30
- RELEASE-NOTES-2.20
- RELEASE-NOTES-2.14
- RELEASE-NOTES-2.45
- RELEASE-NOTES-2.54
- RELEASE-NOTES-0.11
- RELEASE-NOTES-2.31
- RELEASE-NOTES-2.21
- RELEASE-NOTES-2.15
- RELEASE-NOTES-2.44
- RELEASE-NOTES-2.35
- RELEASE-NOTES-2.9
- RELEASE-NOTES-2.50
- RELEASE-NOTES-0.15
- RELEASE-NOTES-2.40
- RELEASE-NOTES-2.11
- RELEASE-NOTES-3.8
- RELEASE-NOTES-2.25
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[entities/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[entities/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[entities/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[entities/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[entities/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[entities/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[entities/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[entities/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[entities/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[entities/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[entities/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[entities/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[entities/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[entities/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[concepts/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[concepts/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[concepts/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[concepts/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/bp-README.md|Kubernetes 最佳实践指南]] — Cross-reference
- [[concepts/production-operations-best-practices.md|Production Operations Best Practices]] — Cross-reference
- [[concepts/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[skills/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/learn-README.md|新人上手快速路径（Quick Start）]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/learn-03-oncall-handoff.md|Day 3: 值班交接 SOP]] — Cross-reference
- [[skills/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[skills/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[skills/learn-inner-training.md|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/learn-public-training.md|Kubernetes 培训：Public Training]] — Cross-reference
- [[skills/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage.md|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
