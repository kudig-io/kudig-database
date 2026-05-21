---
title: opentelemetry-collector v0.47 Release Notes
description: opentelemetry-collector v0.47 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.47 Release Notes 是什么
- 如何 opentelemetry-collector v0.47 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.47
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.47 Release Notes

Source: [v0.47.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.47.0)

## v0.47.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.47.0

### 🛑 Breaking changes 🛑

- Remove `Type` funcs in pdata (#4933)
- pdata: deprecate funcs working with InternalRep (#4957)
- Remove all deprecated funcs/structs from v0.46.0 (#4995)

### 🚩 Deprecations 🚩

- Deprecate `pdata.AttributeMap.Delete` in favor of `pdata.AttributeMap.Remove` (#4914)
- Deprecate consumerhelper, move helpers to consumer (#5006)

### 💡 Enhancements 💡

- Add `pdata.AttributeMap.RemoveIf`, which is a more performant way to remove multiple keys (#4914)
- Add `pipeline` key with pipeline identifier to processor loggers (#4968)
- Add a new yaml provider, allows providing yaml bytes (#4998)

### 🧰 Bug fixes 🧰

- Collector `Run` will now exit when a context cancels (#4954)
- Add missing droppedAttributesCount to pdata generated resource (#4979)
- Collector `Run` will now set state to `Closed` if startup fails (#4974)
