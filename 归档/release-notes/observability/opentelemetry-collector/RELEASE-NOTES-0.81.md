---
title: opentelemetry-collector v0.81 Release Notes
description: opentelemetry-collector v0.81 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.81 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.81 Release Notes 是什么
- 如何 opentelemetry-collector v0.81 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.81
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




# opentelemetry-collector v0.81 Release Notes

Source: [v0.81.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.81.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.81.0

## v0.81.0

### 🛑 Breaking changes 🛑

- `[[Service|service]]`: Remove 'service.connectors' featuregate (#7952)

### 💡 Enhancements 💡

- `HTTPServerSettings`: Add zstd support to HTTPServerSettings (#7927)
  This adds ability to decompress zstd-compressed HTTP requests to| all receivers that use HTTPServerSettings.
- `cmd/builder`: Add "--skip-generate" option to make builder skip source generation (#7541)
- `confighttp`: Add support for additional content decoders via `WithDecoder` server option (#7977)
- `connectortest`: Add helpers to aid the construction of `connector.TracesRouter`, `connector.MetricsRouter`, and `connector.LogsRouter` instances to `connectortest`. (#7672)
- `confighttp`: Add `response_headers` configuration option on HTTPServerSettings. It allows for additional headers to be attached to each HTTP response sent to the client (#7328)
- `otlpreceiver, otlphttpexporter, otlpexporter, configgrpc`: Upgrade github.com/mostynb/go-grpc-compression and switch to nonclobbering imports (#7920)
  consumers of this library should not have their [[gRPC|grpc]] codecs overridden
- `otlphttpexporter`: Treat partial success responses as errors (#6686)

### 🧰 Bug fixes 🧰

- `HTTPServerSettings`: Ensure requests with unsupported Content-Encoding return HTTP 400 Bad Request (#7927)

<!-- risk-assessed -->
