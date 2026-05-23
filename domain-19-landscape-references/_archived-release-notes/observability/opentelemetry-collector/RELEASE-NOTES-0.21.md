---
title: opentelemetry-collector v0.21 Release Notes
description: opentelemetry-collector v0.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.21 Release Notes 是什么
- 如何 opentelemetry-collector v0.21 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.21 Release Notes

Source: [v0.21.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.21.0)

# v0.21.0 Beta

## 🛑 Breaking changes 🛑

- Remove deprecated function `IsValid` from trace/span ID (#2522)
- Remove accessors for deprecated status code (#2521)

## 💡 Enhancements 💡

- `otlphttp` exporter: Add `compression` option for gzip encoding of outgoing http requests (#2502)
- Add `ScrapeErrors` struct to `consumererror` to simplify errors usage (#2414)
- Add `cors_allowed_headers` option to `confighttp` (#2454)
- Add SASL/SCRAM authentication mechanism on `kafka` receiver and exporter (#2503)

## 🧰 Bug fixes 🧰

- `otlp` receiver: Sets the correct deprecated status code before sending data to the pipeline (#2521)
- Fix `IsPermanent` to account for wrapped errors (#2455)
- `otlp` exporter: Preserve original error messages (#2459)


## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.