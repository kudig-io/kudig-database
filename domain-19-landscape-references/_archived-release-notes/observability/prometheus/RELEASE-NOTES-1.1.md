---
title: prometheus v1.1 Release Notes
description: prometheus v1.1 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v1.1 Release Notes 是什么
- 如何 prometheus v1.1 Release Notes
trigger_keywords:
- prometheus
- v1.1
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

# [[Prometheus|prometheus]] v1.1 Release Notes

Source: [v1.1.3](https://github.com/prometheus/prometheus/releases/tag/v1.1.3)

- [ENHANCEMENT] Use golang-builder base image for tests in CircleCI.
- [ENHANCEMENT] Added unit tests for federation.
- [BUGFIX] Correctly de-dup metric families in federation output.
