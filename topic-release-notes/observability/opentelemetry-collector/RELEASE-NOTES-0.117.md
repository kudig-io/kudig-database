---
title: opentelemetry-collector v0.117 Release Notes
description: opentelemetry-collector v0.117 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.117 Release Notes 是什么
- 如何 opentelemetry-collector v0.117 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.117
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.117 Release Notes

Source: [v0.117.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.117.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.117.0

## End User Changelog

## v1.23.0/v0.117.0

### 🛑 Breaking changes 🛑

- `otelcol`: Remove warnings when 0.0.0.0 is used (#11713, #8510)

### 🧰 Bug fixes 🧰

- `internal/sharedcomponent`: Fixed bug where sharedcomponent would use too much memory remembering all the previously reported statuses (#11826)

## API Changelog

## v1.23.0/v0.117.0

### 🛑 Breaking changes 🛑

- `pdata/pprofile`: Remove duplicate Attributes field from profile (#11932)
- `connector`: Remove deprecated connectorprofiles module, use xconnector instead. (#11778)
- `consumererror`: Remove deprecated consumererrorprofiles module, use xconsumererror instead. (#11778)
- `consumer`: Remove deprecated consumerprofiles module, use xconsumer instead. (#11778)
- `exporterhelper`: Remove deprecated exporterhelperprofiles module, use xexporterhelper instead. (#11778)
- `exporter`: Remove deprecated exporterprofiles module, use xexporter instead. (#11778)
- `pipeline`: Remove deprecated pipelineprofiles module, use xpipeline instead. (#11778)
- `processorhelper`: Remove deprecated processorhelperprofiles module, use xprocessorhelper instead. (#11778)
- `processor`: Remove deprecated processorprofiles module, use xprocessor instead. (#11778)
- `receiver`: Remove deprecated receiverprofiles module, use xreceiver instead. (#11778)
- `exporterhelper`: Remove Merge function from experimental Request interface (#12012)

### 🚩 Deprecations 🚩

- `mdatagen`: Deprecate component_test in favor of metadatatest (#11812)
- `receivertest`: Deprecate receivertest.NewNopFactoryForType (#11993)
- `extension/experimental`: Deprecate extension/experimental in favor of extension/xextension (#12010)
- `scraperhelper`: Move scraperhelper under scraper and in a separate module. (#11003)

### 💡 Enhancements 💡

- `scrapertest`: Add scrapertest package in a separate module (#11988)
- `pdata`: Upgrade pdata to opentelemetry-proto v1.5.0 (#11932)