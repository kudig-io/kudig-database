---
title: opentelemetry-collector v0.14 Release Notes
description: opentelemetry-collector v0.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.14 Release Notes 是什么
- 如何 opentelemetry-collector v0.14 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.14
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

# opentelemetry-collector v0.14 Release Notes

Source: [v0.14.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.14.0)

# v0.14.0 Beta

## 🚀 New components 🚀

- `otlphttp` exporter which implements OTLP over HTTP protocol.

## 🛑 Breaking changes 🛑

- Rename consumer.TraceConsumer to consumer.TracesConsumer #1974
- Rename component.TraceReceiver to component.TracesReceiver #1975
- Rename component.TraceProcessor to component.TracesProcessor #1976
- Rename component.TraceExporter to component.TracesExporter #1975
- Deprecate NopExporter, add NopConsumer (#1972)
- Deprecate SinkExporter, add SinkConsumer (#1973)
- Move `tailsampling` processor to contrib (#2012)
- Remove NewAttributeValueSlice (#2028) and mark NewAttributeValue as deprecated (#2022)
- Remove pdata.StringValue (#2021)
- Remove pdata.InitFromAttributeMap, use CopyTo if needed (#2042)
- Remove SetMapVal and SetArrayVal for pdata.AttributeValue (#2039)

## 💡 Enhancements 💡

- `zipkin` exporter: Add queue retry to zipkin (#1971)
- `prometheus` exporter: Add `send_timestamps` option (#1951)
- `filter` processor: Add `expr` pdata.Metric filtering support (#1940, #1996)
- `attribute` processor: Add log support (#1934)
- `logging` exporter: Add index for histogram buckets count (#2009)
- `otlphttp` exporter: Add correct handling of server error responses (#2016)
- `prometheusremotewrite` exporter:
  - Add user agent header to outgoing http request (#2000)
  - Convert histograms to cumulative (#2049)
  - Return permanent errors (#2053)
  - Add external labels (#2044)
- `hostmetrics` receiver: Use scraper controller (#1949)
- Change Span/Trace ID to be byte array (#2001)
- Add `simple` metrics helper to facilitate building pdata.Metrics in receivers (#1540)
- Improve diagnostic logging for exporters (#2020)
- Add obsreport to receiverhelper scrapers (#1961)
- Update OTLP to 0.6.0 and use the new Span Status code (#2031)
- Add support of partial requests for logs and metrics to the exporterhelper (#2059)

## 🧰 Bug fixes 🧰

- `logging` exporter: Added array serialization (#1994)
- `zipkin` receiver: Allow receiver to parse string tags (#1893)
- `batch` processor: Fix shutdown race (#1967)
- Guard for nil data points (#2055)