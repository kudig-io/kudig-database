---
title: opentelemetry-collector v0.123 Release Notes
description: opentelemetry-collector v0.123 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.123 Release Notes 是什么
- 如何 opentelemetry-collector v0.123 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.123
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.123 Release Notes

Source: [v0.123.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.123.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.123.0

## End User Changelog


### ❗ Known Issues ❗

- This version increases memory usage by ~0.5 MB per component in the pipelines because a separate Zap Core logger is 
  initialized for each component. The issue is partially fixed in v0.126.0 for users who write logs to stdout, but do
  not export logs via OTLP. See https://github.com/open-telemetry/opentelemetry-collector/issues/13014 for more details.
- `service/telemetry`: Missing attributes in internal Collector logs (#12870).
  - See issue description for workarounds.

### 🛑 Breaking changes 🛑

- `service/telemetry`: Mark `telemetry.disableAddressFieldForInternalTelemetry` as beta, usage of deprecated service::telemetry::address are ignored (#12756)
  To restore the previous behavior disable `telemetry.disableAddressFieldForInternalTelemetry` feature gate.
- `exporterbatch`: Remove deprecated fields `min_size_items` and `max_size_items` from batch config. (#12684)

### 🚩 Deprecations 🚩

- `otlpexporter`: Mark BatcherConfig as deprecated, use `sending_queue::batch` instead (#12726)
- `exporterhelper`: Deprecate `blocking` in favor of `block_on_overflow`. (#12710)
- `exporterhelper`: Deprecate configuring exporter batching separately. Use `sending_queue::batch` instead. (#12772)
  Moving the batching configuration to `sending_queue::batch` requires setting `sending_queue::sizer` to `items`
  which means that `sending_queue::queue_size` needs to be also increased by the average batch size number (roughly 
  x5000 for the default batching configuration).
  See https://github.com/open-telemetry/opentelemetry-collector/tree/main/exporter/exporterhelper#configuration
  

### 💡 Enhancements 💡

- `exporterhelper`: Add support to configure batching in the sending queue. (#12746)
- `exporterhelper`: Add support for wait_for_result, remove disabled_queue (#12742)
  This has a side effect for users of the experimental BatchConfig with the queue disabled, since not this is | uses only NumCPU() consumers.
- `exporterhelper`: Allow exporter memory queue to use different type of sizers. (#12708)
- `service`: Add "telemetry.newPipelineTelemetry" feature gate to inject component-identifying attributes in internal telemetry (#12217)
  With the feature gate enabled, all internal telemetry (metrics/traces/logs) will include some of
  the following instrumentation scope attributes:
  - `otelcol.component.kind`
  - `otelcol.component.id`
  - `otelcol.pipeline.id`
  - `otelcol.signal`
  - `otelcol.signal.output`
  
  These attributes are defined in the [Pipeline Component Telemetry RFC](https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/component-universal-telemetry.md#attributes),
  and identify the component instance from which the telemetry originates.
  They are added automatically without changes to component code.
  
  These attributes were already included in internal logs as regular log attributes, starting from
  v0.120.0. For consistency with other signals, they have been switched to scope attributes (with
  the exception of logs emitted to standard output), and are now enabled by the feature gate.
  
  Please make sure that the exporter / backend endpoint you use has support for instrumentation
  scope attributes before using this feature. If the internal telemetry is exported to another
  Collector, a transform processor could be used to turn them into other kinds of attributes if
  necessary.
  
- `exporterhelper`: Enable support to do batching using `bytes` sizer (#12751)
- `service`: Add config key to set metric views used for internal telemetry (#10769)
  The `service::telemetry::metrics::views` config key can now be used to explicitly set the list of
  metric views used for internal telemetry, mirroring `meter_provider::views` in the SDK config.
  This can be used to disable specific internal metrics, among other uses.
  
  This key will cause an error if used alongside other features which would normally implicitly create views, such as:
  - not setting `service::telemetry::metrics::level` to `detailed`;
  - enabling the `telemetry.disableHighCardinalityMetrics` feature flag.
  

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix order of starting between queue and batch. (#12705)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `otlpreceiver/otlpexporter/otlphttpexporter`: Avoid using go embedded messages in Config (#12718)
- `exporterqueue`: Move Queue interface to internal, disallow alternative implementations (#12680)
- `extensionauth, configauth`: Remove deprecated types and functions from `extensionauth` and `configauth` packages. (#12672)
  This includes:
  - `extensionauth.NewClient`,
  - `extensionauth.ClientOption` and all its implementations,
  - `extensionauth.NewServer`,
  - `extensionauth.ServerOption` and all its implementations and
  - `configauth.Authenticator.GetClientAuthenticator`.
  
- `exporterhelper`: Remove deprecated converter types from exporterhelper (#12686)
- `exporterbatch`: Remove deprecated fields `min_size_items` and `max_size_items` from batch config. (#12684)

### 🚩 Deprecations 🚩

- `exporterhelper`: Deprecate BatcherConfig, SizeConfig and WithBatcher in favor of the new QueueBatchConfig. (#12748)
- `exporterbatcher`: Deprecated Config, SizeConfig, SizerType, SizerType[Requests|Items|Bytes], NewDefaultConfig. Use alias from exporterhelper. (#12707)
- `exporterqueue`: Deprecated Config, NewDefaultConfig, Encoding, ErrQueueFull. Use alias from exporterhelper. (#12706)
- `exporterhelper`: Deprecate exporterhelper WithRequestQueue in favor of WithQueueBatch (#12679)
- `exporterhelper`: Deprecate `QueueConfig` in favor of `QueueBatchConfig`. (#12746)

### 💡 Enhancements 💡

- `extensionauth`: Mark module as stable (#11006)
- `processor`: Mark module as stable. (#12677)
- `processorhelper`: Split processorhelper into a separate module. (#12678)

<!-- previous-version -->
