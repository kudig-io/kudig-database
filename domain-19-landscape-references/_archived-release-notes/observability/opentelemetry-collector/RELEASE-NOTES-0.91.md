---
title: opentelemetry-collector v0.91 Release Notes
description: opentelemetry-collector v0.91 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.91 Release Notes 是什么
- 如何 opentelemetry-collector v0.91 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.91
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.91 Release Notes

Source: [v0.91.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.91.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.91.0

## v0.91.0

### 💡 Enhancements 💡

- `statusreporting`: Automates status reporting upon the completion of component.Start(). (#7682)
- `[[Service|service]]`: add resource attributes as labels to otel metrics to ensures backwards compatibility with OpenCensus metrics. (#9029)
- `semconv`: Generated Semantic conventions 1.21. (#9056)
- `config/confighttp`: Exposes http/2 transport settings to enable health check and workaround golang http/2 issue https://github.com/golang/go/issues/59690 (#9022)
- `cmd/builder`: running builder version on binaries installed with `go install` will output the version specified at the suffix. (#8770)

### 🧰 Bug fixes 🧰

- `exporterhelper`: fix missed metric aggregations (#9048)
  This ensures that context cancellation in the exporter doesn't interfere with metric aggregation. The OTel
  SDK currently returns if there's an error in the context used in `Add`. This means that if there's a
  cancelled context in an export, the metrics are now recorded.
  
- `service`: Fix bug where MutatesData would not correctly propagate through connectors. (#9053)