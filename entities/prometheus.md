---
title: Prometheus
description: '## 概述'
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

- [[domain-06-observability/10-monitoring-metrics-prometheus.md|10-monitoring-metrics-prometheus]]
- [[domain-06-observability/99-prometheus-enterprise-guide.md|99-prometheus-enterprise-guide]]
- [[domain-06-observability/01-prometheus-enterprise-monitoring.md|01-prometheus-enterprise-monitoring]]
- [[domain-19-landscape-references/graduated/prometheus/prometheus.md|prometheus]]
- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.32.md|RELEASE-NOTES-2.32]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.22.md|RELEASE-NOTES-2.22]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.47.md|RELEASE-NOTES-2.47]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.16.md|RELEASE-NOTES-2.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.36.md|RELEASE-NOTES-2.36]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.53.md|RELEASE-NOTES-2.53]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.12.md|RELEASE-NOTES-2.12]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.43.md|RELEASE-NOTES-2.43]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.26.md|RELEASE-NOTES-2.26]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.37.md|RELEASE-NOTES-2.37]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.52.md|RELEASE-NOTES-2.52]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.13.md|RELEASE-NOTES-2.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.42.md|RELEASE-NOTES-2.42]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.27.md|RELEASE-NOTES-2.27]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.33.md|RELEASE-NOTES-2.33]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.23.md|RELEASE-NOTES-2.23]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.46.md|RELEASE-NOTES-2.46]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.17.md|RELEASE-NOTES-2.17]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.4.md|RELEASE-NOTES-2.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.38.md|RELEASE-NOTES-2.38]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.28.md|RELEASE-NOTES-2.28]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.18.md|RELEASE-NOTES-2.18]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.49.md|RELEASE-NOTES-2.49]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.19.md|RELEASE-NOTES-2.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.48.md|RELEASE-NOTES-2.48]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.5.md|RELEASE-NOTES-2.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.39.md|RELEASE-NOTES-2.39]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.29.md|RELEASE-NOTES-2.29]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.7.md|RELEASE-NOTES-3.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.10.md|RELEASE-NOTES-3.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.11.md|RELEASE-NOTES-3.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.34.md|RELEASE-NOTES-2.34]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.51.md|RELEASE-NOTES-2.51]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.41.md|RELEASE-NOTES-2.41]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.10.md|RELEASE-NOTES-2.10]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.9.md|RELEASE-NOTES-3.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.24.md|RELEASE-NOTES-2.24]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.55.md|RELEASE-NOTES-2.55]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.30.md|RELEASE-NOTES-2.30]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.20.md|RELEASE-NOTES-2.20]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.14.md|RELEASE-NOTES-2.14]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.45.md|RELEASE-NOTES-2.45]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.54.md|RELEASE-NOTES-2.54]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.31.md|RELEASE-NOTES-2.31]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.21.md|RELEASE-NOTES-2.21]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.15.md|RELEASE-NOTES-2.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.44.md|RELEASE-NOTES-2.44]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.35.md|RELEASE-NOTES-2.35]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.9.md|RELEASE-NOTES-2.9]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.50.md|RELEASE-NOTES-2.50]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.40.md|RELEASE-NOTES-2.40]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.11.md|RELEASE-NOTES-2.11]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.8.md|RELEASE-NOTES-3.8]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.25.md|RELEASE-NOTES-2.25]]
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/kudig-ecosystem-guide|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/autoscaling-strategies|Autoscaling Strategies]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/bp-README|Kubernetes 最佳实践指南]] — Cross-reference
- [[concepts/production-operations-best-practices|Production Operations Best Practices]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/learn-README|新人上手快速路径（Quick Start）]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/k8s-monitoring-guide|Kubernetes 监控最佳实践]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/learn-03-oncall-handoff|Day 3: 值班交接 SOP]] — Cross-reference
- [[skills/monitoring-fta|监控与告警异常故障树分析]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/learn-inner-training|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/kubelet-eviction-mechanism|kubelet 资源驱逐机制]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/learn-public-training|Kubernetes 培训：Public Training]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
