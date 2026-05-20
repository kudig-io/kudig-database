---
title: opentelemetry-collector v0.16 Release Notes
description: opentelemetry-collector v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.16 Release Notes 是什么
- 如何 opentelemetry-collector v0.16 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.16
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.16 Release Notes

Source: [v0.16.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.16.0)

# v0.16.0 Beta

## 🛑 Breaking changes 🛑

- Rename Push functions to be consistent across signals in `exporterhelper` (#2203)

## 💡 Enhancements 💡

- Change default OTLP/gRPC port number to 4317, also continue receiving on legacy port
  55680 during transition period (#2104).
- `kafka` exporter: Add support for exporting metrics as otlp Protobuf. (#1966)
- Move scraper helpers to its own `scraperhelper` package (#2185)
- Add `componenthelper` package to help build components (#2186)
- Remove usage of custom init/stop in `scraper` and use start/shutdown from `component` (#2193)
- Add more trace annotations, so zpages are more useful to determine failures (#2206)
- Add support to skip TLS verification (#2202)
- Expose non-nullable metric types (#2208)
- Expose non-nullable elements from slices of pointers (#2200)

## 🧰 Bug fixes 🧰

- Change InstrumentationLibrary to be non-nullable (#2196)
- Add support for slices to non-pointers, use non-nullable AnyValue (#2192)
- Fix `--set` flag to work with `{}` in configs (#2162)

