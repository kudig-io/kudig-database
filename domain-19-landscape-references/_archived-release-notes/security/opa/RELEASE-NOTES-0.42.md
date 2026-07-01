---
title: opentelemetry-collector v0.42 Release Notes
description: opentelemetry-collector v0.42 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.42 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.42 Release Notes 是什么
- 如何 opentelemetry-collector v0.42 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.42
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.42 Release Notes

Source: [v0.42.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.42.0)


## v0.42.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.42.0

## 🛑 Breaking changes 🛑

- Remove `configmapprovider.NewInMemory()` (#4507)
- Disallow direct implementation of `configmapprovider.Retrieved` (#4577)
- `configauth`: remove interceptor functions from the ServerAuthenticator interface (#4583)
- Replace ConfigMapProvider and ConfigUnmarshaler in collector settings by one simpler ConfigProvider (#4590)
- Remove deprecated consumererror.Combine (#4597)
- Remove `configmapprovider.NewDefault`, `configmapprovider.NewExpand`, `configmapprovider.NewMerge` (#4600)
  - The merge functionality is now embedded into `[[Service|service]].NewConfigProvider` (#4637).
- Move `configtest.LoadConfig` and `configtest.LoadConfigAndValidate` to `servicetest` (#4606)
- Builder: Remove deprecated `include-core` flag (#4616)
- Collector telemetry level must now be accessed through an atomic function. (#4549)

## 💡 Enhancements 💡

- `confighttp`: add client-side compression support. (#4441)
  - Each exporter should remove `compression` field if they have and should use `confighttp.HTTPClientSettings`
- Allow more zap logger configs: `disable_caller`, `disable_stacktrace`, `output_paths`, `error_output_paths`, `initial_fields` (#1048)
- Allow the custom zap logger encoding (#4532)
- Collector self-metrics may now be configured through the configuration file. (#4069)
  - CLI flags for configuring self-metrics are deprecated and will be removed
    in a future release.
  - `service.telemetry.metrics.level` and `service.telemetry.metrics.address`
    should be used to configure collector self-metrics.
- `configauth`: add helpers to create new server authenticators. (#4558)
- Refactor `configgrpc` for compression methods (#4624)
- Add an option to allow `config.Map` conversion in the `service.ConfigProvider` (#4634)
- Added support to expose [[gRPC|gRPC]] framework's logs as part of collector logs (#4501)
- Builder: Enable unmarshal exact to help finding hard to find typos #4644

## 🧰 Bug fixes 🧰

- Fix merge config map provider to close the watchers (#4570)
- Fix expand map provider to call close on the base provider (#4571)
- Fix correct the value of `otelcol_exporter_send_failed_requests` (#4629)
- `otlp` receiver: Fix legacy port cfg value override and HTTP server starting bug (#4631)
