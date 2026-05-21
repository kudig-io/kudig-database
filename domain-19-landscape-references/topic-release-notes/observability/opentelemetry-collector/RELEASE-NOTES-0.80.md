---
title: opentelemetry-collector v0.80 Release Notes
description: opentelemetry-collector v0.80 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.80 Release Notes 是什么
- 如何 opentelemetry-collector v0.80 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.80
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

# opentelemetry-collector v0.80 Release Notes

Source: [v0.80.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.80.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.80.0

## v1.0.0-rcv0013/v0.80.0

### 🚩 Deprecations 🚩

- `service`: Deprecate service.PipelineConfig in favor of pipelines.Config. (#7854)

### 💡 Enhancements 💡

- `service`: Added dry run flag to validate config file without running collector. (#4671)
- `configtls`: Allow TLS Settings to be provided in memory in addition to filepath. (#7313)
- `connector`: Updates the way connector nodes are built to always pass a fanoutconsumer to their factory functions. (#7672, #7673)
- `otlp`: update otlp protos to v0.20.0 (#7839)
- `configauth`: Split config/configauth into its own module (#7895)
- `configgrpc, confighttp, config/internal`: Split confighttp, configgrpc, and config/internal into separate modules (#7895)
- `confignet`: Split config/confignet into its own module (#7895)
- `configopaque`: Split config/configopaque into its own module (#7895)
- `configtelemetry`: Split config/configtelemetry into its own module (#7895)
- `configtls`: Split config/configtls into its own module (#7895)
- `configcompression`: Split config/configcompression into its own module (#7895)
- `extension`: Splitting `extension/auth` into separate module (#7054)
- `connector`: Split connector into its own module (#7895)
- `extension`: split extension module into its own module (#7306)
- `processor`: Split the processor into its own go module (#7307)
- `confighttp`: Avoid re-creating the compressors for every request. (#7859)
- `otlpexporter`: Treat partial success responses as errors (#6686)
- `service/pipelines`: Add pipelines.Config to remove duplicate of the pipelines configuration (#7854)
