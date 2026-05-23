---
title: opentelemetry-collector v0.44 Release Notes
description: opentelemetry-collector v0.44 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.44 Release Notes 是什么
- 如何 opentelemetry-collector v0.44 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.44
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

# opentelemetry-collector v0.44 Release Notes

Source: [v0.44.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.44.0)

## v0.44.0 Beta

## 🛑 Breaking changes 🛑

- Deprecate `[[Service|service]].NewConfigProvider`, and a new version `service.MustNewConfigProvider` (#4734).
- Updated to OTLP 0.12.0. Deprecated traces and metrics messages that existed
  in 0.11.0 are no longer converted to the messages and fields that replaced the deprecated ones.
  Received deprecated messages and fields will be now ignored. In OTLP/JSON in the
  instrumentationLibraryLogs object the "logs" field is now named "logRecords" (#4724)

## 💡 Enhancements 💡

- Invalid requests now return an appropriate unsupported (`405`) or method not allowed (`415`) response (#4735)
- `client.Info`: Add Host property for Metadata (#4736)