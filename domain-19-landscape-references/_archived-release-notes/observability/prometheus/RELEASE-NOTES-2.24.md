---
title: prometheus v2.24 Release Notes
description: prometheus v2.24 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.24 Release Notes 是什么
- 如何 prometheus v2.24 Release Notes
trigger_keywords:
- prometheus
- v2.24
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

# [[Prometheus|prometheus]] v2.24 Release Notes

Source: [v2.24.1](https://github.com/prometheus/prometheus/releases/tag/v2.24.1)

* [ENHANCEMENT] Cache basic authentication results to significantly improve performance of HTTP endpoints (via an update of prometheus/exporter-toolkit).
* [BUGFIX] Prevent user enumeration by timing requests sent to authenticated HTTP endpoints (via an update of prometheus/exporter-toolkit).
