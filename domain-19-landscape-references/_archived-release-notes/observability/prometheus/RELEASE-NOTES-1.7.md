---
title: prometheus v1.7 Release Notes
description: prometheus v1.7 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v1.7 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v1.7 Release Notes 是什么
- 如何 prometheus v1.7 Release Notes
trigger_keywords:
- prometheus
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v1.7 Release Notes

Source: [v1.7.2](https://github.com/prometheus/prometheus/releases/tag/v1.7.2)

* [BUGFIX] Correctly remove all targets from DNS [[Service|service]] discovery if the
  corresponding DNS query succeeds and returns an empty result.
* [BUGFIX] Correctly parse resolution input in expression browser.
* [BUGFIX] Consistently use UTC in the date picker of the expression browser.
* [BUGFIX] Correctly handle multiple ports in Marathon service discovery.
* [BUGFIX] Fix HTML escaping so that HTML templates compile with Go1.9.
* [BUGFIX] Prevent number of remote write shards from going negative.
* [BUGFIX] In the graphs created by the expression browser, render very large
  and small numbers in a readable way.
* [BUGFIX] Fix a rarely occurring iterator issue in varbit encoded chunks.
