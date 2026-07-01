---
title: thanos v0.32 Release Notes
description: thanos v0.32 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.32 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.32 Release Notes 是什么
- 如何 thanos v0.32 Release Notes
trigger_keywords:
- thanos
- v0.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Thanos|thanos]] v0.32 Release Notes

Source: [v0.32.5](https://github.com/thanos-io/thanos/releases/tag/v0.32.5)

This patch release brings a fix for Receive, for a bug that allowed the head series limiter to be run without explicitly setting it. It also brings a fix for Store GW, for a bug that caused `/api/v1/labels` to not filter external labels.

Alongside this, we also build with Go 1.21.3 and `golang.org/x/net` v0.17 in this release to address [Go CVE](https://groups.google.com/g/golang-announce/c/iNNxDTCjZvo).
Please try it out and let us know if you spot any problems! Enjoy! 🎉

# Changes

## Fixed

- [#6615](https://github.com/thanos-io/thanos/pull/6615) [#6805](https://github.com/thanos-io/thanos/pull/6805): Build with Go 1.21 and bump golang.org/x/net to v0.17 for addressing [Go CVE](https://groups.google.com/g/golang-announce/c/iNNxDTCjZvo).
- [#6802](https://github.com/thanos-io/thanos/pull/6802) Receive: head series limiter should not run if no head series limit is set.
- [#6816](https://github.com/thanos-io/thanos/pull/6816) Store: fix [[Prometheus|prometheus]] store label values matches for external labels.
