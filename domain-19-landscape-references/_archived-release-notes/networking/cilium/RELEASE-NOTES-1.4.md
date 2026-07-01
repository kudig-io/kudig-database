---
title: cilium v1.4 Release Notes
description: cilium v1.4 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.4 Release Notes 是什么
- 如何 cilium v1.4 Release Notes
trigger_keywords:
- cilium
- v1.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---



# [[Cilium|cilium]] v1.4 Release Notes

Source: [v1.4.10](https://github.com/cilium/cilium/releases/tag/v1.4.10)

Summary of Changes
------------------

**Important Bug Fixes**

* [[Envoy|Envoy]] is updated to release 1.12.2, including important security fixes (#9742, @jrajahalme)
  * Fixes CVE-2019-18801, CVE-1019-18802, CVE-1019-18838
  * For more information, see [Envoy 1.12.2 Release Notes](https://groups.google.com/forum/#!topic/envoy-announce/BjgUTDTKAu8)

**Misc**

* bugtool: add cilium node list output (#9474, @ianvernon)


Changes
-------

```
   Ian Vernon (1):
         bugtool: add `cilium node list` output

   Jarno Rajahalme (8):
         Envoy: Do not configure policy name
         envoy: Update to the latest API
         Dockerfile: Use latest Envoy image
         envoy: Update image for Envoy CVEs 2019-10-08
         envoy: Update to release 1.12 with Cilium TLS support
         envoy: Update to release 1.12.1
         Dockerfile: Use Envoy image that always resumes NPDS
         envoy: Update to 1.12.2
```
```