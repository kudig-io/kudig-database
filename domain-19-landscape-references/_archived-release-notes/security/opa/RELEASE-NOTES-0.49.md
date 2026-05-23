---
title: opentelemetry-collector v0.49 Release Notes
description: opentelemetry-collector v0.49 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.49 Release Notes 是什么
- 如何 opentelemetry-collector v0.49 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.49
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.49 Release Notes

Source: [v0.49.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.49.0)


## v0.49.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.49.0

### 🛑 Breaking changes 🛑

- Remove deprecated structs/funcs from previous versions (#5131)
- Do not set TraceProvider to global otel (#5138)
- Remove deprecated funcs from otlpgrpc (#5144)
- Add Scheme to MapProvider interface (#5068)
- Do not set MeterProvider to global otel (#5146)
- Make `InstrumentationLibrary<signal>ToScope` helper functions unexported (#5164)
- Remove Log's "ShortName" from logging exporter output (#5172)

### 🚩 Deprecations 🚩

- All pdata related APIs from model (model/pdata, model/otlp and model/otlpgrpc) are deprecated in
  favor of packages in the new pdata module separated by telemetry signal type (#5168)
  - `model/pdata`, `model/otlp` -> `pdata/pcommon`, `pdata/plog`, `pdata/pmetric`, `pdata/ptrace`
  - `model/otlpgrpc` -> `pdata/plog/plogotlp`, `pdata/pmetric/pmetricotlp`, `pdata/ptrace/ptraceotlp`
- Deprecate configmapprovider package, replace with mapconverter (#5167)
- Deprecate `[[Service|service]].MustNewConfigProvider` and `service.MustNewDefaultConfigProvider`in favor of `service.NewConfigProvider` (#4936)

### 💡 Enhancements 💡

- OTLP HTTP receiver will use HTTP/2 over TLS if client supports it (#5109) 
- Add `ObservedTimestamp` field to `pdata.LogRecord` (#5171)

### 🧰 Bug fixes 🧰

- Setup the correct meter provider if telemetry.useOtelForInternalMetrics featuregate enabled (#5146)
- Fix pdata.Value.asRaw() to correctly return elements of Slice and Map type (#5153)
- Update pdata.Slice.asRaw() to return raw representation of Slice and Map elements (#5157)
- The codepath through the OTLP receiver for [[gRPC|gRPC]] was not translating the InstrumentationLibrary* to Scope* (#5189)
