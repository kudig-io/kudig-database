---
title: prometheus v2.13 Release Notes
description: prometheus v2.13 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.13 Release Notes 是什么
- 如何 prometheus v2.13 Release Notes
trigger_keywords:
- prometheus
- v2.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.13 Release Notes

Source: [v2.13.1](https://github.com/prometheus/prometheus/releases/tag/v2.13.1)

* [BUGFIX] Fix panic in ARM builds of Prometheus. #6110
* [BUGFIX] promql: fix potential panic in the query logger. #6094
* [BUGFIX] Multiple errors of http: superfluous response.WriteHeader call in the logs. #6145
