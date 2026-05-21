---
title: grafana v1.9 Release Notes
description: grafana v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- flux
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v1.9 Release Notes 是什么
- 如何 grafana v1.9 Release Notes
trigger_keywords:
- grafana
- v1.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

# grafana v1.9 Release Notes

Source: [v1.9.1](https://github.com/grafana/grafana/releases/tag/v1.9.1)

Minor new features and fixes:

**Enhancements**
- [Issue #1028](https://github.com/grafana/grafana/issues/1028). Graph: New legend option `hideEmtpy` to hide series with only null values from legend
- [Issue #1242](https://github.com/grafana/grafana/issues/1242). OpenTSDB: Downsample query field now supports interval template variable
- [Issue #1126](https://github.com/grafana/grafana/issues/1126). InfluxDB: Support more than 10 series name segments when using alias `$number` patterns

**Fixes**
- [Issue #1251](https://github.com/grafana/grafana/issues/1251). Graph: Fix for y axis and scaled units (GiB etc) caused rounding, for example 400 GiB instead of 378 GiB
- [Issue #1199](https://github.com/grafana/grafana/issues/1199). Graph: fix for series tooltip when one series is hidden/disabled
- [Issue #1207](https://github.com/grafana/grafana/issues/1207). Graphite: movingAverage / movingMedian parameter type impovement, now handles int and interval parameter

Go to [grafana.org/download](http://grafana.org/download/) for downloads. [Blog post](http://grafana.org/blog/2014/11/17/grafana-1-9-0-rc1-released.html) with v1.9.0 release highlights. [Demo dashboard](http://play.grafana.org/#/dashboard/db/new-features-in-v19) showing of some of the new features in 1.9.0. 
