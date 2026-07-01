---
title: opentelemetry-collector v0.33 Release Notes
description: opentelemetry-collector v0.33 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.33 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.33 Release Notes 是什么
- 如何 opentelemetry-collector v0.33 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.33
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.33 Release Notes

Source: [v0.33.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.33.0)

## v0.33.0 Beta

## 🛑 Breaking changes 🛑

- Rename `configloader` interface to `configunmarshaler` (#3774)
- Remove `LabelsMap` from all the metrics points (#3706)
- Update generated K8S attribute labels to fix capitalization (#3823) 

## 💡 Enhancements 💡

- Collector has now full support for metrics proto v0.9.0.
