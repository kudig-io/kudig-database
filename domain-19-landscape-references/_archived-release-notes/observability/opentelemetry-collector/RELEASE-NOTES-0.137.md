---
title: opentelemetry-collector v0.137 Release Notes
description: opentelemetry-collector v0.137 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.137 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.137 Release Notes 是什么
- 如何 opentelemetry-collector v0.137 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.137
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.137 Release Notes

Source: [v0.137.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.137.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.137.0

## End User Changelog

### 💡 Enhancements 💡

- `cmd/mdatagen`: Improve validation for resource attribute `enabled` field in metadata files (#12722)
  Resource attributes now require an explicit `enabled` field in metadata.yaml files, while regular attributes
  are prohibited from having this field. This improves validation and prevents configuration errors.
  
- `all`: Changelog entries will now have their component field checked against a list of valid components. (#13924)
  This will ensure a more standardized changelog format which makes it easier to parse.
- `pkg/pdata`: Mark featuregate pdata.useCustomProtoEncoding as stable (#13883)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pkg/exporterhelper`: Remove all experimental symbols in exporterhelper (#11143)
  They have all been moved to xexporterhelper
  

### 🚩 Deprecations 🚩

- `all`: [[Service|service]]/telemetry.TracesConfig is deprecated (#13904)
  This type alias has been added to otelconftelemetry.TracesConfig,
  where the otelconf-based telemetry implementation now lives.
  

### 💡 Enhancements 💡

- `all`: Mark configoptional as stable (#13403)
- `all`: Mark configauth module as 1.0 (#9476)
- `pkg/pdata`: Mark featuregate pdata.useCustomProtoEncoding as stable (#13883)

<!-- previous-version -->
