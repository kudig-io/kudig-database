---
title: grafana v3.1 Release Notes
description: grafana v3.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v3.1 Release Notes 是什么
- 如何 grafana v3.1 Release Notes
trigger_keywords:
- grafana
- v3.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

# grafana v3.1 Release Notes

Source: [v3.1.1](https://github.com/grafana/grafana/releases/tag/v3.1.1)

For full details read the [Grafana 3.1 Release](http://grafana.org/blog/2016/07/12/grafana-3-1-released.html) blog post.

[Download](http://grafana.org/download/)
[Installation](http://docs.grafana.org/installation/)

### 3.1.1 Fixes
- **IFrame embedding**: Fixed issue of using full iframe height, fixes [#5605](https://github.com/grafana/grafana/issues/5606)
- **Panel PNG rendering**: Fixed issue detecting render completion, fixes [#5605](https://github.com/grafana/grafana/issues/5606)
- **Elasticsearch**: Fixed issue with templating query and json parse error, fixes [#5615](https://github.com/grafana/grafana/issues/5615)
- **Tech**: Upgraded JQuery to 2.2.4 to fix Security vulnerabilitie in 2.1.4, fixes [#5627](https://github.com/grafana/grafana/issues/5627)
- **Graphite**: Fixed issue with mixed data sources and Graphite, fixes [#5617](https://github.com/grafana/grafana/issues/5617)
- **Templating**: Fixed issue with template variable query was issued multiple times during dashboard load, fixes [#5637](https://github.com/grafana/grafana/issues/5637)
- **Zoom**: Fixed issues with zoom in and out on embedded (iframed) panel, fixes [#4489](https://github.com/grafana/grafana/issues/4489), [#5666](https://github.com/grafana/grafana/issues/5666)
- **Templating**: Row/Panel repeat issue when saving dashboard caused dupes to appear, fixes [#5591](https://github.com/grafana/grafana/issues/5591)
