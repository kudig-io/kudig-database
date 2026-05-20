---
title: grafana v3.0 Release Notes
description: grafana v3.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v3.0 Release Notes 是什么
- 如何 grafana v3.0 Release Notes
trigger_keywords:
- grafana
- v3.0
- Release
- Notes
- release
- notes
---

# grafana v3.0 Release Notes

Source: [v3.0.4](https://github.com/grafana/grafana/releases/tag/v3.0.4)

[Download](http://grafana.org/download/)
[Installation](http://docs.grafana.org/installation/)

## Bug Fixes
- **Panel**: Fixed blank dashboard issue when switching to other dashboard while in fullscreen edit mode, fixes [#5163](https://github.com/grafana/grafana/pull/5163)
- **Templating**: Fixed issue with nested multi select variables and cascading and updating child variable selection state, fixes [#4861](https://github.com/grafana/grafana/pull/4861)
- **Templating**: Fixed issue with using templated data source in another template variable query, fixes [#5165](https://github.com/grafana/grafana/pull/5165)
- **Singlestat gauge**: Fixed issue with gauge render position, fixes [#5143](https://github.com/grafana/grafana/pull/5143)
- **Home dashboard**: Fixes broken home dashboard api, fixes [#5167](https://github.com/grafana/grafana/issues/5167)
