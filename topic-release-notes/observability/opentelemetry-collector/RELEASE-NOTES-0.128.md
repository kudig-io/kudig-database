---
title: opentelemetry-collector v0.128 Release Notes
description: opentelemetry-collector v0.128 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.128 Release Notes 是什么
- 如何 opentelemetry-collector v0.128 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.128
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.128 Release Notes

Source: [v0.128.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.128.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.128.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `service/telemetry`: Mark "telemetry.disableAddressFieldForInternalTelemetry" as stable (#13152)

### 💡 Enhancements 💡

- `confighttp`: Update the HTTP server span naming to use the HTTP method and route pattern instead of the path. (#12468)
  The HTTP server span name will now be formatted as `<http.request.method> <http.route>`.
  If a route pattern is not available, it will fall back to `<http.request.method>`.
  
- `service`: Use configured loggers to log errors as soon as it is available (#13081)
- `service`: Remove stabilized featuregate useOtelWithSDKConfigurationForInternalTelemetry (#13152)

### 🧰 Bug fixes 🧰

- `telemetry`: Add generated resource attributes to the printed log messages. (#13110)
  If service.name, service.version, or service.instance.id are not specified in the config, they will be generated automatically.
  This change ensures that these attributes are also included in the printed log messages.
  
- `mdatagen`: Fix generation when there are no events in the metadata. (#13123)
- `confmap`: Do not panic on assigning nil maps to non-nil maps (#13117)
- `pdata`: Fix event_name skipped when unmarshalling LogRecord from JSON (#13127)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: Remove deprecated NewProfilesRequestExporter function from xexporterhelper package (#13157)
- `confighttp`: Remove pointer to field `cookies` in confighttp.ClientConfig (#13116)
- `otlpreceiver`: Use `configoptional.Optional` to define optional configuration sections in the OTLP receiver. Remove `Unmarshal` method. (#13119)
- `confighttp,configgrpc`: Rename `ClientConfig.TLSSetting` and `ServerConfig.TLSSetting` to `ClientConfig.TLS` and `ServerConfig.TLS`. (#13115)
- `pdata/pprofile`: Upgrade the OTLP protobuf definitions to version 1.7.0 (#13075)
  Note that the batcher is temporarily a noop.
- `pipeline`: Remove deprecated MustNewID[WithName] (#13139)

### 🚀 New components 🚀

- `configoptional`: Add a new configoptional module to support optional configuration fields. (#12981)

### 💡 Enhancements 💡

- `pdata`: Introduce `MoveAndAppendTo` methods to the generated primitive slices (#13074)
- `pdata`: Upgrade the OTLP protobuf definitions to version 1.7.0 (#13075)

### 🧰 Bug fixes 🧰

- `confmap`: Correctly distinguish between `nil` and empty map values on the `ToStringMap` method (#13161)
  This means that `ToStringMap()` method can now return a nil map if the original value was `nil`.
  If you were not doing so already, make sure to check for `nil` before writing to the map to avoid panics.
  
- `confighttp`: Make the `NewDefaultServerConfig` function return a nil TLS config by default. (#13129)
  - The previous default was a TLS config with no certificates, which would fail at runtime.
  

<!-- previous-version -->
