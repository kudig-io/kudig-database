---
title: opentelemetry-collector v0.144 Release Notes
description: opentelemetry-collector v0.144 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.144 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.144 Release Notes 是什么
- 如何 opentelemetry-collector v0.144 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.144
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.144 Release Notes

Source: [v0.144.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.144.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.144.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `pkg/exporterhelper`: Change verbosity level for otelcol_exporter_queue_batch_send_size metric to detailed. (#14278)
- `pkg/service`: Remove deprecated `telemetry.disableHighCardinalityMetrics` feature gate. (#14373)
- `pkg/service`: Remove deprecated `[[Service|service]].noopTracerProvider` feature gate. (#14374)

### 🚩 Deprecations 🚩

- `exporter/otlp_grpc`: Rename `otlp` exporter to `otlp_grpc` exporter and add deprecated alias `otlp`. (#14403)
- `exporter/otlp_http`: Rename `otlphttp` exporter to `otlp_http` exporter and add deprecated alias `otlphttp`. (#14396)

### 💡 Enhancements 💡

- `cmd/builder`: Avoid duplicate CLI error logging in generated collector binaries by relying on cobra's error handling. (#14317)
- `cmd/mdatagen`: Add the ability to disable attributes at the metric level and re-aggregate data points based off of these new dimensions (#10726)
- `cmd/mdatagen`: Add optional `display_name` and `description` fields to metadata.yaml for human-readable component names (#14114)
  The `display_name` field allows components to specify a human-readable name in metadata.yaml.
  When provided, this name is used as the title in generated README files.
  The `description` field allows components to include a brief description in generated README files.
  
- `cmd/mdatagen`: Validate stability level for entities (#14425)
- `pkg/xexporterhelper`: Reenable batching for profiles (#14313)
- `receiver/nop`: add profiles signal support (#14253)

### 🧰 Bug fixes 🧰

- `pkg/exporterhelper`: Fix reference count bug in partition batcher (#14444)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pkg/config/confighttp`: Replace `ServerConfig.Endpoint` with `NetAddr confignet.AddrConfig`, enabling more flexible transport configuration. (#14187, #8752)
  This change adds "transport" as a configuration option, allowing users to specify
  different transport protocols (e.g., "tcp", "unix").
  

### 🚩 Deprecations 🚩

- `pkg/scraperhelper`: Deprecate the `AddScraper` method. (#14428)

### 🚀 New components 🚀

- `pkg/xscraperhelper`: Add xscraperhelper for the experimental OTel profiling signal. (#14235)

### 💡 Enhancements 💡

- `all`: Add support for deprecated component type aliases (#14208)
  To add a deprecated type alias to a component factory, use the `WithDeprecatedTypeAlias` option.
  ```go
  return xexporter.NewFactory(
      metadata.Type,
      createDefaultConfig,
      xexporter.WithTraces(createTracesExporter, metadata.TracesStability),
      xexporter.WithDeprecatedTypeAlias("old_component_name"),
  )
  ```
  When the alias is used in configuration, a deprecation warning will be automatically logged, and the component will function normally using the original implementation.
  
- `cmd/mdatagen`: Add the ability to disable attributes at the metric level and re-aggregate data points based off of these new dimensions (#10726)
- `extension/xextension`: Add deprecated type alias support for extensions via `xextension` module (#14208)
  Extensions can now register deprecated type aliases using the experimental `xextension.WithDeprecatedTypeAlias` option.
  ```go
  return xextension.NewFactory(
      metadata.Type,
      createDefaultConfig,
      createExtension,
      metadata.Stability,
      xextension.WithDeprecatedTypeAlias("old_extension_name"),
  )
  ```
  When the alias is used in configuration, a deprecation warning will be automatically logged, and the extension will function normally using the original implementation.
  
- `pkg/consumer/consumertest`: Add ProfileCount() (#14251)
- `pkg/exporterhelper`: Add support for profile samples metrics (#14423)
- `pkg/receiverhelper`: Add support for profile samples metrics (#14226)
- `pkg/scraperhelper`: Introduce `AddMetricsScraper` to be more explicit than `AddScraper`. (#14428)
- `receiver/otlp`: Add metrics tracking the number of receiver, refused and failed profile samples (#14226)

### 🧰 Bug fixes 🧰

- `pkg/xconnector`: Add component ID type validation to all xconnector Create methods (#14357)

<!-- previous-version -->


<!-- risk-assessed -->
