---
title: linkerd v0.3 Release Notes
description: linkerd v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- linkerd v0.3 Release Notes 是什么
- 如何 linkerd v0.3 Release Notes
trigger_keywords:
- linkerd
- v0.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# linkerd v0.3 Release Notes

Source: [v0.3.1](https://github.com/linkerd/linkerd2/releases/tag/v0.3.1)

Conduit 0.3.1 improves Conduit's resilience and transparency.

* Proxy (data plane)
  * The proxy now makes fewer changes to requests and responses being proxied. In particular,
    requests and responses without bodies or with empty bodies are better supported.
  * HTTP/1 requests with different `Host` header fields are no longer sent on the same HTTP/1
    connection even when those hostnames resolve to the same IP address.
  * A connection leak during proxying of non-HTTP TCP connections was fixed.
  * The proxy now handles unavailable services more gracefully by timing out while waiting for an
    endpoint to become available for the service.
* Command-line interface
  * `$KUBECONFIG` with multiple paths is now supported. (PR #482 by @hypnoglow).
  * `conduit check` now checks for the availability of a Conduit update. (PR #460 by @ahume).
* Service Discovery
  * Kubernetes services with type `ExternalName` are now supported.
* Control Plane
  * The proxy is injected into the control plane during installation to improve the control plane's
    resilience and to "dogfood" the proxy.
  * The control plane is now more resilient regarding networking failures.
* Documentation
  * The markdown source for the documentation published at https://conduit.io/docs/ is now open
    source at https://github.com/runconduit/conduit/tree/master/doc.

