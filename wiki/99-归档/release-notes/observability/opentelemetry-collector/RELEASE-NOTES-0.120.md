---
title: opentelemetry-collector v0.120 Release Notes
description: opentelemetry-collector v0.120 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.120 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.120 Release Notes 是什么
- 如何 opentelemetry-collector v0.120 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.120
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




# opentelemetry-collector v0.120 Release Notes

Source: [v0.120.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.120.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.120.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `all`: Added support for go1.24, bumped minimum version to 1.23 (#12370)
- `mdatagen`: Removing deprecated generated funcs and a few test funcs as well. (#12304)
- `[[Service|service]]`: Align component logger attributes with those defined in RFC (#12217)
  See [Pipeline Component Telemetry RFC](https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/component-universal-telemetry.md#attributes)
  

### 💡 Enhancements 💡

- `otlpreceiver`: Update stability for logs (#12335)
- `exporterhelper`: Implement sync disabled queue used when batching is enabled. (#12245)
- `exporterhelper`: Enable the new pull-based batcher in exporterhelper (#12291)
- `exporterhelper`: Update queue size after the element is done exported (#12399)
  After this change the active queue size will include elements in the process of being exported.
- `otelcol`: Add featuregate command to display information about available features (#11998)
  The featuregate command allows users to view detailed information about feature gates
  including their status, stage, and description.
  

### 🧰 Bug fixes 🧰

- `memorylimiter`: Logger no longer attributes to single signal, pipeline, or component. (#12217)
- `otlpreceiver`: Logger no longer attributes to random signal when receiving multiple signals. (#12217)
- `exporterhelper`: Fix undefined behavior access to request after send to next component. This causes random memory access. (#12281)
- `exporterhelper`: Fix default batcher to correctly call all done callbacks exactly once (#12247)
- `otlpreceiver`: Fix OTLP http receiver to correctly set Retry-After (#12367)
- `otlphttpexporter`: Fix parsing logic for Retry-After in OTLP http protocol. (#12366)
  The value of Retry-After field can be either an HTTP-date or delay-seconds and the current logic only parsed delay-seconds.
- `cmd/builder`: Ensure unique aliases for modules with same suffix (#12201)

## API Changelog

### 🛑 Breaking changes 🛑

- `configauth`: Remove NewDefaultAuthentication (#12223)
  The value returned by this function will always cause an error on startup.
  In `configgrpc.Client/ServerConfig.Auth`, `nil` should be used instead to disable authentication.
  
- `otelcol`: Make the `ConfigProvider` interface a struct (#12297)
  Calls to `NewConfigProvider` will now return `*ConfigProvider`,
  but will otherwise work the same as before.
  
- `extension`: Remove `extension.Settings.ModuleInfo` (#12296)
  - The functionality is now available as an optional, hidden interface on `service`'s implementation of the `Host`
  
- `component`: Remove deprecated field `component.TelemetrySettings.MetricsLevel`. (#11061)
- `confighttp`: Add `ToClientOption` type and add it to signature of `ToClient` method. (#12353)
  - This has no use for now, it may be used in the future.
  
- `mdatagen`: Remove unused not_component config for mdatagen (#12237)

### 🚩 Deprecations 🚩

- `component/componenttest`: Deprecate CheckReceiverMetrics in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckReceiverMetrics`
- `component/componenttest`: Deprecate CheckReceiverTraces in componenenttest (#12185)
  Use the `metadatatest.AssertEqualMetric` series of functions instead of `obsreporttest.CheckReceiverTraces`
- `component`: Deprecate `ConfigValidator` and `ValidateConfig` (#11524)
  Please use `Validator` and `Validate` respectively from `xconfmap`.
- `receiver, scraper, processor, exporter, extension`: Deprecate existing MakeFactoryMap functions in favor of generic implementation (#12222)
- `extension, connector, processor, receiver, exporter, scraper`: Deprecate `Create*` methods from `Create*Func` types. (#12305)
- `extensiontest, connectortest, processortest, receivertest, exportertest, scrapertest`: Deprecate `*test.NewNopSettings` in favor of `*test.NewNopSettingsWithType` (#12305)

### 🚀 New components 🚀

- `xconfmap`: Create the xconfmap module and add the `Validator` interface and `Validate` function to facilitate config validation (#11524)

### 💡 Enhancements 💡

- `configgrpc`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.
- `confignet`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.
- `configtls`: Add the `omitempty` mapstructure tag to struct fields (#12191)
  This results in unset fields not being rendered when marshaling.
- `consumer`: Clarify that data cannot be accessed after Consume* func is called. (#12284)
- `pdata/pprofile`: Introduce aggregation temporality constants (#12253)

### 🧰 Bug fixes 🧰

- `configgrpc`: Apply configured Headers automatically (#12307)
  configgrpc now calls metadata.AppendToOutgoingContext automatically in an interceptor.
  Components that were manually using metadata.NewOutgoingContext as a workaround no longer need to
  do so, unless they are overwriting or adding header keys.
  
- `configgrpc`: Set Auth to nil in NewDefaultClientConfig/NewDefaultServerConfig (#12223)
  The value that was used previously would always cause an error on startup.
  
- `exporterqueue`: Fix async queue to propagate cancellation all they way to the queue (#12282)
- `otlpreceiver`: Fix OTLP http receiver to correctly set Retry-After (#12367)
- `extension`: Explicitly error out at extension creation time if there is a type mismatch. (#12305)

<!-- risk-assessed -->
