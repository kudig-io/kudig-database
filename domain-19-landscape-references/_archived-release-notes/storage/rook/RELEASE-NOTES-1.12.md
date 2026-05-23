---
title: rook v1.12 Release Notes
description: rook v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.12 Release Notes 是什么
- 如何 rook v1.12 Release Notes
trigger_keywords:
- rook
- v1.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Rook|rook]] v1.12 Release Notes

Source: [v1.12.11](https://github.com/rook/rook/releases/tag/v1.12.11)

# Improvements
Rook v1.12.11 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- exporter: Skip reconcile on exporter deletion (#13597, @travisn)
- [[Helm|helm]]: Allow configuring monitoring interval (#13408, @charlie-haley)
- core: Golang linter issues with variables in loops and update linter version (#13324, @travisn)
- multus: Use nginx-unprivileged image from quay (#13506, @BlaineEXE)
