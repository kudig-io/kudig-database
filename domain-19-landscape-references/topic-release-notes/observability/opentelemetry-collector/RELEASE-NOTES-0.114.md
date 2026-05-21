---
title: opentelemetry-collector v0.114 Release Notes
description: opentelemetry-collector v0.114 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.114 Release Notes 是什么
- 如何 opentelemetry-collector v0.114 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.114
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.114 Release Notes

Source: [v0.114.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.114.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.114.0

## End User Changelog

## v1.20.0/v0.114.0

### 💡 Enhancements 💡

- `cmd/builder`: Allow for replacing of local Providers and Converters when building custom collector with ocb. (#11649)
  Use the property `path` under `gomod` to replace an go module with a local folder in
  builder-config.yaml. Ex:
  ```
  providers:
    - gomod: module.url/my/custom/provider v1.2.3
      path: /path/to/local/provider
  ```
  
- `cmd/builder`: Allow configuring `confmap.Converter` components in ocb. (#11582)
  If no converters are specified, there will be no converters added.
  Currently, the only published converter is `expandconverter` which is 
  deprecated as of v0.107.0, but can still be added for testing purposes.
  
  To configure a custom converter, make sure your converter implements the converter
  interface and is published as a go module (or replaced locally if not published).
  You may then use the `converters` key in your OCB build manifest with a list of
  Go modules (and replaces as necessary) to include your converter.
  
  Please note that converters are order-dependent. The confmap will apply converters
  in order of which they are listed in your manifest if there is more than one.
  
- `all`: shorten time period before removing an unmaintained component from 6 months to 3 months (#11664)

### 🧰 Bug fixes 🧰

- `all`: Updates dialer timeout section documentation in confignet README (#11685)
- `scraperhelper`: If the scraper shuts down, do not scrape first. (#11632)
  When the scraper is shutting down, it currently will scrape at least once.
  With this change, upon receiving a shutdown order, the receiver's scraperhelper will exit immediately.

## API Changes

## v1.20.0/v0.114.0

### 🛑 Breaking changes 🛑

- `extensiontest`: Make extensiontest into its own module (#11463)
- `component`: Make componenttest into its own module (#11464)
- `expandconverter`: Remove deprecated expandvar converter (#11672)
- `exporter`: Remove deprecated funcs Create[*]Exporter and [*]ExporterStability (#11662)
- `exporterhelper`: Remove derprecated NewLogs[Request]Exporter funcs (#11661)
- `extension`: Remove deprecated funcs CreateExtension and ExtensionStability (#11663)
- `processortest`: Remove deprecated func NewUnhealthyProcessorCreateSettings (#11665)

### 🚩 Deprecations 🚩

- `component`: Deprecate `TelemetrySettings.LeveledMeterProvider` and undo deprecation of `TelemetrySettings.MeterProvider` (#11061)
- `scraperhelper`: Deprecate Scraper.ID func, pass type when register Scraper (#11238)
