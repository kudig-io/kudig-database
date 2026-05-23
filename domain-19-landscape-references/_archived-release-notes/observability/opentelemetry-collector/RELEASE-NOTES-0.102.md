---
title: opentelemetry-collector v0.102 Release Notes
description: opentelemetry-collector v0.102 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.102 Release Notes 是什么
- 如何 opentelemetry-collector v0.102 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.102
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

# opentelemetry-collector v0.102 Release Notes

Source: [v0.102.1](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.102.1)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.102.1

### This release addresses CVE-2024-36129 ([GHSA-c74f-6mfw-mm4v](https://github.com/open-telemetry/opentelemetry-collector/security/advisories/GHSA-c74f-6mfw-mm4v)) fully.

## End User Changelog

### 🧰 Bug fixes 🧰

- `configrpc`: Use own compressors for zstd (#10323)
   Before this change, the zstd compressor we used didn't respect the max message size. This addresses CVE-2024-36129 (GHSA-c74f-6mfw-mm4v) on `configgrpc`.