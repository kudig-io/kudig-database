---
title: linkerd v18.9 Release Notes
description: linkerd v18.9 Release Notes — Kubernetes 生产运维知识库
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
- linkerd v18.9 Release Notes 是什么
- 如何 linkerd v18.9 Release Notes
trigger_keywords:
- linkerd
- v18.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---

# linkerd v18.9 Release Notes

Source: [v18.9.1](https://github.com/linkerd/linkerd2/releases/tag/v18.9.1)

* Web UI
  * **New** Default landing page provides namespace overview with expandable
    sections
  * **New** Breadcrumb navigation at the top of the dashboard
  * **Improved** Tap and Top pages
    * Table rendering performance improvements via throttling
    * Tables now link to resource detail pages
    * Tap an entire namespace when no resource is specified
    * Tap websocket errors provide more descriptive text
    * Consolidated source and destination columns
  * Misc ui updates
    * Metrics tables now include a small success rate chart
    * Improved latency formatting for seconds latencies
    * Renamed upstream/downstream to inbound/outbound
    * Sidebar scrolls independently from main panel, scrollbars hidden when not
      needed
    * Removed social links from sidebar
* CLI
  * **New** `linkerd check` now validates Linkerd proxy versions and readiness
  * **New** `linkerd inject` now provides an injection status report, and warns
    when resources are not injectable
  * **New** `linkerd top` now has a `--hide-sources` flag, to hide the source
    column and collapse top results accordingly
* Control Plane
  * Updated Prometheus to v2.4.0, Grafana to 5.2.4