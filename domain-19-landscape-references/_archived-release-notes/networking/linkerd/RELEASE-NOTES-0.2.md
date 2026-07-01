---
title: linkerd v0.2 Release Notes
description: linkerd v0.2 Release Notes — Kubernetes 生产运维知识库
summary: linkerd v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- mysql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- linkerd v0.2 Release Notes 是什么
- 如何 linkerd v0.2 Release Notes
trigger_keywords:
- linkerd
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- mysql-basics
---



# [[Linkerd|linkerd]] v0.2 Release Notes

Source: [v0.2.0](https://github.com/linkerd/linkerd2/releases/tag/v0.2.0)

## v0.2.0

This is a big milestone! With this release, Conduit adds support for HTTP/1.x and raw TCP traffic, meaning it should "just work" for most applications that are running on [[Kubernetes|Kubernetes]] without additional configuration.

* Data plane
  * Conduit now transparently proxies all TCP traffic, including HTTP/1.x and HTTP/2.
    (See caveats below.)
* Command-line interface
  * Improved error handling for the `tap` command
  * `tap` also now works with HTTP/1.x traffic
* Dashboard
  * Minor UI appearance tweaks
  * [[Deployments|Deployments]] now searchable from the dashboard sidebar

Caveats:
* Conduit will automatically work for most protocols. However, applications that use WebSockets, HTTP tunneling/proxying, or protocols such as MySQL and SMTP, will require some additional configuration. See the [documentation](https://conduit.io/adding-your-[[Service|service]]/#protocol-support) for details.
* Conduit doesn't yet support external DNS lookups. These will be addressed in an upcoming release.
* There are known issues with Conduit's telemetry pipeline that prevent it from scaling beyond a few nodes. These will be addressed in an upcoming release.
* Conduit is still experimental! Please help us by [filing issues and contributing pull requests](https://github.com/runconduit/conduit/issues/new).