---
title: opentelemetry-collector v0.70 Release Notes
description: opentelemetry-collector v0.70 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.70 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
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
- opentelemetry-collector v0.70 Release Notes 是什么
- 如何 opentelemetry-collector v0.70 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.70
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




# opentelemetry-collector v0.70 Release Notes

Source: [v0.70.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.70.0)

### Collector images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.70.0

### Collector builder binaries here: https://github.com/open-telemetry/opentelemetry-collector/releases/tag/cmd%2Fbuilder%2Fv0.70.0

## v1.0.0-RC4/v0.70.0

### 🛑 Breaking changes 🛑

- `pdata`: Start enforcing [[gRPC|grpc]] server implementation to embed UnimplementedGRPCServer, dissallow client implementation (#6966)
- `config/configgrpc`: Change configgrpc.GRPCClientSettings.Headers type to map[string]configopaque.String (#6852)
  Use `configopaque.String(str)` and `string(opaque)` to turn a string opaque/clear.
- `pdata`: Remove deprecated pcommon.Value.Equal (#6860)

### 🚩 Deprecations 🚩

- `pdata`: Deprecate pcommon.Map.Sort(). (#6688)
- `featuregate`: Deprecate GetRegistry in favor of GlobalRegistry (#6979)

### 💡 Enhancements 💡

- `builder`: Add remote debug option for otel-collector to builder (#6149)
- `connector`: Add Builder (#6867)
- `cmd/builder`: Add support for connector configurations (#6789)
- `exporter/otlphttp`: Retry only on status code 429/502/503/504 (#6845)
- `featuregate`: Reduce contention in featuregate by using sync.Map instead of mutex. (#6980)

### 🧰 Bug fixes 🧰

- `loggingexporter`: Fix undefined symbol errors on building otelcorecol for other platforms than darwin, linux, windows. (#6924)
- `otlpexporter`: Fix a dataloss bug in persistent storage when collector shuts down or restarts (#6771)

<!-- risk-assessed -->
