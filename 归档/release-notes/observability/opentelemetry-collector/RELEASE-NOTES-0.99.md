---
title: opentelemetry-collector v0.99 Release Notes
description: opentelemetry-collector v0.99 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.99 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.99 Release Notes 是什么
- 如何 opentelemetry-collector v0.99 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.99
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.99 Release Notes

Source: [v0.99.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.99.0)

## End User Changelog

### 🛑 Breaking changes 🛑

- `builder`: Add strict version checking when using the builder. Add the temporary flag `--skip-strict-versioning `for skipping this check. (#9896)
  Strict version checking will error on major and minor version mismatches 
  between the `otelcol_version` configured and the builder version or versions 
  in the go.mod. This check can be temporarily disabled by using the `--skip-strict-versioning` 
  flag. This flag will be removed in a future minor version.
  
- `telemetry`: Distributed internal metrics across different levels. (#7890)
  The internal metrics levels are updated along with reported metrics:
  - The default level is changed from `basic` to `normal`, which can be overridden with `[[Service|service]]::telmetry::metrics::level` configuration.
  - Batch processor metrics are updated to be reported starting from `normal` level:
    - `processor_batch_batch_send_size` 
    - `processor_batch_metadata_cardinality`
    - `processor_batch_timeout_trigger_send`
    - `processor_batch_size_trigger_send`
  - [[gRPC|GRPC]]/HTTP server and client metrics are updated to be reported starting from `detailed` level:
    - http.client.* metrics
    - http.server.* metrics
    - rpc.server.* metrics
    - rpc.client.* metrics
  

### 💡 Enhancements 💡

- `confighttp`: Disable concurrency in zstd compression (#8216)
- `cmd/builder`: Allow configuring `confmap.Provider`s in the builder. (#4759)
  If no providers are specified, the defaults are used.
  The default providers are: env, file, http, https, and yaml.
  
  To configure providers, use the `providers` key in your OCB build
  manifest with a list of Go modules for your providers.
  The modules will work the same as other Collector components.
  
- `mdatagen`: enable goleak tests by default via mdatagen (#9959)
- `cmd/mdatagen`: support excluding some metrics based on string and regexes in resource_attributes (#9661)
- `cmd/mdatagen`: Generate config and factory tests covering their requirements. (#9940)
  The tests are moved from cmd/builder.
  
- `confmap`: Add `ProviderSettings`, `ConverterSettings`, `ProviderFactories`, and `ConverterFactories` fields to `confmap.ResolverSettings` (#9516)
  This allows configuring providers and converters, which are instantiated by `NewResolver` using the given factories.

### 🧰 Bug fixes 🧰

- `exporter/otlp`: Allow DNS scheme to be used in endpoint (#4274)
- `service`: fix record sampler configuration (#9968)
- `service`: ensure the tracer provider is configured via go.opentelemetry.io/contrib/config (#9967)
- `otlphttpexporter`: Fixes a bug that was preventing the otlp http exporter from propagating status. (#9892)
- `confmap`: Fix decoding negative configuration values into uints (#9060)

## API Changelog

### 🛑 Breaking changes 🛑

- `component`: Removed deprecated function `GetExporters` from `component.Host` interface (#9987)

### 🚩 Deprecations 🚩

- `confighttp`: deprecate ToClientContext, ToServerContext, ToListenerContext, replaced by ToClient, ToServer, ToListener (#9807)
- `configtls`: Deprecates `ClientConfig.LoadTLSConfigContext` and `ServerConfig.LoadTLSConfigContext`, use `ClientConfig.LoadTLSConfig` and `ServerConfig.LoadTLSConfig` instead. (#9945)
- `confmap`: Deprecate the `Providers` and `Converters` fields in `confmap.ResolverSettings` (#9516)
  Use the `ProviderFactories` and `ConverterFactories` fields instead.

### 💡 Enhancements 💡

- `configauth`: Adds `NewDefault*` functions for all the config structs. (#9821)
- `configtls`: Adds `NewDefault*` functions for all the config structs. (#9658)
- `pmetric`: Support metric.metadata in pdata/pmetric (#10006)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.99.0

<!-- risk-assessed -->
