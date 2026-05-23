---
title: cilium v0.8 Release Notes
description: cilium v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cilium
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v0.8 Release Notes 是什么
- 如何 cilium v0.8 Release Notes
trigger_keywords:
- cilium
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
created: "2026-05-23"
---

# [[Cilium|cilium]] v0.8 Release Notes

Source: [v0.8.2](https://github.com/cilium/cilium/releases/tag/v0.8.2)

- Separate state directory inside runtime directory (#537)
- Fix all remaining testsuites and have Jenkins fail properly on all failures (#513)
- policy: Support carrying part of the path in the name (#533)
- Temporary fix: Set net.ipv6.conf.all.disable_ipv6=1 as Docker disables it by mistake (#544)