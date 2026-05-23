---
title: opentelemetry-collector v0.22 Release Notes
description: opentelemetry-collector v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.22 Release Notes 是什么
- 如何 opentelemetry-collector v0.22 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.22 Release Notes

Source: [v0.22.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.22.0)

# v0.22.0 Beta

## 🛑 Breaking changes 🛑

- Rename ServiceExtension to just Extension (#2581)
- Remove `consumerdata.TraceData` (#2551)
- Move `consumerdata.MetricsData` to `internaldata.MetricsData` (#2512)
- Remove custom OpenCensus sematic conventions that have equivalent in otel (#2552)
- Move ScrapeErrors and PartialScrapeError to `scrapererror` (#2580)
- Remove support for deprecated unmarshaler `CustomUnmarshaler`, only `Unmarshal` is supported (#2591)
- Remove deprecated componenterror.CombineErrors (#2598)
- Rename `pdata.TimestampUnixNanos` to `pdata.Timestamp` (#2549)

## 💡 Enhancements 💡

- `[[Prometheus|prometheus]]` exporter: Re-implement on top of `github.com/prometheus/client_golang/prometheus` and add `metric_expiration` option
- `logging` exporter: Add support for AttributeMap (#2609)
- Add semantic conventions for instrumentation library (#2602)

## 🧰 Bug fixes 🧰

- `otlp` receiver: Fix `Shutdown()` bug (#2564)
- `batch` processor: Fix Shutdown behavior (#2537)
- `logging` exporter: Fix handling the loop for empty attributes (#2610)
- `prometheusremotewrite` exporter: Fix counter name check (#2613)


## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.