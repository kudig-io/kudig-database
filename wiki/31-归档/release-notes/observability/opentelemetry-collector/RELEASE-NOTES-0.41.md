---
title: opentelemetry-collector v0.41 Release Notes
description: opentelemetry-collector v0.41 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.41 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.41 Release Notes 是什么
- 如何 opentelemetry-collector v0.41 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.41
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




# opentelemetry-collector v0.41 Release Notes

Source: [v0.41.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.41.0)

## 🛑 Breaking changes 🛑

- Remove reference to `defaultcomponents` in core and deprecate `include_core` flag (#4087)
- Remove `config.NewConfigMapFrom[File|Buffer]`, add testonly version (#4502)
- `configtls`: TLS 1.2 is the new default mininum version (#4503)
- `confighttp`: `ToServer` now accepts a `component.Host`, in line with [[gRPC|gRPC]]'s counterpart (#4514)
- CORS configuration for OTLP/HTTP receivers has been moved into a `cors:` block, instead of individual `cors_allowed_origins` and `cors_allowed_headers` settings (#4492)

## 💡 Enhancements 💡

- OTLP/HTTP receivers now support setting the `Access-Control-Max-Age` header for CORS caching. (#4492)
- `client.Info` pre-populated for all receivers using common helpers like `confighttp` and `configgrpc` (#4423)

## 🧰 Bug fixes 🧰

- Fix handling of corrupted records by persistent buffer (experimental) (#4475)

Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.41.0

<!-- risk-assessed -->
