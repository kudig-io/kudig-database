---
title: opentelemetry-collector v0.148 Release Notes
description: opentelemetry-collector v0.148 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.148 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.148 Release Notes 是什么
- 如何 opentelemetry-collector v0.148 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.148
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
---



# opentelemetry-collector v0.148 Release Notes

Source: [v0.148.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.148.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.148.0

## End User Changelog

### ❗ Known Issues ❗

- `[[Service|service]]`: The collector's internal [[Prometheus|Prometheus]] metrics endpoint (`:8888`) now emits OTel service labels with underscore
  names (`service_name`, `service_instance_id`, `service_version`) instead of dot-notation names (`service.name`,
  `service.instance.id`, `service.version`). Users scraping this endpoint with the Prometheus receiver will see these renamed
  labels in resource and datapoint attributes. As a workaround, add the following `metric_relabel_configs` to your scrape
  config in prometheus receiver:
  ```yaml
  metric_relabel_configs:
    - source_labels: [service_name]
      target_label: service.name
    - source_labels: [service_instance_id]
      target_label: service.instance.id
    - source_labels: [service_version]
      target_label: service.version
    - regex: service_name|service_instance_id|service_version
      action: labeldrop
  ```
  See https://github.com/open-telemetry/opentelemetry-collector/issues/14814 for details and updates.

### 🛑 Breaking changes 🛑

- `all`: Change metric units to be singular to match OTel specification, e.g. `{requests}` -> `{request}` (#14753)

### 💡 Enhancements 💡

- `cmd/mdatagen`: Add deprecated_type field to allow specifying an alias for component types. (#14718)
- `cmd/mdatagen`: Generate entity-scoped MetricsBuilder API that enforces entity-metric associations at compile time (#14659)
- `cmd/mdatagen`: Skip generating reaggregation config options for metrics that have no aggregatable attributes. (#14689)
- `pkg/service`: The internal status reporter no longer drops repeated Ok and RecoverableError statuses (#14282)
  Status events can now carry metadata and there's value in allowing them to be emitted despite the status value itself 
  not changing.
  

### 🧰 Bug fixes 🧰

- `cmd/builder`: Add `.exe` to output binary names when building for Windows targets. (#12591)
- `exporter/debug`: Add printing of metric metadata in detailed verbosity. (#14667)
- `exporter/otlp_grpc`: Prevent nil pointer panic when push methods are called before the OTLP exporter initializes its gRPC clients. (#14663)
  When the sending queue and retry are disabled, calling ConsumeTraces,
  ConsumeMetrics, ConsumeLogs, or ConsumeProfiles before the OTLP exporter
  initializes its gRPC clients could cause a nil pointer dereference panic.
  The push methods now return an error instead of panicking.
  
- `exporter/otlp_http`: Show the actual destination URL in error messages when request URL is modified by middleware. (#14673)
  Unwraps the `*url.Error` returned by `http.Client.Do()` to prevent misleading error logs when a middleware extension dynamically updates the endpoint.
  
- `pdata/pprofile`: Switch the dictionary of dictionary tables entries only once when merging profiles (#14709)
  For dictionary table data, we used to switch their dictionaries when doing
  the switch for the data that uses them.
  However, when an entry is associated with multiple other data (several
  samples can use the same stack), we would have been switching the
  dictionaries of the entry multiple times.
  
  We now switch dictionaries for dictionary table data only once, before
  switching the resource profiles.
  

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `cmd/mdatagen`: Generate a per-metric config type `<MetricName>Config` for each metric when `reaggregation_enabled: true` (#14689)
  Metrics with aggregatable attributes get `AggregationStrategy` and `EnabledAttributes []<MetricName>AttributeKey`
  fields; others get `Enabled` only. Typed attribute key constants (e.g. `DefaultMetricAttributeKeyStringAttr`)
  replace `[]string`, eliminating runtime attribute slice allocations. Validation errors now list valid attributes
  and strategies by name. Components using `reaggregation_enabled: true` will need to update references to `MetricConfig`.
  
- `pdata/pprofile`: Update protoID for message Sample fields (#14652)

### 💡 Enhancements 💡

- `cmd/mdatagen`: Extend mdatagen tool to generate go config structs for OpenTelemetry collector components. (#14561)
  The component config go code is generated from `config` section of the metadata.yaml file.
- `cmd/mdatagen`: Extend mdatagen tool to generate config JSON schema for OpenTelemetry components. (#14543)
  The component config JSON schema is generated from `config` section of the metadata.yaml file.
- `cmd/mdatagen`: Schema generation for mdatagen-controlled config parts (#14562)
  The schema is generated in separate yaml file and used to create full component schema.
- `pdata/pprofile`: Implement reference based attributes in Profiles (#14546)
- `pkg/pdata`: Upgrade the OTLP protobuf definitions to version 1.10.0 (#14766)
- `pkg/service`: The internal status reporter no longer drops repeated Ok and RecoverableError statuses (#14282)
  Status events can now carry metadata and there's value in allowing them to be emitted despite the status value itself 
  not changing.
  
- `pkg/xexporterhelper`: Add logic to cleanup partitions from memory which are not being used for specific time period. (#14526)
- `pkg/xpdata`: Add `NewEntity` constructor, `Entity.CopyToResource`, and `EntityAttributeMap.All` to `pdata/xpdata/entity` (#14659)

### 🧰 Bug fixes 🧰

- `cmd/mdatagen`: Preserve custom extensions (e.g., `x-customType`, `x-pointer`) during schema reference resolution. (#14713, #14565)
  Fixes an issue where custom properties defined locally on a node were 
  overwritten and lost when resolving a `$ref` to an external/internal schema.
  
- `pkg/xexporterhelper`: Fix race when partition is being removed from LRU and new items are being added at the same time. (#14526)

<!-- previous-version -->
