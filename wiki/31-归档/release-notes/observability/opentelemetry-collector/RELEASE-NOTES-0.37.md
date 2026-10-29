---
title: opentelemetry-collector v0.37 Release Notes
description: opentelemetry-collector v0.37 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.37 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.37 Release Notes 是什么
- 如何 opentelemetry-collector v0.37 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.37
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




# opentelemetry-collector v0.37 Release Notes

Source: [v0.37.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.37.0)

## v0.37.0 Beta

## 🛑 Breaking changes 🛑

- Move `configcheck.ValidateConfigFromFactories` as internal function in [[Service|service]] package (#3876)
- Rename `configparser.Parser` as `config.Map` (#4075)
- Rename `component.DefaultBuildInfo()` to `component.NewDefaultBuildInfo()` (#4129)
- Rename `consumererror.Permanent` to `consumererror.NewPermanent` (#4118)
- Rename `config.NewID` to `config.NewComponentID` and `config.NewIDFromString` to `config.NewComponentIDFromString` (#4137)
- Rename `config.NewIDWithName` to `config.NewComponentIDWithName` (#4151)
- Move `extension/storage` to `extension/experimental/storage` (#4082)
- Rename `obsreporttest.SetupRecordedMetricsTest()` to `obsreporttest.SetupTelemetry()` and `obsreporttest.TestTelemetrySettings` to `obsreporttest.TestTelemetry` (#4157)

## 💡 Enhancements 💡

- Add Gen dependabot into CI (#4083)
- Update OTLP to v0.10.0 (#4045).
- Add Flags field to NumberDataPoint, HistogramDataPoint, SummaryDataPoint (#4081).
- Add feature gate library (#4108)
- Add version to the internal telemetry metrics (#4140)

## 🧰 Bug fixes 🧰

- Fix panic when not using `service.NewCommand` (#4139)

<!-- risk-assessed -->
