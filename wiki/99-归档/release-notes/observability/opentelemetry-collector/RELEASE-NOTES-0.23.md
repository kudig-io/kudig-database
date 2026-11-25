---
title: opentelemetry-collector v0.23 Release Notes
description: opentelemetry-collector v0.23 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- docker
- opa
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.23 Release Notes 是什么
- 如何 opentelemetry-collector v0.23 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- policy-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.23 Release Notes

Source: [v0.23.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.23.0)

# v0.23.0 Beta

## 🛑 Breaking changes 🛑

- Move fanout consumers to fanoutconsumer package (#2615)
- Rename ExporterObsReport to Exporter (#2658)
- Rename ProcessorObsReport to Processor (#2657)
- Remove ValidateConfig and add Validate on the Config struct (#2665)
- Rename pdata Size to OtlpProtoSize (#2726)
- Rename [Traces|Metrics|Logs]Consumer to [Traces|Metrics|Logs] (#2761)
- Remove public access for `componenttest.Example*` components:
  - Users of these structs for testing configs should use the newly added `componenttest.Nop*` (update all components name in the config `example*` -> `nop` and use `componenttest.NopComponents()`).
  - Users of these structs for sink like behavior should use `consumertest.*Sink`.

## 💡 Enhancements 💡

- `hostmetrics` receiver: List labels along with respective metrics in metadata (#2662)
- `exporter` helper: Remove obsreport.ExporterContext, always add exporter name as a tag to the metrics (#2682)
- `[[Jaeger|jaeger]]` exporter: Change to not use internal data (#2698)
- `kafka` receiver: Change to not use internal data (#2697)
- `zipkin` receiver: Change to not use internal data (#2699)
- `kafka` exporter: Change to not use internal data (#2696)
- Ensure that extensions can be created and started multiple times (#2679)
- Use otlp request in logs wrapper, hide members in the wrapper (#2692)
- Add MetricsWrapper to dissallow access to internal representation (#2693)
- Add TracesWrapper to dissallow access to internal representation (#2721)
- Allow multiple OTLP receivers to be created (#2743)

## 🧰 Bug fixes 🧰

- `[[Prometheus|prometheus]]` exporter: Fix to work with standard labels that follow the naming convention of using periods instead of underscores (#2707)
- Propagate name and transport for `prometheus` receiver and exporter (#2680)
- `zipkin` receiver: Ensure shutdown correctness (#2765)


## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.


<!-- risk-assessed -->
