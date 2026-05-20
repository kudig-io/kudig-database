---
title: opa v0.53 Release Notes
description: opa v0.53 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.53 Release Notes 是什么
- 如何 opa v0.53 Release Notes
trigger_keywords:
- opa
- v0.53
- Release
- Notes
- release
- notes
---

# opa v0.53 Release Notes

Source: [v0.53.1](https://github.com/open-policy-agent/opa/releases/tag/v0.53.1)

This is a bug fix release addressing the following issues:

### Runtime, Tooling, SDK
- plugins/logs: Previously while passing the decision log plugins's status to the Status API, the plugin held the mutex while a status upload was in process. This had the potential to block new decisions from being written to the plugin's buffer. To avoid this situation, a local copy of plugin's status is created ([#5966](https://github.com/open-policy-agent/opa/pull/5966)) authored by @ashutosh-narkar
- download: Public docker repositories require an authorization handshake where the client needs to respond to challenges marked by the `WWW-Authenticate` header of a `401 Unauthorized` response. Errors were returned when downloading a public image as it was assumed that authorization is not necessary for public repositories. This fix addresses this issue by challenging any `401 Unauthorized` responses by passing it to the docker.Authorizer ([#5902](https://github.com/open-policy-agent/opa/issues/5902)) authored by @DerGut
- `opa fmt`: Fix panic encountered while processing policies with comprehensions written on multiple lines with comments in these lines ([#5798](https://github.com/open-policy-agent/opa/issues/5798)) authored by @Trolloldem

### Topdown and Rego
- built-in function `object.subset`: Fix an issue in `object.subset` related to incorrect results being generated when arrays are provided as an input ([#5968](https://github.com/open-policy-agent/opa/issues/5968)) authored by @DCRUNNN
- planner: Fix the optimization check for overlapping ref rules ([#5964](https://github.com/open-policy-agent/opa/issues/5964)) authored by @srenatus