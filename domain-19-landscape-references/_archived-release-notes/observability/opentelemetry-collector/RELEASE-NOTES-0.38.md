---
title: opentelemetry-collector v0.38 Release Notes
description: opentelemetry-collector v0.38 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.38 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.38 Release Notes 是什么
- 如何 opentelemetry-collector v0.38 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.38
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.38 Release Notes

Source: [v0.38.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.38.0)

## 🛑 Breaking changes 🛑

- Removed `configauth.HTTPClientAuthenticator` and `configauth.GRPCClientAuthenticator` in favor of `configauth.ClientAuthenticator`. (#4255)
- Rename `parserprovider.MapProvider` as `config.MapProvider`. (#4178)
- Rename `parserprovider.Watchable` as `config.WatchableMapProvider`. (#4178)
- Remove deprecated no-op flags to setup Collector's logging "--log-level", "--log-profile", "--log-format". (#4213)
- Move `cmd/pdatagen` as internal package `model/internal/cmd/pdatagen`. (#4243)
- Use directly the ComponentID in configauth. (#4238)
- Refactor configauth, getters use the map instead of iteration. (#4234)
- Change scraperhelper to follow the recommended append model for pdata. (#4202)

## 💡 Enhancements 💡

- Update proto to 0.11.0. (#4209)
- Change pdata to use the newly added [Traces|Metrics|Logs]Data. (#4214)
- Add ExponentialHistogram field to pdata. (#4219)
- Make sure otlphttp exporter tests include TraceID and SpanID. (#4268)
- Use multimod tool in release process. (#4229)
- Change queue metrics to use opencensus metrics instead of stats, close to otel-go. (#4220)
- Make receiver data delivery guarantees explicit (#4262)
- Simplify unmarshal logic by adding more supported hooks. (#4237)
- Add unmarshaler for otlpgrpc.[*]Request and otlpgrp.[*]Response (#4215)