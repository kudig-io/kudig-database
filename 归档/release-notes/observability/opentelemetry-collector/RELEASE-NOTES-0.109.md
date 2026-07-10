---
title: opentelemetry-collector v0.109 Release Notes
description: opentelemetry-collector v0.109 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.109 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.109 Release Notes 是什么
- 如何 opentelemetry-collector v0.109 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.109
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




# opentelemetry-collector v0.109 Release Notes

Source: [v0.109.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.109.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.109.0

## End User Changelog

### 🐛 Known bugs 🐛

- The `ocb` binary has an identified bug caused by the fact that some of the providers have been marked stable and the default providers in the `ocb` binary still use the unstable version. In order to fix this explicitly add the default providers in your otel builder config, if not already configured:

```
providers:
  - gomod: go.opentelemetry.io/collector/confmap/provider/envprovider v1.15.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/fileprovider v1.15.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/httpprovider v0.109.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/httpsprovider v0.109.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/yamlprovider v0.109.0
``` 

Alternatively you can pass the `--skip-strict-version-check` flag.

### 🛑 Breaking changes 🛑

- `scraperhelper`: Remove deprecated `ObsReport`, `ObsReportSettings`, `NewObsReport` types/funcs (#11086)
- `confmap`: Remove stable `confmap.strictlyTypedInput` gate (#11008)
- `confmap`: Removes stable `confmap.unifyEnvVarExpansion` feature gate. (#11007)
- `ballastextension`: Removes the deprecated ballastextension (#10671)
- `[[Service|service]]`: Removes stable `service.disableOpenCensusBridge` feature gate (#11009)

### 🚩 Deprecations 🚩

- `processorhelper`: These funcs are not used anywhere, marking them deprecated. (#11083)

### 🚀 New components 🚀

- `extension/experimental/storage`: Move `extension/experimental/storage` into a separate module (#11022)

### 💡 Enhancements 💡

- `configtelemetry`: Add guidelines for each level of component telemetry (#10286)
- `service`: move `useOtelWithSDKConfigurationForInternalTelemetry` gate to beta (#11091)
- `service`: implement a no-op tracer provider that doesn't propagate the context (#11026)
  The no-op tracer provider supported by the SDK incurs a memory cost of propagating the context no matter
  what. This is not needed if tracing is not enabled in the Collector. This implementation of the no-op tracer
  provider removes the need to allocate memory when tracing is disabled.
  
- `envprovider`: Mark module as stable (#10982)
- `fileprovider`: Mark module as stable (#10983)
- `processor`: Add incoming and outgoing counts for processors using processorhelper. (#10910)
  Any processor using the processorhelper package (this is most processors) will automatically report
  incoming and outgoing item counts. The new metrics are:
  - otelcol_processor_incoming_spans
  - otelcol_processor_outgoing_spans
  - otelcol_processor_incoming_metric_points
  - otelcol_processor_outgoing_metric_points
  - otelcol_processor_incoming_log_records
  - otelcol_processor_outgoing_log_records
  

### 🧰 Bug fixes 🧰

- `configgrpc`: Change the value of max_recv_msg_size_mib from uint64 to int to avoid a case where misconfiguration caused an integer overflow. (#10948)
- `exporterqueue`: Fix a bug in persistent queue that Offer can becomes deadlocked when queue is almost full (#11015)


## API Changes


### 🛑 Breaking changes 🛑

- `Remove `extensiontest` StatusWatcher helpers`: They were unused. They may be added back on a different module or after `componentstatus` is marked 1.0
 (#11044)
- `pprofile`: Change Profile ID field from a byte array to a custom data type (#11048)
- `connector`: Remove deprecated connector builder (#11019)
- `exporter`: Remove deprecated exporter builder (#11019)
- `extension`: Remove deprecated extension builder (#11019)
- `processor`: Remove deprecated processor builder (#11019)
- `receiver`: Remove deprecated receiver builder (#11019)

### 🚩 Deprecations 🚩

- `configtelemetry`: Deprecating `TelemetrySettings.MeterProvider` in favour of `TelemetrySettings.LeveledMeterProvider` (#10912)
- `extension`: Deprecate `extension.ConfigWatcher`, `extension.PipelineWatcher` and `extension.Dependent` in favor of equivalents in the `extensioncapabilities` module. (#11000)
- `scraperhelper`: deprecate NewScraper, should use NewScraperWithComponentType (#11082)

### 🚀 New components 🚀

- `extensioncapabilities`: Create a new module for optional extension capabilities. (#11000)

### 💡 Enhancements 💡

- `connectorprofiles`: Add ProfilesRouterAndConsumer interface, and NewProfilesRouter method. (#11023)
- `pprofileotlp`: Introduce grpc service implementation of pprofileotlp (#11048)
- `pprofile`: Introduce marshalling and unmarshalling of pprofile data (#11048)
- `service`: Support profiles in the service package (#11024)


<!-- risk-assessed -->
