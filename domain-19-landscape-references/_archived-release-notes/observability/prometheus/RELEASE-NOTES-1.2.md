---
title: prometheus v1.2 Release Notes
description: prometheus v1.2 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v1.2 Release Notes 是什么
- 如何 prometheus v1.2 Release Notes
trigger_keywords:
- prometheus
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v1.2 Release Notes

Source: [v1.2.3](https://github.com/prometheus/prometheus/releases/tag/v1.2.3)

- [BUGFIX] Correctly handle end time before start time in range queries.
- [BUGFIX] Correctly handle empty Regex entry in relabel config.
- [BUGFIX] MOD (`%`) operator doesn't panic with small floating point numbers.
- [BUGFIX] Updated miekg/dns vendoring to pick up upstream bug fixes.
- [ENHANCEMENT] Improved DNS error reporting.
