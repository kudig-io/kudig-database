---
title: grafana v7.1 Release Notes
description: grafana v7.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v7.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v7.1 Release Notes 是什么
- 如何 grafana v7.1 Release Notes
trigger_keywords:
- grafana
- v7.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---



# grafana v7.1 Release Notes

Source: [v7.1.5](https://github.com/grafana/grafana/releases/tag/v7.1.5)

[Download Page](https://grafana.com/grafana/download/7.1.5)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-1/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-1-x/32967)

### Features / Enhancements
* **Stats**: Stop counting the same user multiple times. [#26777](https://github.com/grafana/grafana/pull/26777), [@sakjur](https://github.com/sakjur)

### Bug Fixes
* **Alerting**: remove LongToWide call in alerting. [#27140](https://github.com/grafana/grafana/pull/27140), [@kylebrandt](https://github.com/kylebrandt)
* **AzureMonitor**: fix panic introduced in 7.1.4 when unit was unspecified and alias was used. [#27113](https://github.com/grafana/grafana/pull/27113), [@kylebrandt](https://github.com/kylebrandt)
* **Variables**: Fixes issue with All variable not being resolved. [#27151](https://github.com/grafana/grafana/pull/27151), [@hugohaggmark](https://github.com/hugohaggmark)