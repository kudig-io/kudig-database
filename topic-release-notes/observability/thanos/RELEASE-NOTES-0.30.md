---
title: thanos v0.30 Release Notes
description: thanos v0.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.30 Release Notes 是什么
- 如何 thanos v0.30 Release Notes
trigger_keywords:
- thanos
- v0.30
- Release
- Notes
- release
- notes
---

# thanos v0.30 Release Notes

Source: [v0.30.2](https://github.com/thanos-io/thanos/releases/tag/v0.30.2)

## What's Changed
* Fixed panic because of nil sampler in https://github.com/thanos-io/thanos/pull/6066 by @xBazilio
* Fix store-gateway deadlock due to not close BlockSeriesClient in https://github.com/thanos-io/thanos/pull/6086 by @alanprot 


**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.30.1...v0.30.2