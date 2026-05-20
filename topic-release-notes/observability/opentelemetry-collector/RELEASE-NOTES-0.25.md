---
title: opentelemetry-collector v0.25 Release Notes
description: opentelemetry-collector v0.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- kafka
- job
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.25 Release Notes 是什么
- 如何 opentelemetry-collector v0.25 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.25
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.25 Release Notes

Source: [v0.25.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.25.0)

# v0.25.0 Beta

## 🛑 Breaking changes 🛑

- Rename ForEach (in pdata) with Range to be consistent with sync.Map (#2931)
- Rename `componenthelper.Start` to `componenthelper.StartFunc` (#2880)
- Rename `componenthelper.Stop` to `componenthelper.StopFunc` (#2880)
- Remove `exporterheleper.WithCustomUnmarshaler`, `processorheleper.WithCustomUnmarshaler`, `receiverheleper.WithCustomUnmarshaler`, `extensionheleper.WithCustomUnmarshaler`, implement `config.CustomUnmarshaler` interface instead (#2867)
- Remove `component.CustomUnmarshaler` implement `config.CustomUnmarshaler` interface instead (#2867)
- Remove `testutil.HostPortFromAddr`, users can write their own parsing helper (#2919)
- Remove `configparser.DecodeTypeAndName`, use `config.IDFromString` (#2869)
- Remove `config.NewViper`, users should use `config.NewParser` (#2917)
- Remove `testutil.WaitFor`, use `testify.Eventually` helper if needed (#2920)
- Remove testutil.WaitForPort, users can use testify.Eventually (#2926)
- Rename `processorhelper.NewTraceProcessor` to `processorhelper.NewTracesProcessor` (#2935)
- Rename `exporterhelper.NewTraceExporter` to `exporterhelper.NewTracesExporter` (#2937)
- Remove InitEmptyWithCapacity, add EnsureCapacity and Clear (#2845)
- Rename traces methods/objects to include Traces in Kafka receiver (#2966)

## 💡 Enhancements 💡

- Add `validatable` interface with `Validate()` to all `config.<component>` (#2898)
  - add the empty `Validate()` implementation for all component configs
- **Experimental**: Add a config source manager that wraps the interaction with config sources (#2857, #2903, #2948)
- `kafka` exporter: Key jaeger messages on traceid (#2855)
- `scraperhelper`: Don't try to count metrics if scraper returns an error (#2902)
- Extract ConfigFactory in a ParserProvider interface (#2868)
- `prometheus` exporter: Allows Summary metrics to be exported to Prometheus (#2900)
- `prometheus` receiver: Optimize `dpgSignature` function (#2945)
- `kafka` receiver: Add logs support (#2944)

## 🧰 Bug fixes 🧰

- `prometheus` receiver:
  - Treat Summary and Histogram metrics without "_sum" counter as valid metric (#2812)
  - Add `job` and `instance` as well-known labels (#2897)
- `prometheusremotewrite` exporter:
  - Sort Sample by Timestamp to avoid out of order errors (#2941)
  - Remove incompatible queued retry (#2951)
- `kafka` receiver: Fix data race with batchprocessor (#2957)
- `jaeger` receiver: Jaeger agent should not report ErrServerClosed (#2965)