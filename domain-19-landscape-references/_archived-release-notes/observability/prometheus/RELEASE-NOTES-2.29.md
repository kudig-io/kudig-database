---
title: prometheus v2.29 Release Notes
description: prometheus v2.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.29 Release Notes 是什么
- 如何 prometheus v2.29 Release Notes
trigger_keywords:
- prometheus
- v2.29
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

# [[Prometheus|prometheus]] v2.29 Release Notes

Source: [v2.29.2](https://github.com/prometheus/prometheus/releases/tag/v2.29.2)

* [BUGFIX] Fix [[Kubernetes|Kubernetes]] SD failing to discover [[Ingress|Ingress]] in Kubernetes v1.22. #9205
* [BUGFIX] Fix data race in loading write-ahead-log (WAL). #9259