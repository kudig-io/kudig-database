---
title: opentelemetry-collector v0.84 Release Notes
description: opentelemetry-collector v0.84 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.84 Release Notes 是什么
- 如何 opentelemetry-collector v0.84 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.84
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.84 Release Notes

Source: [v0.84.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.84.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.84.0

## User Facing Changes

### 💡 Enhancements 💡

- `loggingexporter`: Adds exemplars logging to the logging exporter when `detailed` verbosity level is set. (#7912)
- `configgrpc`: Allow any registered gRPC load balancer name to be used. (#8262)
- `service`: add OTLP export for internal traces (#8106)
- `configgrpc`: Add support for :authority pseudo-header in grpc client (#8228)

### 🧰 Bug fixes 🧰

- `otlphttpexporter`: Fix the handling of the HTTP response to ignore responses not encoded as protobuf (#8263)

## Go API Changes

### 💡 Enhancements 💡

- `exporter/exporterhelper`: Introduce a new exporter helper that operates over client-provided requests instead of pdata (#7874)
  The following experimental API is introduced in exporter/exporterhelper package:
    - `NewLogsRequestExporter`: a new exporter helper for logs.
    - `NewMetricsRequestExporter`: a new exporter helper for metrics.
    - `NewTracesRequestExporter`: a new exporter helper for traces.
    - `Request`: an interface for client-defined requests.
    - `RequestItemsCounter`: an optional interface for counting the number of items in a Request.
    - `LogsConverter`: an interface for converting plog.Logs to Request.
    - `MetricsConverter`: an interface for converting pmetric.Metrics to Request.
    - `TracesConverter`: an interface for converting ptrace.Traces to Request.
    All the new APIs are intended to be used by exporters that need to operate over client-provided requests instead of pdata.
  
- `otlpreceiver`: Export HTTPConfig as part of the API for creating the otlpreceiver configuration. (#8175)
  Changes signature of receiver/otlpreceiver/config.go type httpServerSettings to HTTPConfig.