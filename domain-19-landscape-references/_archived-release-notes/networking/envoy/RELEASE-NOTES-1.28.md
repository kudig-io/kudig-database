---
title: envoy v1.28 Release Notes
description: envoy v1.28 Release Notes — Kubernetes 生产运维知识库
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
- envoy v1.28 Release Notes 是什么
- 如何 envoy v1.28 Release Notes
trigger_keywords:
- envoy
- v1.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Envoy|envoy]] v1.28 Release Notes

Source: [v1.28.7](https://github.com/envoyproxy/envoy/releases/tag/v1.28.7)

**Summary of changes**

[CVE-2024-45808](https://github.com/envoyproxy/envoy/security/advisories/GHSA-p222-xhp9-39rc): Malicious log injection via access logs
[CVE-2024-45806](https://github.com/envoyproxy/envoy/security/advisories/GHSA-ffhv-fvxq-r6mf): Potential manipulate `x-envoy` headers from external sources
[CVE-2024-45810](https://github.com/envoyproxy/envoy/security/advisories/GHSA-qm74-x36m-555q): Envoy crashes for LocalReply in http async client

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.28.7
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.28.7/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.28.7/version_history/v1.28/v1.28.7
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.28.6...v1.28.7

Signed-off-by: Boteng Yao <boteng@google.com>
Signed-off-by: Ryan Northey <ryan@synca.io>