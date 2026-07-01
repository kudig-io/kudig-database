---
title: grafana v1.4 Release Notes
description: grafana v1.4 Release Notes — Kubernetes 生产运维知识库
summary: grafana v1.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v1.4 Release Notes 是什么
- 如何 grafana v1.4 Release Notes
trigger_keywords:
- grafana
- v1.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---



# grafana v1.4 Release Notes

Source: [v1.4.0](https://github.com/grafana/grafana/releases/tag/v1.4.0)

New Features:
#44 Annotations! Required a lot of work to get right. Read [wiki](https://github.com/torkelo/grafana/wiki/Annotations) article for more info. Supported annotations data sources are graphite metrics and graphite events. Support for more will be added in the future! 
#35 Support for multiple graphite servers! (Read [wiki](https://github.com/torkelo/grafana/wiki/Multiple-datasources) article for more)
#116 Back to dashboard link in top menu to easily exist full screen / edit mode.
#114, #97 Legend values now use the same y axes formatter
#77 Improvements and polish to the light theme

Changes:
#98 Stack is no longer by default turned on in graph display settings.
Hide controls (Ctrl+h) now hides the sub menu row (where filtering, and annotations are). So if you had filtering enabled and hide controls enabled you will not see the filtering sub menu. 

Fixes: 
#94 Fix for bug that caused dashboard settings to sometimes not contain timepicker tab.
#110 Graph with many many metrics caused legend to push down graph editor below screen. You can now scroll in edit mode & full screen mode for graphs with lots of series & legends. 
#104 Improvement to graphite target editor, select wildcard now gives you a "select metric" link for the next node. 
#105 Added zero as a possible node value in groupByAlias function
