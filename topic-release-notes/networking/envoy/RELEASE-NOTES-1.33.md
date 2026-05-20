---
title: envoy v1.33 Release Notes
description: envoy v1.33 Release Notes — Kubernetes 生产运维知识库
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
- envoy v1.33 Release Notes 是什么
- 如何 envoy v1.33 Release Notes
trigger_keywords:
- envoy
- v1.33
- Release
- Notes
- release
- notes
---

# envoy v1.33 Release Notes

Source: [v1.33.14](https://github.com/envoyproxy/envoy/releases/tag/v1.33.14)

**Summary of changes**:

* Security updates:

  Resolve dependency CVEs:
  - c-ares/CVE-2025-0913:
      Use after free can crash Envoy due to malfunctioning or compromised DNS.

While a potentially severe bug in some cloud environments, this has limited exploitability
as any attacker would require control of DNS.

Envoy advisory is here https://github.com/envoyproxy/envoy/security/advisories/GHSA-fg9g-pvc4-776f

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.33.14
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.33.14/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.33.14/version_history/v1.33/v1.33.14
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.33.13...v1.33.14

Signed-off-by: Ryan Northey <ryan@synca.io>
Signed-off-by: Boteng Yao <boteng@google.com>