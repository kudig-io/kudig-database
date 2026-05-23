---
title: linkerd v18.7 Release Notes
description: linkerd v18.7 Release Notes — Kubernetes 生产运维知识库
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
- linkerd v18.7 Release Notes 是什么
- 如何 linkerd v18.7 Release Notes
trigger_keywords:
- linkerd
- v18.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Linkerd|linkerd]] v18.7 Release Notes

Source: [v18.7.3](https://github.com/linkerd/linkerd2/releases/tag/v18.7.3)

Linkerd2 v18.7.3 completes the rebranding from Conduit to Linkerd2, and improves
overall performance and stability.

* Proxy
  * **Improved** CPU utilization by ~20%
* Web UI
  * **Experimental** `/tap` page now supports additional filters
* Control Plane
  * Updated all k8s.io dependencies to 1.11.1