---
title: grafana v4.4 Release Notes
description: grafana v4.4 Release Notes — Kubernetes 生产运维知识库
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
- grafana v4.4 Release Notes 是什么
- 如何 grafana v4.4 Release Notes
trigger_keywords:
- grafana
- v4.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

# grafana v4.4 Release Notes

Source: [v4.4.3](https://github.com/grafana/grafana/releases/tag/v4.4.3)

[Download Page](http://grafana.org/download)
[Installation Guide](http://docs.grafana.org/installation/)

To view screenshots and examples of the new features read the [What's New in v4.4](http://docs.grafana.org/guides/whats-new-in-v4-4/) article.

## Bug Fixes

* **Search**: Fix for issue that casued search view to hide  when you clicked starred or tags filters, fixes [#8981](https://github.com/grafana/grafana/issues/8981)
* **Modals**: ESC key now closes modal again, fixes [#8981](https://github.com/grafana/grafana/issues/8988), thx [@j-white](https://github.com/j-white)