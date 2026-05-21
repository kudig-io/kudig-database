---
title: prometheus v3.4 Release Notes
description: prometheus v3.4 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v3.4 Release Notes 是什么
- 如何 prometheus v3.4 Release Notes
trigger_keywords:
- prometheus
- v3.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v3.4 Release Notes

Source: [v3.4.2](https://github.com/prometheus/prometheus/releases/tag/v3.4.2)

* [BUGFIX] OTLP receiver: Fix default configuration not being respected if the `otlp:` block is unset in the config file. #16693
