---
title: prometheus v2.38 Release Notes
description: prometheus v2.38 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.38 Release Notes 是什么
- 如何 prometheus v2.38 Release Notes
trigger_keywords:
- prometheus
- v2.38
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
created: "2026-05-23"
---

# [[Prometheus|prometheus]] v2.38 Release Notes

Source: [v2.38.0](https://github.com/prometheus/prometheus/releases/tag/v2.38.0)

* [FEATURE]: Web: Add a `/api/v1/format_query` HTTP API endpoint that allows pretty-formatting PromQL expressions. #11036 #10544 #11005
* [FEATURE]: UI: Add support for formatting PromQL expressions in the UI. #11039
* [FEATURE]: DNS SD: Support MX records for discovering targets. #10099
* [FEATURE]: Templates: Add `toTime()` template function that allows converting sample timestamps to Go `time.Time` values. #10993
* [ENHANCEMENT]: [[Kubernetes|Kubernetes]] SD: Add `__meta_kubernetes_service_port_number` meta label indicating the [[Service|service]] port number. #11002 #11053
* [ENHANCEMENT]: Kubernetes SD: Add `__meta_kubernetes_pod_container_image` meta label indicating the container image. #11034 #11146
* [ENHANCEMENT]: PromQL: When a query panics, also log the query itself alongside the panic message. #10995
* [ENHANCEMENT]: UI: Tweak colors in the dark theme to improve the contrast ratio. #11068
* [ENHANCEMENT]: Web: Speed up calls to `/api/v1/rules` by avoiding locks and using atomic types instead. #10858
* [ENHANCEMENT]: Scrape: Add a `no-default-scrape-port` feature flag, which omits or removes any default HTTP (`:80`) or HTTPS (`:443`) ports in the target's scrape address. #9523
* [BUGFIX]: TSDB: In the WAL watcher metrics, expose the `type="exemplar"` label instead of `type="unknown"` for exemplar records. #11008
* [BUGFIX]: TSDB: Fix race condition around allocating series IDs during chunk snapshot loading. #11099
