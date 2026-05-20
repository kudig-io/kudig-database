---
title: envoy v1.7 Release Notes
description: envoy v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.7 Release Notes 是什么
- 如何 envoy v1.7 Release Notes
trigger_keywords:
- envoy
- v1.7
- Release
- Notes
- release
- notes
---

# envoy v1.7 Release Notes

Source: [v1.7.1](https://github.com/envoyproxy/envoy/releases/tag/v1.7.1)

* Security fix related to the x-envoy-original-dst-host header. The header is now opt-in.
  https://github.com/envoyproxy/envoy/pull/4046 and https://github.com/envoyproxy/envoy/pull/4051.