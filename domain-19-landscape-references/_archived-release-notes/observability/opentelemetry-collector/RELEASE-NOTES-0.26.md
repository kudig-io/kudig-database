---
title: opentelemetry-collector v0.26 Release Notes
description: opentelemetry-collector v0.26 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.26 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- kafka
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.26 Release Notes 是什么
- 如何 opentelemetry-collector v0.26 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.26
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
---



# opentelemetry-collector v0.26 Release Notes

Source: [v0.26.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.26.0)

# v0.26.0 Beta

## 🛑 Breaking changes 🛑

- Change `With*Unmarshallers` signatures in Kafka exporter/receiver (#2973)
- Rename `marshall` to `marshal` in all the occurrences (#2977)
- Remove `componenterror.ErrAlreadyStarted` and `componenterror.ErrAlreadyStopped`, components should not protect against this, [[Service|Service]] will start/stop once.
- Rename `ApplicationStartInfo` to `BuildInfo`
- Rename `ApplicationStartInfo.ExeName` to `BuildInfo.Command`
- Rename `ApplicationStartInfo.LongName` to `BuildInfo.Description`

## 💡 Enhancements 💡

- `kafka` exporter: Add logs support (#2943)
- Add AppendEmpty and deprecate Append for slices (#2970)
- Update mdatagen to create factories of init instead of new (#2978)
- `zipkin` receiver: Reduce the judgment of zipkin v1 version (#2990)
- Custom authenticator logic to accept a `component.Host` which will extract the authenticator to use based on a new authenticator name property (#2767)
- `prometheusremotewrite` exporter: Add `resource_to_telemetry_conversion` config option (#3031)
- `logging` exporter: Extract OTLP text logging (#3082)
- Format timestamps as strings instead of int in otlptext output (#3088)
- Add darwin arm64 build (#3090)

## 🧰 Bug fixes 🧰

- Fix [[Jaeger|Jaeger]] receiver to honor TLS Settings (#2866)
- `zipkin` translator: Handle missing starttime case for zipkin json v2 format spans (#2506)
- `[[Prometheus|prometheus]]` exporter: Fix OTEL resource label drops (#2899)
- `prometheusremotewrite` exporter:
  - Enable the queue internally (#2974)
  - Don't drop instance and job labels (#2979)
- `jaeger` receiver: Wait for server goroutines exit on shutdown (#2985)
- `logging` exporter: Ignore invalid handle on close (#2994)
- Fix service zpages (#2996)
- `batch` processor: Fix to avoid reordering and send max size (#3029)