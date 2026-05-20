---
title: opentelemetry-collector v0.125 Release Notes
description: opentelemetry-collector v0.125 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.125 Release Notes 是什么
- 如何 opentelemetry-collector v0.125 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.125
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.125 Release Notes

Source: [v0.125.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.125.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.125.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `service`: Lowercase values for 'otelcol.component.kind' attributes. (#12865)
- `service`: Restrict the `telemetry.newPipelineTelemetry` feature gate to metrics. (#12856, #12933)
  The "off" state of this feature gate introduced a regression, where the Collector's internal logs were missing component attributes. See issue #12870 for more details on this bug.
  
  On the other hand, the "on" state introduced an issue with the Collector's default internal metrics, because the Prometheus exporter does not currently support instrumentation scope attributes.
  
  To solve both of these issues, this change turns on the new scope attributes for logs and traces by default regardless of the feature gate.
  However, the new scope attributes for metrics stay locked behind the feature gate, and will remain off by default until the Prometheus exporter is updated to support scope attributes.
  
  Please understand that enabling the `telemetry.newPipelineTelemetry` feature gate may break the export of Collector metrics through, depending on your configuration.
  Having a `batch` processor in multiple pipelines is a known trigger for this.
  
  This comes with a breaking change, where internal logs exported through OTLP will now use instrumentation scope attributes to identify the source component instead of log attributes.
  This does not affect the Collector's stderr output. See the changelog for v0.123.0 for a more detailed description of the gate's effects.
  

### 💡 Enhancements 💡

- `mdatagen`: Add support for attributes for telemetry configuration in metadata. (#12919)
- `configmiddleware`: Add extensionmiddleware interface. (#12603, #9591)
- `configgrpc`: Add gRPC middleware support. (#12603, #9591)
- `confighttp`: Add HTTP middleware support. (#12603, #9591, #7441)
- `configmiddleware`: Add configmiddleware struct. (#12603, #9591)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Do not ignore the `num_consumers` setting when batching is enabled. (#12244)
- `exporterhelper`: Reject elements larger than the queue capacity (#12847)
- `mdatagen`: Add time and plog package imports (#12907)
- `confmap`: Maintain nil values when marshaling or unmarshaling nil slices (#11882)
  Previously, nil slices were converted to empty lists, which are semantically different
  than a nil slice. This change makes this conversion more consistent when encoding
  or decoding config, and these values are now maintained.
  

<!-- previous-version -->

## API Changelog

### 🚩 Deprecations 🚩

- `extensionauthtest`: Deprecate NewErrorClient in favor of NewErrClient. (#12874)

### 💡 Enhancements 💡

- `xextension/storage`: ErrStorageFull error added to xextension/storage contract (#12925)
- `pdata`: Add MoveTo to pcommon.Value, only type missing this (#12877)

### 🧰 Bug fixes 🧰

- `pdata`: Fix MoveTo when moving to the same destination (#12887)

<!-- previous-version -->
