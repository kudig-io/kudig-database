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

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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
- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[zot]] — zot
- [[openfga]] — OpenFGA
- [[headlamp]] — Headlamp
- [[23-实体/15-参考与索引/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
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
- [[23-实体/15-参考与索引/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[23-实体/15-参考与索引/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[23-实体/15-参考与索引/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[23-实体/15-参考与索引/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[23-实体/15-参考与索引/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[23-实体/15-参考与索引/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[22-概念/08-可靠性与运维/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[22-概念/08-可靠性与运维/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[22-概念/11-交叉分析/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[22-概念/08-可靠性与运维/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]] — Cross-reference
- [[22-概念/10-最佳实践/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[22-概念/07-调度与资源/autoscaling-strategies.md|Autoscaling Strategies]] — Cross-reference
- [[22-概念/12-研究/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[22-概念/12-研究/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[22-概念/03-网络/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[22-概念/12-研究/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[22-概念/10-最佳实践/bp-README.md|Kubernetes 最佳实践指南]] — Cross-reference
- [[22-概念/10-最佳实践/production-operations-best-practices.md|Production Operations Best Practices]] — Cross-reference
- [[22-概念/12-研究/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-README.md|新人上手快速路径（Quick Start）]] — Cross-reference
- [[26-技能/05-网络/networkpolicy/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-03-oncall-handoff.md|Day 3: 值班交接 SOP]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[26-技能/02-控制面/apiserver/诊断排障/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-inner-training.md|Kubernetes 培训：Inner Training]] — Cross-reference
- [[26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-public-training.md|Kubernetes 培训：Public Training]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage.md|存储故障排查]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/诊断排障/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/13-observability-architecture|15 - Kubernetes 可观测性架构体系]]
- [[01-集群基础/02-设计原则/17-observability-design-principles|16 - 可观测性设计原则]]
- [[01-集群基础/02-设计原则/15-service-mesh-architecture|14 - 服务网格与微服务架构设计]]
- [[01-集群基础/02-设计原则/01-design-principles-foundations|01 - Kubernetes 设计原则与哲学 (Foundations)]]
- [[01-集群基础/03-控制平面/16-kube-proxy-deep-dive|kube-proxy 深度解析 (kube-proxy Deep Dive)]]
- [[02-工作负载/00-总览/02-kubernetes-multi-tenant-architecture|Kubernetes 多租户与资源隔离生产架构]]
- [[02-工作负载/01-核心工作负载/23-resource-management|16 - 资源管理表]]
- [[02-工作负载/02-Java-on-K8s/05-java-cicd-tekton-argocd|Java CI/CD on Kubernetes: Tekton + ArgoCD 实践指南 [topic-java-kubernetes]]]
- [[04-应用模式/02-行业架构/19-cloudnative-devops-architecture|云原生 DevOps 平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/04-im-rtc-architecture|实时通信 (IM / RTC) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/94-smart-prison|智慧监狱架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/38-supply-chain-finance|供应链金融架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/91-urban-air-mobility|低空经济（eVTOL/UAM）架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/21-cross-border-ecommerce|跨境电商架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/02-mini-program-architecture|小程序平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/53-new-retail-dtc|新零售 DTC 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/85-hydrogen-energy|氢能源架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/01-ecommerce-architecture|电商系统 Kubernetes 生产架构设计 (应用模式)]]
- [[04-应用模式/02-行业架构/93-digital-twin-factory|数字孪生工厂架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/54-social-gaming-metaverse|社交游戏与元宇宙社交架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/22-nev-connected-vehicle|新能源车联网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/33-crossborder-warehouse|跨境电商海外仓架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/62-distributed-energy|分布式能源架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/42-secondhand-circular|二手交易与循环经济架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/80-tsn-network|时间敏感网络 TSN 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/74-immersive-xr|沉浸式 XR 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/51-smart-manufacturing-mes|智能制造 MES 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/23-xinchuang-it-innovation|信创替代架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/47-smart-mining|智慧矿山架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/57-digital-therapeutics|数字疗法与互联网医疗架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/92-smart-sports-venue|智慧体育场馆架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/11-smart-retail-architecture|智慧零售与新零售 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/25-quantitative-trading|证券量化交易架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/46-satellite-internet|卫星互联网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/86-solid-state-battery|固态电池架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/66-space-internet|太空互联网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/72-digital-twin-city|数字孪生城市架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/32-smart-restaurant|智慧餐饮架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/56-smart-elderly-care|智慧养老架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/95-industrial-metaverse|工业元宇宙架构设计 — 阿里云视角]]
- [[05-网络/01-K8s网络核心/26-ingress-monitoring-troubleshooting|133 - Ingress 监控与故障排查]]
- [[05-网络/04-API网关/12-api-gateway-observability|12 - API 网关可观测性：指标、日志与链路追踪]]
- [[07-数据库中间件/01-数据库/02-postgresql-enterprise-database|PostgreSQL 企业级数据库高可用架构]]
- [[08-安全/05-供应链/11-compliance-automation-audit|合规自动化与审计 (Compliance Automation and Audit)]]
- [[08-安全/05-供应链/09-policy-controller-verification|Policy Controller 镜像验证 (Policy Controller Image Verification)]]
- [[09-可观测性/00-总览/04-elastic-stack-enterprise-observability|Elastic Stack企业级可观测性平台深度实践]]
- [[10-平台工程/01-构建/15-backstage-idp-guide|Backstage 内部开发者平台 (IDP) 构建指南]]
- [[10-平台工程/02-运维/19-kubernetes-v1.33-platform-ops-guide|Kubernetes v1.29-v1.33 平台运维新特性指南]]
- [[10-平台工程/02-运维/09-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[10-平台工程/03-治理/02-performance-benchmarking-tuning|性能基准测试与调优 (Performance Benchmarking & Tuning)]]
- [[10-平台工程/04-开发体验/04-platform-team-topology|平台团队拓扑与运营 (Platform Team Topology and Operations)]]
- [[14-容器运行时/03-containerd-CRI-O/18-ebpf-runtime-security-falco-tetragon|eBPF 运行时安全：Falco/Tetragon/Tracee 部署与威胁检测]]
- [[15-AI基础设施/02-AI-Agents/15-agent-corpus-gap-analysis|Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]]]
- [[15-AI基础设施/02-AI-Agents/30-agent-harness-engineering|Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/18-agentscope-tool-system|AgentScope 工具系统与 MCP 集成 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/47-openclaw-tools-mechanism|OpenClaw TOOLS.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/37-agent-harness-multi-agent|Agent Harness 多 Agent 编排 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/45-openclaw-user-mechanism|OpenClaw USER.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/MEMORY|记忆系统 (02-ai-agents)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/USER|用户画像 — ACK 运维工程师 (02-ai-agents)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/SOUL|KuDig Doctor — 角色人格与绝对红线 (02-ai-agents)]]
- [[16-专项技术/02-WebAssembly/04-wasmcloud-platform|wasmCloud 平台]]
- [[17-系统基础/01-Linux/15-linux-commands-reference|Linux 命令大全参考]]
- [[17-系统基础/02-硬件/18-hardware-failure-case-studies|硬件问题实战案例库]]
- [[17-系统基础/04-K8s事件/12-autoscaling-events|12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]
- [[17-系统基础/04-K8s事件/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]]
- [[17-系统基础/06-知识字典/observability/log-aggregation-with-loki|日志聚合与 Loki]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components|Kubernetes 系统组件指标]]
- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring|告警与 SLO 监控工程]]
- [[17-系统基础/06-知识字典/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[17-系统基础/06-知识字典/operations/greenops-and-carbon-aware-computing|GreenOps 与碳感知计算]]
- [[17-系统基础/06-知识字典/platform-engineering/api-priority-and-fairness|API 优先级与公平性（API Priority and Fairness）]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring|Volume Health Monitoring（卷健康监控）]]
- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis|Node NotReady 状态深度诊断]]
- [[19-故障诊断/02-资源排障/07-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]]
- [[19-故障诊断/04-高级排障/structural-symptom-mapping-layer|症状快速映射层 (Symptom-SOP-RootCause Mapping) [topic-structural-trouble-shooting]]]
- [[19-故障诊断/06-FTA故障树/18-typical-scenarios|第十八章：典型场景完整方案 (故障诊断)]]
- [[22-概念/06-可观测性/prometheus-argocd-monitoring|Prometheus 与 ArgoCD 监控集成]]
- [[22-概念/08-可靠性与运维/K8s-故障分布与-MTTR-基准|K8s 问题分布与 MTTR 基准]]
- [[22-概念/08-可靠性与运维/Structural-Troubleshooting-Framework|Structural Troubleshooting Framework]]
- [[22-概念/11-交叉分析/etcd-×-Prometheus|etcd × Prometheus]]
- [[22-概念/11-交叉分析/服务网格 × 零信任安全|服务网格 x 零信任安全]]
- [[22-概念/11-交叉分析/apiserver-×-Prometheus|apiserver × Prometheus]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/清单规范/01-pod-specification-complete|03 - Pod 完整规格说明书]]
- [[26-技能/04-工作负载/pod/清单规范/04-poddisruptionbudget-reference|28 - PodDisruptionBudget YAML 配置参考]]
- [[26-技能/04-工作负载/pod/清单规范/05-advanced-pod-patterns|35 - 高级 Pod 模式与调度策略 YAML 配置参考]]
- [[26-技能/04-工作负载/pod/诊断排障/structural-01-pod-troubleshooting|Pod 故障排查与运行机制深度指南 [topic-structural-trouble-shooting]]]
