---
title: envoy v1.29 Release Notes
description: envoy v1.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.29 Release Notes 是什么
- 如何 envoy v1.29 Release Notes
trigger_keywords:
- envoy
- v1.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# envoy v1.29 Release Notes

Source: [v1.29.12](https://github.com/envoyproxy/envoy/releases/tag/v1.29.12)

**Summary of changes**:

- [CVE-2024-53270](https://github.com/envoyproxy/envoy/security/advisories/GHSA-q9qv-8j52-77p3):  HTTP/1: sending overload crashes when the request is reset beforehand

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.29.12
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.29.12/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.29.12/version_history/v1.29/v1.29.12
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.29.11...v1.29.12

Signed-off-by: Ryan Northey <ryan@synca.io>
Signed-off-by: Boteng Yao <boteng@google.com>