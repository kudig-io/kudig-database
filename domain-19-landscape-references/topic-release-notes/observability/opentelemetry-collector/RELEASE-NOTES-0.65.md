---
title: opentelemetry-collector v0.65 Release Notes
description: opentelemetry-collector v0.65 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.65 Release Notes 是什么
- 如何 opentelemetry-collector v0.65 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.65
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

# opentelemetry-collector v0.65 Release Notes

Source: [v0.65.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.65.0)

Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.65.0

### 🛑 Breaking changes 🛑

- `featuregate`: Capitalize `featuregate.Stage` string values, remove Stage prefix. (#6490)
- `configtelemetry`: Update values returned by `Level.String` and `Level.MarshalText` method. (#6490)
  - All returned strings are capitalized.
  - "" is returned for integers that are out of Level enum range.
  - It also affects `Level.Marshal` output, but it's not a problem because `Unmarshal` method accepts strings in
  all cases, e.g. "normal", "Normal" and "NORMAL".
  
- `featuregate`: Make impossible to implement RegistryOption outside `featuregate` package (#6532)
- `service/telemetry`: Remove unit suffixes from metrics exported by the otel-go prometheus exporter. (#6403)
- `obsreport`: `obsreport.New[Receiver|Scraper|Processor|Exporter]` returns error now (#6458)
- `configgrpc`: Remove deprecated funcs in `configgrpc`. (#6529)
  - `configgrpc.GRPCClientSettings.ToDialOptions`
  - `configgrpc.GRPCServerSettings.ToServerOption`
  
- `config/configtest`: Remove deprecated `configtest` package. (#6542)
- `config`: Remove deprecated types and funcs from config. Use `component` package. (#6511)
  - config.ComponentID
  - config.Type
  - config.DataType
  - config.Receiver
  - config.UnmarshalReceiver
  - config.Processor
  - config.UnmarshalProcessor
  - config.Exporter
  - config.UnmarshalExporter
  - config.Extension
  - config.UnmarshalExtension
  
- `featuregate`: Remove deprecated funcs and struct members from `featuregate` package (#6523)
  - featuregate.Gate.ID
  - featuregate.Gate.Description
  - featuregate.Gate.Enabled
  - featuregate.Registry.Register
  - featuregate.Registry.MustRegister
  
- `experimental`: Remove experimental configsource code. (#6558)
- `component`: Update values returned by `StabilityLevel.String` method. (#6490)
  - All returned strings are capitalized.
  - "Undefined" is returned only for `StabilityLevelUndefined`.
  - "" is returned for integers that are out of StabilityLevel enum range.
  

### 🚩 Deprecations 🚩

- `pdata`: Deprecate `pcommon.[Span|Trace]ID.HexString` methods. Call `hex.EncodeToString` explicitly instead. (#6514)
- `obsreport`: deprecate `obsreport.MustNew[Receiver|Scraper|Processor|Exporter]` in favor of `obsreport.New[Receiver|Scraper|Processor|Exporter]` (#6458)
  - Deprecate `obsreport.MustNewReceiver()` in favor of `obsreport.NewReceiver()`
  - Deprecate `obsreport.MustNewScraper()` in favor of `obsreport.NewScraper()`
  - Deprecate `obsreport.MustNewProcessor()` in favor of `obsreport.NewProcessor()`
  - Deprecate `obsreport.MustNewExporter()` in favor of `obsreport.NewExporter()`
  
- `component`: Deprecate `component.Receiver`, `component.Processor`, and `component.Exporter`. (#6553)
- `featuregate`: Deprecate Get prefix funcs for `featuregate.Gate` (#6528)
  `featuregate.Gate.GetID` -> `featuregate.Gate.ID`
  `featuregate.Gate.GetDescription` -> `featuregate.Gate.Description`
  
- `component`: Deprecate `component.Config.Validate` in favor of `component.ValidateConfig` (#6572)
- `component`: Deprecate `StabilityLevelInDevelopment` enum const in favor of `StabilityLevelDevelopment`. (#6561)
  Also rename all mentions of "In development" stability level to "Development".
- `service`: Deprecate `service.[Starting|Running|Closing|Closed]` in favor of `service.State[Starting|Running|Closing|Closed]` (#6492)

### 💡 Enhancements 💡

- `component`: `component.Extension` is temporarily set to be an alias of `component.Component` which will be reverted once it's moved to the `extension` package. Change your `component.Host.GetExtensions()` implementation to return `map[ID]component.Component` instead of `map[ID]component.Extension` (#6553)
- `pdata`: Return error from `pcommon.[Value|Map|Slice].FromRaw` when unsupported type. (#6579)
- `batchprocessor`: instrument the `batch` processor with OpenTelemetry Go SDK (#6423)
- `obsreport`: Instrument `obsreport.Scraper` metrics with otel-go (#6460)
- `service/collector`: Support SIGHUP configuration reloading (#5966)
- `component`: Split component into its own package (#6187)
  The import path for the component module can now be access directly:
    - `go.opentelemetry.io/collector/component`
- `consumer`: Split consumer into its own package (#6186)
  The import path for the consumer module can now be accessed directly:
    - `go.opentelemetry.io/collector/consumer`
- `featuregate`: Split featuregate into its own package (#6526)
  The import path for the featuregate module can now be accessed directly:
    - `go.opentelemetry.io/collector/featuregate`

### 🧰 Bug fixes 🧰

- `service`: Disallow duplicate references to processors within a single pipeline (#6540)
