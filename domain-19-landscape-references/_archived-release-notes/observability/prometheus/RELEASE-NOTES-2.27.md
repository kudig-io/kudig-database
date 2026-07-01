---
title: prometheus v2.27 Release Notes
description: prometheus v2.27 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.27 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.27 Release Notes 是什么
- 如何 prometheus v2.27 Release Notes
trigger_keywords:
- prometheus
- v2.27
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v2.27 Release Notes

Source: [v2.27.1](https://github.com/prometheus/prometheus/releases/tag/v2.27.1)

This release contains a bug fix for a security issue in the API endpoint. An
attacker can craft a special URL that redirects a user to any endpoint via an
HTTP 302 response. See the [security advisory][GHSA-vx57-7f4q-fpc7] for more details.

[GHSA-vx57-7f4q-fpc7]:https://github.com/prometheus/prometheus/security/advisories/GHSA-vx57-7f4q-fpc7

This vulnerability has been reported by Aaron Devaney from MDSec.

* [BUGFIX] SECURITY: Fix arbitrary redirects under the /new endpoint (CVE-2021-29622)
