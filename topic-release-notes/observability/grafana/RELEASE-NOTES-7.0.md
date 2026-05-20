---
title: grafana v7.0 Release Notes
description: grafana v7.0 Release Notes — Kubernetes 生产运维知识库
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
- grafana v7.0 Release Notes 是什么
- 如何 grafana v7.0 Release Notes
trigger_keywords:
- grafana
- v7.0
- Release
- Notes
- release
- notes
---

# grafana v7.0 Release Notes

Source: [v7.0.6](https://github.com/grafana/grafana/releases/tag/v7.0.6)

[Download Page](https://grafana.com/grafana/download/7.0.6)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-0/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-0-x/29381)

### Bug fixes

* **Templating**: Fixed recursive queries triggered when switching dashboard settings view [#26137](https://github.com/grafana/grafana/pull/26137)
* **Templating**: Fix recursive loop of template variable queries when changing ad-hoc-variable [#26191](https://github.com/grafana/grafana/pull/26191)
* **Auth**: Add support for forcing authentication in anonymous mode and modify SignIn to use it instead of redirect [#25567](https://github.com/grafana/grafana/pull/25567)
* **Auth**: Fix POST request failures with anonymous access [#26049](https://github.com/grafana/grafana/pull/26049)
