---
title: Observability Tools
description: 可观测性工具知识域 — Grafana/Datadog/New Relic/Zabbix 企业级部署、性能 Profiling、eBPF 可观测
category: subdomain
tags:
- grafana
- datadog
- new-relic
- profiling
- ebpf
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 可观测性工具 Observability Tools

> 企业级可观测性平台选型、部署与运维实践。

## 平台对比矩阵

| 平台 | 部署模式 | 核心能力 | 成本模型 |
|------|----------|----------|----------|
| Grafana Stack | 自托管/Cloud | Metrics+Traces+Logs+Profiles | 开源免费/Cloud按量 |
| Datadog | SaaS | APM+Infra+Logs+RUM | 按主机/GB计费 |
| New Relic | SaaS | Full-Stack APM | 按数据摄入量 |
| Zabbix | 自托管 | 基础设施监控 | 开源免费 |
| Dynatrace | SaaS/Managed | AI 驱动 APM | 按主机单元 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[可观测性/工具/02-grafana-enterprise-observability.md\|Grafana 企业可观测]] | Grafana 全栈部署与配置 | advanced |
| [[可观测性/工具/05-datadog-enterprise-apm.md\|Datadog APM]] | Datadog APM 企业实践 | advanced |
| [[可观测性/工具/05-datadog-enterprise-monitoring.md\|Datadog 监控]] | Datadog 基础设施监控 | intermediate |
| [[可观测性/工具/07-zabbix-enterprise-monitoring.md\|Zabbix 监控]] | Zabbix 企业级部署 | intermediate |
| [[可观测性/工具/08-new-relic-enterprise-apm.md\|New Relic APM]] | New Relic 全栈可观测 | intermediate |
| [[可观测性/工具/26-troubleshooting-tools.md\|排障工具]] | kubectl/istioctl/tcpdump 等 | intermediate |
| [[可观测性/工具/27-performance-profiling-tools.md\|性能 Profiling]] | pprof/async-profiler/Pyroscope | advanced |
| [[可观测性/工具/30-ebpf-observability.md\|eBPF 可观测]] | Cilium Hubble/Pixie/bpftrace | advanced |

## 选型决策指南

- **初创/小团队** → Grafana Stack（开源、灵活、低成本）
- **中大型企业** → Datadog/Dynatrace（开箱即用、AI 分析）
- **合规/离线** → Zabbix + Prometheus（自托管、可控）
- **云原生深度** → Grafana + eBPF（零侵入、内核级可见性）

## Related

- [[可观测性/指标/index.md|指标 Metrics]]
- [[可观测性/链路追踪/index.md|链路追踪 Tracing]]
- [[可观测性/SLO-SLI/index.md|SLO & SLI]]
