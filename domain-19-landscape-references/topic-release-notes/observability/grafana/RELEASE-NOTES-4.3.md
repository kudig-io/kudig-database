---
title: grafana v4.3 Release Notes
description: grafana v4.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- flux
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v4.3 Release Notes 是什么
- 如何 grafana v4.3 Release Notes
trigger_keywords:
- grafana
- v4.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

# grafana v4.3 Release Notes

Source: [v4.3.2](https://github.com/grafana/grafana/releases/tag/v4.3.2)

[Download Page](http://grafana.org/download)
[Installation Guide](http://docs.grafana.org/installation/)

To view screenshots and examples of the new features read the [What's New in v4.3](http://docs.grafana.org/guides/whats-new-in-v4-3/) article.

## Bug fixes

* **InfluxDB**: Fixed issue with query editor not showing ALIAS BY input field when in text editor mode [#8459](https://github.com/grafana/grafana/issues/8459)
* **Graph Log Scale**: Fixed issue with log scale going below x-axis [#8244](https://github.com/grafana/grafana/issues/8244)
* **Playlist**: Fixed dashboard play order issue [#7688](https://github.com/grafana/grafana/issues/7688)
* **Elasticsearch**: Fixed table query issue with ES 2.x [#8467](https://github.com/grafana/grafana/issues/8467), thx [@goldeelox](https://github.com/goldeelox)

## Changes
* **Lazy Loading Of Panels**: Panels are no longer loaded as they are scrolled into view, this was reverted due to Chrome bug, might be reintroduced when Chrome fixes it's JS blocking behavior on scroll. [#8500](https://github.com/grafana/grafana/issues/8500)