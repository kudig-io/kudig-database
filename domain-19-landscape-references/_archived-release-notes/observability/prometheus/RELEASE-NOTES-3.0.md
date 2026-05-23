---
title: prometheus v3.0 Release Notes
description: prometheus v3.0 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v3.0 Release Notes 是什么
- 如何 prometheus v3.0 Release Notes
trigger_keywords:
- prometheus
- v3.0
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

# [[Prometheus|prometheus]] v3.0 Release Notes

Source: [v3.0.1](https://github.com/prometheus/prometheus/releases/tag/v3.0.1)

The first bug fix release for Prometheus 3.

* [BUGFIX] Promql: Make subqueries left open. #15431
* [BUGFIX] Fix memory leak when query log is enabled. #15434
* [BUGFIX] Support utf8 names on /v1/label/:name/values endpoint. #15399
