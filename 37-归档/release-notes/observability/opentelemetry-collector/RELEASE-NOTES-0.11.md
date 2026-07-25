---
title: opentelemetry-collector v0.11 Release Notes
description: opentelemetry-collector v0.11 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- docker
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
- opentelemetry-collector v0.11 Release Notes 是什么
- 如何 opentelemetry-collector v0.11 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.11 Release Notes

Source: [v0.11.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.11.0)

## v0.11.0 Beta

## 🛑 Breaking changes 🛑

- Rename [[Service|service]].Start() to Run() since it's a blocking call
- Fix slice Append to accept by value the element in pdata
- Change CreateTraceProcessor and CreateMetricsProcessor to use the same parameter order as receivers/logs processor and exporters.
- Prevent accidental use of LogsToOtlp and LogsFromOtlp and the OTLP data structs (#1703)
- Remove SetType from configmodels, ensure all registered factories set the type in config (#1798)
- Move process telemetry to service/internal (#1794)

## 💡 Enhancements 💡

- Add map and array attribute value type support (#1656)
- Add authentication support to kafka (#1632)
- Implement InstrumentationLibrary translation to [[Jaeger|jaeger]] (#1645)
- Add public functions to export pdata to ExportXServicesRequest Protobuf bytes (#1741)
- Expose telemetry level in the configtelemetry (#1796)
- Add configauth package (#1807)
- Add config to docker image (#1792)

## 🧰 Bug fixes 🧰

- Use zap int argument for int values instead of conversion (#1779)
- Add support for gzip encoded payload in OTLP/HTTP receiver (#1581)
- Return proto status for OTLP receiver when failed (#1788)

<!-- risk-assessed -->
