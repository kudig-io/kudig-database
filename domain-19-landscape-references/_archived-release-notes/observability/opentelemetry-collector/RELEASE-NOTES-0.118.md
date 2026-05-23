---
title: opentelemetry-collector v0.118 Release Notes
description: opentelemetry-collector v0.118 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.118 Release Notes 是什么
- 如何 opentelemetry-collector v0.118 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.118
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.118 Release Notes

Source: [v0.118.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.118.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.118.0

## End User Changelog

### 💡 Enhancements 💡

- `exporterhelper`: Add blocking option to control queue behavior when full (#12090)
- `debugexporter`: Add EventName to debug exporter for Logs. EventName was added as top-level field in the LogRecord from 1.5.0 of proto definition. (#11966)
- `confighttp`: Added support for configuring compression levels. (#10467)
  A new configuration option called CompressionParams has been added to confighttp. | This allows users to configure the compression levels for the confighttp client.
- `exporterhelper`: Change the memory queue implementation to not pre-allocate capacity objects. (#12070)
  This change improves memory usage of the collector under low utilization and is a prerequisite for supporting different other size limitations (number of items, bytes).

### 🧰 Bug fixes 🧰

- `mdatagen`: apply fieldalignment to generated code (#12121)
- `otelcoltest`: Set `DefaultScheme` to `env` in the test `ConfigProvider` to replicate the default provider used by the Collector. (#12066)

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterqueue`: Change Queue Size and Capacity to return explicit int64. (#12076)
- `receiver/scraperhelper`: Removing the deprecated receiver/scraperhelper package (#12054)
- `processorteset`: Revert the nop_processor.NewNopSettings change, as it is no longer needed (#11433)
- `experimental/storage`: Remove deprecated package/module experimental/storage (#12109)
- `mdatagen`: Remove deprecated generated_component_telemetry_test file from being generated and delete it. (#12068)
- `receivertest`: Remove deprecated receivertest.NewNopFactoryForType (#12110)

### 🚩 Deprecations 🚩

- `componenttest`: Deprecate CheckScraperMetrics in componenenttest (#12105)
  Use `metadatatest.AssertMetrics` instead of `obsreporttest.CheckScraperMetrics`
- `scraperhelper`: Deprecate `scraperhelper.NewScraperControllerReceiver` and `scraperhelper.ScraperControllerOption`. (#12103)
  Use `scraperhelper.NewMetricsController` instead of `scraperhelper.NewScraperControllerReceiver` | Use `scraperhelper.ScraperControllerOption` instead of `scraperhelper.ControllerOption`

### 💡 Enhancements 💡

- `exporterhelper`: Add capability for memory and persistent queue to block when add items (#12074)
- `scraper/scraperhelper`: Add obs_logs for scraper/scraperhelper (#12036)
  This change adds obs for logs in scraper/scraperhelper, also introduced new metrics for scraping logs.
- `mdatagen`: Add scraper component type support to mdatagen (#12092)
- `mdatagen`: Add tracing support in metadatatest (#12106)
- `exporterhelper`: Change persistent queue to not use sized channel, improve memory usage and simplify sized_channel. (#12060)
- `confighttp`: Added support for configuring compression levels. (#10467)
  A new configuration option called CompressionParams has been added to confighttp. | This allows users to configure the compression levels for the confighttp client.
