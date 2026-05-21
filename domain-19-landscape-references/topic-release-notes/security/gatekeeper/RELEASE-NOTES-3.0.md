---
title: gatekeeper v3.0 Release Notes
description: gatekeeper v3.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gatekeeper v3.0 Release Notes 是什么
- 如何 gatekeeper v3.0 Release Notes
trigger_keywords:
- gatekeeper
- v3.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# gatekeeper v3.0 Release Notes

Source: [v3.0.3](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.0.3)

This alpha release includes breaking changes and bug fixes.

## Breaking Changes ⚠️ 
* Rename deny rule to violation (#169)
* Change to HA-Compatible Status Schemas (#159)
* Fix CT name validation (https://github.com/open-policy-agent/frameworks/pull/27)
* Only require kind for Constraint Templates (https://github.com/open-policy-agent/frameworks/pull/29)
* Handle namespaceselector and empty namespaces (https://github.com/open-policy-agent/frameworks/pull/26)

## Bug Fixes 🐞
* Detect/handle invalid syntax in k8scontainerlimits (#167)
* Handle namespaceselector failure (#155)

Please report any issues here: https://github.com/open-policy-agent/gatekeeper/issues/new