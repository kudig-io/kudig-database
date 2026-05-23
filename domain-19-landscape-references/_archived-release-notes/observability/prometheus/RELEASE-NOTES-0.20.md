---
title: prometheus v0.20 Release Notes
description: prometheus v0.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- prometheus
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v0.20 Release Notes 是什么
- 如何 prometheus v0.20 Release Notes
trigger_keywords:
- prometheus
- v0.20
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

# [[Prometheus|prometheus]] v0.20 Release Notes

Source: [0.20.0](https://github.com/prometheus/prometheus/releases/tag/0.20.0)

This release contains multiple breaking changes to the configuration schema.
- [FEATURE] Allow configuring multiple Alertmanagers
- [FEATURE] Add server name to TLS configuration
- [FEATURE] Add labels for all node addresses and discover node port if available in [[Kubernetes|Kubernetes]] SD
- [ENHANCEMENT] More meaningful configuration errors
- [ENHANCEMENT] Round scraping timestamps to milliseconds in web UI
- [ENHANCEMENT] Make number of storage fingerprint locks configurable
- [BUGFIX] Fix date parsing in console template graphs
- [BUGFIX] Fix static console files in Docker images
- [BUGFIX] Fix console JS XHR requests for IE11
- [BUGFIX] Add missing path prefix in new status page
- [CHANGE] Rename `target_groups` to `static_configs` in config files
- [CHANGE] Rename `names` to `files` in file SD configuration
- [CHANGE] Remove kubelet port config option in Kubernetes SD configuration
