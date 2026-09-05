---
title: Observability
description: 可观测性知识域 — 指标、日志、链路追踪、告警、SLO/SLI、eBPF 可观测、OpenTelemetry 全链路
summary: 可观测性知识域入口，涵盖 Prometheus/Grafana 指标、Loki/Fluentd 日志、Jaeger/Tempo 链路、Alertmanager 告警、OpenTelemetry、eBPF 可观测
category: domain
tags:
- observability
- monitoring
- logging
- tracing
- alerting
- opentelemetry
- ebpf
tier: core
created: '2026-05-23'
last_updated: '2026-08-25'
difficulty: intermediate
audience:
- 所有工程师
- SRE
estimated_read_time: 10min
---
# 可观测性 Observability

> 指标、日志、链路追踪、告警、SLO/SLI 与工具集。生产环境故障排查的第一入口。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[09-可观测性/00-总览/index.md\|总览/]] | 架构总览 | 三大支柱/可观测性成熟度/架构设计 |
| [[09-可观测性/01-指标/index.md\|指标/]] | 指标监控 | Prometheus/Grafana/PromQL/自定义指标 |
| [[09-可观测性/02-日志/index.md\|日志/]] | 日志管理 | Loki/Fluentd/日志规范/结构化日志 |
| [[09-可观测性/03-链路追踪/index.md\|链路追踪/]] | 分布式追踪 | Jaeger/Tempo/OpenTelemetry/采样策略 |
| [[09-可观测性/04-告警/index.md\|告警/]] | 告警体系 | Alertmanager/告警规则/告警收敛/On-Call |
| [[09-可观测性/05-SLO-SLI/index.md\|SLO-SLI/]] | SLO 框架 | SLO/SLI/Error Budget/多窗口告警 |
| [[09-可观测性/06-工具/index.md\|工具/]] | 工具集 | eBPF/Pixie/OpenTelemetry Collector |

## 跨域导航

- [[15-AI基础设施/README.md|AI基础设施]]
- [[16-专项技术/README.md|专项技术]]
- [[18-云厂商/README.md|云厂商]]
- [[11-发布变更/README.md|发布变更]]
- [[12-可靠性/README.md|可靠性]]
- [[06-存储/README.md|存储]]
- [[08-安全/README.md|安全]]
- [[14-容器运行时/README.md|容器运行时]]
- [[02-工作负载/README.md|工作负载]]
- [[10-平台工程/README.md|平台工程]]
- [[04-应用模式/README.md|应用模式]]
- [[19-故障诊断/README.md|故障诊断]]
- [[07-数据库中间件/README.md|数据库中间件]]
- [[03-清单模式/README.md|清单模式]]
- [[13-生产运维/README.md|生产运维]]
- [[21-生态参考/README.md|生态参考]]
- [[17-系统基础/README.md|系统基础]]
- [[05-网络/README.md|网络]]
- [[01-集群基础/README.md|集群基础]]

## Related

- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]
