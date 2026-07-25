---
title: opentelemetry-collector v0.97 Release Notes
description: opentelemetry-collector v0.97 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.97 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.97 Release Notes 是什么
- 如何 opentelemetry-collector v0.97 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.97
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.97 Release Notes

Source: [v0.97.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.97.0)

## End User Changelog

### 🛑 Breaking changes 🛑

- `telemetry`: Remove telemetry.useOtelForInternalMetrics stable feature gate (#9752)

### 🚀 New components 🚀

- `exporter/nop`: Add the `nopexporter` to serve as a placeholder exporter in a pipeline (#7316)
  This is primarily useful for starting the Collector with only extensions enabled
  or to test Collector pipeline throughput.
  
- `receiver/nop`: Add the `nopreceiver` to serve as a placeholder receiver in a pipeline (#7316)
  This is primarily useful for starting the Collector with only extensions enabled.

### 💡 Enhancements 💡

- `configtls`: Validates TLS min_version and max_version (#9475)
  Introduces `Validate()` method in TLSSetting.
- `configcompression`: Mark module as Stable. (#9571)
- `cmd/mdatagen`: Use go package name for the scope name by default and add an option to provide the scope name in metadata.yaml. (#9693)
- `cmd/mdatagen`: Generate the lifecycle tests for components by default. (#9683)
  It's encouraged to have lifecycle tests for all components enadled, but they can be disabled if needed 
  in metadata.yaml with `skip_lifecycle: true` and `skip_shutdown: true` under `tests` section.
  
- `cmd/mdatagen`: optimize the mdatagen for the case like batchprocessor which use a common struct to implement consumer.Traces, consumer.Metrics, consumer.Logs in the meantime. (#9688)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix persistent queue size backup on reads. (#9740)
- `processor/batch`: Prevent starting unnecessary goroutines. (#9739)
- `otlphttpexporter`: prevent error on empty response body when content type is application/json (#9666)
- `confmap`: confmap honors `Unmarshal` methods on config embedded structs. (#6671)
- `otelcol`: Respect telemetry configuration when running as a Windows [[Service|service]] (#5300)

## Go API Changelog

### 🛑 Breaking changes 🛑

- `configgrpc`: Remove deprecated `ToServer` function. (#9787)
- `confignet`: Change `Transport` field from `string` to `TransportType` (#9385)
- `component`: Change underlying type of `component.Type` to an opaque struct. (#9208)
- `obsreport`: Remove deprecated obsreport/obsreporttest package. (#9724)
- `component`: Remove deprecated error `ErrNilNextConsumer` (#9322)
- `connector`: Remove `LogsRouter`, `MetricsRouter` and `TracesRouter`. Use `LogsRouterAndConsumer`, `MetricsRouterAndConsumer`, `TracesRouterAndConsumer` respectively instead. (#9095)
- `receiver`: Remove deprecated struct `ScraperControllerSettings` and function `NewDefaultScraperControllerSettings` (#6767)
- `confmap`: Remove deprecated `provider.New` methods, use `NewWithSettings` moving forward. (#9443)

### 🚩 Deprecations 🚩

- `configgrpc`: Deprecated `ToServerContext`, use `ToServer` instead. (#9787)
- `configgrpc`: Deprecate `SanitizedEndpoint` (#9788)

### 💡 Enhancements 💡

- `exporterhelper`: Add experimental batching capabilities to the exporter helper (#8122)
- `confignet`: Adds `NewDefault*` functions for all the config structs. (#9656)
- `configtls`: Validates TLS min_version and max_version (#9475)
  Introduces `Validate()` method in TLSSetting.
- `exporterhelper`: Invalid exporterhelper options now make the exporter creation error out instead of panicking. (#9717)
- `components`: Give NoOp components a unique name (#9637)



### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.97.0

<!-- risk-assessed -->
