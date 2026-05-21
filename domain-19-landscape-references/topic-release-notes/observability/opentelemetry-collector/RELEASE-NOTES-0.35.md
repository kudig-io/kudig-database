---
title: opentelemetry-collector v0.35 Release Notes
description: opentelemetry-collector v0.35 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.35 Release Notes 是什么
- 如何 opentelemetry-collector v0.35 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.35 Release Notes

Source: [v0.35.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.35.0)

## v0.35.0 Beta

## 🛑 Breaking changes 🛑

- Remove the legacy gRPC port(`55680`) support in OTLP receiver (#3966)
- Rename configparser.Parser to configparser.ConfigMap (#3964)
- Remove obsreport.ScraperContext, embed into StartMetricsOp (#3969)
- Remove dependency on deprecated go.opentelemetry.io/otel/oteltest (#3979)
- Remove deprecated pdata.AttributeValueToString (#3953)
- Remove deprecated pdata.TimestampFromTime. Closes: #3925 (#3935)

## 💡 Enhancements 💡

- Add TelemetryCreateSettings (#3984)
- Only initialize collector telemetry once (#3918)
- Add trace context info to LogRecord log (#3959)
- Add new view for AWS ECS health check extension. (#3776)