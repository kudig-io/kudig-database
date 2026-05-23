---
title: prometheus v2.25 Release Notes
description: prometheus v2.25 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.25 Release Notes 是什么
- 如何 prometheus v2.25 Release Notes
trigger_keywords:
- prometheus
- v2.25
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

# [[Prometheus|prometheus]] v2.25 Release Notes

Source: [v2.25.2](https://github.com/prometheus/prometheus/releases/tag/v2.25.2)

* [BUGFIX] Fix the ingestion of scrapes when the wall clock changes, e.g. on suspend. #8601
