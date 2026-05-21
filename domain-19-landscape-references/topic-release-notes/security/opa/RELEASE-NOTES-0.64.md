---
title: opa v0.64 Release Notes
description: opa v0.64 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.64 Release Notes 是什么
- 如何 opa v0.64 Release Notes
trigger_keywords:
- opa
- v0.64
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.64 Release Notes

Source: [v0.64.1](https://github.com/open-policy-agent/opa/releases/tag/v0.64.1)

This is a bug fix release addressing the following issues:

- ci: Pin GitHub Actions macos runner version. The architecture of the GitHub Actions Runner `macos-latest` was changed from `amd64` to `arm64` and as a result `darwin/amd64` binary wasn't released ([#6720](https://github.com/open-policy-agent/opa/issues/6720)) authored by @suzuki-shunsuke
- plugins/discovery: Update comparison logic used in the discovery plugin for handling overrides. This fixes a panic that resulted from the comparison of uncomparable types ([#6723](https://github.com/open-policy-agent/opa/pull/6723)) authored by @ashutosh-narkar

