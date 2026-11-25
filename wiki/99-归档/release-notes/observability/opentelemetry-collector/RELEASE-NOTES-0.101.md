---
title: opentelemetry-collector v0.101 Release Notes
description: opentelemetry-collector v0.101 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.101 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.101 Release Notes 是什么
- 如何 opentelemetry-collector v0.101 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.101
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




# opentelemetry-collector v0.101 Release Notes

Source: [v0.101.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.101.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.101.0

## End User Changelog

### 💡 Enhancements 💡

- `mdatagen`: generate documentation for internal telemetry (#10170)
- `mdatagen`: add ability to use metadata.yaml to automatically generate instruments for components (#10054)
  The `telemetry` section in metadata.yaml is used to generate
  instruments for components to measure telemetry about themselves.
  
- `confmap`: Allow Converters to write logs during startup (#10135)
- `otelcol`: Enable logging during configuration resolution (#10056)

### 🧰 Bug fixes 🧰

- `mdatagen`: Run package tests when goleak is skipped (#10125)

## Go API Changelog

### 🛑 Breaking changes 🛑

- `confighttp`: Removes deprecated functions `ToClientContext`, `ToListenerContext`, and `ToServerContext`. (#10138)
- `confmap`: Deprecate `NewWithSettings` for all Providers and `New` for all Converters (#10134)
  Use `NewFactory` instead for all affected modules.
- `confmap`: Remove deprecated `Providers` and `Converters` from `confmap.ResolverSettings` (#10173)
  Use `ProviderSettings` and `ConverterSettings` instead.

### 🧰 Bug fixes 🧰

- `otelcol`: Add explicit mapstructure tags to main configuration struct (#10152)
- `confmap`: Support string-like types as map keys when marshaling (#10137)


<!-- risk-assessed -->
