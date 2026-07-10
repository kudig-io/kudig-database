---
title: opentelemetry-collector v0.27 Release Notes
description: opentelemetry-collector v0.27 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- kafka
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.27 Release Notes 是什么
- 如何 opentelemetry-collector v0.27 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.27
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.27 Release Notes

Source: [v0.27.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.27.0)

# v0.27.0 Beta

## 🛑 Breaking changes 🛑

- Change `Marshal` signatures in kafkaexporter's Marshalers to directly convert pdata to `sarama.ProducerMessage` (#3162)
- Remove `tracetranslator.DetermineValueType`, only used internally by Zipkin (#3114)
- Remove OpenCensus conventions, should not be used (#3113)
- Remove Zipkin specific translation constants, move to internal (#3112)
- Remove `tracetranslator.TagHTTPStatusCode`, use `conventions.AttributeHTTPStatusCode` (#3111)
- Remove OpenCensus status constants and transformation (#3110)
- Remove `tracetranslator.AttributeArrayToSlice`, not used in core or contrib (#3109)
- Remove `internaldata.MetricsData`, same APIs as for traces (#3156)
- Rename `config.IDFromString` to `NewIDFromString`, remove `MustIDFromString` (#3177)
- Move consumerfanout package to internal (#3207)
- Canonicalize enum names in pdata. Fix usage of uppercase names (#3208)

## 💡 Enhancements 💡

- Use `config.ComponentID` for obsreport receiver/scraper (#3098)
- Add initial implementation of the consumerhelper (#3146)
- Add Collector version to [[Prometheus|Prometheus]] Remote Write Exporter user-agent header (#3094)
- Refactor processorhelper to use consumerhelper, split by signal type (#3180)
- Use consumerhelper for exporterhelper, add WithCapabilities (#3186)
- Set capabilities for all core exporters, remove unnecessary funcs (#3190)
- Add an internal sharedcomponent to be shared by receivers with shared resources (#3198)
- Allow users to configure the Prometheus remote write queue (#3046)
- Mark internaldata traces translation as deprecated for external usage (#3176)

## 🧰 Bug fixes 🧰

- Fix Prometheus receiver metric start time and reset determination logic. (#3047)
  - The receiver will no longer drop the first sample for `counter`, `summary`, and `histogram` metrics.
- The Prometheus remote write exporter will no longer force `counter` metrics to have a `_total` suffix. (#2993)
- Remove locking from [[Jaeger|jaeger]] receiver start and stop processes (#3070)
- Fix batch processor metrics reorder, improve performance (#3034)
- Fix batch processor traces reorder, improve performance (#3107)
- Fix batch processor logs reorder, improve performance (#3125)
- Avoid one unnecessary allocation in [[gRPC|grpc]] OTLP exporter (#3122)
- `batch` processor: Validate that batch config max size is greater than send size (#3126)
- Add capabilities to consumer, remove from processor (#2770)
- Remove internal protos usage in Prometheusremotewrite exporter (#3184)
- `prometheus` receiver: Honor Prometheus external labels (#3127)
- Validate that remote write queue settings are not negative (#3213)

<!-- risk-assessed -->
