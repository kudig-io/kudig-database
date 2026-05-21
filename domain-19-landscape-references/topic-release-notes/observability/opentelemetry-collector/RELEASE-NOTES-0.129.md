---
title: opentelemetry-collector v0.129 Release Notes
description: opentelemetry-collector v0.129 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.129 Release Notes 是什么
- 如何 opentelemetry-collector v0.129 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.129
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

# opentelemetry-collector v0.129 Release Notes

Source: [v0.129.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.129.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.129.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: Remove deprecated sending_queue::blocking options, use sending_queue::block_on_overflow. (#13211)

### 💡 Enhancements 💡

- `mdatagen`: Taught mdatagen to print the `go list` stderr output on failures, and to run `go list` where the metadata file is. (#13205)
- `service`: Support setting `sampler` and `limits` under `service::telemetry::traces` (#13201)
  This allows users to enable sampling and set span limits on internal Collector traces using the
  OpenTelemetry SDK declarative configuration.
  
- `pdata/pprofile`: Add new helper methods `FromLocationIndices` and `PutLocation` to read and modify the content of locations. (#13150)
- `exporterhelper`: Preserve request span context and client information in the persistent queue. (#11740, #13220, #13232)
  It allows internal collector spans and client information to propagate through the persistent queue used by 
  the exporters. The same way as it's done for the in-memory queue.
  Currently, it is behind the exporter.PersistRequestContext feature gate, which can be enabled by adding 
  `--feature-gates=exporter.PersistRequestContext` to the collector command line. An exporter buffer stored by
  a previous version of the collector (or by a collector with the feature gate disabled) can be read by a newer
  collector with the feature enabled. However, the reverse is not supported: a buffer stored by a newer collector with
  the feature enabled cannot be read by an older collector (or by a collector with the feature gate disabled).
  

### 🧰 Bug fixes 🧰

- `pdata`: Fix copying of optional fields when the source is unset. (#13268)
- `service`: Only allocate one set of internal log sampling counters (#13014)
  The case where logs are only exported to stdout was fixed in v0.126.0;
  this new fix also covers the case where logs are exported through OTLP.
  

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `semconv`: Removing deprecated semconv package (#13071)
- `configgrpc,confighttp`: Unify return type of `NewDefault*Config` functions to return a struct instead of a pointer. (#13169)
- `exporterhelper`: QueueBatchEncoding interface is changed to support marshaling and unmarshaling of request context. (#13188)

### 💡 Enhancements 💡

- `pdata/pprofile`: Introduce `Equal` method on the `Mapping` type (#13197)
- `configoptional`: Make unmarshaling into `None[T]` work the same as unmarshaling into `(*T)(nil)`. (#13168)
- `configoptional`: Add a confmap.Marshaler implementation for configoptional.Optional (#13196)
- `pdata/pprofile`: Introduce `Equal` methods on the `Line` and `Location` types (#13150)
- `pdata/pprofile`: Add new helper method `SetMapping` to set a new mapping on a location. (#13197)

### 🧰 Bug fixes 🧰

- `confmap`: Distinguish between empty and nil values when marshaling `confmap.Conf` structs. (#13196)

<!-- previous-version -->
