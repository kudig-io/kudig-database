---
title: Observability 可观测性知识图谱索引
description: 可观测性知识图谱索引，聚合监控、日志、追踪、告警、可观测性架构等所有相关内容
summary: 可观测性知识图谱索引，聚合监控、日志、追踪、告警、可观测性架构等所有相关内容
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Observability 可观测性知识图谱索引

> 知识图谱索引：按关键字 **Observability** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 可观测性架构
- 16 - 可观测性设计原则
- 01 - [[entities/kubernetes.md|Kubernetes 可观测性架构体系]]
- 00 - 可观测性开源项目索引

### 指标监控
- 02 - 指标监控体系详解
- 10 - 监控和指标表
- 11 - 自定义指标适配器与HPA扩展

### 日志系统
- 03 - 日志收集架构详解
- 08 - 日志审计与合规管理
- 09 - 事件与审计日志管理
- 12 - 日志和审计表

### 分布式追踪
- 04 - 分布式追踪体系

### 告警与 SLO
- 05 - 告警管理策略
- 06 - 监控告警实战与最佳实践
- 18 - SLO/SLI体系建设与管理

### 监控仪表板
- 07 - 监控仪表板设计与最佳实践

### 集群健康
- 13 - 集群健康检查指南

### 生产级监控
- 15 - 大规模集群监控最佳实践
- 16 - 多集群统一监控治理
- 17 - 监控成本优化与治理
- 19 - 监控安全与合规治理
- 20 - 监控平台高可用与灾备
- 21 - 监控运维手册与应急响应
- 22 - 可观测性平台最佳实践与案例
- 23 - 企业可观测性实施路线图
- 24 - 可观测性工具生态系统

### 混沌工程
- 14 - 混沌工程实践

### 故障排查工具
- 25 - Kubernetes 生产环境故障排查全攻略
- 26 - 故障排查增强工具
- 27 - 性能分析与调优工具

### Java 与 Kubernetes
- 99 - Java 应用 Kubernetes 可观测性整合指南
- 99 - Kubernetes v1.29-v1.33 可观测性新特性指南

## 关联文档 (K8s集成)

### 控制平面
- 控制平面监控与可观测性

### 工作负载
- 06 - 工作负载监控与告警体系

### 网络监控
- 25 - Ingress 监控与故障排查

### 存储监控
- 12 - 存储监控告警与性能调优

### 平台运维
- 05 - 运维指标体系建设
- 06 - 监控告警体系
- 18 - 平台可观测性深度实践

### AI 基础设施
- 13 - AI平台可观测性体系
- 25 - LLM可观测性与监控体系
- 36 - AI平台增强可观测性

### 故障排查域
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/04-monitoring-alerting-troubleshooting|30 - 监控告警故障排查]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/04-enterprise-monitoring-alerting-system|39 - 企业级监控告警体系]]

### 结构化故障排查
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/01-monitoring-observability-troubleshooting|可观测性故障排查指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|OpenTelemetry Collector 故障排查指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/03-ebpf-observability-troubleshooting|eBPF 可观测性故障排查指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 扩展参考

### 企业级监控方案
- Prometheus企业级监控系统深度实践
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-06-observability/07-tools/01-grafana-enterprise-observability|02 grafana enterprise observability]]
- OpenTelemetry分布式追踪与可观测性深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-06-observability/07-tools/04-zabbix-enterprise-monitoring|07 zabbix enterprise monitoring]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-06-observability/07-tools/05-new-relic-enterprise-apm|08 new relic enterprise apm]]

### 日志管理与分析
- ELK Stack企业级日志管理系统深度实践
- Fluentd企业级日志收集与处理深度实践
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-06-observability/03-logging/04-loki-enterprise-log-aggregation|03 loki enterprise log aggregation]]
- 企业级日志治理与合规审计深度实践
- Graylog 企业级日志管理平台深度实践
- Splunk企业级日志分析与安全智能平台深度实践
- Elastic Stack企业级可观测性平台深度实践

### 分布式追踪指南
- K8s 分布式追踪实践指南 (Jaeger / Tempo / OpenTelemetry)
- Prometheus 企业级监控部署指南

### 技能卡片
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-skills/13-monitoring-alerting-failure|监控告警体系故障诊断与修复]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-skills/14-logging-pipeline-failure|日志收集与管理故障诊断与修复]]
- [[domain-10-troubleshooting-diagnostics/技能体系/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复]]

### 术语词典
- [[domain-17-system-foundation/知识字典/observability/alerting-and-slo-monitoring.md|告警与 SLO 监控工程]]
- [[domain-17-system-foundation/知识字典/observability/llm-observability.md|LLM 可观测性]]
- [[domain-17-system-foundation/知识字典/observability/log-aggregation-with-loki.md|日志聚合与 Loki]]
- [[domain-17-system-foundation/知识字典/observability/logging-architecture.md|日志架构]]
- [[domain-17-system-foundation/知识字典/observability/metrics-for-kubernetes-object-states.md|Kubernetes 对象状态指标]]
- [[domain-17-system-foundation/知识字典/observability/metrics-for-kubernetes-system-components.md|Kubernetes 系统组件指标]]
- [[domain-17-system-foundation/知识字典/observability/observability.md|可观测性]]
- [[domain-17-system-foundation/知识字典/observability/opentelemetry-and-distributed-tracing.md|OpenTelemetry 与分布式链路追踪]]
- [[domain-17-system-foundation/知识字典/observability/system-logs.md|系统日志]]
- [[domain-17-system-foundation/知识字典/observability/traces-for-kubernetes-system-components.md|Kubernetes 系统组件链路追踪]]

### K8s 事件
- 01 - Kubernetes 事件系统架构与 API 参考
- 02 - Pod 与容器生命周期事件
- 03 - 镜像拉取事件
- 04 - 探针与健康检查事件
- 05 - 调度与抢占事件
- 06 - 节点生命周期与状态事件
- 07 - Deployment 与 ReplicaSet 控制器事件
- 08 - StatefulSet 与 DaemonSet 控制器事件
- 09 - Job 与 CronJob 批处理事件
- 10 - Service 与网络事件
- 11 - 存储与卷事件
- 12 - 自动扩缩容事件
- 13 - 安全、准入控制与 RBAC 事件
- 14 - Namespace、资源管理与垃圾回收事件
- 15 - 生态系统与插件事件

### 速查表
- [[domain-17-system-foundation/速查卡/promql.md|PromQL 速查表]]
- [[domain-17-system-foundation/速查卡/k8s.md|Kubernetes 生产环境速查卡]]
- [[domain-17-system-foundation/速查卡/networking.md|网络诊断速查表]]
- [[domain-17-system-foundation/速查卡/tls-pki.md|TLS/SSL 与 PKI 速查表]]

### CNCF 生态
- Prometheus
- Grafana
- Thanos
- OpenTelemetry
- Jaeger
- Fluentd
-  Loki
- Argo
- KubeEdge

### 演示文稿
- Kubernetes 可观测性全栈培训


<!-- risk-assessed -->
