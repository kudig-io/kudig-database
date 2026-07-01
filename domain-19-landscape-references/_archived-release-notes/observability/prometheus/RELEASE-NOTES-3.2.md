---
title: prometheus v3.2 Release Notes
description: prometheus v3.2 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v3.2 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v3.2 Release Notes 是什么
- 如何 prometheus v3.2 Release Notes
trigger_keywords:
- prometheus
- v3.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v3.2 Release Notes

Source: [v3.2.1](https://github.com/prometheus/prometheus/releases/tag/v3.2.1)

* [BUGFIX] Don't send Accept header `escape=allow-utf-8` when `metric_name_validation_scheme: legacy` is configured. #16061
