---
title: opentelemetry-collector v0.47 Release Notes
description: opentelemetry-collector v0.47 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.47 Release Notes — Kubernetes 生产运维知识库
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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


<!-- risk-assessed -->
