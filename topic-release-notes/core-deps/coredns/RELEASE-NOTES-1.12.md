---
title: coredns v1.12 Release Notes
description: coredns v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- coredns
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- coredns v1.12 Release Notes 是什么
- 如何 coredns v1.12 Release Notes
trigger_keywords:
- coredns
- v1.12
- Release
- Notes
- release
- notes
---

# coredns v1.12 Release Notes

Source: [v1.12.4](https://github.com/coredns/coredns/releases/tag/v1.12.4)

This release improves stability and security, fixing context propagation in DoH, label offset handling
in the file plugin, and connection leaks in gRPC and transfer. It also adds support for the prefer option
in loadbalance, introduces timeouts to the metrics server, and fixes several security vulnerabilities
(see details in related security advisories).


## Brought to You By

Archy
Ilya Kulakov
Olli Janatuinen
Qasim Sarfraz
Syed Azeez
Ville Vesilehto
wencyu
Yong Tang


## Noteworthy Changes

* core: Improve caddy.GracefulServer conformance checks (https://github.com/coredns/coredns/pull/7416)
* core: Propagate HTTP request context in DoH (https://github.com/coredns/coredns/pull/7491)
* plugin/file: Fix label offset problem in ClosestEncloser (https://github.com/coredns/coredns/pull/7465)
* plugin/grpc: Check proxy list length in policies (https://github.com/coredns/coredns/pull/7512)
* plugin/grpc: Fix span leak and deadline on error attempt (https://github.com/coredns/coredns/pull/7487)
* plugin/header: Remove deprecated syntax (https://github.com/coredns/coredns/pull/7436)
* plugin/loadbalance: Support prefer option (https://github.com/coredns/coredns/pull/7433)
* plugin/metrics: Add timeouts to metrics HTTP server (https://github.com/coredns/coredns/pull/7469)
* plugin/trace: Migrate dd-trace-go v1 to v2 (https://github.com/coredns/coredns/pull/7466)
* plugin/transfer: Fix goroutine leak on axfr err (https://github.com/coredns/coredns/pull/7516)
