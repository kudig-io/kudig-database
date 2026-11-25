---
title: opentelemetry-collector v0.110 Release Notes
description: opentelemetry-collector v0.110 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.110 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.110 Release Notes 是什么
- 如何 opentelemetry-collector v0.110 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.110
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.110 Release Notes

Source: [v0.110.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.110.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.110.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `processorhelper`: Update incoming/outgoing metrics to a single metric with a `otel.signal` attributes. (#11144)
  The following metrics were added in the previous version
  - otelcol_processor_incoming_spans
  - otelcol_processor_outgoing_spans
  - otelcol_processor_incoming_metric_points
  - otelcol_processor_outgoing_metric_points
  - otelcol_processor_incoming_log_records
  - otelcol_processor_outgoing_log_records
  
  They are being replaced with the following to more closely align with OTEP 259:
  - otelcol_processor_incoming_items
  - otelcol_processor_outgoing_items
  
- `processorhelper`: Remove deprecated `[Traces|Metrics|Logs]`Inserted funcs (#11151)
- `config`: Mark UseLocalHostAsDefaultHostfeatureGate as stable (#11235)

### 🚩 Deprecations 🚩

- `processorhelper`: deprecate accepted/refused/dropped metrics (#11201)
  The following metrics are being deprecated as they were only used in a single
  processor:
    - `otelcol_processor_accepted_log_records`
    - `otelcol_processor_accepted_metric_points`
    - `otelcol_processor_accepted_spans`
    - `otelcol_processor_dropped_log_records`
    - `otelcol_processor_dropped_metric_points`
    - `otelcol_processor_dropped_spans`
    - `otelcol_processor_refused_log_records`
    - `otelcol_processor_refused_metric_points`
    - `otelcol_processor_refused_spans`
  

### 💡 Enhancements 💡

- `pdata`: Add support to MoveTo for Map, allow avoiding copies (#11175)
- `mdatagen`: Add stability field to telemetry metrics, allowing the generated description to include a stability string. (#11160)
- `confignet`: Mark module as Stable. (#9801)
- `confmap/provider/envprovider`: Support default values when env var is empty (#5228)
- `mdatagen`: mdatagen `validateMetrics()` support validate metrics in `telemetry.metric` (#10925)
- `[[Service|service]]/telemetry`: Mark useOtelWithSDKConfigurationForInternalTelemetry as stable (#7532)
- `mdatagen`: Use cobra for the command, add version flag (#11196)

### 🧰 Bug fixes 🧰

- `service`: Ensure process telemetry is registered when internal telemetry is configured with readers instead of an address. (#11093)
- `mdatagen`: Fix incorrect generation of metric tests with boolean attributes. (#11169)
- `otelcol`: Fix the Windows Event Log configuration when running the Collector as a Windows service. (#5297, #11051)
- `builder`: Honor build_tags in config (#11156)
- `builder`: Fix version for providers in the default config (#11123)
- `cmd/builder`: Temporarily disable strict versioning checks (#11129, #11152)
  The strict versioning check may be enabled by default in a future version once all configuration providers are stabilized.
  
- `confmap`: Fix loading config of a component from a different source. (#11154)
  This issue only affected loading the whole component config, loading parts of a component config from a different source was working correctly.
  

## API Changes

### 🛑 Breaking changes 🛑

- `otlpexporter`: The `TimeoutSettings` field in `otlpexporter.Config` was renamed to `TimeoutConfig`. (#11132)
- `connector`: Change `TracesRouterAndConsumer`, `NewTracesRouter`, `MetricsRouterAndConsumer`, `NewMetricsRouter`, `LogsRouterAndConsumer`, and `NewLogsRouter` to use `pipeline.ID` instead of `component.ID`. (#11204)
- `extension`: Remove deprecated extension interfaces. (#11043)
  They are now available in the `extensioncapabilities` module.
  

### 🚩 Deprecations 🚩

- `exporterhelper`: Deprecate TimeoutSettings/QueueSettings in favor of TimeoutConfig/QueueConfig. (#6767)
- `configgrpc`: Deprecate `ClientConfig.ToClientConn`/`ServerConfig.ToServer` in favor of `ToClientConnWithOptions`/`ToServerWithOptions` (#9480)
  Users providing a grpc.DialOption/grpc.ServerOption should now wrap them into
  a generic option with `WithGrpcDialOption`/`WithGrpcServerOption`.
  
- `componentprofiles`: Deprecates `DataTypeProfiles`. Use `SignalProfiles` instead. (#11204)
- `componentstatus`: Deprecates `NewInstanceID`, `AllPipelineIDs`, and `WithPipelines`. Use `NewInstanceIDWithPipelineIDs`, `AllPipelineIDsWithPipelineIDs`, and `WithPipelineIDs` instead. (#11204)
- `exporterqueue`: Deprecates `Settings.DataType`. Use `Settings.Signal` instead. (#11204)
- `service`: Deprecates `pipelines.Config`. Use `pipelines.ConfigWithPipelineID` instead. (#11204)
- `component`: Deprecates `DataType`, `DataTypeTraces`, `DataTypeMetrics`, and `DataTypeLogs`. Use `pipeline.Signal`, `SignalTraces`, `SignalMetrics`, and `SignalLogs` instead. (#11204)
- `service`: Deprecates service's implementation of `GetExporters` interface.  Use `GetExportersWithSignal` instead. (#11249)
- `scraperhelper`: Deprecate NewScraperWithComponentType, should use NewScraper (#11159)

### 🚀 New components 🚀

- `pipeline`: Adds new `pipeline` module to house the concept of pipeline ID and Signal. (#11209)

### 💡 Enhancements 💡

- `pdata`: Add support to MoveTo for Map, allow avoiding copies (#11175)
- `options`: Avoid using private types in public APIs and also protect options to be implemented outside this module. (#11054)
- `mdatagen`: Avoid using private types in public APIs and also protect options to be implemented outside this module. (#11040)
- `consumertest`: Introduce SampleCount method in ProfilesSink struct. (#11225)
- `otlpreceiver`: Support profiles in the OTLP receiver (#11071)




<!-- risk-assessed -->
