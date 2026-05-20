---
title: opentelemetry-collector v0.138 Release Notes
description: opentelemetry-collector v0.138 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.138 Release Notes 是什么
- 如何 opentelemetry-collector v0.138 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.138
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.138 Release Notes

Source: [v0.138.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.138.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.138.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `all`: Remove deprecated type `TracesConfig` (#14036)
- `pkg/exporterhelper`: Add default values for `sending_queue::batch` configuration. (#13766)
  Setting `sending_queue::batch` to an empty value now results in the same setup as the default batch processor configuration.
  
- `all`: Add unified print-config command with mode support (redacted, unredacted), json support (unstable), and validation support. (#11775)
  This replaces the `print-initial-config` command. See the `service` package README for more details. The original command name `print-initial-config` remains an alias, to be retired with the feature flag.

### 💡 Enhancements 💡

- `all`: Add `keep_alives_enabled` option to ServerConfig to control HTTP keep-alives for all components that create an HTTP server. (#13783)
- `pkg/otelcol`: Avoid unnecessary mutex in collector logs, replace by atomic pointer (#14008)
- `cmd/mdatagen`: Add lint/ordering validation for metadata.yaml (#13781)
- `pdata/xpdata`: Refactor JSON marshaling and unmarshaling to use `pcommon.Value` instead of `AnyValue`. (#13837)
- `pkg/exporterhelper`: Expose `MergeCtx` in exporterhelper's queue batch settings` (#13742)

### 🧰 Bug fixes 🧰

- `all`: Fix zstd decoder data corruption due to decoder pooling for all components that create an HTTP server. (#13954)
- `pkg/otelcol`: Remove UB when taking internal logs and move them to the final zapcore.Core (#14009)
  This can happen because of a race on accessing `logsTaken`.
- `pkg/confmap`: Fix a potential race condition in confmap by closing the providers first. (#14018)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pkg/xexporterhelper`: Remove definition of Sizer from public API and ability to configure. (#14001)
  Now that Request has both Items/Bytes sizes no need to allow custom sizers.
  
- `pkg/service`: The `service.Settings` type now requires a `telemetry.Factory` to be provided (#4970)

### 🚩 Deprecations 🚩

- `pdata/pprofile`: Deprecated `PutAttribute` helper method (#14016, #14041)
- `pdata/pprofile`: Deprecated `PutLocation` helper method (#14019)

### 💡 Enhancements 💡

- `all`: Add `keep_alives_enabled` option to ServerConfig to control HTTP keep-alives for all components that create an HTTP server. (#13783)
- `pkg/pdata`: Add pcommon.Map helper to add a key to the map if does not exists (#14023)
- `pdata/pprofile`: Introduce `Equal` method on the `KeyValueAndUnit` type (#14041)
- `pkg/pdata`: Add `RemoveIf` method to primitive slice types (StringSlice, Int64Slice, UInt64Slice, Float64Slice, Int32Slice, ByteSlice) (#14027)
- `pdata/pprofile`: Introduce `SetAttribute` helper method (#14016, #14041)
- `pdata/pprofile`: Introduce `SetLocation` helper method (#14019)
- `pdata/pprofile`: Introduce `Equal` method on the `Stack` type (#13952)

<!-- previous-version -->
