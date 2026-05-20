---
title: opa v0.30 Release Notes
description: opa v0.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.30 Release Notes 是什么
- 如何 opa v0.30 Release Notes
trigger_keywords:
- opa
- v0.30
- Release
- Notes
- release
- notes
---

# opa v0.30 Release Notes

Source: [v0.30.2](https://github.com/open-policy-agent/opa/releases/tag/v0.30.2)

This is a bugfix release that modifies the AWS credential provider to use POST
instead of GET for retrieving AWS STS tokens. The GET method can leak
credentials into the debug log if the AWS STS endpoint is unavailable.