---
title: opentelemetry-collector v0.7 Release Notes
description: opentelemetry-collector v0.7 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.7 Release Notes 是什么
- 如何 opentelemetry-collector v0.7 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
- logging-basics
- tracing-basics
- observability-basics
---



# opentelemetry-collector v0.7 Release Notes

Source: [v0.7.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.7.0)

# v0.7.0 Beta

# 🚀 New components 🚀

- Receivers
  - `fluentfoward` runs a TCP server that accepts events via the [Fluent Forward protocol](https://github.com/fluent/fluentd/wiki/Forward-Protocol-Specification-v1) (#1173)
- Exporters
  - `kafka` exports traces to Kafka (#1439)
- Extensions
  - **Experimental** `fluenetbit` facilitates running a FluentBit subprocess of the collector (#1381)

# 💡 Enhancements 

- Updated `golang/protobuf` from v1.3.5 to v1.4.2 (#1308)
- Updated `opencensus-proto` from v0.2.1 to v0.3.0 (#1308)
- Added round_robin `balancer_name` as an option to [[gRPC|gRPC]] client settings (#1353)
- `hostmetrics` receiver
  - Switch to using perf counters to get disk io metrics on Windows (#1340)
  - Add device filter for file system (#1379) and disk (#1378) scrapers
  - Record process physical & virtual memory stats separately (#1403)
  - Scrape system.disk.time on Windows (#1408)
  - Add disk.pending_operations metric (#1428)
  - Add network interface label to network metrics (#1377)
- Add `exporterhelper` (#1351) and `processorhelper` (#1359) factories
- Update OTLP to latest version (#1384)
- Disable timeout, retry on failure and sending queue for `logging` exporter (#1400)
- Add support for retry and sending queue for `[[Jaeger|jaeger]]` exporter (#1401)
- Add batch size bytes metric to `batch` processor (#1270)
- `otlp` receiver: Add Log Support (#1444)
- Allow to configure read/write buffer sizes for http Client (#1447)
- Update DB conventions to latest and add exception conventions (#1452)

# 🧰 Bug fixes 🧰

- Fix `resource` processor for old metrics (#1412)
- `jaeger` receiver: Do not try to stop if failed to start. Collector service will do that (#1434)
