---
title: opentelemetry-collector v0.113 Release Notes
description: opentelemetry-collector v0.113 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.113 Release Notes 是什么
- 如何 opentelemetry-collector v0.113 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.113
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

# opentelemetry-collector v0.113 Release Notes

Source: [v0.113.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.113.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.113.0

## End User Changelog

## v1.19.0/v0.113.0

### 🛑 Breaking changes 🛑

- `internal/fanoutconsumer`: Extract internal/fanoutconsumer as a separate go module (#11441)
- `builder`: Remove builder support to build old version, and the otelcol_version config (#11405)
  User should remove this property from their config, to build older versions use older builders.
- `receiver`: Make receivertest into its own module (#11462)
- `builder`: Remove deprecated flags from Builder (#11576)
  Here is the list of flags | --name, --description, --version, --otelcol-version, --go, --module
- `internal/sharedcomponent`: Extract internal/sharedcomponent as a separate go module (#11442)

### 💡 Enhancements 💡

- `mdatagen`: Add otlp as supported [[Distribution|distribution]] (#11527)
- `batchprocessor`: Move single shard batcher creation to the constructor (#11594)
- `service`: add support for using the otelzap bridge and emit logs using the OTel Go SDK (#10544)

### 🧰 Bug fixes 🧰

- `service`: ensure traces and logs emitted by the otel go SDK use the same resource information (#11578)
- `config/configgrpc`: Patch for bug in the grpc-go NewClient that makes the way the hostname is resolved incompatible with the way proxy setting are applied. (#11537)
- `builder`: Update builder default providers to lastest stable releases (#11566)

## API Changes

## v1.19.0/v0.113.0

### 🛑 Breaking changes 🛑

- `builder`: Remove deprecated flags from Builder (#11576)
  Here is the list of flags | --name, --description, --version, --otelcol-version, --go, --module

### 🚀 New components 🚀

- `processorhelperprofiles`: Add processorhelperprofiles to support profiles signal (#11556)

### 💡 Enhancements 💡

- `mdatagen`: Add newTelemetrySettings to be generated all the time even for pkg class (#11535)
- `debugexporter`: Add profiles support to debug exporter (#11155)
- `component`: Add UnmarshalText for StabilityLevel (#11520)

