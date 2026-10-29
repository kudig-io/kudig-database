---
title: opentelemetry-collector v0.16 Release Notes
description: opentelemetry-collector v0.16 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- opentelemetry-collector v0.16 Release Notes 是什么
- 如何 opentelemetry-collector v0.16 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.16 Release Notes

Source: [v0.16.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.16.0)

# v0.16.0 Beta

## 🛑 Breaking changes 🛑

- Rename Push functions to be consistent across signals in `exporterhelper` (#2203)

## 💡 Enhancements 💡

- Change default OTLP/gRPC port number to 4317, also continue receiving on legacy port
  55680 during transition period (#2104).
- `kafka` exporter: Add support for exporting metrics as otlp Protobuf. (#1966)
- Move scraper helpers to its own `scraperhelper` package (#2185)
- Add `componenthelper` package to help build components (#2186)
- Remove usage of custom init/stop in `scraper` and use start/shutdown from `component` (#2193)
- Add more trace annotations, so zpages are more useful to determine failures (#2206)
- Add support to skip TLS verification (#2202)
- Expose non-nullable metric types (#2208)
- Expose non-nullable elements from slices of pointers (#2200)

## 🧰 Bug fixes 🧰

- Change InstrumentationLibrary to be non-nullable (#2196)
- Add support for slices to non-pointers, use non-nullable AnyValue (#2192)
- Fix `--set` flag to work with `{}` in configs (#2162)



<!-- risk-assessed -->
