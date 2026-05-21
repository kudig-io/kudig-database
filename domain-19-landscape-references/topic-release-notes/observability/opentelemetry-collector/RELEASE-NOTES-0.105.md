---
title: opentelemetry-collector v0.105 Release Notes
description: opentelemetry-collector v0.105 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.105 Release Notes 是什么
- 如何 opentelemetry-collector v0.105 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.105
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.105 Release Notes

Source: [v0.105.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.105.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.105.0

## End User Changelog

## v1.12.0/v0.105.0

### 🛑 Breaking changes 🛑

- `service`: add `service.disableOpenCensusBridge` feature gate which is enabled by default to remove the dependency on OpenCensus (#10414)
- `confmap`: Promote `confmap.strictlyTypedInput` feature gate to beta. (#10552)
  This feature gate changes the following:
  - Configurations relying on the implicit type casting behaviors listed on [#9532](https://github.com/open-telemetry/opentelemetry-collector/issues/9532) will start to fail.
  - Configurations using URI expansion (i.e. `field: ${env:ENV}`) for string-typed fields will use the value passed in `ENV` verbatim without intermediate type casting.
  

### 💡 Enhancements 💡

- `configtls`: Mark module as stable. (#9377)
- `confmap`: Remove extra closing parenthesis in sub-config error (#10480)
- `configgrpc`: Update the default load balancer strategy to round_robin (#10319)
  To restore the behavior that was previously the default, set `balancer_name` to `pick_first`.
- `cmd/builder`: Add go module info the builder generated code. (#10570)
- `otelcol`: Add go module to components subcommand. (#10570)
- `confmap`: Add explanation to errors related to `confmap.strictlyTypedInput` feature gate. (#9532)
- `confmap`: Allow using `map[string]any` values in string interpolation (#10605)

### 🧰 Bug fixes 🧰

- `builder`: provide context when a module in the config is missing its gomod value (#10474)
- `confmap`: Fixes issue where confmap could not escape `$$` when `confmap.unifyEnvVarExpansion` is enabled. (#10560)
- `mdatagen`: fix generated comp test for extensions and unused imports in templates (#10477)
- `otlpreceiver`: Fixes a bug where the otlp receiver's http response was not properly translating grpc error codes to http status codes. (#10574)
- `exporterhelper`: Fix incorrect deduplication of otelcol_exporter_queue_size and otelcol_exporter_queue_capacity metrics if multiple exporters are used. (#10444)
- `service/telemetry`: Add ability to set service.name for spans emitted by the Collector (#10489)
- `internal/localhostgate`: Correctly log info message when `component.UseLocalHostAsDefaultHost` is enabled (#8510)

## Go API Changelog

## v1.12.0/v0.105.0

### 🛑 Breaking changes 🛑

- `otelcol`: Obtain the Collector's effective config from otelcol.Config (#10139)
  `otelcol.Collector` will now marshal `confmap.Conf` objects from `otelcol.Config` itself.
- `otelcoltest`: Remove deprecated methods `LoadConfigWithSettings` and `LoadConfigAndValidateWithSettings` (#10512)

### 🚩 Deprecations 🚩

- `configauth`: Deprecated `Authentication.GetClientAuthenticatorContext` and `Authentication.GetServerAuthenticatorContext` (#10578)
- `otelcol`: Deprecate `otelcol.ConfmapProvider` (#10139)
  `otelcol.Collector` will now marshal `confmap.Conf` objects from `otelcol.Config` itself.
- `otelcol`: Deprecate `(*otelcol.ConfigProvider).GetConfmap` (#10139)
  Call `(*confmap.Conf).Marshal(*otelcol.Config)` to get the Collector's configuration.
- `exporterhelper`: Deprecate the obsreport API in the exporterhelper package. (#10592)

### 🚀 New components 🚀

- `consumer/consumerprofiles`: Allow handling profiles in consumer. (#10464)
