---
title: prometheus v0.16 Release Notes
description: prometheus v0.16 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v0.16 Release Notes 是什么
- 如何 prometheus v0.16 Release Notes
trigger_keywords:
- prometheus
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v0.16 Release Notes

Source: [0.16.2](https://github.com/prometheus/prometheus/releases/tag/0.16.2)

- [FEATURE] Multiple authentication options for EC2 discovery added
- [FEATURE] Several meta labels for EC2 discovery added
- [FEATURE] Allow full URLs in static target groups (used e.g. by the `blackbox_exporter`)
- [FEATURE] Add Graphite remote-storage integration
- [FEATURE] Create separate Kubernetes targets for services and their endpoints
- [FEATURE] Add `clamp_{min,max}` functions to PromQL
- [FEATURE] Omitted time parameter in API query defaults to now
- [ENHANCEMENT] Less frequent time series file truncation
- [ENHANCEMENT] Instrument number of  manually deleted time series
- [ENHANCEMENT] Ignore lost+found directory during storage version detection
- [CHANGE] Kubernetes `masters` renamed to `api_servers`
- [CHANGE] "Healthy" and "unhealthy" targets are now called "up" and "down" in the web UI
- [CHANGE] Remove undocumented 2nd argument of the `delta` function.
  (This is a BREAKING CHANGE for users of the undocumented 2nd argument.)
- [BUGFIX] Return proper HTTP status codes on API errors
- [BUGFIX] Fix Kubernetes authentication configuration
- [BUGFIX] Fix stripped OFFSET from in rule evaluation and display
- [BUGFIX] Do not crash on failing Consul SD initialization
- [BUGFIX] Revert changes to metric auto-completion
- [BUGFIX] Add config overflow validation for TLS configuration
- [BUGFIX] Skip already watched Zookeeper nodes in serverset SD
- [BUGFIX] Don't federate stale samples
- [BUGFIX] Move NaN to end of result for `topk/bottomk/sort/sort_desc/min/max`
- [BUGFIX] Limit extrapolation of `delta/rate/increase`
- [BUGFIX] Fix unhandled error in rule evaluation

Some changes to the Kubernetes service discovery were integration since
it was released as a beta feature.
