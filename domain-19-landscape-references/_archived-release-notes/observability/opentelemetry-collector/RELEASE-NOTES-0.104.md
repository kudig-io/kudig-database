---
title: opentelemetry-collector v0.104 Release Notes
description: opentelemetry-collector v0.104 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.104 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.104 Release Notes 是什么
- 如何 opentelemetry-collector v0.104 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.104
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.104 Release Notes

Source: [v0.104.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.104.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.104.0


:warning: **This release includes 2 very important breaking changes.**
1. The `otlpreceiver` now uses `localhost` by default instead of `0.0.0.0`. This may break the receiver in containerized environments like [[Kubernetes|Kubernetes]]. If you depend on `0.0.0.0` disable the `component.UseLocalHostAsDefaultHost` feature gate or explicitly set the endpoint to `0.0.0.0`.
2. Expansion of BASH-style environment variables, such as `$FOO` is no longer supported by default. If you depend on this syntax, disable the `confmap.unifyEnvVarExpansion` feature gate, but know that the feature will be removed in the future in favor of `${env:FOO}`.


## End User Changelog

### 🛑 Breaking changes 🛑

- `filter`: Remove deprecated `filter.CombinedFilter` (#10348)
- `otelcol`: By default, `otelcol.NewCommand` and `otelcol.NewCommandMustSetProvider` will set the `DefaultScheme` to `env`. (#10435)
- `expandconverter`: By default expandconverter will now error if it is about to expand `$FOO` syntax. Update configuration to use `${env:FOO}` instead or disable the `confmap.unifyEnvVarExpansion` feature gate. (#10435)
- `otlpreceiver`: Switch to `localhost` as the default for all endpoints. (#8510)
  Disable the `component.UseLocalHostAsDefaultHost` feature gate to temporarily get the previous default.
  

### 💡 Enhancements 💡

- `confighttp`: Add support for cookies in HTTP clients with `cookies::enabled`. (#10175)
  The method `confighttp.ToClient` will return a client with a `cookiejar.Jar` which will reuse cookies from server responses in subsequent requests.
- `exporter/debug`: In `normal` verbosity, display one line of text for each telemetry record (log, data point, span) (#7806)
- `exporter/debug`: Add option `use_internal_logger` (#10226)
- `configretry`: Mark module as stable. (#10279)
- `debugexporter`: Print Span.TraceState() when present. (#10421)
  Enables viewing sampling threshold information (as by OTEP 235 samplers).
- `processorhelper`: Add "inserted" metrics for processors. (#10353)
  This includes the following metrics for processors:
  - `processor_inserted_spans`
  - `processor_inserted_metric_points`
  - `processor_inserted_log_records`
  

### 🧰 Bug fixes 🧰

- `otlpexporter`: Update validation to support both dns:// and dns:/// (#10449)
- `service`: Fixed a bug that caused otel-collector to fail to start with ipv6 metrics endpoint service telemetry. (#10011)

## Go API Changelog

### 🛑 Breaking changes 🛑

- `otelcol`: The `otelcol.NewCommand` now requires at least one provider be set. (#10436)
- `component/componenttest`: Added additional "inserted" count to `TestTelemetry.CheckProcessor*` methods. (#10353)

### 🚩 Deprecations 🚩

- `otelcoltest`: Deprecates `LoadConfigWithSettings` and `LoadConfigAndValidateWithSettings`.  Use `LoadConfig` and `LoadConfigAndValidate` instead. (#10417)
- `otelcol`: The `otelcol.NewCommandMustSetProvider` is deprecated. Use `otelcol.NewCommand` instead. (#10436)

### 🚀 New components 🚀

- `otelcoltest`: Split off go.opentelemetry.io/collector/otelcol/otelcoltest into its own module (#10417)

### 💡 Enhancements 💡

- `pdata/pprofile`: Add pprofile wrapper to convert proto into pprofile. (#10401)
- `pdata/testdata`: Add pdata testdata for profiles. (#10401)
