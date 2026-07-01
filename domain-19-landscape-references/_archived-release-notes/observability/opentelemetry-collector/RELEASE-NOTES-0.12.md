---
title: opentelemetry-collector v0.12 Release Notes
description: opentelemetry-collector v0.12 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.12 Release Notes 是什么
- 如何 opentelemetry-collector v0.12 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.12
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



# opentelemetry-collector v0.12 Release Notes

Source: [v0.12.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.12.0)

## 🚀 New components 🚀

- `configauth` package with the auth settings that can be used by receivers (#1807, #1808, #1809, #1810)
- `perfcounters` package that uses perflib for host metrics receiver (#1835, #1836, #1868, #1869, #1870)

## 💡 Enhancements 💡

- Remove `queued_retry` and enable `otlp` metrics receiver in default config (#1823, #1838)
- Add `limit_percentage` and `spike_limit_percentage` options to `memorylimiter` processor (#1622)
- `hostmetrics` receiver:
  - Collect additional labels from partitions in the filesystems scraper (#1858)
  - Add filters for mount point and filesystem type (#1866)
- Add cloud.provider semantic conventions (#1865)
- `attribute` processor: Add log support (#1783)
- Deprecate OpenCensus-based internal data structures (#1843)
- Introduce SpanID data type, not yet used in Protobuf messages ($1854, #1855)
- Enable `otlp` trace by default in the released docker image (#1883)
- `tailsampling` processor: Combine batches of spans into a single batch (#1864)
- `filter` processor: Update to use pdata (#1885)
- Allow MSI upgrades (#1914)

## 🧰 Bug fixes 🧰

- `[[Prometheus|prometheus]]` receiver: Print a more informative message about 'up' metric value (#1826)
- Use custom data type and custom JSON serialization for traceid (#1840)
- Skip creation of redundant nil resource in translation from OC if there are no combined metrics (#1803)
- `tailsampling` processor: Only send to next consumer once (#1735)
- Report Windows pagefile usage in bytes (#1837)
- Fix issue where Prometheus SD config cannot be parsed (#1877)
