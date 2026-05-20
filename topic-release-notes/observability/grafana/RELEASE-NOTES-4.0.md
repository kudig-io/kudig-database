---
title: grafana v4.0 Release Notes
description: grafana v4.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v4.0 Release Notes 是什么
- 如何 grafana v4.0 Release Notes
trigger_keywords:
- grafana
- v4.0
- Release
- Notes
- release
- notes
---

# grafana v4.0 Release Notes

Source: [v4.0.2](https://github.com/grafana/grafana/releases/tag/v4.0.2)

### Enhancements
- **Playlist**: Add support for kiosk mode [#6727](https://github.com/grafana/grafana/issues/6727)

### Bugfixes
- **Alerting**: Add alert message to webhook notifications [#6807](https://github.com/grafana/grafana/issues/6807)
- **Alerting**: Fixes a bug where avg() reducer treated null as zero. [#6879](https://github.com/grafana/grafana/issues/6879)
- **PNG Rendering**: Fix for server side rendering when using non default http addr bind and domain setting [#6813](https://github.com/grafana/grafana/issues/6813)
- **PNG Rendering**: Fix for server side rendering when setting enforce_domain to true [#6769](https://github.com/grafana/grafana/issues/6769)
- **Webhooks**: Add content type json to outgoing webhooks [#6822](https://github.com/grafana/grafana/issues/6822)
- **Keyboard shortcut**: Fixed zoom out shortcut [#6837](https://github.com/grafana/grafana/issues/6837)
- **Webdav**: Adds basic auth headers to webdav uploader [#6779](https://github.com/grafana/grafana/issues/6779)

[Download](http://grafana.org/download/4_0_2/)
