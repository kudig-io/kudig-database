---
title: opentelemetry-collector v0.112 Release Notes
description: opentelemetry-collector v0.112 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.112 Release Notes 是什么
- 如何 opentelemetry-collector v0.112 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.112
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.112 Release Notes

Source: [v0.112.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.112.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.112.0

## End User Changelog

## v1.18.0/v0.112.0

### 🛑 Breaking changes 🛑

- `consumer/consumererror`: Extract consumer/consumererror as a separate go module (#11440)
- `exporter/expotertest`: Put expotertest into its own module (#11461)
- `service`: Remove stable gate component.UseLocalHostAsDefaultHost (#11412)

### 🚩 Deprecations 🚩

- `processortest`: Deprecated 'NewUnhealthyProcessorCreateSettings'. Use NewNopSettings instead. (#11307)

### 💡 Enhancements 💡

- `mdatagen`: Added generated_package_name config field to support custom generated package name. (#11231)
- `mdatagen`: Generate documentation for components with resource attributes only (#10705)
- `confighttp`: Adding support for lz4 compression into the project (#9128)
- `service`: Hide profiles support behind a feature gate while it remains alpha. (#11477)
- `exporterhelper`: Retry sender will fail fast when the context timeout is shorter than the next retry interval. (#11183)

### 🧰 Bug fixes 🧰

- `cmd/builder`: Fix default configuration for builder for httpprovider, httpsprovider, and yamlprovider. (#11357)
- `processorhelper`: Fix issue where in/out parameters were not recorded when error was returned from consumer. (#11351)

## API Changes

## v1.18.0/v0.112.0

### 🛑 Breaking changes 🛑

- `service`: Change Host to not implement GetExportersWithSignal (#11444)
  Use Host.GetExporters if still needed.
- `componentstatus`: Remove deprecated `NewInstanceIDWithPipelineIDs`, `AllPipelineIDsWithPipelineIDs`, and `WithPipelineIDs`. Use `NewInstanceID`, `AllPipelineIDs` and `WithPipelines` instead. (#11363)
- `configgrpc`: Removed deprecated `ClientConfig.ToClientConnWithOptions`/`ServerConfig.ToServerWithOptions`. (#11359, #9480)
  These methods were renamed to `ClientConfig.ToClientConn`/`ServerConfig.ToServer` in v0.111.0.
- `connector`: Put connectortest in its own module (#11216)
- `exporter`: Disables setting batch option to batch sender directly. (#10368)
  Removed WithRequestBatchFuncs(BatcherOption) in favor of WithBatchFuncs(Option), where | BatcherOption is a function that operates on batch sender and Option is one that operates | on BaseExporter
- `exporter`: Made mergeFunc and mergeSplitFunc required method of exporter.Request (#10368)
  mergeFunc and mergeSplitFunc used to be part of the configuration pass to the exporter. Now it is changed | to be a method function of request.
- `componentprofiles`: Move componentprofiles to pipelineprofiles (#11421)
- `processor`: Put processortest in its own module (#11218)
- `receivertest`: Removed deprecated `NewNopFactoryForTypeWithSignal`. Use `NewNopFactoryForType` instead. (#11362)
- `processor`: Remove deprecated funcs from processor package (#11368)
- `receiver`: Remove deprecated funcs from receiver package (#11367)
- `processorhelper`: Remove deprecated funcs/types from processorhelper & componenttest (#11302)
- `service`: Remove deprecated `pipelines.ConfigWithPipelineID` and `Config.PipelinesWithPipelineID`. Use `pipelines.Config` and `Config.Pipelines` instead. (#11361)

### 🚩 Deprecations 🚩

- `extension`: Deprecate funcs that repeat extension in name (#11413)
  Factory.CreateExtension -> Factory.Create |
  Factory.ExtensionStability -> Factory.Stability
  
- `exporter`: Deprecate funcs that repeate exporter in name (#11370)
  Factory.Create[Traces|Metrics|Logs|Profiles]Exporter -> Factory.Create[Traces|Metrics|Logs|Profiles] |
  Factory.[Traces|Metrics|Logs|Profiles]ExporterStability -> Factory.[Traces|Metrics|Logs|Profiles]Stability
  

### 🚀 New components 🚀

- `consumererrorprofiles`: Add new module consumereerrorprofiles for consumer error profiles. (#11131)

### 💡 Enhancements 💡

- `configcompression`: Add support for lz4 compression (#9128)
- `otlpexporter`: Add profiles support to OTLP exporter (#11435)
- `otlphttpexporter`: Add profiles support to OTLP HTTP exporter (#11450)