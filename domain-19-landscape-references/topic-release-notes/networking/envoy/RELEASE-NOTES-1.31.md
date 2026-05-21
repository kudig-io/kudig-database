---
title: envoy v1.31 Release Notes
description: envoy v1.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.31 Release Notes 是什么
- 如何 envoy v1.31 Release Notes
trigger_keywords:
- envoy
- v1.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# envoy v1.31 Release Notes

Source: [v1.31.10](https://github.com/envoyproxy/envoy/releases/tag/v1.31.10)

**Summary of changes**:

* Observability:
  - Fixed division by zero bug in Dynatrace sampling controller.

* Release:
  - Fixed permissions for distroless config directory.
  - Updated container images (Ubuntu/distroless).

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.31.10
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.31.10/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.31.10/version_history/v1.31/v1.31.10
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.31.9...v1.31.10

Signed-off-by: Ryan Northey <ryan@synca.io>
Signed-off-by: Rohit Agrawal <rohit.agrawal@databricks.com>