---
title: longhorn v1.0 Release Notes
description: longhorn v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- longhorn v1.0 Release Notes 是什么
- 如何 longhorn v1.0 Release Notes
trigger_keywords:
- longhorn
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Longhorn|longhorn]] v1.0 Release Notes

Source: [v1.0.2](https://github.com/longhorn/longhorn/releases/tag/v1.0.2)

Longhorn v1.0.2 is a stable release.

## Enhancement
1. [Now Longhorn ships with a default Pod Security Policy](https://github.com/longhorn/longhorn/issues/1671).
2. [Enhanced Air Gap installation experience using Chart](https://github.com/longhorn/longhorn/issues/1641).

## Bug Fixes
1. [Fix an issue](https://github.com/longhorn/longhorn/issues/1647) that might cause Longhorn installation stuck due to upgrade failure.
2. [Speed up](https://github.com/longhorn/longhorn/issues/1655) the Longhorn installation process.
3. [Fix an issue](https://github.com/longhorn/longhorn/issues/1665) that might cause Longhorn installation stuck due to `Incompatible` Engine image.
4. [Fix an issue](https://github.com/longhorn/longhorn/pull/1585) with [[containerd|containerd]].

## Upgrade:
Live upgrade from v1.0.2 from v1.0.0 or v1.0.1 is supported. See [here](https://longhorn.io/docs/1.0.2/deploy/upgrade/) for the upgrade instructions.