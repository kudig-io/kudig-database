---
title: opentelemetry-collector v0.139 Release Notes
description: opentelemetry-collector v0.139 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.139 Release Notes 是什么
- 如何 opentelemetry-collector v0.139 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.139
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.139 Release Notes

Source: [v0.139.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.139.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.139.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `cmd/mdatagen`: Make stability.level a required field for metrics (#14070)
- `cmd/mdatagen`: Replace `optional` field with `requirement_level` field for attributes in metadata schema (#13913)
  The `optional` boolean field for attributes has been replaced with a `requirement_level` field that accepts enum values: `required`, `conditionally_required`, `recommended`, or `opt_in`.
  - `required`: attribute is always included and cannot be excluded
  - `conditionally_required`: attribute is included by default when certain conditions are met (replaces `optional: true`)
  - `recommended`: attribute is included by default but can be disabled via configuration (replaces `optional: false`)
  - `opt_in`: attribute is not included unless explicitly enabled in user config
  When `requirement_level` is not specified, it defaults to `recommended`.
  
- `pdata/pprofile`: Remove deprecated `PutAttribute` helper method (#14082)
- `pdata/pprofile`: Remove deprecated `PutLocation` helper method (#14082)

### 💡 Enhancements 💡

- `all`: Add FIPS and non-FIPS implementations for allowed TLS curves (#13990)
- `cmd/builder`: Set CGO_ENABLED=0 by default, add the `cgo_enabled` configuration to enable it. (#10028)
- `pkg/config/configgrpc`: Errors of type status.Status returned from an Authenticator extension are being propagated as is to the upstream client. (#14005)
- `pkg/config/configoptional`: Adds new `configoptional.AddEnabledField` feature gate that allows users to explicitly disable a `configoptional.Optional` through a new `enabled` field. (#14021)
- `pkg/exporterhelper`: Replace usage of gogo proto for persistent queue metadata (#14079)
- `pkg/pdata`: Remove usage of gogo proto and generate the structs with pdatagen (#14078)

### 🧰 Bug fixes 🧰

- `exporter/debug`: add queue configuration (#14101)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `all`: Change type of `configgrpc.ClientConfig.Headers`, `confighttp.ClientConfig.Headers`, and `confighttp.ServerConfig.ResponseHeaders` (#13930)
  `configopaque.MapList` is a new alternative to `map[string]configopaque.String` which can unmarshal
  both maps and lists of name/value pairs.
  
  For example, if `headers` is a field of type `configopaque.MapList`,
  then the following YAML configs will unmarshal to the same thing:
  ```yaml
  headers:
    "foo": "bar"
  
  headers:
  - name: "foo"
    value: "bar"
  ```
  
- `pdata/pprofile`: Update `SetFunction` to return the function's ID rather than update the Line (#14016, #14032)
- `pdata/pprofile`: Update `SetLink` to return the link's ID rather than update the Sample (#14016, #14031)
- `pdata/pprofile`: Update `SetMapping` to return the mapping's ID rather than update the Location (#14016, #14030)
- `pkg/otelcol`: Require a telemetry factory to be injected through otelcol.Factories (#4970)
  otelcol.Factories now has a required Telemetry field,
  which contains the telemetry factory to be used by the service.
  Set it to otelconftelemetry.NewFactory() for the existing behavior.
  
- `pkg/pdata`: Remove unused generated code from pprofile (#14073)
  Experimental package, ok to break since not used.

### 💡 Enhancements 💡

- `pdata/pprofile`: Introduce `SetStack` method (#14007)
- `pdata/xpdata`: Add high-level Entity API for managing entities attached to resources (#14042)
  Introduces `Entity`, `EntitySlice`, and `EntityAttributeMap` types that provide a user-friendly interface
  for working with resource entities. The new API ensures consistency between entity and resource attributes
  by sharing the underlying attribute map, and prevents attribute conflicts between entities. This API may
  eventually replace the generated protobuf-based API for better usability.
  

### 🧰 Bug fixes 🧰

- `cmd/mdatagen`: Fix mdatagen generated_metrics for connectors (#12402)

<!-- previous-version -->
