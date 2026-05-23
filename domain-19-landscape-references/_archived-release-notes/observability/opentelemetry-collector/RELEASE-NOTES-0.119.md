---
title: opentelemetry-collector v0.119 Release Notes
description: opentelemetry-collector v0.119 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.119 Release Notes 是什么
- 如何 opentelemetry-collector v0.119 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.119
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

# opentelemetry-collector v0.119 Release Notes

Source: [v0.119.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.119.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.119.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: Rename exporter span signal specific attributes (e.g. "sent_spans" / "send_failed_span") to "items.sent" / "items.failed". (#12165)
- `cmd/mdatagen`: Remove dead field `telemetry::level` (#12144)
- `exporterhelper`: Change exporter ID to be a Span level attribute instead on each event. (#12164)
  This does not have an impact on the level of information emitted, but on the structure of the Span.
- `cmd/mdatagen`: Remove `level` field from metrics definition (#12145)
  This mechanism will be added back once a new views mechanism is implemented.

### 💡 Enhancements 💡

- `configtls`: Allow users to mention their preferred curve types for ECDHE handshake (#12174)
- `[[Service|service]]`: remove custom code and instead use config package to instantiate meter provider. (#11611)
- `otelcol`: Adds support for listing config providers in components command's output (#11570)
- `general`: Reduce memory allocations when loading configuration and parsing component names (#11964)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix bug that the exporter with new batcher may have been marked as non mutation. (#12239)
  Only affects users that manually turned on `exporter.UsePullingBasedExporterQueueBatcher` featuregate.
- `service`: Preserve URL normalization logic that was present before. (#12254)
- `confighttp`: confighttp.ToServer now sets ErrorLog with a default logger backed by Zap (#11820)
  
  This change ensures that the http.Server's ErrorLog is correctly set using Zap's logger at the error level, addressing the issue of error logs being printed using a different logger.
  
- `exporterhelper`: Fix context propagation for DisabledBatcher (#12231)
- `mdatagen`: apply fieldalignment to generated code (#12125)
- `mdatagen`: Fix bug where Histograms were marked as not supporting temporaly aggregation (#12168)
- `exporterhelper`: Fix MergeSplit issue that ignores the initial message size. (#12257)
- `service`: Include validation errors from telemetry.Config when validating the service config (#12100)
  Previously validation errors were only printed to the console
- `service-telemetry`: pass the missing async error channel into service telemetry settings (#11417)

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: Change queue to embed the async consumers. (#12242)
- `exporterqueue`: Change Queue interface to return a callback instead of an index (#8122)
- `cmd/mdatagen`: Allow passing OTel Metric SDK options to the generated `SetupTelemetry` function. (#12166)
- `exporterhelper`: Rename exporter span signal specific attributes (e.g. "sent_spans" / "send_failed_span") to "items.sent" / "items.failed". (#12165)
- `component`: Change underlying type for `component.Kind` to be a struct. (#12214)
- `extension`: Change `extension.Extension` to be an interface that embeds `component.Component` instead of an alias (#11443)
- `component/componenttest`: Remove deprecated `CheckScraperMetrics` functions (#12183)
- `scraperhelper`: Remove deprecated ScrapperControllerOption and NewScraperControllerMetrics from scraperhelper. (#12147)

### 🚩 Deprecations 🚩

- `metadatatest`: Deprecate metadatatest.Telemetry in favor of componenttest.Telemetry (#12218)
  metadatatest.Telemetry -> componenttest.Telemetry |
  metadatatest.SetupTelemetry -> componenttest.NewTelemetry |
  metadatatest.Telemetry.NewSettings -> metadatatest.NewSettings |
  metadatatest.Telemetry.AssertMetrics -> metadatatest.AssertEqual* |
  
- `component/componenttest`: Deprecate `CheckExporterEnqueue*` functions in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckExporterEnqueue*` functions.
- `component/componenttest`: Deprecate CheckExporterLogs in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckExporterLogs`
- `component/componenttest`: Deprecate CheckExporterMetricGauge in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckReceiverMetricGauge`
- `component/componenttest`: Deprecate CheckExporterMetrics in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckExporterMetrics`
- `component/componenttest`: Deprecate CheckExporterTraces in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckExporterTraces`
- `component/componenttest`: Deprecate CheckReceiverLogs in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckReceiverLogs`
- `mdatagen`: Make registration of callback for async metric always optional. (#12204)
  Deprecate `metadata.TelemetryBuilder.Init*` and `metadata.With*Callback` in favor of `metadata.TelemetryBuilder.Register*Callback`
- `component`: Deprecate `component.TelemetrySettings.MetricsLevel` in favor of using views and 'Enabled' method. (#12159)
  - Components will temporarily need the service to support using views.
  

### 💡 Enhancements 💡

- `componenttest`: Add helper to get a metric for componentest.Telemetry (#12215)
- `componenttest`: Extract componenttest.Telemetry as generic struct for telemetry testing (#12151)
- `mdatagen`: Generate assert function for each metric in mdatagen (#12179)
- `metadatatest`: Generate NewSettings that accepts componenttest.Telemetry (#12216)
- `pdata/pprofile`: Add new helper method `FromAttributeIndices` to build a `pcommon.Map` out of `AttributeIndices`. (#12176)
- `scraper`: Support logs scraper (#12116)
- `component`: Allow `component.ValidateConfig` to recurse through all fields in a config object (#11524)
- `component`: Show path to invalid config in errors returned from `component.ValidateConfig` (#12108)

### 🧰 Bug fixes 🧰

- `mdatagen`: All register callbacks to async instruments can now be unregistered by calling `metadata.TelemetryBuilder.Shutdown()` (#12204)
- `mdatagen`: Fix bug where Histograms were marked as not supporting temporaly aggregation (#12168)
