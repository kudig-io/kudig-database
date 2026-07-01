---
title: grafana v7.2 Release Notes
description: grafana v7.2 Release Notes — Kubernetes 生产运维知识库
summary: grafana v7.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- mysql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v7.2 Release Notes 是什么
- 如何 grafana v7.2 Release Notes
trigger_keywords:
- grafana
- v7.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- mysql-basics
---



# grafana v7.2 Release Notes

Source: [v7.2.2](https://github.com/grafana/grafana/releases/tag/v7.2.2)

[Download Page](https://grafana.com/grafana/download/7.2.2)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-2/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-2-x/36321)

### Features / Enhancements
**Caution:** Please do not use/enable the `database_metrics` feature flag. It will corrupt MySQL database tables. See [#28440](https://github.com/grafana/grafana/issues/28440) for more information.

~~**Instrumentation**: Add counters and histograms for database queries. [#28236](https://github.com/grafana/grafana/pull/28236), [@bergquist](https://github.com/bergquist)~~
* **Instrumentation**: Add histogram for request duration. [#28364](https://github.com/grafana/grafana/pull/28364), [@bergquist](https://github.com/bergquist)
* **Instrumentation**: Adds environment_info metric. [#28355](https://github.com/grafana/grafana/pull/28355), [@bergquist](https://github.com/bergquist)

### Bug Fixes
* **CloudWatch**: Fix custom metrics. [#28391](https://github.com/grafana/grafana/pull/28391), [@aknuds1](https://github.com/aknuds1)