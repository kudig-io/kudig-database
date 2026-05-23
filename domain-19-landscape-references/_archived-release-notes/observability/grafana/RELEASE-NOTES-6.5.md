---
title: grafana v6.5 Release Notes
description: grafana v6.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v6.5 Release Notes 是什么
- 如何 grafana v6.5 Release Notes
trigger_keywords:
- grafana
- v6.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# grafana v6.5 Release Notes

Source: [v6.5.3](https://github.com/grafana/grafana/releases/tag/v6.5.3)

[Download Page](https://grafana.com/grafana/download/6.5.3)
[What's New Highlights](https://grafana.com/docs/guides/whats-new-in-v6-5/)
[Release Notes](https://community.grafana.com/t/release-notes-v6-5-x/22704)

### Features / Enhancements
* **API**: Validate redirect_to cookie has valid (Grafana) url . [#21057](https://github.com/grafana/grafana/pull/21057), [@papagian](https://github.com/papagian)

### Bug Fixes
* **AdHocFilter**: Shows SubMenu when filtering directly from table. [#21017](https://github.com/grafana/grafana/pull/21017), [@hugohaggmark](https://github.com/hugohaggmark)
* **Cloudwatch**: Fixed crash when switching from cloudwatch data source. [#21376](https://github.com/grafana/grafana/pull/21376), [@torkelo](https://github.com/torkelo)
* **DataLinks**: Sanitize data/panel link URLs. [#21140](https://github.com/grafana/grafana/pull/21140), [@dprokop](https://github.com/dprokop)
* **Elastic**: Fix multiselect variable interpolation for logs. [#20894](https://github.com/grafana/grafana/pull/20894), [@ivanahuckova](https://github.com/ivanahuckova)
* **[[Prometheus|Prometheus]]**: Fixes so user can change HTTP Method in config settings. [#21055](https://github.com/grafana/grafana/pull/21055), [@hugohaggmark](https://github.com/hugohaggmark)
* **Prometheus**: Prevents validation of inputs when clicking in them without changing the value. [#21059](https://github.com/grafana/grafana/pull/21059), [@hugohaggmark](https://github.com/hugohaggmark)
* **Rendering**: Fix panel PNG rendering when using sub url & serve_from_sub_path = true. [#21306](https://github.com/grafana/grafana/pull/21306), [@bgranvea](https://github.com/bgranvea)
* **Table**: Matches column names with unescaped regex characters. [#21164](https://github.com/grafana/grafana/pull/21164), [@hugohaggmark](https://github.com/hugohaggmark)