---
title: thanos v0.2 Release Notes
description: thanos v0.2 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.2 Release Notes 是什么
- 如何 thanos v0.2 Release Notes
trigger_keywords:
- thanos
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Thanos|thanos]] v0.2 Release Notes

Source: [v0.2.1](https://github.com/thanos-io/thanos/releases/tag/v0.2.1)

Xmas patch to release 2 critical fixes (Azure, DNS SD) and awesome, new store UI page.

This also includes first mitigation for https://github.com/improbable-eng/thanos/issues/335

Changelog also available [here](./CHANGELOG.md). 

### Added

- Relabel drop for Thanos Ruler to enable replica label drop and alert deduplication on AM side.
- Query: Stores UI page available at `/stores`.

![](./docs/img/query_ui_stores.png)

### Fixed

- Thanos Rule Alertmanager DNS SD bug.
- DNS SD bug when having SRV results with different ports.
- Move handling of HA alertmanagers to be the same as [[Prometheus|Prometheus]].
- Azure iteration implementation flaw.