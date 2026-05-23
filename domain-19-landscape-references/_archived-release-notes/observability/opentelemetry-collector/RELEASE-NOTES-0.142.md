---
title: opentelemetry-collector v0.142 Release Notes
description: opentelemetry-collector v0.142 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.142 Release Notes 是什么
- 如何 opentelemetry-collector v0.142 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.142
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.142 Release Notes

Source: [v0.142.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.142.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.142.0

## End User Changelog

### 💡 Enhancements 💡

- `exporter/debug`: Add logging of dropped attributes, events, and links counts in detailed verbosity (#14202)
- `extension/memory_limiter`: The memorylimiter extension can be used as an HTTP/GRPC middleware. (#14081)
- `pkg/config/configgrpc`: Statically validate [[gRPC|gRPC]] endpoint (#10451)
  This validation was already done in the OTLP exporter. It will now be applied to any gRPC client.
  
- `pkg/service`: Add support to disabling adding resource attributes as zap fields in internal logging (#13869)
  Note that this does not affect logs exported through OTLP.
  

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pdata/xpdata`: Rename `Entity.IDAttributes()` to `Entity.IdentifyingAttributes()` and `Entity.DescriptionAttributes()` to `Entity.DescriptiveAttributes()` to align with OpenTelemetry specification terminology for attributes. (#14275)
- `pkg/exporterhelper`: Use `configoptional.Optional` for the `exporterhelper.QueueBatchConfig` (#14155)
  It's recommended to change the field type in your component configuration to be `configoptional.Optional[exporterhelper.QueueBatchConfig]` to keep the `enabled` subfield. Use configoptional.Some(exporterhelper.NewDefaultQueueConfig()) to enable by default. Use configoptional.Default(exporterhelper.NewDefaultQueueConfig()) to disable by default.
  

### 🚩 Deprecations 🚩

- `pkg/service`: Deprecate Settings.LoggingOptions and telemetry.LoggerSettings.ZapOptions, add telemetry.LoggerSettings.BuildZapLogger (#14002)
  BuildZapLogger provides a more flexible way to build the Zap logger,
  since the function will have access to the zap.Config. This is used
  in otelcol to install a Windows Event Log output when the zap config
  does not specify any file output.
  

### 💡 Enhancements 💡

- `pdata/pprofile`: add ProfileCount() (#14239)

### 🧰 Bug fixes 🧰

- `pkg/confmap`: Ensure that embedded structs are not overwritten after Unmarshal is called (#14213)
  This allows embedding structs which implement Unmarshal and contain a configopaque.String.
  

<!-- previous-version -->
