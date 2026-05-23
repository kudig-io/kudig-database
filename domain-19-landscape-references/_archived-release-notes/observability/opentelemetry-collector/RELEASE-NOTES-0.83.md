---
title: opentelemetry-collector v0.83 Release Notes
description: opentelemetry-collector v0.83 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.83 Release Notes 是什么
- 如何 opentelemetry-collector v0.83 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.83
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

# opentelemetry-collector v0.83 Release Notes

Source: [v0.83.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.83.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.83.0

## User Facing Changes

### 💡 Enhancements 💡

- `extension`: Add optional `ConfigWatcher` interface (#6596)
  Extensions implementing this interface will be notified of the Collector's effective config.
- `otelcol`: Add optional `ConfmapProvider` interface for Config Providers (#6596)
  This allows providing the Collector's configuration as a marshaled confmap.Conf object
  from a ConfigProvider
  
- `[[Service|service]]`: Add `CollectorConf` field to `service.Settings` (#6596)
  This field is intended to be used by the Collector to pass its effective configuration to the service.

## Go API Changes


### 🛑 Breaking changes 🛑

- `all`: Remove go 1.19 support, bump minimum to go 1.20 and add testing for 1.21 (#8207)

### 💡 Enhancements 💡

- `changelog`: Generate separate changelogs for end users and package consumers (#8153)