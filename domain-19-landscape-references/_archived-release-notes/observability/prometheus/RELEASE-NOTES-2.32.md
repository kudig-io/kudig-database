---
title: prometheus v2.32 Release Notes
description: prometheus v2.32 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.32 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.32 Release Notes 是什么
- 如何 prometheus v2.32 Release Notes
trigger_keywords:
- prometheus
- v2.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v2.32 Release Notes

Source: [v2.32.1](https://github.com/prometheus/prometheus/releases/tag/v2.32.1)

* [BUGFIX] Scrape: Fix reporting metrics when sample limit is reached during the report. #9996
* [BUGFIX] Scrape: Ensure that scrape interval and scrape timeout are always set. #10023
* [BUGFIX] TSDB: Expose and fix bug in iterators' `Seek()` method. #10030
