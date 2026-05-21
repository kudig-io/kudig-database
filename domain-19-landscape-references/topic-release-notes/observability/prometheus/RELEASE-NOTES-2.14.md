---
title: prometheus v2.14 Release Notes
description: prometheus v2.14 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.14 Release Notes 是什么
- 如何 prometheus v2.14 Release Notes
trigger_keywords:
- prometheus
- v2.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.14 Release Notes

Source: [v2.14.0](https://github.com/prometheus/prometheus/releases/tag/v2.14.0)

* [SECURITY/BUGFIX] UI: Ensure warnings from the API are escaped. #6279
* [FEATURE] API: `/api/v1/status/runtimeinfo` and `/api/v1/status/buildinfo` endpoints added for use by the React UI. #6243
* [FEATURE] React UI: implement the new experimental React based UI. #5694 and many more
  * Can be found by under `/new`.
  * Not all pages are implemented yet.
* [FEATURE] Status: Cardinality statistics added to the Runtime & Build Information page. #6125
* [ENHANCEMENT/BUGFIX] Remote write: fix delays in remote write after a compaction. #6021
* [ENHANCEMENT] UI: Alerts can be filtered by state. #5758
* [BUGFIX] API: lifecycle endpoints return 403 when not enabled. #6057
* [BUGFIX] Build: Fix Solaris build. #6149
* [BUGFIX] Promtool: Remove false duplicate rule warnings when checking rule files with alerts. #6270
* [BUGFIX] Remote write: restore use of deduplicating logger in remote write. #6113
* [BUGFIX] Remote write: do not reshard when unable to send samples. #6111
* [BUGFIX] Service discovery: errors are no longer logged on context cancellation. #6116, #6133
* [BUGFIX] UI: handle null response from API properly. #6071
