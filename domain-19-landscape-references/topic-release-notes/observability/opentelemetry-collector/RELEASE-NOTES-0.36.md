---
title: opentelemetry-collector v0.36 Release Notes
description: opentelemetry-collector v0.36 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.36 Release Notes 是什么
- 如何 opentelemetry-collector v0.36 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.36
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.36 Release Notes

Source: [v0.36.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.36.0)

## v0.36.0 Beta

## 🛑 Breaking changes 🛑

- Remove deprecated pdata.AttributeMapToMap (#3994)
- Move ValidateConfig from configcheck to configtest (#3956)
- Remove `mem-ballast-size-mib`, already deprecated and no-op (#4005)
- Remove `semconv.AttributeMessageType` (#4020)
- Remove `semconv.AttributeHTTPStatusText` (#4015)
- Remove squash on `configtls.TLSClientSetting` and move TLS client configs under `tls` (#4063)
- Rename TLS server config `*configtls.TLSServerSetting` from `tls_settings` to `tls` (#4063)
- Split `service.Collector` from the `cobra.Command` (#4074)
- Rename `memorylimiter` to `memorylimiterprocessor` (#4064)

## 💡 Enhancements 💡

- Create new semconv package for v1.6.1 (#3948)
- Add AttributeValueBytes support to AsString (#4002)
- Add AttributeValueTypeBytes support to AttributeMap.AsRaw (#4003)
- Add MeterProvider to TelemetrySettings (#4031)
- Add configuration to setup collector logs via config file. (#4009)