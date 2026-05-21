---
title: calico v2.4 Release Notes
description: calico v2.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- calico v2.4 Release Notes 是什么
- 如何 calico v2.4 Release Notes
trigger_keywords:
- calico
- v2.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---

# calico v2.4 Release Notes

Source: [v2.4.1](https://github.com/projectcalico/calico/releases/tag/v2.4.1)

# Release notes for Calico v2.4.1

## Changes to [libcalico-go](https://github.com/projectcalico/libcalico-go)
 - [#488](https://github.com/projectcalico/libcalico-go/pull/488): bugfix: fix handling of empty namespaceSelector when using Kubernetes datastore driver (@gunjan5)
 - [#486](https://github.com/projectcalico/libcalico-go/pull/486): bugfix: properly resync node IPs during Felix restart in Kubernetes datastore driver (@bcreane)