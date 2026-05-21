---
title: Observability 可观测性知识图谱索引
description: 可观测性知识图谱索引，聚合监控、日志、追踪、告警、可观测性架构等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- observability
- monitoring
- logging
- tracing
- alerting
- prometheus
- grafana
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 30min
intent_queries:
- Observability 可观测性知识图谱索引 是什么
- 可观测性相关内容
trigger_keywords:
- Observability
- 可观测性
- 监控
- 日志
- 追踪
- 告警
- Prometheus
- Grafana
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
- ebpf-basics
- logging-basics
- tracing-basics
- observability-basics
---

# Observability 可观测性知识图谱索引

> 知识图谱索引：按关键字 **Observability** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 可观测性架构
- [[domain-01-cluster-fundamentals/16-observability-design-principles|16 - 可观测性设计原则]]
- [[domain-06-observability/01-observability-architecture-overview|01 - Kubernetes 可观测性架构体系]]
- [[domain-06-observability/00-open-source-projects-index|00 - 可观测性开源项目索引]]

### 指标监控
- [[domain-06-observability/02-monitoring-metrics-system|02 - 指标监控体系详解]]
- [[domain-06-observability/10-monitoring-metrics-prometheus|10 - 监控和指标表]]
- [[domain-06-observability/11-custom-metrics-adapter|11 - 自定义指标适配器与HPA扩展]]

### 日志系统
- [[domain-06-observability/03-logging-architecture|03 - 日志收集架构详解]]
- [[domain-06-observability/08-logging-audit-compliance|08 - 日志审计与合规管理]]
- [[domain-06-observability/09-events-audit-logs|09 - 事件与审计日志管理]]
- [[domain-06-observability/12-logging-auditing|12 - 日志和审计表]]

### 分布式追踪
- [[domain-06-observability/04-distributed-tracing|04 - 分布式追踪体系]]

### 告警与 SLO
- [[domain-06-observability/05-alerting-management|05 - 告警管理策略]]
- [[domain-06-observability/06-monitoring-alerting-practice|06 - 监控告警实战与最佳实践]]
- [[domain-06-observability/18-slo-sli-system|18 - SLO/SLI体系建设与管理]]

### 监控仪表板
- [[domain-06-observability/07-monitoring-dashboards|07 - 监控仪表板设计与最佳实践]]

### 集群健康
- [[domain-06-observability/13-cluster-health-check|13 - 集群健康检查指南]]

### 生产级监控
- [[domain-06-observability/15-enterprise-scale-monitoring|15 - 大规模集群监控最佳实践]]
- [[domain-06-observability/16-multi-cluster-monitoring-governance|16 - 多集群统一监控治理]]
- [[domain-06-observability/17-monitoring-cost-optimization|17 - 监控成本优化与治理]]
- [[domain-06-observability/19-security-compliance-governance|19 - 监控安全与合规治理]]
- [[domain-06-observability/20-high-availability-disaster-recovery|20 - 监控平台高可用与灾备]]
- [[domain-06-observability/21-monitoring-playbooks|21 - 监控运维手册与应急响应]]
- [[domain-06-observability/22-best-practices-case-studies|22 - 可观测性平台最佳实践与案例]]
- [[domain-06-observability/23-enterprise-implementation-roadmap|23 - 企业可观测性实施路线图]]
- [[domain-06-observability/24-observability-tool-ecosystem|24 - 可观测性工具生态系统]]

### 混沌工程
- [[domain-06-observability/14-chaos-engineering|14 - 混沌工程实践]]

### 故障排查工具
- [[domain-06-observability/25-troubleshooting-overview|25 - Kubernetes 生产环境故障排查全攻略]]
- [[domain-06-observability/26-troubleshooting-tools|26 - 故障排查增强工具]]
- [[domain-06-observability/27-performance-profiling-tools|27 - 性能分析与调优工具]]

### Java 与 Kubernetes
- [[domain-06-observability/99-java-observability-kubernetes-guide|99 - Java 应用 Kubernetes 可观测性整合指南]]
- [[domain-06-observability/99-kubernetes-v1.33-observability-guide|99 - Kubernetes v1.29-v1.33 可观测性新特性指南]]

## 关联文档 (K8s集成)

### 控制平面
- [[domain-01-cluster-fundamentals/05-plane-monitoring-observability|控制平面监控与可观测性]]

### 工作负载
- [[domain-02-workloads-applications/06-workload-monitoring-alerting|06 - 工作负载监控与告警体系]]

### 网络监控
- [[domain-03-networking-traffic/25-ingress-monitoring-troubleshooting|25 - Ingress 监控与故障排查]]

### 存储监控
- [[domain-04-storage-data/12-storage-monitoring-alerting|12 - 存储监控告警与性能调优]]

### 平台运维
- [[domain-07-platform-engineering/05-operations-metrics-system|05 - 运维指标体系建设]]
- [[domain-07-platform-engineering/06-monitoring-alerting-system|06 - 监控告警体系]]
- [[domain-07-platform-engineering/18-platform-observability-practice|18 - 平台可观测性深度实践]]

### AI 基础设施
- [[domain-14-ai-ml-infra/13-ai-platform-observability|13 - AI平台可观测性体系]]
- [[domain-14-ai-ml-infra/25-llm-observability|25 - LLM可观测性与监控体系]]
- [[domain-14-ai-ml-infra/36-ai-platform-observability-enhanced|36 - AI平台增强可观测性]]

### 故障排查域
- [[domain-10-troubleshooting-diagnostics/30-monitoring-alerting-troubleshooting|30 - 监控告警故障排查]]
- [[domain-10-troubleshooting-diagnostics/39-enterprise-monitoring-alerting-system|39 - 企业级监控告警体系]]

### 结构化故障排查
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting|可观测性故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting|OpenTelemetry Collector 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/03-ebpf-observability-troubleshooting|eBPF 可观测性故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 扩展参考

### 企业级监控方案
- [[domain-06-observability/01-prometheus-enterprise-monitoring|Prometheus企业级监控系统深度实践]]
- [[domain-06-observability/02-grafana-enterprise-observability|Grafana Enterprise Observability Platform 深度实践]]
- [[domain-06-observability/03-opentelemetry-distributed-tracing|OpenTelemetry分布式追踪与可观测性深度实践]]
- [[domain-06-observability/04-thanos-enterprise-metrics-federation|Thanos Enterprise Metrics Federation and Long-term Storage]]
- [[domain-06-observability/05-datadog-enterprise-apm|Datadog企业级APM深度实践]]
- [[domain-06-observability/05-datadog-enterprise-monitoring|Datadog 企业级监控平台深度实践]]
- [[domain-06-observability/07-zabbix-enterprise-monitoring|Zabbix Enterprise Monitoring Platform 深度实践]]
- [[domain-06-observability/08-new-relic-enterprise-apm|New Relic Enterprise APM Platform 深度实践]]

### 日志管理与分析
- [[domain-06-observability/01-elk-stack-enterprise-logging|ELK Stack企业级日志管理系统深度实践]]
- [[domain-06-observability/02-fluentd-enterprise-log-processing|Fluentd企业级日志收集与处理深度实践]]
- [[domain-06-observability/03-loki-enterprise-log-aggregation|Loki Enterprise Log Aggregation and Analytics Platform]]
- [[domain-06-observability/04-enterprise-log-governance-compliance|企业级日志治理与合规审计深度实践]]
- [[domain-06-observability/04-graylog-enterprise-logging|Graylog 企业级日志管理平台深度实践]]
- [[domain-06-observability/04-splunk-enterprise-siem|Splunk企业级日志分析与安全智能平台深度实践]]
- [[domain-06-observability/06-elastic-stack-enterprise-observability|Elastic Stack企业级可观测性平台深度实践]]

### 分布式追踪指南
- [[domain-06-observability/99-distributed-tracing-guide|K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)]]
- [[domain-06-observability/99-prometheus-enterprise-guide|Prometheus 企业级监控部署指南]]

### 技能卡片
- [[domain-10-troubleshooting-diagnostics/topic-skills/15-monitoring-alerting-failure|监控告警体系故障诊断与修复]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/16-logging-pipeline-failure|日志收集与管理故障诊断与修复]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL|K8s Node NotReady 诊断与修复]]

### 术语词典
- [[domain-17-system-foundation/topic-dictionary/observability/alerting-and-slo-monitoring|告警与 SLO 监控工程]]
- [[domain-17-system-foundation/topic-dictionary/observability/llm-observability|LLM 可观测性]]
- [[domain-17-system-foundation/topic-dictionary/observability/log-aggregation-with-loki|日志聚合与 Loki]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging-architecture|日志架构]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-for-kubernetes-object-states|Kubernetes 对象状态指标]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-for-kubernetes-system-components|Kubernetes 系统组件指标]]
- [[domain-17-system-foundation/topic-dictionary/observability/observability|可观测性]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[domain-17-system-foundation/topic-dictionary/observability/system-logs|系统日志]]
- [[domain-17-system-foundation/topic-dictionary/observability/traces-for-kubernetes-system-components|Kubernetes 系统组件链路追踪]]

### K8s 事件
- [[domain-17-system-foundation/01-event-system-architecture|01 - Kubernetes 事件系统架构与 API 参考]]
- [[domain-17-system-foundation/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[domain-17-system-foundation/03-image-pull-events|03 - 镜像拉取事件]]
- [[domain-17-system-foundation/04-probe-health-check-events|04 - 探针与健康检查事件]]
- [[domain-17-system-foundation/05-scheduling-preemption-events|05 - 调度与抢占事件]]
- [[domain-17-system-foundation/06-node-lifecycle-condition-events|06 - 节点生命周期与状态事件]]
- [[domain-17-system-foundation/07-deployment-replicaset-events|07 - Deployment 与 ReplicaSet 控制器事件]]
- [[domain-17-system-foundation/08-statefulset-daemonset-events|08 - StatefulSet 与 DaemonSet 控制器事件]]
- [[domain-17-system-foundation/09-job-cronjob-batch-events|09 - Job 与 CronJob 批处理事件]]
- [[domain-17-system-foundation/10-service-networking-events|10 - Service 与网络事件]]
- [[domain-17-system-foundation/11-storage-volume-events|11 - 存储与卷事件]]
- [[domain-17-system-foundation/12-autoscaling-events|12 - 自动扩缩容事件]]
- [[domain-17-system-foundation/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]]
- [[domain-17-system-foundation/14-namespace-resource-gc-events|14 - Namespace、资源管理与垃圾回收事件]]
- [[domain-17-system-foundation/15-ecosystem-addon-events|15 - 生态系统与插件事件]]

### 速查表
- [[domain-17-system-foundation/topic-cheat-sheet/promql|PromQL 速查表]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|Kubernetes 生产环境速查卡]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking|网络诊断速查表]]
- [[domain-17-system-foundation/topic-cheat-sheet/tls-pki|TLS/SSL 与 PKI 速查表]]

### CNCF 生态
- [[domain-19-landscape-references/graduated/prometheus/prometheus|Prometheus]]
- [[domain-19-landscape-references/graduated/grafana/grafana|Grafana]]
- [[domain-19-landscape-references/incubating/thanos/thanos|Thanos]]
- [[domain-19-landscape-references/incubating/opentelemetry/opentelemetry|OpenTelemetry]]
- [[domain-19-landscape-references/graduated/jaeger/jaeger|Jaeger]]
- [[domain-19-landscape-references/graduated/fluentd/fluentd|Fluentd]]
- [[domain-19-landscape-references/sandbox/loki/loki| Loki]]
- [[domain-19-landscape-references/graduated/argo/argo|Argo]]
- [[domain-19-landscape-references/graduated/kubeedge/kubeedge|KubeEdge]]

### 演示文稿
- [[domain-11-production-operations/topic-presentations/kubernetes-observability-presentation|Kubernetes 可观测性全栈培训]]
