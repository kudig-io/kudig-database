---
title: prometheus v0.17 Release Notes
description: prometheus v0.17 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v0.17 Release Notes 是什么
- 如何 prometheus v0.17 Release Notes
trigger_keywords:
- prometheus
- v0.17
- Release
- Notes
- release
- notes
---

# prometheus v0.17 Release Notes

Source: [0.17.0](https://github.com/prometheus/prometheus/releases/tag/0.17.0)

This version no longer works with Alertmanager 0.0.4 and earlier!
The alerting rule syntax has changed as well but the old syntax is supported
up until version 0.18.

All regular expressions in PromQL are anchored now, matching the behavior of
regular expressions in config files.
- [CHANGE] Integrate with Alertmanager 0.1.0 and higher
- [CHANGE] Degraded storage mode renamed to rushed mode
- [CHANGE] New alerting rule syntax
- [CHANGE] Add label validation on ingestion
- [CHANGE] Regular expression matchers in PromQL are anchored
- [FEATURE] Add `without` aggregation modifier
- [FEATURE] Send alert resolved notifications to Alertmanager
- [FEATURE] Allow millisecond precision in configuration file
- [FEATURE] Support AirBnB's Smartstack Nerve for service discovery
- [ENHANCEMENT] Storage switches less often between regular and rushed mode.
- [ENHANCEMENT] Storage switches into rushed mode if there are too many memory chunks.
- [ENHANCEMENT] Added more storage instrumentation
- [ENHANCEMENT] Improved instrumentation of notification handler
- [BUGFIX] Do not count head chunks as chunks waiting for persistence
- [BUGFIX] Handle OPTIONS HTTP requests to the API correctly
- [BUGFIX] Parsing of ranges in PromQL fixed
- [BUGFIX] Correctly validate URL flag parameters
- [BUGFIX] Log argument parse errors
- [BUGFIX] Properly handle creation of target with bad TLS config
- [BUGFIX] Fix of checkpoint timing issue
