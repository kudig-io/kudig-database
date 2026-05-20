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
- rag
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
---

# Observability 可观测性知识图谱索引

> 知识图谱索引：按关键字 **Observability** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 可观测性架构
- [16 - 可观测性设计原则](./domain-2-design-principles/16-observability-design-principles.md)
- [01 - Kubernetes 可观测性架构体系](./domain-8-observability/01-observability-architecture-overview.md)
- [00 - 可观测性开源项目索引](./domain-8-observability/00-open-source-projects-index.md)

### 指标监控
- [02 - 指标监控体系详解](./domain-8-observability/02-monitoring-metrics-system.md)
- [10 - 监控和指标表](./domain-8-observability/10-monitoring-metrics-prometheus.md)
- [11 - 自定义指标适配器与HPA扩展](./domain-8-observability/11-custom-metrics-adapter.md)

### 日志系统
- [03 - 日志收集架构详解](./domain-8-observability/03-logging-architecture.md)
- [08 - 日志审计与合规管理](./domain-8-observability/08-logging-audit-compliance.md)
- [09 - 事件与审计日志管理](./domain-8-observability/09-events-audit-logs.md)
- [12 - 日志和审计表](./domain-8-observability/12-logging-auditing.md)

### 分布式追踪
- [04 - 分布式追踪体系](./domain-8-observability/04-distributed-tracing.md)

### 告警与 SLO
- [05 - 告警管理策略](./domain-8-observability/05-alerting-management.md)
- [06 - 监控告警实战与最佳实践](./domain-8-observability/06-monitoring-alerting-practice.md)
- [18 - SLO/SLI体系建设与管理](./domain-8-observability/18-slo-sli-system.md)

### 监控仪表板
- [07 - 监控仪表板设计与最佳实践](./domain-8-observability/07-monitoring-dashboards.md)

### 集群健康
- [13 - 集群健康检查指南](./domain-8-observability/13-cluster-health-check.md)

### 生产级监控
- [15 - 大规模集群监控最佳实践](./domain-8-observability/15-enterprise-scale-monitoring.md)
- [16 - 多集群统一监控治理](./domain-8-observability/16-multi-cluster-monitoring-governance.md)
- [17 - 监控成本优化与治理](./domain-8-observability/17-monitoring-cost-optimization.md)
- [19 - 监控安全与合规治理](./domain-8-observability/19-security-compliance-governance.md)
- [20 - 监控平台高可用与灾备](./domain-8-observability/20-high-availability-disaster-recovery.md)
- [21 - 监控运维手册与应急响应](./domain-8-observability/21-monitoring-playbooks.md)
- [22 - 可观测性平台最佳实践与案例](./domain-8-observability/22-best-practices-case-studies.md)
- [23 - 企业可观测性实施路线图](./domain-8-observability/23-enterprise-implementation-roadmap.md)
- [24 - 可观测性工具生态系统](./domain-8-observability/24-observability-tool-ecosystem.md)

### 混沌工程
- [14 - 混沌工程实践](./domain-8-observability/14-chaos-engineering.md)

### 故障排查工具
- [25 - Kubernetes 生产环境故障排查全攻略](./domain-8-observability/25-troubleshooting-overview.md)
- [26 - 故障排查增强工具](./domain-8-observability/26-troubleshooting-tools.md)
- [27 - 性能分析与调优工具](./domain-8-observability/27-performance-profiling-tools.md)

### Java 与 Kubernetes
- [99 - Java 应用 Kubernetes 可观测性整合指南](./domain-8-observability/99-java-observability-kubernetes-guide.md)
- [99 - Kubernetes v1.29-v1.33 可观测性新特性指南](./domain-8-observability/99-kubernetes-v1.33-observability-guide.md)

## 关联文档 (K8s集成)

### 控制平面
- [控制平面监控与可观测性](./domain-3-control-plane/05-plane-monitoring-observability.md)

### 工作负载
- [06 - 工作负载监控与告警体系](./domain-4-workloads/06-workload-monitoring-alerting.md)

### 网络监控
- [25 - Ingress 监控与故障排查](./domain-5-networking/25-ingress-monitoring-troubleshooting.md)

### 存储监控
- [12 - 存储监控告警与性能调优](./domain-6-storage/12-storage-monitoring-alerting.md)

### 平台运维
- [05 - 运维指标体系建设](./domain-9-platform-ops/05-operations-metrics-system.md)
- [06 - 监控告警体系](./domain-9-platform-ops/06-monitoring-alerting-system.md)
- [18 - 平台可观测性深度实践](./domain-9-platform-ops/18-platform-observability-practice.md)

### AI 基础设施
- [13 - AI平台可观测性体系](./domain-11-ai-infra/13-ai-platform-observability.md)
- [25 - LLM可观测性与监控体系](./domain-11-ai-infra/25-llm-observability.md)
- [36 - AI平台增强可观测性](./domain-11-ai-infra/36-ai-platform-observability-enhanced.md)

### 故障排查域
- [30 - 监控告警故障排查](./domain-12-troubleshooting/30-monitoring-alerting-troubleshooting.md)
- [39 - 企业级监控告警体系](./domain-12-troubleshooting/39-enterprise-monitoring-alerting-system.md)

### 结构化故障排查
- [可观测性故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting.md)
- [OpenTelemetry Collector 故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting.md)
- [eBPF 可观测性故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/03-ebpf-observability-troubleshooting.md)
- [FinOps 成本优化与云费用故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md)

## 扩展参考

### 企业级监控方案
- [Prometheus企业级监控系统深度实践](./domain-20-enterprise-monitoring-alerting/01-prometheus-enterprise-monitoring.md)
- [Grafana Enterprise Observability Platform 深度实践](./domain-20-enterprise-monitoring-alerting/02-grafana-enterprise-observability.md)
- [OpenTelemetry分布式追踪与可观测性深度实践](./domain-20-enterprise-monitoring-alerting/03-opentelemetry-distributed-tracing.md)
- [Thanos Enterprise Metrics Federation and Long-term Storage](./domain-20-enterprise-monitoring-alerting/04-thanos-enterprise-metrics-federation.md)
- [Datadog企业级APM深度实践](./domain-20-enterprise-monitoring-alerting/05-datadog-enterprise-apm.md)
- [Datadog 企业级监控平台深度实践](./domain-20-enterprise-monitoring-alerting/05-datadog-enterprise-monitoring.md)
- [Zabbix Enterprise Monitoring Platform 深度实践](./domain-20-enterprise-monitoring-alerting/07-zabbix-enterprise-monitoring.md)
- [New Relic Enterprise APM Platform 深度实践](./domain-20-enterprise-monitoring-alerting/08-new-relic-enterprise-apm.md)

### 日志管理与分析
- [ELK Stack企业级日志管理系统深度实践](./domain-21-logging-management-analytics/01-elk-stack-enterprise-logging.md)
- [Fluentd企业级日志收集与处理深度实践](./domain-21-logging-management-analytics/02-fluentd-enterprise-log-processing.md)
- [Loki Enterprise Log Aggregation and Analytics Platform](./domain-21-logging-management-analytics/03-loki-enterprise-log-aggregation.md)
- [企业级日志治理与合规审计深度实践](./domain-21-logging-management-analytics/04-enterprise-log-governance-compliance.md)
- [Graylog 企业级日志管理平台深度实践](./domain-21-logging-management-analytics/04-graylog-enterprise-logging.md)
- [Splunk企业级日志分析与安全智能平台深度实践](./domain-21-logging-management-analytics/04-splunk-enterprise-siem.md)
- [Elastic Stack企业级可观测性平台深度实践](./domain-20-enterprise-monitoring-alerting/06-elastic-stack-enterprise-observability.md)

### 分布式追踪指南
- [K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)](./domain-20-enterprise-monitoring-alerting/99-distributed-tracing-guide.md)
- [Prometheus 企业级监控部署指南](./domain-20-enterprise-monitoring-alerting/99-prometheus-enterprise-guide.md)

### 技能卡片
- [监控告警体系故障诊断与修复](./topic-skills/15-monitoring-alerting-failure.md)
- [日志收集与管理故障诊断与修复](./topic-skills/16-logging-pipeline-failure.md)
- [K8s Node NotReady 诊断与修复](./topic-skills/skill-set/k8s-node-notready/SKILL.md)

### 术语词典
- [告警与 SLO 监控工程](./topic-dictionary/observability/alerting-and-slo-monitoring.md)
- [LLM 可观测性](./topic-dictionary/observability/llm-observability.md)
- [日志聚合与 Loki](./topic-dictionary/observability/log-aggregation-with-loki.md)
- [日志架构](./topic-dictionary/observability/logging-architecture.md)
- [Kubernetes 对象状态指标](./topic-dictionary/observability/metrics-for-kubernetes-object-states.md)
- [Kubernetes 系统组件指标](./topic-dictionary/observability/metrics-for-kubernetes-system-components.md)
- [可观测性](./topic-dictionary/observability/observability.md)
- [OpenTelemetry 与分布式链路追踪](./topic-dictionary/observability/opentelemetry-and-distributed-tracing.md)
- [系统日志](./topic-dictionary/observability/system-logs.md)
- [Kubernetes 系统组件链路追踪](./topic-dictionary/observability/traces-for-kubernetes-system-components.md)

### K8s 事件
- [01 - Kubernetes 事件系统架构与 API 参考](./domain-33-kubernetes-events/01-event-system-architecture.md)
- [02 - Pod 与容器生命周期事件](./domain-33-kubernetes-events/02-pod-container-lifecycle-events.md)
- [03 - 镜像拉取事件](./domain-33-kubernetes-events/03-image-pull-events.md)
- [04 - 探针与健康检查事件](./domain-33-kubernetes-events/04-probe-health-check-events.md)
- [05 - 调度与抢占事件](./domain-33-kubernetes-events/05-scheduling-preemption-events.md)
- [06 - 节点生命周期与状态事件](./domain-33-kubernetes-events/06-node-lifecycle-condition-events.md)
- [07 - Deployment 与 ReplicaSet 控制器事件](./domain-33-kubernetes-events/07-deployment-replicaset-events.md)
- [08 - StatefulSet 与 DaemonSet 控制器事件](./domain-33-kubernetes-events/08-statefulset-daemonset-events.md)
- [09 - Job 与 CronJob 批处理事件](./domain-33-kubernetes-events/09-job-cronjob-batch-events.md)
- [10 - Service 与网络事件](./domain-33-kubernetes-events/10-service-networking-events.md)
- [11 - 存储与卷事件](./domain-33-kubernetes-events/11-storage-volume-events.md)
- [12 - 自动扩缩容事件](./domain-33-kubernetes-events/12-autoscaling-events.md)
- [13 - 安全、准入控制与 RBAC 事件](./domain-33-kubernetes-events/13-security-admission-rbac-events.md)
- [14 - Namespace、资源管理与垃圾回收事件](./domain-33-kubernetes-events/14-namespace-resource-gc-events.md)
- [15 - 生态系统与插件事件](./domain-33-kubernetes-events/15-ecosystem-addon-events.md)

### 速查表
- [PromQL 速查表](./topic-cheat-sheet/promql.md)
- [Kubernetes 生产环境速查卡](./topic-cheat-sheet/k8s.md)
- [网络诊断速查表](./topic-cheat-sheet/networking.md)
- [TLS/SSL 与 PKI 速查表](./topic-cheat-sheet/tls-pki.md)

### CNCF 生态
- [Prometheus](./domain-34-cncf-landscape/graduated/prometheus/prometheus.md)
- [Grafana](./domain-34-cncf-landscape/graduated/grafana/grafana.md)
- [Thanos](./domain-34-cncf-landscape/incubating/thanos/thanos.md)
- [OpenTelemetry](./domain-34-cncf-landscape/incubating/opentelemetry/opentelemetry.md)
- [Jaeger](./domain-34-cncf-landscape/graduated/jaeger/jaeger.md)
- [Fluentd](./domain-34-cncf-landscape/graduated/fluentd/fluentd.md)
- [ Loki](./domain-34-cncf-landscape/sandbox/loki/loki.md)
- [Argo](./domain-34-cncf-landscape/graduated/argo/argo.md)
- [KubeEdge](./domain-34-cncf-landscape/graduated/kubeedge/kubeedge.md)

### 演示文稿
- [Kubernetes 可观测性全栈培训](./topic-presentations/kubernetes-observability-presentation.md)
