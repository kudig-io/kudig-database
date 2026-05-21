---
title: linkerd v0.5 Release Notes
description: linkerd v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- linkerd v0.5 Release Notes 是什么
- 如何 linkerd v0.5 Release Notes
trigger_keywords:
- linkerd
- v0.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# linkerd v0.5 Release Notes

Source: [v0.5.0](https://github.com/linkerd/linkerd2/releases/tag/v0.5.0)

Conduit v0.5.0 introduces a new, experimental feature that automatically
enables Transport Layer Security between Conduit proxies to secure
application traffic. It also adds support for HTTP protocol upgrades, so
applications that use WebSockets can now benefit from Conduit.

* Security
  * **New** `conduit install --tls=optional` enables automatic, opportunistic
    TLS. See [the docs][auto-tls] for more info.
* Production Readiness
  * The proxy now transparently supports HTTP protocol upgrades to support, for
    instance, WebSockets.
  * The proxy now seamlessly forwards HTTP `CONNECT` streams.
  * Controller services are now configured with liveness and readiness probes.
* User Interface
  * `conduit stat` now supports a virtual `authority` resource that aggregates
    traffic by the `:authority` (or `Host`) header of an HTTP request.
  * `dashboard`, `stat`, and `tap` have been updated to describe TLS state for
    traffic.
  * `conduit tap` now has more detailed information, including the direction of
    each message (outbound or inbound).
  * `conduit stat` now more-accurately records histograms for low-latency services.
  * `conduit dashboard` now includes error messages when a Conduit-enabled pod fails.
* Internals
  * Prometheus has been upgraded to v2.3.1.
  * A potential live-lock has been fixed in HTTP/2 servers.
  * `conduit tap` could crash due to a null-pointer access. This has been fixed.

[auto-tls]: docs/automatic-tls.md
