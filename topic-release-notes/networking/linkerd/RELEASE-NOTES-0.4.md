---
title: linkerd v0.4 Release Notes
description: linkerd v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- linkerd v0.4 Release Notes 是什么
- 如何 linkerd v0.4 Release Notes
trigger_keywords:
- linkerd
- v0.4
- Release
- Notes
- release
- notes
---

# linkerd v0.4 Release Notes

Source: [v0.4.4](https://github.com/linkerd/linkerd2/releases/tag/v0.4.4)

## v0.4.4

Conduit v0.4.4 continues to improve production suitability and sets up internals for the
upcoming v0.5.0 release.

* Production Readiness
  * The destination service has been mostly-rewritten to improve safety and correctness,
    especially during controller initialization.
  * Readiness and Liveness checks have been added for some controller components.
  * RBAC settings have been expanded so that Prometheus can access node-level metrics.
* User Interface
  * Ad blockers like uBlock prevented the Conduit dashboard from fetching API data. This
    has been fixed.
  * The UI now highlights pods that have failed to start a proxy.
* Internals
  * Various dependency upgrades, including Rust 1.26.2.
  * TLS testing continues to bear fruit, precipitating stability improvements to
    dependencies like Rustls.

Special thanks to @alenkacz for improving docker build times!