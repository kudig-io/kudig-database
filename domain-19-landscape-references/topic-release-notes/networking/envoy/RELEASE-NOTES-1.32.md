---
title: envoy v1.32 Release Notes
description: envoy v1.32 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- docker
- kafka
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.32 Release Notes 是什么
- 如何 envoy v1.32 Release Notes
trigger_keywords:
- envoy
- v1.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- kafka-basics
---

# envoy v1.32 Release Notes

Source: [v1.32.13](https://github.com/envoyproxy/envoy/releases/tag/v1.32.13)

**Summary of changes**:

* Security updates:

  Resolve dependency CVEs:
  - CVE-2025-0725: curl
  - CVE-2024-7246: gRPC
  - CVE-2024-11407: gRPC
  - CVE-2025-27817: kafka
  - CVE-2025-27818: kafka
  - CVE-2024-51745: wasmtime
  - CVE-2025-53901: wasmtime

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.32.13
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.32.13/
**Release notes**:

https://www.envoyproxy.io/docs/envoy/v1.32.13/version_history/v1.32/v1.32.13
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.32.12...v1.32.13

Signed-off-by: Ryan Northey <ryan@synca.io>
Signed-off-by: Rohit Agrawal <rohit.agrawal@databricks.com>
