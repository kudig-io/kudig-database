---
title: prometheus v2.2 Release Notes
description: prometheus v2.2 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.2 Release Notes 是什么
- 如何 prometheus v2.2 Release Notes
trigger_keywords:
- prometheus
- v2.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v2.2 Release Notes

Source: [v2.2.1](https://github.com/prometheus/prometheus/releases/tag/v2.2.1)

* [BUGFIX] Fix data loss in TSDB on compaction
* [BUGFIX] Correctly stop timer in remote-write path
* [BUGFIX] Fix deadlock triggered by loading targets page
* [BUGFIX] Fix incorrect buffering of samples on range selection queries
* [BUGFIX] Handle large index files on windows properly