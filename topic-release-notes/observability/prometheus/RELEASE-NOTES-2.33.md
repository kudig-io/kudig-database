---
title: prometheus v2.33 Release Notes
description: prometheus v2.33 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.33 Release Notes 是什么
- 如何 prometheus v2.33 Release Notes
trigger_keywords:
- prometheus
- v2.33
- Release
- Notes
- release
- notes
---

# prometheus v2.33 Release Notes

Source: [v2.33.5](https://github.com/prometheus/prometheus/releases/tag/v2.33.5)

The binaries published with this release are built with Go1.17.8 to avoid [CVE-2022-24921](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2022-24921).

* [BUGFIX] Remote-write: Fix deadlock between adding to queue and getting batch. #10395
