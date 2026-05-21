---
title: opentelemetry-collector v0.31 Release Notes
description: opentelemetry-collector v0.31 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.31 Release Notes 是什么
- 如何 opentelemetry-collector v0.31 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.31
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

# opentelemetry-collector v0.31 Release Notes

Source: [v0.31.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.31.0)

## v0.31.0 Beta

## 🛑 Breaking changes 🛑

- Remove Resize() from pdata slice APIs (#3675)
- Remove the ballast allocation when `mem-ballast-size-mib` is set in command line (#3626)
  - Use [`ballast extension`](./extension/ballastextension/README.md) to set memory ballast instead.
- Rename `DoubleDataPoint` to `NumberDataPoint` (#3633)
- Remove `IntHistogram` (#3676)

## 💡 Enhancements 💡

- Update to OTLP 0.8.0:
  - Translate `IntHistogram` to `Histogram` in `otlp_wrappers` (#3676)
  - Translate `IntGauge` to `Gauge` in `otlp_wrappers` (#3619)
  - Translate `IntSum` to `Sum` in `otlp_wrappers` (#3621)
  - Update `NumberDataPoint` to support `DoubleVal` and `IntVal` (#3689)
  - Update `Exemplar` to use `oneOfPrimitiveValue` (#3699)
  - Remove `IntExemplar` and `IntExemplarSlice` from `pdata` (#3705)
  - Mark `IntGauge`/`IntSum`/`IntDataPoint` as deprecated (#3707)
  - Remove `IntGauge`/`IntSum` from `batchprocessor` (#3718)
  - `prometheusremotewrite` exporter: Convert to new Number metrics (#3714)
  - `prometheus` receiver: Convert to new Number metrics (#3716)
  - `prometheus` exporter: Convert to new Number metrics (#3709)
  - `hostmetrics` receiver: Convert to new Number metrics (#3710)
  - `opencensus`: Convert to new Number metrics (#3708)
  - `scraperhelper` receiver: Convert to new Number metrics (#3717)
  - `testbed`: Convert to new Number metrics (#3719)
  - `expoerterhelper`: Convert `resourcetolabel` to new Number metrics (#3723)
- `configauth`: Prepare auth API to return a context (#3618)
- `pdata`:
  - Implement `Equal()` for map-valued `AttributeValues` (#3612)
  - Add `[Type]Slice.Sort(func)` to sort slices (#3671)
- `memorylimiter`:
  - Add validation on ballast size between `memorylimiter` and `ballastextension` (#3532)
  - Access Ballast extension via `Host.GetExtensions` (#3634)
- `prometheusremotewrite` exporter: Add a WAL implementation without wiring up (#3597)
- `prometheus` receiver: Add `metricGroup.toDistributionPoint` pdata conversion (#3667)
- Use `ComponentID` as identifier instead of config (#3696)
- `zpages`: Move config validation from factory to `Validate` (#3697)
- Enable `tracez` z-pages from otel-go, disable opencensus (#3698)
- Convert temporality and monotonicity for deprecated sums (#3729)

## 🧰 Bug fixes 🧰

- `otlpexporter`: Allow endpoint to be configured with a scheme of `http` or `https` (#3575)
- Handle errors when reloading the collector service (#3615)
- Do not report fatal error when `cmux.ErrServerClosed` (#3703)
- Fix bool attribute equality in `pdata` (#3688)
