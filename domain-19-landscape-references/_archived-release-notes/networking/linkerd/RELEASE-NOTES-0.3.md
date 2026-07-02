---
title: linkerd v0.3 Release Notes
description: linkerd v0.3 Release Notes — Kubernetes 生产运维知识库
summary: linkerd v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Linkerd|linkerd]] v0.3 Release Notes

Source: [v0.3.1](https://github.com/linkerd/linkerd2/releases/tag/v0.3.1)

Conduit 0.3.1 improves Conduit's resilience and transparency.

* Proxy (data plane)
  * The proxy now makes fewer changes to requests and responses being proxied. In particular,
    requests and responses without bodies or with empty bodies are better supported.
  * HTTP/1 requests with different `Host` header fields are no longer sent on the same HTTP/1
    connection even when those hostnames resolve to the same IP address.
  * A connection leak during proxying of non-HTTP TCP connections was fixed.
  * The proxy now handles unavailable services more gracefully by timing out while waiting for an
    endpoint to become available for the [[Service|service]].
* Command-line interface
  * `$KUBECONFIG` with multiple paths is now supported. (PR #482 by @hypnoglow).
  * `conduit check` now checks for the availability of a Conduit update. (PR #460 by @ahume).
* Service Discovery
  * [[Kubernetes|Kubernetes]] services with type `ExternalName` are now supported.
* Control Plane
  * The proxy is injected into the control plane during installation to improve the control plane's
    resilience and to "dogfood" the proxy.
  * The control plane is now more resilient regarding networking failures.
* Documentation
  * The markdown source for the documentation published at https://conduit.io/docs/ is now open
    source at https://github.com/runconduit/conduit/tree/master/doc.



<!-- risk-assessed -->
