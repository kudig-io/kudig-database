---
title: opentelemetry-collector v0.8 Release Notes
description: opentelemetry-collector v0.8 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.8 Release Notes 是什么
- 如何 opentelemetry-collector v0.8 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.8 Release Notes

Source: [v0.8.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.8.0)

# v0.8.0 Beta

# 🚀 New components 🚀

- `groupbytrace` processor that waits for a trace to be completed (#1362)

# 💡 Enhancements 💡

- Migrate `zipkin` receiver/exporter to the new interfaces (#1484)
- Migrate `[[Prometheus|prometheus]]` receiver/exporter to the new interfaces (#1477, #1515)
- Add new FactoryUnmarshaler support to all components, deprecate old way (#1468)
- Update `fileexporter` to write data in OTLP (#1488)
- Add extension factory helper (#1485)
- Host scrapers: Use same scrape time for all data points coming from same source (#1473)
- Make logs SeverityNumber publicly available (#1496)
- Add recently included conventions for k8s and container resources (#1519)
- Add new config StartTimeMetricRegex to `prometheus` receiver (#1511)
- Convert Zipkin receiver and exporter to use OTLP (#1446)

# 🧰 Bug fixes 🧰

- Infer OpenCensus resource type based on [[OpenTelemetry|OpenTelemetry]]'s semantic conventions (#1462)
- Fix log adapter in `prometheus` receiver (#1493)
- Avoid frequent errors for process telemetry on Windows (#1487)


<!-- risk-assessed -->
