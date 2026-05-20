---
title: kops v1.9 Release Notes
description: kops v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.9 Release Notes 是什么
- 如何 kops v1.9 Release Notes
trigger_keywords:
- kops
- v1.9
- Release
- Notes
- release
- notes
---

# kops v1.9 Release Notes

Source: [1.9.2](https://github.com/kubernetes/kops/releases/tag/1.9.2)

Cherry-picks of important fixes:

* Add AuthenticationTokenWebhook flag #5231
* Don't repeatedly download nodeup #5462
* Introduce a global backoff to rate limit failed image downloads #5464
* Fix containerRegistry for Kubernetes < 1.10 #5353
* set GracePeriodSeconds to -1 #5143 
