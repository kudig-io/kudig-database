---
title: opentelemetry-collector v0.67 Release Notes
description: opentelemetry-collector v0.67 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.67 Release Notes 是什么
- 如何 opentelemetry-collector v0.67 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.67
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.67 Release Notes

Source: [v0.67.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.67.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.67.0

## v1.0.0-RC1/v0.67.0

We are excited to announce that the `pdata` module is now available as a
v1.0.0 release candidate.  While breaking changes may still happen in this
module before v1.0.0, we believe it is ready for final assessment and validation
and hope to make a v1.0.0 release soon.

### 🛑 Breaking changes 🛑

- `overwritepropertiesconverter`: Remove deprecated package `overwritepropertiesconverter` (#6656)
- `pdata`: Change [...Slice|Map].Sort methods to not return any value (#6660)
- `featuregate`: remove deprecated functions (#6594)
  - `featuregate.GetID()`
  - `featuregate.GetDescription()`
- `obsreport`: remove deprecated functions. (#6595)
  - `obsreport.MustNewExporter`
  - `obsreport.MustNewProcessor`
  - `obsreport.MustNewReceiver`
  - `obsreport.MustNewScraper`
- `[[Service|service]]`: Remove deprecated `service.State` enum values. (#6605)
- `component`: Remove deprecated func/types from component (#6606)
- `config`: Remove deprecated config.Pipelines and config.Pipeline (#6664)
- `ballastextension`: Remove deprecated `ballastextension.MemoryBallast` type (#6628)
- `component`: Remove `Validate()` from component.*Config interfaces and make it optional interface (#6544)
- `confmap`: Splitting confmap into its own module (#6185)
  The import path for the confmap module can now be access directly:
  - `go.[[OpenTelemetry|opentelemetry]].io/collector/confmap`

### 🚩 Deprecations 🚩

- `service`: Deprecate service.[Collector|NewSvcHandler|CollectorSettings|State|NewCommand] in favor of otelcol package" (#6608)
  - Deprecate `service.Config` in favor of `otelcol.Config`.
  - Deprecate `service.ConfigProvider` in favor of `otelcol.ConfigProvider`.
  - Deprecate `service.NewConfigProvider` in favor of `otelcol.NewConfigProvider`.
  - Deprecate `service.CollectorSettings` in favor of `otelcol.CollectorSettings`.
  - Deprecate `service.Collector` in favor of `otelcol.Collector`.
  - Deprecate `service.New` in favor of `otelcol.NewCollector`.
  - Deprecate `service.State` in favor of `otelcol.State`.
  - Deprecate `service.NewSvcHandler` in favor of `otelcol.NewSvcHandler`.
  - Deprecate `service.NewCommand` in favor of `otelcol.NewCommand`.
- `obsreporttest`: Deprecate obsreporttest.Check* in favor of TestTelemetry.Check (#6678)
- `component`: Deprecate Exporter related types/funcs from component package in favor of exporter package. (#6578)
  - `component.ExporterCreateSettings` -> `exporter.CreateSettings`
  - `component.CreateTracesExporterFunc` -> `exporter.CreateTracesFunc`
  - `component.CreateMetricsExporterFunc` -> `exporter.CreateMetricsFunc`
  - `component.CreateLogsExporterFunc` -> `exporter.CreateLogsFunc`
  - `component.ExporterFactory` -> `exporter.Factory`
  - `component.NewExporterFactory` -> `exporter.NewFactory`
  - `component.MakeExporterFactoryMap` -> `exporter.MakeFactoryMap`
  - `componenttest.NewNopExporterCreateSettings` -> `exportertest.NewNopCreateSettings`
  - `componenttest.NewNopExporterFactory` -> `exportertest.NewNopFactory`
- `component`: Change Config to be opaque for otel collector core. (#4714)
  - Deprecate `component.Config.ID()` in favor of `component.[*]CreateSettings.ID`.
  - Deprecate `component.Config.SetIDName()`, no replacement needed since ID in settings is public member.
  - Deprecate `obsreporttest.SetupTelemetry` in favor of `obsreporttest.SetupTelemetryWithID`.
  
- `component`: Deprecate `component.Unmarshal[*]Config` in favor of `component.UnmarshalConfig` (#6613)
- `component`: Deprecate Extension related types/funcs from component package in favor of extension package. (#6578)
  - `component.Extension` -> `extension.Extension`
  - `component.PipelineWatcher` -> extension.PipelineWatcher
  - `component.ExtensionCreateSettings` -> `extension.CreateSettings`
  - `component.CreateExtensionFunc` -> `extension.CreateFunc`
  - `component.ExtensionFactory` -> `extension.Factory`
  - `component.NewExtensionFactory` -> `extension.NewFactory`
  - `component.MakeExtensionFactoryMap` -> `extension.MakeFactoryMap`
  - `componenttest.NewNopExtensionCreateSettings` -> `extensiontest.NewNopCreateSettings`
  - `componenttest.NewNopExtensionFactory` -> `extensiontest.NewNopFactory`
- `component`: Deprecate `Receiver` related structs and functions in favor of `receiver` package (#6687)
- `component`: Deprecate `component.[Exporter|Extension|Processor|Receiver]Config` in favor of `component.Config` (#6578)
- `pdata`: Remove deprecated funcs `pdata.[Span|Trace]ID.HexString` (#6627)

### 💡 Enhancements 💡

- `config/configopaque`: Add new `configopaque.String` type alias for opaque strings. (#5653)
- `service`: Added components sub command which outputs components in collector [[Distribution|distribution]]. (#4671)
- `component`: Define new component type 'connectors' (#6577)
- `connector`: Add connector factory (#6611)
- `connector`: Add connector types (TracesConnector, MetricsConnector, LogsConnector) (#6689)
- `connectortest`: Add connector/connectortest package (#6711)
- `component`: Add recursive validation check for configs (#4584)
- `service`: Improve config error messages, split Validate functionality (#6665)
- `extension/authextension`: Define new authextension package and use new package in collector repo (#6467)
  - configauth.ClientAuthenticator -> auth.Client
  - configauth.NewClientAuthenticator -> auth.NewClient
  - configauth.ClientOption -> auth.ClientOption
  - configauth.WithClientStart -> auth.WithClientStart
  - configauth.WithClientShutdown -> auth.WithClientShutdown
  - configauth.WithClientRoundTripper -> auth.WithClientRoundTripper
  - configauth.WithPerRPCCredentials -> auth.WithClientPerRPCCredentials
  - configauth.ServerAuthenticator -> auth.Server
  - configauth.NewServerAuthenticator -> auth.NewServer
  - configauth.Option -> auth.ServerOption
  - configauth.AuthenticateFunc -> auth.ServerAuthenticateFunc
  - configauth.WithAuthenticate -> auth.WithServerAuthenticate
  - configauth.WithStart -> auth.WithServerStart
  - configauth.WithShutdown -> auth.WithServerShutdown
- `obsreport`: Instrument `obsreport.Processor` metrics with otel-go (#6607)
- `pdata`: Add ability to clear optional fields (#6474)

### 🧰 Bug fixes 🧰

- `otlpexporter`: Fix nil panic from otlp exporter in case of errors during Start. (#6633)
- `service`: Stop notification for signals before shutdown, increase channel size. (#6522)
- `confmap`: Fix support for concatenating envvars with colon (#6580)
- `otlpexporter`: Fix a bug that exporter persistent queue is sending duplicate data after restarting. (#6692)