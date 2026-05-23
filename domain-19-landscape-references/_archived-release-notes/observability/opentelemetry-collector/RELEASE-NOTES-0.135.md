---
title: opentelemetry-collector v0.135 Release Notes
description: opentelemetry-collector v0.135 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.135 Release Notes 是什么
- 如何 opentelemetry-collector v0.135 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.135
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

# opentelemetry-collector v0.135 Release Notes

Source: [v0.135.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.135.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.135.0

## End User Changelog

### 💡 Enhancements 💡

- `exporterhelper`: Add new `exporter_queue_batch_send_size` and `exporter_queue_batch_send_size_bytes` metrics, showing the size of telemetry batches from the exporter. (#12894)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pdata/pprofile`: Remove deprecated AddAttribute method (#13764)

### 💡 Enhancements 💡

- `configmiddleware`: Stabilize `configmiddleware` module (#13422)
  This only stabilizes the configuration interface but does not stabilize the middlewares themselves or the way of implementing them.
- `xpdata`: Add experimental MapBuilder struct to optimize pcommon.Map construction (#13617)

<!-- previous-version -->
