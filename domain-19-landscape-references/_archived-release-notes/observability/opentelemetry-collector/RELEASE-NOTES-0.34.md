---
title: opentelemetry-collector v0.34 Release Notes
description: opentelemetry-collector v0.34 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.34 Release Notes 是什么
- 如何 opentelemetry-collector v0.34 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.34
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# [[OpenTelemetry|opentelemetry]]-collector v0.34 Release Notes

Source: [v0.34.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.34.0)

## v0.34.0 Beta

Release artifacts can be found [here](https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.34.0)

## 🛑 Breaking changes 🛑

- Artifacts are no longer published in this repository, check [here](https://github.com/open-telemetry/opentelemetry-collector-releases) (#3941)
- Remove deprecated `tracetranslator.AttributeValueToString` and `tracetranslator.AttributeMapToMap` (#3873)
- Change semantic conventions for status (code, msg) as per specifications (#3872)
- Add `pdata.NewTimestampFromTime`, deprecate `pdata.TimestampFromTime` (#3868)
- Add `pdata.NewAttributeMapFromMap`, deprecate `pdata.AttributeMap.InitFromMap` (#3936)
- Move `fileexporter` to contrib (#3474)
- Move `jaegerexporter` to contrib (#3474)
- Move `kafkaexporter` to contrib (#3474)
- Move `opencensusexporter` to contrib (#3474)
- Move `prometheusexporter` to contrib (#3474)
- Move `prometheusremotewriteexporter` to contrib (#3474)
- Move `zipkinexporter` to contrib (#3474)
- Move `attributeprocessor` to contrib (#3474)
- Move `filterprocessor` to contrib (#3474)
- Move `probabilisticsamplerprocessor` to contrib (#3474)
- Move `resourceprocessor` to contrib (#3474)
- Move `spanprocessor` to contrib (#3474)
- Move `hostmetricsreceiver` to contrib (#3474)
- Move `jaegerreceiver` to contrib (#3474)
- Move `kafkareceiver` to contrib (#3474)
- Move `opencensusreceiver` to contrib (#3474)
- Move `prometheusreceiver` to contrib (#3474)
- Move `zipkinreceiver` to contrib (#3474)
- Move `bearertokenauthextension` to contrib (#3474)
- Move `healthcheckextension` to contrib (#3474)
- Move `oidcauthextension` to contrib (#3474)
- Move `pprofextension` to contrib (#3474)
- Move `translator/internaldata` to contrib (#3474)
- Move `translator/trace/jaeger` to contrib (#3474)
- Move `translator/trace/zipkin` to contrib (#3474)
- Move `testbed` to contrib (#3474)
- Move `exporter/exporterhelper/resource_to_telemetry` to contrib (#3474)
- Move `processor/processorhelper/attraction` to contrib (#3474)
- Move `translator/conventions` to `model/semconv` (#3901)