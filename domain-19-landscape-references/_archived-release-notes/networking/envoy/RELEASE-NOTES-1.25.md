---
title: envoy v1.25 Release Notes
description: envoy v1.25 Release Notes — Kubernetes 生产运维知识库
summary: envoy v1.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.25 Release Notes 是什么
- 如何 envoy v1.25 Release Notes
trigger_keywords:
- envoy
- v1.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Envoy|envoy]] v1.25 Release Notes

Source: [v1.25.11](https://github.com/envoyproxy/envoy/releases/tag/v1.25.11)

repo: Release v1.25.11

Summary of changes:

* Fixed a bug where processing of deferred streams with the value of
  ``http.max_requests_per_io_cycle`` more than 1, can cause a crash.

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.25.11
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.25.11/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.25.11/version_history/v1.25/v1.25.0
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.25.10...v1.25.11

Signed-off-by: Ryan Northey <ryan@synca.io>