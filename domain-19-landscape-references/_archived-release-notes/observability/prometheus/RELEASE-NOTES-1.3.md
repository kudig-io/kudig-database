---
title: prometheus v1.3 Release Notes
description: prometheus v1.3 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v1.3 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v1.3 Release Notes 是什么
- 如何 prometheus v1.3 Release Notes
trigger_keywords:
- prometheus
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v1.3 Release Notes

Source: [v1.3.1](https://github.com/prometheus/prometheus/releases/tag/v1.3.1)

This bug-fix release pulls in the fixes from the 1.2.3 release.
- [BUGFIX] Correctly handle empty Regex entry in relabel config.
- [BUGFIX] MOD (`%`) operator doesn't panic with small floating point numbers.
- [BUGFIX] Updated miekg/dns vendoring to pick up upstream bug fixes.
- [ENHANCEMENT] Improved DNS error reporting.
