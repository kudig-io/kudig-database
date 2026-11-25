---
title: opentelemetry-collector v0.10 Release Notes
description: opentelemetry-collector v0.10 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- opa
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.10 Release Notes 是什么
- 如何 opentelemetry-collector v0.10 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- policy-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.10 Release Notes

Source: [v0.10.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.10.0)

# v0.10.0 Beta

# 🛑 Breaking changes 🛑

- **Update OTLP to v0.5.0, incompatible metrics protocol.**
- Remove support for propagating summary metrics in OtelCollector.
  - This is a temporary change, and will affect mostly OpenCensus users who use metrics.

# 💡 Enhancements 💡
- Support zipkin proto in `kafka` receiver (#1646)
- [[Prometheus|Prometheus]] Remote Write Exporter supporting [[Cortex|Cortex]] (#1577, #1643)
- Add deployment environment semantic convention (#1722)
- Add logs support to `batch` and `resource` processors (#1723, #1729)

# 🧰 Bug fixes 🧰
- Identify config error when expected map is other value type (#1641)
- Fix Kafka receiver closing ready channel multiple times (#1696)
- Fix a panic issue while processing Zipkin spans with an empty [[Service|service]] name (#1742)
- Zipkin Receiver: Always set the endtime (#1750)

<!-- risk-assessed -->
