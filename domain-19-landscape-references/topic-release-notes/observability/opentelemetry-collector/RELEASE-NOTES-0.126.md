---
title: opentelemetry-collector v0.126 Release Notes
description: opentelemetry-collector v0.126 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.126 Release Notes 是什么
- 如何 opentelemetry-collector v0.126 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.126
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.126 Release Notes

Source: [v0.126.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.126.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.126.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `configauth`: Removes deprecated `configauth.Authentication` and `extensionauthtest.NewErrorClient` (#12992)
  The following have been removed:
  - `configauth.Authentication` use `configauth.Config` instead
  - `extensionauthtest.NewErrorClient` use `extensionauthtest.NewErr` instead
  

### 💡 Enhancements 💡

- `service`: Replace `go.opentelemetry.io/collector/semconv` usage with `go.opentelemetry.io/otel/semconv` (#12991)
- `confmap`: Update the behavior of the confmap.enableMergeAppendOption feature gate to merge only component lists. (#12926)
- `service`: Add item count metrics defined in Pipeline Component Telemetry RFC (#12812)
  See [Pipeline Component Telemetry RFC](https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/component-universal-telemetry.md) for more details:
    - `otelcol.receiver.produced.items`
    - `otelcol.processor.consumed.items`
    - `otelcol.processor.produced.items`
    - `otelcol.connector.consumed.items`
    - `otelcol.connector.produced.items`
    - `otelcol.exporter.consumed.items`
  
- `tls`: Add trusted platform module (TPM) support to TLS authentication. (#12801)
  Now the TLS allows the use of TPM for loading private keys (e.g. in TSS2 format).
  

### 🧰 Bug fixes 🧰

- `exporterhelper`: Add validation error for batch config if min_size is greater than queue_size. (#12948)
- `telemetry`: Allocate less memory per component when OTLP exporting of logs is disabled (#13014)
- `confmap`: Use reflect.DeepEqual to avoid panic when confmap.enableMergeAppendOption feature gate is enabled. (#12932)
- `internal telemetry`: Add resource attributes from telemetry.resource to the logger (#12582)
  Resource attributes from telemetry.resource were not added to the internal
  console logs.
  
  Now, they are added to the logger as part of the "resource" field.
  
- `confighttp and configcompression`: Fix handling of `snappy` content-encoding in a backwards-compatible way (#10584, #12825)
  The collector used the Snappy compression type of "framed" to handle the HTTP
  content-encoding "snappy".  However, this encoding is typically used to indicate
  the "block" compression variant of "snappy".  This change allows the collector to:
  - When receiving a request with encoding 'snappy', the server endpoints will peek
    at the first bytes of the payload to determine if it is "framed" or "block" snappy,
    and will decompress accordingly.  This is a backwards-compatible change.
  
  If the feature-gate "confighttp.framedSnappy" is enabled, you'll see new behavior for both client and server:
  - Client compression type "snappy" will now compress to the "block" variant of snappy
    instead of "framed". Client compression type "x-snappy-framed" will now compress to the "framed" variant of snappy.
  - Servers will accept both "snappy" and "x-snappy-framed" as valid content-encodings.
  
- `tlsconfig`: Disable TPM tests on MacOS/Darwin (#12964)

<!-- previous-version -->

## API Changelog

### 🚩 Deprecations 🚩

- `configauth`: Deprecate `configauth.Authentication` in favor of `configauth.Config`. (#12875)

### 💡 Enhancements 💡

- `cmd/mdatagen`: Add type definition for events in mdatagen (#12571)
- `cmd/mdatagen`: Add functions for processing structured events in mdatagen (#12571)

<!-- previous-version -->
