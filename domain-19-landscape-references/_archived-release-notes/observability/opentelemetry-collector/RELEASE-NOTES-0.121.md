---
title: opentelemetry-collector v0.121 Release Notes
description: opentelemetry-collector v0.121 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.121 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.121 Release Notes 是什么
- 如何 opentelemetry-collector v0.121 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.121
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.121 Release Notes

Source: [v0.121.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.121.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.121.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `confighttp`: Make the client config options `max_idle_conns`, `max_idle_conns_per_host`, `max_conns_per_host`, and `idle_conn_timeout` integers (#9478)
  All four options can be set to `0` where they were previously set to `null`

### 🚩 Deprecations 🚩

- `exporterhelper`: Deprecate `min_size_items` and `max_size_items` in favor of `min_size` and `max_size`. (#12486)

### 💡 Enhancements 💡

- `mdatagen`: Add `converter` and `provider` module classes (#12467)
- `pipeline`: output pipeline name with signal as signal[/name] format in logs. (#12410)
- `memorylimiter`: Add support to configure min GC intervals for soft and hard limits. (#12450)
- `otlpexporter`: Update the stability level for logs, it has been as stable as traces and metrics for some time. (#12423)
- `[[Service|service]]`: Create a new subcommand to dump the initial configuration after resolving/merging. (#11479)
  To use the `print-initial-config` subcommand, invoke the Collector with the subcommand and corresponding feature gate: `otelcol print-initial-config --feature-gates=otelcol.printInitialConfig --config=config.yaml`.
  Note that the feature gate enabling this flag is currently in alpha stability, and the subcommand may
  be changed in the future.
  
- `memorylimiterprocessor`: Add support for profiles. (#12453)
- `otelcol`: Converters are now available in the `components` command. (#11900, #12385)
- `component`: Mark module as stable (#9376)
- `confmap`: Surface YAML parsing errors when they happen at the top-level. (#12180)
  This adds context to some instances of the error "retrieved value (type=string) cannot be used as a Conf", which typically happens because of invalid YAML documents
  
- `pprofile`: Add LinkIndex attribute to the generated Sample type (#12485)
- `exporterhelper`: Stabilize exporter.UsePullingBasedExporterQueueBatcher and remove old batch sender (#12425)
- `mdatagen`: Update metadata schema with new fields without enforcing them (#12359)

### 🧰 Bug fixes 🧰

- `service`: Fix crash at startup when converting from v0.2.0 to v0.3.0 (#12438)
- `service`: fix bug in parsing service::telemetry configuration (#12437)
- `exporterhelper`: Fix bug where the error logged when conversion of data fails is always nil (#12510)
- `mdatagen`: Adds back missing import for filter when emitting resource attributes (#12455)

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterqueue`: Remove exporterqueue.Factory in favor of the NewQueue function, and merge configs for memory and persistent. (#12509)
  As a side effect of this change, no alternative implementation of the queue are supported and the Queue interface will be hidden.
- `exporterhelper`: Update MergeSplit function signature to use the new SizeConfig (#12486)
- `extension, connector, processor, receiver, exporter, scraper`: Remove deprecated `Create*` methods from `Create*Func` types. (#12305)
  The `xconnector.CreateMetricsToProfilesFunc.CreateMetricsToProfiles` method has been removed without a deprecation.
  
- `component`: Remove deprecated function and interface `ConfigValidator` and `ValidateConfig`. (#11524)
  - Use `xconfmap.Validator` and `xconfmap.Validate` instead.
  
- `receiver, scraper, processor, exporter, extension`: Remove deprecated MakeFactoryMap functions in favor of generic implementation (#12222)
- `exporterhelper`: Change the signature of the exporterhelper.WithQueueRequest to accept Encoding instead of the Factory. (#12509)
- `component/componenttest`: Removing the deprecated `CheckReceiverMetrics` and `CheckReceiverTraces` functions. (#12185)

### 🚩 Deprecations 🚩

- `componenttest`: Deprecated componenttest.TestTelemetry in favor of componenttest.Telemetry (#12419)
- `connector, exporter, extension, processor, receiver, scraper`: Add type parameter to `NewNopSettings` and deprecate `NewNopSettingsWithType` (#12305)
- `exporterhelper`: Deprecate MinSizeConfig and MaxSizeItems. (#12486)
- `extension/extensionauth`: Deprecate methods on `*Func` types. (#12480)
- `extension/auth, extension/auth/authtest`: Deprecate extension/auth and the related test module in favor of extension/extensionauth (#12478)

### 🚀 New components 🚀

- `service/hostcapabilities`: create `service/hostcapabilities` module (#12296, #12375)
  Removes getExporters interface in service/internal/graph.
  Removes getModuleInfos interface in service/internal/graph.
  Creates interface ExposeExporters in service/hostcapabilities to expose GetExporters function.
  Creates interface ModuleInfo in service/hostcapabilities to expose GetModuleInfos function.
  

### 💡 Enhancements 💡

- `exporterhelper`: Adds the config API to support serialized bytes based batching (#3262)
- `configauth`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.
- `confighttp`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.
- `otelcol`: Converters are now available in the `components` command. (#11900, #12385)
- `extension`: Mark module as stable (#11005)
- `pcommon.Map`: preallocate go map in Map.AsRaw() (#12406)
- `exporterhelper`: Stabilize exporter.UsePullingBasedExporterQueueBatcher and remove old batch sender (#12425)
- `service`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.

### 🧰 Bug fixes 🧰

- `mdatagen`: Fix broken imports in the generated files. (#12298)
- `processor, connector, exporter, receiver`: Explicitly error out at component creation time if there is a type mismatch. (#12305)