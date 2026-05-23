---
title: prometheus v2.4 Release Notes
description: prometheus v2.4 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.4 Release Notes 是什么
- 如何 prometheus v2.4 Release Notes
trigger_keywords:
- prometheus
- v2.4
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

# [[Prometheus|prometheus]] v2.4 Release Notes

Source: [v2.4.3](https://github.com/prometheus/prometheus/releases/tag/v2.4.3)

* [BUGFIX] Fix panic when using custom EC2 API for SD #4672
* [BUGFIX] Fix panic when Zookeeper SD cannot connect to servers #4669
* [BUGFIX] Make the skip_head an optional parameter for snapshot API #4674