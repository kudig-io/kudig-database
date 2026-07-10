---
title: opentelemetry-collector v0.136 Release Notes
description: opentelemetry-collector v0.136 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.136 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.136 Release Notes 是什么
- 如何 opentelemetry-collector v0.136 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.136
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




# opentelemetry-collector v0.136 Release Notes

Source: [v0.136.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.136.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.136.0

## End User Changelog

### 💡 Enhancements 💡

- `xpdata`: Add Serialization and Deserialization of AnyValue (#12826)
- `debugexporter`: add support for batching (#13791)
  The default queue size is 1
- `configtls`: Add early validation for TLS server configurations to fail fast when certificates are missing instead of failing at runtime. (#13130, #13245)
- `mdatagen`: Expose stability level in generated metric documentation (#13748)
- `internal/tools`: Add support for modernize in Makefile (#13796)

### 🧰 Bug fixes 🧰

- `otelcol`: Fix a potential deadlock during collector shutdown. (#13740)
- `otlpexporter`: fix the validation of unix socket endpoints (#13826)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: Remove deprecated function NewRequestsSizer (#13803)
- `pdata/pprofile`: Upgrade the OTLP protobuf definitions to version 1.8.0 (#13758, #13825, #13839)
- `pdata/pprofile`: Remove deprecated ProfilesDictionary method (#13858)

### 🚩 Deprecations 🚩

- `exporterhelper`: Deprecate all experimental symbols in exporterhelper and move them to xexporterhelper (#11143)

### 💡 Enhancements 💡

- `configoptional`: Add `GetOrInsertDefault` method to `configoptional.Optional` (#13856)
  This method inserts a default or zero value into a `None`/`Default` `Optional` before `Get`ting its inner value.
  
- `exporter`: Stabilize exporter module. (#12978)
  This does not stabilize the exporterhelper module or configuration
- `pdata`: Upgrade the OTLP protobuf definitions to version 1.8.0 (#13758)

<!-- previous-version -->


<!-- risk-assessed -->
