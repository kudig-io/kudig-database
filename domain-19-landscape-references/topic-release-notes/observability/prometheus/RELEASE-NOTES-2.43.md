---
title: prometheus v2.43 Release Notes
description: prometheus v2.43 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.43 Release Notes 是什么
- 如何 prometheus v2.43 Release Notes
trigger_keywords:
- prometheus
- v2.43
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.43 Release Notes

Source: [v2.43.1+stringlabels](https://github.com/prometheus/prometheus/releases/tag/v2.43.1%2Bstringlabels)

Special release build that incorporates performance improvements using
the stringlabels Go tag. This release aims to provide a more efficient and
faster solution for users managing large-scale deployments or facing performance
issues with the default Prometheus binaries.

The new labels data structure replaces the existing label/value storage with a
single string, reducing heap size and improving performance in most cases. It
enables Prometheus to use fewer system resources, particularly in
memory-intensive environments.
